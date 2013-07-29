/*
 * Converter.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef PROTOBUF_CONVERTER_HPP_
#define PROTOBUF_CONVERTER_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Type/Type.h"
#include "Type/Type.def"
#include "Type/ConverterImpl.hpp"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t);
Type::Ptr    deprotobuffy(FactoryNest FN, const proto::Type& t);

template<class FN>
struct Converter<Type, proto::Type, FN> {

    ////////////////////////////////////////////////////////////////////////////
    // Type::Ptr
    ////////////////////////////////////////////////////////////////////////////
    static Type::ProtoPtr toProtobuf(Type::Ptr t) {
        auto res = util::uniq(new proto::Type());

        if (false) {}
#define HANDLE_TYPE(NAME, CLASS) \
        else if (auto* tt = llvm::dyn_cast<type::CLASS>(t)) { \
            auto* proto = ConverterImpl<type::CLASS, type::proto::CLASS, FN> \
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

    static Type::Ptr fromProtobuf(FN fn, const proto::Type& t) {
#define HANDLE_TYPE(NAME, CLASS) \
        if (t.HasExtension(type::proto::CLASS::ext)) { \
            const auto& ext = t.GetExtension(type::proto::CLASS::ext); \
            return ConverterImpl<type::CLASS, type::proto::CLASS, FN> \
                   ::fromProtobuf(fn, ext); \
        }
#include "Type/Type.def"
        BYE_BYE(Type::Ptr, "Should not happen!");
    }

};

template<class FN>
struct Converter<Term, proto::Term, FN> {

    ////////////////////////////////////////////////////////////////////////////
    // Term::Ptr
    ////////////////////////////////////////////////////////////////////////////
    static Term::ProtoPtr toProtobuf(Term::Ptr t) {
        auto res = util::uniq(new proto::Term());

        if (false) {}
#define HANDLE_TERM(NAME, CLASS) \
        else if (auto* tt = llvm::dyn_cast<CLASS>(t)) { \
            auto* proto = ConverterImpl<CLASS, proto::CLASS, FN> \
                          ::toProtobuf(tt); \
            res->SetAllocatedExtension( \
                proto::CLASS::ext, \
                proto \
            ); \
        }
#include "Term/Term.def"
        else BYE_BYE(Term::ProtoPtr, "Should not happen!");

        return std::move(res);
    }

    static Term::Ptr fromProtobuf(FN fn, const proto::Term& t) {

        auto type = Converter<Type, proto::Type, FN>::fromProtobuf(fn, t.type());
        const auto& name = t.name();

#define HANDLE_TERM(NAME, CLASS) \
        if (t.HasExtension(proto::CLASS::ext)) { \
            const auto& ext = t.GetExtension(proto::CLASS::ext); \
            return ConverterImpl<CLASS, proto::CLASS, FN> \
                   ::fromProtobuf(fn, type, name, ext); \
        }
#include "Term/Term.def"
        BYE_BYE(Term::Ptr, "Should not happen!");
    }

};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PROTOBUF_CONVERTER_HPP_ */
