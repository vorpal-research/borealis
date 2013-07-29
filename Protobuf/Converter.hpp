/*
 * Converter.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef PROTOBUF_CONVERTER_HPP_
#define PROTOBUF_CONVERTER_HPP_

#include "Factory/Nest.h"

#include "Protobuf/ConverterUtil.h"

#include "Type/Type.h"
#include "Type/Type.def"
#include "Type/ConverterImpl.hpp"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t);
Type::Ptr    deprotobuffy(FactoryNest FN, const proto::Type& t);

template<class _>
struct Converter<Type, proto::Type, _> {

    static Type::ProtoPtr toProtobuf(Type::Ptr t) {
        auto res = util::uniq(new proto::Type());

        if (false) {}
#define HANDLE_TYPE(NAME, CLASS) \
        else if (auto* tt = llvm::dyn_cast<type::CLASS>(t)) { \
            auto* proto = ConverterImpl<type::CLASS, type::proto::CLASS, _> \
                          ::toProtobuf(tt); \
            res->SetAllocatedExtension( \
                type::proto::CLASS::ext, \
                proto \
            ); \
        }
#include "Type/Type.def"
        else BYE_BYE(Type::ProtoPtr, "Should not happen!");

        return std::move(res);
    }

    static Type::Ptr fromProtobuf(FactoryNest FN, const proto::Type& t) {
#define HANDLE_TYPE(NAME, CLASS) \
        if (t.HasExtension(type::proto::CLASS::ext)) { \
            const auto& ext = t.GetExtension(type::proto::CLASS::ext); \
            return ConverterImpl<type::CLASS, type::proto::CLASS, _> \
                   ::fromProtobuf(FN, ext); \
        }
#include "Type/Type.def"
        BYE_BYE(Type::Ptr, "Should not happen!");
    }

};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PROTOBUF_CONVERTER_HPP_ */
