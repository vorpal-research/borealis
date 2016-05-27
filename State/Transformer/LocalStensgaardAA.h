//
// Created by belyaev on 5/26/16.
//
#ifndef LOCAL_STENSGAARD_AA
#define LOCAL_STENSGAARD_AA

#include <unordered_map>

#include "Util/disjoint_sets.hpp"

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class LocalAABase {
public:
    virtual bool mayAlias(Term::Ptr, Term::Ptr) { return true; }
    virtual void prepare(PredicateState::Ptr) {}

    virtual ~LocalAABase() {}
};

class LocalStensgaardAA: public Transformer<LocalStensgaardAA>, public LocalAABase {

    using token = util::subset<Term::Ptr>*;

    util::disjoint_set<Term::Ptr> relatedPtrs;
    std::unordered_map<Term::Ptr, token, std::hash<Term::Ptr>, Term::DerefEqualsTo> view;
    std::unordered_map<token, token> pointsTo;
    Term::Set noalias;
    Term::Set nonFreeTerms;
    std::unordered_map<Type::Ptr, token> freeSpaces;

public:
    LocalStensgaardAA(FactoryNest FN):
        Transformer<LocalStensgaardAA>(FN) {}

    static bool isNamedTerm(Term::Ptr t) {
        return llvm::is_one_of<
            ValueTerm,
            ConstTerm,
            ArgumentTerm,
            VarArgumentTerm,
            OpaqueVarTerm,
            ReturnValueTerm,
            ReturnPtrTerm
        >(t);
    }

    token quasi() {
        return relatedPtrs.emplace(nullptr);
    }

    token join(token& l, token& r) {
        if(l and r) return l = r = relatedPtrs.unite(l, r);
        if(l) return r = l;
        if(r) return l = r;
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
                }
            } else if(!nonFreeTerms.count(t)) {
                // this is a free term
                join(freeSpaces[t->getType()], token);
            }
            return token;
        }
    }

    Predicate::Ptr transformEquality(EqualityPredicatePtr pred) {
        auto lhvt = pred->getLhv();
        if(!isNamedTerm(lhvt)) return pred;
        nonFreeTerms.insert(lhvt);
        auto ls = get(lhvt);

        if(auto&& load = llvm::dyn_cast<LoadTerm>(pred->getRhv())) {
            auto loads = get(load->getRhv());
            join(pointsTo[loads], ls);
        } else {
            auto allTerms = Term::getFullTermSet(pred->getRhv());
            for(auto&& r: allTerms) {
                auto rs = get(r);
                join(pointsTo[ls], pointsTo[rs]);
            }
        }
        return pred;
    }

    Predicate::Ptr transformAlloca(AllocaPredicatePtr al) {
        auto lhvt = al->getLhv();
        noalias.insert(lhvt);
        nonFreeTerms.insert(lhvt);

        auto ls = get(lhvt);
        pointsTo[ls] = nullptr; // we cannot do anything better now =(
        return al;
    }

    Predicate::Ptr transformMalloc(MallocPredicatePtr al) {
        auto lhvt = al->getLhv();
        noalias.insert(lhvt);
        nonFreeTerms.insert(lhvt);

        auto ls = get(lhvt);
        pointsTo[ls] = nullptr; // we cannot do anything better now =(
        return al;
    }

    Predicate::Ptr transformStore(StorePredicatePtr store) {
        auto ls = get(store->getLhv());
        auto rs = get(store->getRhv());
        join(pointsTo[ls], rs);
        return store;
    }

    bool mayAlias(Term::Ptr l, Term::Ptr r) override {
        if(*l == *r) return true;

        bool lno_alias = noalias.count(l);
        bool rno_alias = noalias.count(r);
        if(lno_alias && rno_alias) return false;

        auto ls = pointsTo[get(l)];
        auto rs = pointsTo[get(r)];
        if(ls and rs) return ls->getRoot() == rs->getRoot();
        else return false;
    }

    virtual void prepare(PredicateState::Ptr ps) override {
        this->transform(ps);
    }
};

} /* namespace borealis */

#endif // LOCAL_STENSGAARD_AA
