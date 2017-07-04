//
// Created by belyaev on 5/26/16.
//
#ifndef LOCAL_STENSGAARD_AA
#define LOCAL_STENSGAARD_AA

#include <unordered_map>

#include <llvm/Support/DOTGraphTraits.h>
#include <llvm/Support/GraphWriter.h>

#include "Term/TermUtils.hpp"

#include "Util/disjoint_sets.hpp"

#include "State/Transformer/Transformer.hpp"

#include "Util/macros.h"
#include "PointerCollector.h"

namespace borealis {

class LocalAABase {
public:
    virtual bool mayAlias(Term::Ptr, Term::Ptr) { return true; }
    virtual void prepare(PredicateState::Ptr) {}
    virtual ~LocalAABase() {}
};

class LocalStensgaardAA: public Transformer<LocalStensgaardAA>, public LocalAABase {

    struct WeirdTermEquals { // FIXME: think
        TermEquals delegate;

        bool operator()(const Term::Ptr& lhv, const Term::Ptr& rhv) const noexcept{
            if(TermUtils::isConstantTerm(lhv) && TermUtils::isConstantTerm(rhv)) {
                // we need shallow equality for constants to escape undesired sharing
                return lhv.get() == rhv.get();
            }
            return delegate(lhv, rhv);
        }
    };

    using token = util::subset<Term::Ptr>*;

    PointerCollector PC;
    util::disjoint_set<Term::Ptr> relatedPtrs;
    std::unordered_map<Term::Ptr, token, TermHash, WeirdTermEquals> view;
    std::unordered_map<token, token> pointsTo;
    Term::Set noalias;
    Term::Set nonFreeTerms;
    std::unordered_map<Type::Ptr, token> freeSpaces;

public:
    LocalStensgaardAA(FactoryNest FN):
        Transformer<LocalStensgaardAA>(FN), PC(FN) {}

    static bool isNamedTerm(Term::Ptr t) {
        return TermUtils::isNamedTerm(t);
    }

    token quasi() {
        return relatedPtrs.emplace(nullptr);
    }

    token join(token& l, token& r) {
        if(l and r) {
            auto&& pl = relatedPtrs.find(pointsTo[l]);
            auto&& pr = relatedPtrs.find(pointsTo[r]);
            auto res = relatedPtrs.unite(l, r);
            pointsTo[l] = pointsTo[r] = pointsTo[res] = pl;
            if(pl != pr) join(pl, pr);
            return l = r = res;
        }
        if(l) {
            auto res = relatedPtrs.find(l);
            auto pres = relatedPtrs.find(pointsTo[l]);
            pointsTo[res] = pointsTo[l] = pres;
            return l = r = res;
        }
        if(r) {
            auto res = relatedPtrs.find(r);
            auto pres = relatedPtrs.find(pointsTo[r]);
            pointsTo[res] = pointsTo[r] = pres;
            return l = r = res;
        }
        return l = r = quasi();
    }

    token get(Term::Ptr t) {
        if(auto k = util::at(view, t)) return k.getUnsafe()->getRoot();
        else {
            auto token = relatedPtrs.emplace(t);
            view[t] = token;

            if(auto var = llvm::dyn_cast<ValueTerm>(t)) {
                if(var->isGlobal()) {
                    // globals are a bit special
                    // they are not free and non-alias
                    nonFreeTerms.insert(t);
                    noalias.insert(t);
                    // but loading them is, in principle, free
                    join(freeSpaces[TypeUtils::getPointerElementType(t->getType())], pointsTo[token]);
                    return token;
                }
            }

            if(
                nonFreeTerms.count(t) == 0
                && isNamedTerm(t)
                && PC.isDereferencedPointer(t)
            ) {
                // this is a free pointer term
                join(freeSpaces[t->getType()], token);
            }
            return token;
        }
    }

    token getDereferenced(Term::Ptr t) {
        return relatedPtrs.find(pointsTo[get(t)]);
    }

