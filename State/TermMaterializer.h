/*
 * TermMaterializer.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef TERMMATERIALIZER_H_
#define TERMMATERIALIZER_H_

#include <sstream>

#include "Predicate/PredicateFactory.h"
#include "Term/TermFactory.h"

#include "Transformer.hpp"

#include "Util/locations.h"

namespace borealis {

class TermMaterializer: public borealis::Transformer<TermMaterializer> {
public:
    TermMaterializer();
    virtual ~TermMaterializer();

    llvm::Value* forName(const std::string& name);
    const NameContext& nameContext();
    TermFactory& factory();

    void failWith(const std::string& message);
    inline void failWith(llvm::Twine twine) {
        return failWith(twine.str());
    }
    // resolve ambiguity
    inline void failWith(const char* r) {
        return failWith(std::string(r));
    }

    Term::Ptr transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr trm) {
        return trm; // FIXME: return ConstantInt;
    }

    Term::Ptr transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr trm) {
        return trm; // FIXME: return Constant;
    }

    Term::Ptr transformOpaqueVarTerm(OpaqueVarTermPtr trm) {
        auto ret = forName(trm->getName());
        return factory().getValueTerm(ret);
    }

    Term::Ptr transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr trm) {
        const llvm::StringRef name = trm->getName();
        const auto& ctx = nameContext();

        if(name == "result") {
            if(ctx.func && ctx.placement == NameContext::Placement::OuterScope)
                return factory().getReturnValueTerm(ctx.func);
            else {
                failWith("\result can only be bound to functions' outer scope");
                return trm;
            }
        } else if(name.startswith("arg")) {
            if(ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
                auto num = name.drop_front(3);
                std::istringstream ist(num.str());

                unsigned val = 0U;
                ist >> val;

                auto argIt = ctx.func->arg_begin();
                std::advance(argIt, val);

                return factory().getArgumentTerm(argIt);
            } else {
                failWith("\arg# can only be bound to functions' outer scope");
                return trm;
            }
        } else {
            failWith("\\" + name + ": unknown builtin");
            return trm;
        }
    }
};

} /* namespace borealis */
#endif /* TERMMATERIALIZER_H_ */
