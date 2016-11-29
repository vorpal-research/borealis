//
// Created by belyaev on 6/30/16.
//

#include "SMT/Portfolio/Solver.h"

#include "SMT/Boolector/Solver.h"
#include "SMT/Z3/Solver.h"
#include "SMT/CVC4/Solver.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/ProtobufConverterImpl.hpp"
#include "State/Transformer/GraphBuilder.h"

#include <chrono>
#include <unordered_map>

#include <unistd.h>
#include <sys/wait.h>

#include <llvm/Support/GraphWriter.h>

#include "Util/macros.h"

static borealis::config::IntConfigEntry force_timeout("portfolio", "force-timeout");
static borealis::config::MultiConfigEntry solvers("portfolio", "use-solver");

namespace borealis {
namespace portfolio_ {

enum class fd_t : int {};
enum class pid_t : ::pid_t {};

inline fd_t fd(int f) { return (fd_t) f; }

inline pid_t pid(::pid_t p) { return (pid_t) p; }

template<class T>
std::underlying_type_t<T> raw(T t) {
    return std::underlying_type_t<T>(t);
}

inline void close(fd_t f) {
    ::close(raw(f));
}

inline void kill(pid_t p, int signal) {
    ::kill(raw(p), signal);
}

static std::ostream &operator<<(std::ostream &ost, fd_t fd) {
    return ost << raw(fd);
}

static std::ostream &operator<<(std::ostream &ost, pid_t d) {
    return ost << raw(d);
}

static bool operator==(pid_t p, int that) {
    return raw(p) == that;
}

static bool operator!=(pid_t p, int that) {
    return raw(p) != that;
}

} /* namespace portfolio_ */
} /* namespace borealis */

namespace std {
    template<> struct hash<borealis::portfolio_::pid_t> {
        size_t operator()(borealis::portfolio_::pid_t p) const noexcept {
            using delegate = std::hash<int>;
            return delegate{}(borealis::portfolio_::raw(p));
        }
    };
}

namespace std {
template<> struct hash<borealis::portfolio_::fd_t> {
    size_t operator()(borealis::portfolio_::fd_t p) const noexcept {
        using delegate = std::hash<int>;
        return delegate{}(borealis::portfolio_::raw(p));
    }
};
}

