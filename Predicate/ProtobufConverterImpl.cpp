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
    auto res = util::uniq(new proto::Predicate());

    res->set_type(static_cast<proto::PredicateType>(p.getType()));

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

Predicate::Ptr protobuf_traits<Predicate>::fromProtobuf(const context_t& fn, const proto::Predicate& p) {
    auto type = static_cast<PredicateType>(p.type());

#define HANDLE_PREDICATE(NAME, CLASS) \
    if (p.HasExtension(proto::CLASS::ext)) { \
        const auto& ext = p.GetExtension(proto::CLASS::ext); \
        return protobuf_traits_impl<CLASS> \
               ::fromProtobuf(fn, type, ext); \
    }
#include "Predicate/Predicate.def"
    BYE_BYE(Predicate::Ptr, "Should not happen!");
}

} // namespace borealis

#include "Util/unmacros.h"

