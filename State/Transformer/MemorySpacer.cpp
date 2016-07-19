#include "State/Transformer/MemorySpacer.h"

#include "Util/macros.h"

namespace borealis {

size_t MemorySpacer::getIndex(void* token) {
    if(token == nullptr) {
        return 0;
    }
    auto&& it = indices.find(token);
    if(it == std::end(indices)) {
        auto ix = indices[token] = currentIndex;
        currentIndex++;
        return ix;
    } else return it->second;
}

size_t MemorySpacer::getMemspace(Term::Ptr t) {
    return getIndex(aa.getDereferenced(t));
}

MemorySpacer::MemorySpacer(FactoryNest FN, PredicateState::Ptr space): Base{FN}, aa{FN}, indices{}, currentIndex{1} {
    dbgs() << "State " << endl << space << endl;
    aa.transform(space);
    dbgs() << "Stensgaard results: " << endl << aa << endl;
    //aa.viewGraph();
}

Term::Ptr MemorySpacer::transformTerm(Term::Ptr t) {
    //if(aa.isNamedTerm(t)) {
        if(auto&& ptr = llvm::dyn_cast<type::Pointer>(t->getType())) {
            auto&& space = getMemspace(t);
            if(not (space == ptr->getMemspace() || 0 == ptr->getMemspace())) {
                aa.viewGraph();
            }
            ASSERT(
                space == ptr->getMemspace() || 0 == ptr->getMemspace(),
                tfm::format("Illegal memory space: %d for term %s: term is already in space %d", space, t, ptr->getMemspace())
            );
            dbgs() << t << " remapped to memory space " << space << endl;
            return t->setType(FN.Type->getPointer(ptr->getPointed(), space));
        }
    //}
    return Base::transformTerm(t);
}

} /* namespace borealis */

#include "Util/unmacros.h"
