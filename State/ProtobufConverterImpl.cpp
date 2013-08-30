/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "State/ProtobufConverterImpl.hpp"

#include "Util/macros.h"

namespace borealis {
////////////////////////////////////////////////////////////////////////////////
// PredicateState::Ptr
////////////////////////////////////////////////////////////////////////////////
PredicateState::ProtoPtr protobuf_traits<PredicateState>::toProtobuf(const normal_t& ps) {
    auto res = util::uniq(new proto::PredicateState());

    if (false) {}
#define HANDLE_STATE(NAME, CLASS) \
    else if (auto* pps = llvm::dyn_cast<CLASS>(&ps)) { \
        auto proto = protobuf_traits_impl<CLASS> \
                      ::toProtobuf(*pps); \
        res->SetAllocatedExtension( \
            proto::CLASS::ext, \
            proto.release() \
        ); \
    }
#include "State/PredicateState.def"
    else BYE_BYE(PredicateState::ProtoPtr, "Should not happen!");

    return std::move(res);
}

PredicateState::Ptr protobuf_traits<PredicateState>::fromProtobuf(const context_t& fn, const proto::PredicateState& ps) {
#define HANDLE_STATE(NAME, CLASS) \
    if (ps.HasExtension(proto::CLASS::ext)) { \
        const auto& ext = ps.GetExtension(proto::CLASS::ext); \
        return protobuf_traits_impl<CLASS> \
               ::fromProtobuf(fn, ext); \
    }
#include "State/PredicateState.def"
    BYE_BYE(PredicateState::Ptr, "Should not happen!");
}

} // namespace borealis

#include "Util/unmacros.h"
