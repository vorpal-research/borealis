/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "Term/ProtobufConverterImpl.hpp"

#include "Util/macros.h"

namespace borealis {
////////////////////////////////////////////////////////////////////////////////
// Term::Ptr
////////////////////////////////////////////////////////////////////////////////
Term::ProtoPtr protobuf_traits<Term>::toProtobuf(const normal_t& t) {

    auto res = util::uniq(new proto_t());

    res->set_allocated_type(
        protobuf_traits<Type>::toProtobuf(*t.getType()).release()
    );
    res->set_name(t.getName());

    if (false) {}
#define HANDLE_TERM(NAME, CLASS) \
    else if (auto* tt = llvm::dyn_cast<CLASS>(&t)) { \
        auto proto = protobuf_traits_impl<CLASS> \
                     ::toProtobuf(*tt); \
        res->SetAllocatedExtension( \
            proto::CLASS::ext, \
            proto.release() \
        ); \
    }
#include "Term/Term.def"
    else BYE_BYE(Term::ProtoPtr, "Should not happen!");

    return std::move(res);
}

Term::Ptr protobuf_traits<Term>::fromProtobuf(const context_t& fn, const proto_t& t) {

    auto type = protobuf_traits<Type>::fromProtobuf(fn, t.type());
    const auto& name = t.name();

#define HANDLE_TERM(NAME, CLASS) \
    if (t.HasExtension(proto::CLASS::ext)) { \
        const auto& ext = t.GetExtension(proto::CLASS::ext); \
        return protobuf_traits_impl<CLASS> \
               ::fromProtobuf(fn, type, name, ext); \
    }
#include "Term/Term.def"
    BYE_BYE(Term::Ptr, "Should not happen!");
}

} // namespace borealis

#include "Util/unmacros.h"
