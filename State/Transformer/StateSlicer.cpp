/*
 * StateSlicer.cpp
 *
 *  Created on: Mar 16, 2015
 *      Author: ice-phoenix
 */

#include "State/Transformer/StateSlicer.h"
#include "State/Transformer/TermCollector.h"

#include "Util/util.h"

namespace borealis {

StateSlicer::StateSlicer(FactoryNest FN, PredicateState::Ptr query) : Base(FN), query(query) { init(); }

void StateSlicer::init() {
    auto&& tc = TermCollector(FN);
    tc.transform(query);

    util::viewContainer(tc.getTerms())
    .filter([](auto&& t) {
        return llvm::isa<ArgumentTerm>(t) ||
                llvm::isa<ReturnValueTerm>(t) ||
                llvm::isa<ValueTerm>(t);
    })
    .foreach([&](auto&& t) {
        if (llvm::isa<type::Pointer>(t->getType())) {
            slicePtrs.insert(t);
        } else {
            sliceVars.insert(t);
        }
    });
}

Predicate::Ptr StateSlicer::transformPredicate(Predicate::Ptr pred) {
    return pred;
}

} /* namespace borealis */
