/*
 * OpaqueNullPtrTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUENULLPTRTERM_H_
#define OPAQUENULLPTRTERM_H_

#include "Protobuf/Gen/Term/OpaqueNullPtrTerm.pb.h"

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueNullPtrTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueNullPtrTerm {
    extend borealis.proto.Term {
        optional OpaqueNullPtrTerm ext = 26;
    }
}

**/
class OpaqueNullPtrTerm: public borealis::Term {

    OpaqueNullPtrTerm(Type::Ptr type):
        Term(
            class_tag(*this),
            type,
            "<nullptr>"
        ) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueNullPtrTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueNullPtrTerm> {
    static Dynamic<Impl> doit(
            const OpaqueNullPtrTerm*,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getNullPtr();
    }
};



template<class FN>
struct ConverterImpl<OpaqueNullPtrTerm, proto::OpaqueNullPtrTerm, FN> {

    typedef Converter<Term, proto::Term, FN> TermConverter;

    static proto::OpaqueNullPtrTerm* toProtobuf(const OpaqueNullPtrTerm*) {
        return util::uniq(new proto::OpaqueNullPtrTerm()).release();
    }

    static Term::Ptr fromProtobuf(
            FN,
            Type::Ptr type,
            const std::string&,
            const proto::OpaqueNullPtrTerm&) {
        return Term::Ptr{ new OpaqueNullPtrTerm(type) };
    }
};

} /* namespace borealis */

#endif /* OPAQUENULLPTRTERM_H_ */