namespace borealis {
namespace portfolio_ {

static std::pair<fd_t, fd_t> mk_pipes() {
    int pipes[2];
    pipe(pipes);
    return { fd(pipes[0]), fd(pipes[1]) };
};

template<class F>
static std::pair<fd_t, pid_t> forked(F f) {
    fd_t ipipe;
    fd_t opipe;
    std::tie(ipipe, opipe) = mk_pipes();

    auto child = pid(fork());
    ASSERT(child != -1, tfm::format("Can't fork : %s", strerror(errno)))

    if(child == 0) {
        signal(SIGTERM, SIG_DFL);
        close(ipipe);
        f(opipe);
        close(opipe);
        exit(0);
    }
    close(opipe);
    dbgs() << "Forked child #" << child << endl;
    return { ipipe, child };
}

template<typename Duration>
static timeval to_timeval(Duration&& d) {
    std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(d);

    timeval tv;
    tv.tv_sec  = sec.count();
    tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(d - sec).count();
    return tv;
}

template<typename Duration, typename Descs>
static std::unordered_set<fd_t> better_select(Duration&& d, Descs&& fdescs) {
    auto tv = to_timeval(std::forward<Duration>(d));

    fd_set fds;
    FD_ZERO(&fds);
    int maxfd = 0;
    for(auto f : fdescs) {
        FD_SET(raw(f), &fds);
        maxfd = std::max(raw(f), maxfd);
    }
    int select_call = select(maxfd + 1, &fds, nullptr, nullptr, (d.count() == 0)? nullptr : &tv);
    ASSERT(select_call != -1, tfm::format("System error on select: %s", strerror(errno)))

    std::unordered_set<fd_t> res;
    if(select_call == 0) {
        return res;
    }
    for(auto&& f: fdescs) {
        if(FD_ISSET(raw(f), &fds)) res.insert(f);
    }
    return std::move(res);
};

struct Solver::Impl {
    unsigned long long memoryStart;
    unsigned long long memoryEnd;
};

Solver::~Solver() {}
Solver::Solver(unsigned long long memoryStart, unsigned long long memoryEnd):
    pimpl_(new Impl{memoryStart, memoryEnd}){}

template <class Logic>
std::pair<fd_t, pid_t> forkSolver(
    size_t memoryStart, size_t memoryEnd,
    PredicateState::Ptr query, PredicateState::Ptr state
) {
    return forked([&](fd_t pipe){
        typename Logic::ExprFactory ef;
        typename Logic::Solver solver(ef, memoryStart, memoryEnd);
        auto res = solver.isViolated(query, state);

        if(res.isUnknown()) sleep(force_timeout.get(0)); // we are not interested in unknowns
        // FIXME: if all solvers break to unknown, this would lead to waiting for timeout, which is not good

        if(!protobuffy(res)->SerializeToFileDescriptor(raw(pipe))) {
            UNREACHABLE("Cannot serialize result to pipe");
        }
    });
};

smt::Result Solver::isViolated(PredicateState::Ptr query, PredicateState::Ptr state) {

    using namespace std::chrono_literals;

    std::unordered_map<fd_t, pid_t> forks;
    std::unordered_map<fd_t, std::string> solverNames;

    auto solversToRun = util::viewContainer(solvers).toHashSet();
    if(solversToRun.count("z3")) {
        auto fork = forkSolver<Z3>(pimpl_->memoryStart, pimpl_->memoryEnd, query, state);
        forks.insert(fork);
        solverNames[fork.first] = "z3";
    }
    if(solversToRun.count("cvc4")) {
        auto fork = forkSolver<CVC4>(     pimpl_->memoryStart, pimpl_->memoryEnd, query, state);
        forks.insert(fork);
        solverNames[fork.first] = "cvc4";
    }
    if(solversToRun.count("boolector")) {
        auto fork = forkSolver<Boolector>(pimpl_->memoryStart, pimpl_->memoryEnd, query, state);
        forks.insert(fork);
        solverNames[fork.first] = "boolector";
    }
    if(solversToRun.count("mathsat")) {
        auto fork = forkSolver<MathSAT>(  pimpl_->memoryStart, pimpl_->memoryEnd, query, state);
        forks.insert(fork);
        solverNames[fork.first] = "mathsat";
    }

    ASSERT(not solversToRun.empty(), "No solvers to run");

    FactoryNest FN;
    smt::proto::Result pres;

    ON_SCOPE_EXIT(
        for(auto&& kv: forks) close(kv.first);
        while(wait(nullptr) != -1);
    )

    auto timeout = std::chrono::milliseconds(force_timeout.get(0));
    auto startTime = std::chrono::steady_clock::now();

    std::unordered_set<fd_t> files;

    while(files.size() < 1) {
        files = better_select(timeout, util::viewContainer(forks).map(LAM(kv, kv.first)));
        timeout -= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);
        if(files.empty()) {
            for(auto&& kv: forks) {
                kill(kv.second, SIGTERM);
                dbgs() << solverNames[kv.first] << ": timed out and was killed" << endl;
            }
            dbgs() << "Acquired result: unknown" << endl;
            return smt::UnknownResult();
        }
    }

    // auto pipe = util::head(files);
    std::vector<std::unique_ptr<borealis::smt::Result>> results;

    for(auto&& kv: forks) {
        if(not files.count(kv.first)) {
            kill(kv.second, SIGTERM);
            dbgs() << solverNames[kv.first] << ": timed out and was killed" << endl;
        }
    }
    for(auto&& pipe : files) {
        if(not pres.ParseFromFileDescriptor(raw(pipe))) {
            UNREACHABLE("Cannot parse result from pipe");
        }

        results.push_back(deprotobuffy(FN, pres));

        dbgs() << solverNames[pipe] << ": " << (results.back()->isSat()? "sat" : (results.back()->isUnsat()? "unsat" : "unknown")) << endl;
    }

    for(auto&& res0: results) {
        for(auto&& res1: results) {
            if(res0->isSat() && res1->isUnsat()) {
                errs() << "Conflicting results in portfolio: ";
                errs() << "Query: " << query << endl;
                errs() << "State: " << state << endl;

                //auto graph = buildGraphRep(state);
                //static int pos = 0;
                //++pos;
                //std::string realFileName = llvm::WriteGraph<PSGraph*>(&graph, "graph-conflict." + util::toString(pos), false);
                //if (realFileName.empty()) continue;
                //llvm::DisplayGraph(realFileName, false, llvm::GraphProgram::DOT);

                //UNREACHABLE(tfm::format(
                //    "Conflicting results in portfolio: \n"
                //    "Query: %s \n"
                //    "State: %s \n",
                //    query, state
                //))
            }
        }
    }

    dbgs() << "Acquired result: "
           << (results.front()->isSat()? "sat" : (results.front()->isUnsat()? "unsat" : "unknown"))
           << endl;

    return *results.front();
}

} /* namespace portfolio_ */
} /* namespace borealis */

#include "Util/unmacros.h"