    Term::Ptr transformLoadTerm(LoadTermPtr t) {
        auto ts = get(t);
        auto loads = get(t->getRhv());
        join(pointsTo[loads], ts);
        return t;
    }

    Term::Ptr transformGepTerm(GepTermPtr t) {
        auto ts = get(t);
        auto bases = get(t->getBase());
        join(pointsTo[bases], pointsTo[ts]);
        return t;
    }

    Term::Ptr transformTernaryTerm(TernaryTermPtr t) {
        auto ts = get(t);
        auto lhs = get(t->getTru());
        auto rhs = get(t->getFls());
        join(pointsTo[ts], pointsTo[lhs]);
        join(pointsTo[ts], pointsTo[rhs]);
        return t;
    }

    Term::Ptr transformCastTerm(CastTermPtr t) {
        auto ts = get(t);
        auto bases = get(t->getRhv());
        join(pointsTo[bases], pointsTo[ts]);
        return t;
    }

    Term::Ptr transformUnaryTerm(UnaryTermPtr t) {
        auto ts = get(t);
        auto bases = get(t->getRhv());
        join(pointsTo[bases], pointsTo[ts]);
        return t;
    }

    Term::Ptr transformBinaryTerm(BinaryTermPtr t) {
        auto ts = get(t);
        auto lhs = get(t->getLhv());
        auto rhs = get(t->getRhv());
        join(pointsTo[ts], pointsTo[lhs]);
        join(pointsTo[ts], pointsTo[rhs]);
        return t;
    }

    Term::Ptr transformAxiomTerm(AxiomTermPtr t) {
        auto ts = get(t);
        auto bases = get(t->getRhv());
        join(pointsTo[bases], pointsTo[ts]);
        return t;
    }

    Predicate::Ptr transformEqualityPredicate(EqualityPredicatePtr pred) {
        auto lhvt = pred->getLhv();
        if(isNamedTerm(lhvt) &&
            (pred->getType() == PredicateType::STATE || pred->getType() == PredicateType::INVARIANT)
            ) nonFreeTerms.insert(lhvt);

        auto ls = get(lhvt);
        auto rs = get(pred->getRhv());

        join(pointsTo[ls], pointsTo[rs]);
        return pred;
    }

    Predicate::Ptr transformAllocaPredicate(AllocaPredicatePtr al) {
        auto lhvt = al->getLhv();
        noalias.insert(lhvt);
        nonFreeTerms.insert(lhvt);

        auto ls = get(lhvt);
        pointsTo[ls] = quasi(); // we cannot do anything better now =(
        return al;
    }

    Predicate::Ptr transformMallocPredicate(MallocPredicatePtr al) {
        auto lhvt = al->getLhv();
        noalias.insert(lhvt);
        nonFreeTerms.insert(lhvt);

        auto ls = get(lhvt);
        pointsTo[ls] = quasi(); // we cannot do anything better now =(
        return al;
    }

    Predicate::Ptr transformStorePredicate(StorePredicatePtr store) {
        auto ls = get(store->getLhv());
        auto rs = get(store->getRhv());
        join(pointsTo[ls], rs);
        return store;
    }

    bool mayAlias(Term::Ptr l, Term::Ptr r) override {
        if(*l == *r) return true;

        bool lno_alias = !!noalias.count(l);
        bool rno_alias = !!noalias.count(r);
        if(lno_alias && rno_alias) return false;

        auto ls = pointsTo[get(l)];
        auto rs = pointsTo[get(r)];
        if(ls and rs) return ls->getRoot() == rs->getRoot();
        else return false;
    }

    virtual void prepare(PredicateState::Ptr ps) override {
        this->transform(ps);
    }

    friend std::ostream& operator<<(std::ostream& ost, const LocalStensgaardAA & aa) {
        for(auto&& kv : aa.view) {
            ost << kv.first << " in " << aa.relatedPtrs.findConst(kv.second) << std::endl;
        }
        for(auto&& kv : aa.pointsTo) {
            ost << aa.relatedPtrs.findConst(kv.first) << " -> " << aa.relatedPtrs.findConst(kv.second) << std::endl;
        }
        return ost;
    }

