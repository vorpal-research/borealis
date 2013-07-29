/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include <llvm/Constant.h>

#include "Term/Term.h"
#include "Util/slottracker.h"

namespace borealis {

class ConstTerm: public borealis::Term {

    util::option<std::string> asString;

    ConstTerm(Type::Ptr type, const std::string& name, util::option<std::string> asString) :
        Term(
                class_tag(*this),
                type,
                name
        ), asString(asString) {};

public:

    MK_COMMON_TERM_IMPL(ConstTerm);

    util::option<std::string> getAsString() const { return asString; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, ConstTerm> {
    static Dynamic<Impl> doit(
            const ConstTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        // FIXME: this is generally fucked up
        return ef.getVarByTypeAndName(t->getType(), t->getName());
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* CONSTTERM_H_ */
