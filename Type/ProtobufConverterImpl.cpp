/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "Type/ProtobufConverterImpl.hpp"

#include "Util/macros.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////
// Type
////////////////////////////////////////////////////////////////////////////////
Type::ProtoPtr protobuf_traits_impl<Type>::toProtobuf(const normal_t& t) {
    auto res = util::uniq(new proto_t());

    if (false) {}
#define HANDLE_TYPE(NAME, CLASS) \
    else if (auto* tt = llvm::dyn_cast<type::CLASS>(&t)) { \
        auto proto = protobuf_traits_impl<type::CLASS> \
                     ::toProtobuf(*tt); \
        res->SetAllocatedExtension( \
            type::proto::CLASS::ext, \
            proto.release() \
        ); \
    }
#include "Type/Type.def"
    else BYE_BYE(Type::ProtoPtr, "Should not happen!");

    return std::move(res);
}

Type::Ptr protobuf_traits_impl<Type>::fromProtobuf(const context_t& fn, const proto_t& t) {
#define HANDLE_TYPE(NAME, CLASS) \
    if (t.HasExtension(type::proto::CLASS::ext)) { \
        const auto& ext = t.GetExtension(type::proto::CLASS::ext); \
        return protobuf_traits_impl<type::CLASS> \
               ::fromProtobuf(fn, ext); \
    }
#include "Type/Type.def"
    BYE_BYE(Type::Ptr, "Should not happen!");
}

} // namespace borealis

#include "Util/unmacros.h"
