//
// Created by belyaev on 4/20/15.
//

#include "State/Transformer/TermValueMapper.h"

#include "Util/functional.hpp"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {

Term::Ptr TermValueMapper::transformValueTerm(ValueTermPtr trm) {
    if(auto&& local = this->FN.Slot->getLocalValue(trm->getVName())) {
        mapping[trm] = local;
    } else if(auto&& global = this->FN.Slot->getGlobalValue(trm->getVName())) {
        mapping[trm] = global;
    } else {
        UNREACHABLE(tfm::format("Variable \"%s\" not found", trm->getVName()));
        return nullptr;
    }
    return trm;
}

Term::Ptr TermValueMapper::transformArgumentTerm(ArgumentTermPtr trm) {
    auto numArg = trm->getIdx();
    auto func = context->getParent()->getParent();
    mapping[trm] = util::viewContainer(func->args()).drop(numArg).map(ops::take_pointer).first_or(nullptr);
    return trm;
}

Term::Ptr TermValueMapper::transformReturnValueTerm(ReturnValueTermPtr trm) {
    auto termInst = getSingleReturnFor(context);
    if(auto returnInst = llvm::dyn_cast<llvm::ReturnInst>(termInst)) {
        mapping[trm] = returnInst->getReturnValue();
    } else if(llvm::dyn_cast<llvm::UnreachableInst>(termInst)) {
    } else UNREACHABLE("Unknown return-type instruction");
    return trm;
}


const std::map<Term::Ptr, const llvm::Value*, TermCompare>& TermValueMapper::getMapping() const {
    return mapping;
}
} /* namespace borealis */

#include "Util/unmacros.h"
