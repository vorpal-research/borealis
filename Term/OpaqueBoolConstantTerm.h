/*
 * OpaqueBoolConstantTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBOOLCONSTANTTERM_H_
#define OPAQUEBOOLCONSTANTTERM_H_

#include "Protobuf/Gen/Term/OpaqueBoolConstantTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueBoolConstantTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueBoolConstantTerm {
    extend borealis.proto.Term {
        optional OpaqueBoolConstantTerm ext = 22;
    }

    optional bool value = 1;
}

**/
class OpaqueBoolConstantTerm: public borealis::Term {

    bool value;

    OpaqueBoolConstantTerm(Type::Ptr type, bool value):
        Term(
            class_tag(*this),
            type,
            value ? "true" : "false"
        ), value(value) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueBoolConstantTerm);

    bool getValue() const { return value; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueBoolConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueBoolConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getBoolConst(t->getValue());
    }
};



template<class FN>
struct ConverterImpl<OpaqueBoolConstantTerm, proto::OpaqueBoolConstantTerm, FN> {

    typedef Converter<Term, proto::Term, FN> TermConverter;

    static proto::OpaqueBoolConstantTerm* toProtobuf(const OpaqueBoolConstantTerm* t) {
        auto res = util::uniq(new proto::OpaqueBoolConstantTerm());
        res->set_value(t->getValue());
        return res.release();
    }

    static Term::Ptr fromProtobuf(
            FN,
            Type::Ptr type,
            const std::string&,
            const proto::OpaqueBoolConstantTerm& t) {
        auto value = t.value();
        return Term::Ptr{ new OpaqueBoolConstantTerm(type, value) };
    }
};

} /* namespace borealis */

#endif /* OPAQUEBOOLCONSTANTTERM_H_ */
