/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "Annotation/ProtobufConverterImpl.hpp"

#include "Util/macros.h"

namespace borealis {
////////////////////////////////////////////////////////////////////////////////
// Annotation::Ptr
////////////////////////////////////////////////////////////////////////////////
Annotation::ProtoPtr protobuf_traits<Annotation>::toProtobuf(const normal_t& t) {
    auto res = util::uniq(new proto_t());

    res->set_allocated_locus(LocusConverter::toProtobuf(t.getLocus()).release());

    if (false) {}
#define HANDLE_ANNOTATION_WITH_BASE(KW, BASE, NAME, CLASS) // ignore the non-standard bases
#define HANDLE_ANNOTATION(KW, NAME, CLASS) \
    else if (auto* tt = llvm::dyn_cast<CLASS>(&t)) { \
        auto proto = protobuf_traits_impl<CLASS> \
                     ::toProtobuf(*tt); \
        res->SetAllocatedExtension( \
            proto::CLASS::ext, \
            proto.release() \
        ); \
    }
#include "Annotation/Annotation.def"
    else BYE_BYE(Annotation::ProtoPtr, "Should not happen!");

    return std::move(res);
}

Annotation::Ptr protobuf_traits<Annotation>::fromProtobuf(const context_t& fn, const proto_t& t) {
    auto locus = LocusConverter::fromProtobuf(nullptr, t.locus());

#define HANDLE_ANNOTATION_WITH_BASE(KW, BASE, NAME, CLASS) // ignore the non-standard bases
#define HANDLE_ANNOTATION(KW, NAME, CLASS) \
    if (t.HasExtension(proto::CLASS::ext)) { \
        const auto& ext = t.GetExtension(proto::CLASS::ext); \
        return protobuf_traits_impl<CLASS> \
               ::fromProtobuf(fn, *locus, ext); \
    }
#include "Annotation/Annotation.def"
    BYE_BYE(Annotation::Ptr, "Should not happen!");
}

} // namespace borealis

#include "Util/unmacros.h"
