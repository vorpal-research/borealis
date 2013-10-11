/*
 * TermRebinder.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef TERMREBINDER_H_
#define TERMREBINDER_H_

#include <llvm/Support/Casting.h>

#include "Passes/Tracker/NameTracker.h"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

class TermRebinder : public borealis::Transformer<TermRebinder> {

    typedef borealis::Transformer<TermRebinder> Base;

    llvm::Function* context;
    NameTracker* resolver;

public:

    TermRebinder(
            llvm::Function& context,
            NameTracker* resolver,
            FactoryNest FN) : Base(FN), context(&context), resolver(resolver) {}

    Term::Ptr transformValueTerm(ValueTermPtr t) {
        using namespace llvm;

        auto* v = resolver->resolve(t->getName(), context);
        if (auto* ri = dyn_cast_or_null<ReturnInst>(v))
            return FN.Term->getReturnValueTerm(ri->getParent()->getParent());
        else if (v)
            return FN.Term->getValueTerm(v);
        else
            return t;
    }

};

} /* namespace borealis */

#endif /* TERMREBINDER_H_ */
