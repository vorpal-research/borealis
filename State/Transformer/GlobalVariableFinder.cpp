//
// Created by abdullin on 10/26/17.
//

#include "GlobalVariableFinder.h"

namespace borealis {

GlobalVariableFinder::GlobalVariableFinder(FactoryNest FN) : Transformer(FN) {}

Term::Ptr GlobalVariableFinder::transformValueTerm(Transformer::ValueTermPtr term) {
    if (term->isGlobal()) {
        auto val = FN.Slot->getGlobalValue(term->getVName());

        if (val && llvm::isa<llvm::GlobalVariable>(val)) globals.insert(val);
    }
    return term;
}

const std::unordered_set<const llvm::Value*> GlobalVariableFinder::getGlobals() const {
    return globals;
}

}   // namespace borealis