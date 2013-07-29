/*
 * ReturnValueTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef RETURNVALUETERM_H_
#define RETURNVALUETERM_H_

#include <llvm/Function.h>

#include "Term/Term.h"
#include "Util/slottracker.h"

namespace borealis {

class ReturnValueTerm: public borealis::Term {

    ReturnValueTerm(Type::Ptr type, const std::string& functionName) :
        Term(
            class_tag(*this),
            type,
            "\\result_" + functionName
        ) {};

public:

    MK_COMMON_TERM_IMPL(ReturnValueTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

template<class Impl>
struct SMTImpl<Impl, ReturnValueTerm> {
    static Dynamic<Impl> doit(
            const ReturnValueTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* RETURNVALUETERM_H_ */
