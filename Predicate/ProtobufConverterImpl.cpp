/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "Predicate/ProtobufConverterImpl.hpp"

#include "Util/macros.h"

namespace borealis {
////////////////////////////////////////////////////////////////////////////////
// Predicate::Ptr
////////////////////////////////////////////////////////////////////////////////
Predicate::ProtoPtr protobuf_traits<Predicate>::toProtobuf(const normal_t& p) {
    auto res = util::uniq(new proto_t());

    res->set_type(static_cast<proto::PredicateType>(p.getType()));

    const auto& loc = p.getLocation();
    if ( ! loc.isUnknown()) {
        res->set_allocated_location(
            protobuf_traits<Locus>::toProtobuf(loc).release()
        );
    }

    if (false) {}
#define HANDLE_PREDICATE(NAME, CLASS) \
    else if (auto* pp = llvm::dyn_cast<CLASS>(&p)) { \
        auto proto = protobuf_traits_impl<CLASS> \
                     ::toProtobuf(*pp); \
        res->SetAllocatedExtension( \
            proto::CLASS::ext, \
            proto.release() \
        ); \
    }
#include "Predicate/Predicate.def"
    else BYE_BYE(Predicate::ProtoPtr, "Should not happen!");

    return std::move(res);
}

Predicate::Ptr protobuf_traits<Predicate>::fromProtobuf(const context_t& fn, const proto_t& p) {
    auto type = static_cast<PredicateType>(p.type());
    auto loc = p.has_location()
               ? *protobuf_traits<Locus>::fromProtobuf(nullptr, p.location())
               : Locus();

    Predicate::Ptr base = Predicate::Ptr{ new Predicate(-1UL, type, loc) };

#define HANDLE_PREDICATE(NAME, CLASS) \
    if (p.HasExtension(proto::CLASS::ext)) { \
        const auto& ext = p.GetExtension(proto::CLASS::ext); \
        return protobuf_traits_impl<CLASS> \
               ::fromProtobuf(fn, base, ext); \
    }
#include "Predicate/Predicate.def"
    BYE_BYE(Predicate::Ptr, "Should not happen!");
}

} // namespace borealis

#include "Util/unmacros.h"
