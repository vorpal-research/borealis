//
// Created by belyaev on 11/5/15.
//

#include "Codegen/CType/ProtobufConverterImpl.hpp"

#include "Util/macros.h"

namespace borealis {

std::unique_ptr<proto::CType> protobuf_traits<CType>::toProtobuf(const normal_t& t) {
    auto res = util::uniq(new proto_t());

    res->set_name(t.getName());

    if (false) {}
#define HANDLE_TYPE(NAME, CLASS) \
    else if (auto* tt = llvm::dyn_cast<CLASS>(&t)) { \
        auto proto = protobuf_traits_impl<CLASS> \
                     ::toProtobuf(*tt); \
        res->SetAllocatedExtension( \
            proto::CLASS::ext, \
            proto.release() \
        ); \
    }
#include "Codegen/CType/CType.def"
    else BYE_BYE(std::unique_ptr<proto_t>, "Should not happen!");

    return std::move(res);
}

CType::Ptr protobuf_traits<CType>::fromProtobuf(context_t fn, const proto_t& t) {
    const auto& name = t.name();

#define HANDLE_TYPE(NAME, CLASS) \
    if (t.HasExtension(proto::CLASS::ext)) { \
        const auto& ext = t.GetExtension(proto::CLASS::ext); \
        return protobuf_traits_impl<CLASS> \
               ::fromProtobuf(fn, name, ext); \
    }
#include "Codegen/CType/CType.def"
    BYE_BYE(CType::Ptr, "Should not happen!");
}

} /* namespace borealis */

#include "Util/unmacros.h"

