/*
 * OpaqueBuiltinTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBUILTINTERM_H_
#define OPAQUEBUILTINTERM_H_

#include "Protobuf/Gen/Term/OpaqueBuiltinTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueBuiltinTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueBuiltinTerm {
    extend borealis.proto.Term {
        optional OpaqueBuiltinTerm ext = 23;
    }

    optional string vname = 1;
}

**/
class OpaqueBuiltinTerm: public borealis::Term {

    const std::string vname;

    OpaqueBuiltinTerm(Type::Ptr type, const std::string& vname):
        Term(
            class_tag(*this),
            type,
            vname
        ), vname(vname) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueBuiltinTerm);

    const std::string& getVName() const { return vname; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueBuiltinTerm> {
    static Dynamic<Impl> doit(
            const OpaqueBuiltinTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"



template<class FN>
struct ConverterImpl<OpaqueBuiltinTerm, proto::OpaqueBuiltinTerm, FN> {

    typedef Converter<Term, proto::Term, FN> TermConverter;

    static proto::OpaqueBuiltinTerm* toProtobuf(const OpaqueBuiltinTerm* t) {
        auto res = util::uniq(new proto::OpaqueBuiltinTerm());
        res->set_vname(t->getVName());
        return res.release();
    }

    static Term::Ptr fromProtobuf(
            FN,
            Type::Ptr type,
            const std::string&,
            const proto::OpaqueBuiltinTerm& t) {
        auto vname = t.vname();
        return Term::Ptr{ new OpaqueBuiltinTerm(type, vname) };
    }
};

} /* namespace borealis */

#endif /* OPAQUEBUILTINTERM_H_ */
