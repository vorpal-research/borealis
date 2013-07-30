/*
 * OpaqueVarTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEVARTERM_H_
#define OPAQUEVARTERM_H_

#include "Protobuf/Gen/Term/OpaqueVarTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueVarTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueVarTerm {
    extend borealis.proto.Term {
        optional OpaqueVarTerm ext = 28;
    }

    optional string vname = 1;
}

**/
class OpaqueVarTerm: public borealis::Term {

    std::string vname;

    OpaqueVarTerm(Type::Ptr type, const std::string& vname):
        Term(
            class_tag(*this),
            type,
            vname
        ), vname(vname) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueVarTerm);

    const std::string& getVName() const { return vname; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueVarTerm> {
    static Dynamic<Impl> doit(
            const OpaqueVarTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"



template<class FN>
struct ConverterImpl<OpaqueVarTerm, proto::OpaqueVarTerm, FN> {

    typedef Converter<Term, proto::Term, FN> TermConverter;

    static proto::OpaqueVarTerm* toProtobuf(const OpaqueVarTerm* t) {
        auto res = util::uniq(new proto::OpaqueVarTerm());
        res->set_vname(t->getVName());
        return res.release();
    }

    static Term::Ptr fromProtobuf(
            FN,
            Type::Ptr type,
            const std::string&,
            const proto::OpaqueVarTerm& t) {
        auto vname = t.vname();
        return Term::Ptr{ new OpaqueVarTerm(type, vname) };
    }
};

} /* namespace borealis */

#endif /* OPAQUEVARTERM_H_ */
