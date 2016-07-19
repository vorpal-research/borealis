//
// Created by belyaev on 6/30/16.
//

#include "SMT/Portfolio/Solver.h"

#include "SMT/Boolector/Solver.h"
#include "SMT/Z3/Solver.h"
#include "SMT/CVC4/Solver.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/ProtobufConverterImpl.hpp"

#include <chrono>
#include <unordered_map>

#include <unistd.h>
#include <sys/wait.h>

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
    auto select_call = select(maxfd + 1, &fds, nullptr, nullptr, (d.count() == 0)? nullptr : &tv);
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

        if(!protobuffy(res)->SerializeToFileDescriptor(raw(pipe))) {
            UNREACHABLE("Cannot serialize result to pipe");
        }
    });
};

smt::Result Solver::isViolated(PredicateState::Ptr query, PredicateState::Ptr state) {

    std::unordered_map<fd_t, pid_t> forks;

    auto solversToRun = util::viewContainer(solvers).toHashSet();
    if(solversToRun.count("z3"))        forks.insert(forkSolver<Z3>(       pimpl_->memoryStart, pimpl_->memoryEnd, query, state));
    if(solversToRun.count("cvc4"))      forks.insert(forkSolver<CVC4>(     pimpl_->memoryStart, pimpl_->memoryEnd, query, state));
    if(solversToRun.count("boolector")) forks.insert(forkSolver<Boolector>(pimpl_->memoryStart, pimpl_->memoryEnd, query, state));
    if(solversToRun.count("mathsat"))   forks.insert(forkSolver<MathSAT>(  pimpl_->memoryStart, pimpl_->memoryEnd, query, state));

    ASSERT(not solversToRun.empty(), "No solvers to run");

    FactoryNest FN;
    smt::proto::Result pres;

    ON_SCOPE_EXIT(
        for(auto&& kv: forks) close(kv.first);
        while(wait(nullptr) != -1);
    )

    auto timeout = force_timeout.get(0);

    auto files = better_select(std::chrono::milliseconds(timeout), util::viewContainer(forks).map(LAM(kv, kv.first)));
    if(files.empty()) {
        for(auto&& kv: forks) kill(kv.second, SIGTERM);
        dbgs() << "Acquired result: unknown" << endl;
        return smt::UnknownResult();
    }

    auto pipe = util::head(files);
    for(auto&& kv: forks) if(kv.first != pipe) kill(kv.second, SIGTERM);

    if(!pres.ParseFromFileDescriptor(raw(pipe))) {
        UNREACHABLE("Cannot parse result from pipe");
    }

    auto&& res = deprotobuffy(FN, pres);

    dbgs() << "Acquired result: "
           << (res->isSat()? "sat" : (res->isUnsat()? "unsat" : "unknown"))
           << endl;

    return *res;
}

} /* namespace portfolio_ */
} /* namespace borealis */

#include "Util/unmacros.h"