    struct GraphRep {
        struct Node {
            std::unordered_set<Term::Ptr> data;
            std::shared_ptr<Node> child;

            auto nodes_view() const {
                auto baseView = child? util::view(&child, &child+1) : util::view(&child, &child);
                return baseView.map([](auto&& sh){ return sh.get(); });
            }
        };

        std::vector<std::shared_ptr<Node>> nodes;

        auto nodes_view() const {
            return borealis::util::viewContainer(nodes).map([](auto&& sh){ return sh.get(); });
        }
    };

    GraphRep asGraph() {
        std::unordered_map<token, std::shared_ptr<GraphRep::Node>> reverse;
        for(auto&& kv : view) {
            auto&& it = reverse.find(relatedPtrs.find(kv.second));
            if(it == std::end(reverse)) {
                auto&& node = reverse[relatedPtrs.find(kv.second)] = std::make_shared<GraphRep::Node>();
                node->data.insert(kv.first);
            } else {
                it->second->data.insert(kv.first);
            }
        }
        for(auto&& kv : pointsTo) {
            auto from = relatedPtrs.find(kv.first);
            if(!reverse.count(from)) reverse[from] = std::make_shared<GraphRep::Node>();
            auto to = relatedPtrs.find(kv.second);
            if(!reverse.count(to)) reverse[to] = std::make_shared<GraphRep::Node>();
            reverse[from]->child = reverse[to];
        }

        return GraphRep{ util::viewContainer(reverse).map([&](auto&& kv){ return kv.second; }).toVector() };
    }

    void viewGraph() {
        auto G = asGraph();
        llvm::ViewGraph(G, "Stensgaard local AA");
    }
};

} /* namespace borealis */


namespace llvm {

template<class GraphType> struct GraphTraits;
template<class GraphType> struct DOTGraphTraits;

template<>
struct GraphTraits<borealis::LocalStensgaardAA::GraphRep> {
    using graph = borealis::LocalStensgaardAA::GraphRep;
    using NodeType = graph::Node;

    static auto emptyNode() {
        static std::shared_ptr<NodeType> ndd = std::make_shared<NodeType>();
        return ndd;
    }

    static inline NodeType* getEntryNode(const graph& g) {
        if(g.nodes.empty()) return emptyNode().get();
        return g.nodes.front().get();
    }

    using ChildIteratorType = decltype(std::declval<NodeType>().nodes_view().begin());

    static inline ChildIteratorType child_begin(NodeType* N) {
        if(!N) return emptyNode()->nodes_view().begin();
        return N->nodes_view().begin();
    }
    static inline ChildIteratorType child_end(NodeType *N) {
        if(!N) return emptyNode()->nodes_view().end();
        return N->nodes_view().end();
    }

    static auto nodes_view(const graph& T) {
        return T.nodes_view();
    }
    using nodes_iterator = decltype(std::declval<graph>().nodes_view().begin());

    static nodes_iterator nodes_begin(const graph& T) {
        return nodes_view(T).begin();
    }

    static nodes_iterator nodes_end(const graph& T) {
        return nodes_view(T).end();
    }
};

template<>
struct DOTGraphTraits<borealis::LocalStensgaardAA::GraphRep>: DefaultDOTGraphTraits {
    DOTGraphTraits(){}
    DOTGraphTraits(bool){}

    template<typename GraphType>
    std::string getNodeLabel(const borealis::LocalStensgaardAA::GraphRep::Node* node, GraphType &) {
        using namespace borealis;
        if(!node) return "@";
        if(node->data.empty()) return "{}";
        {
            std::ostringstream ostr;
            for(auto&& e: node->data) {
                ostr << e << std::endl;
            }
            return ostr.str();
        }
    }

    template<class T>
    bool IsNodeHidden(T&&) { return false; }
};

} /* namespace llvm */

#include "Util/unmacros.h"

#endif // LOCAL_STENSGAARD_AA
