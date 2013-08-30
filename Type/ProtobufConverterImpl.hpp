/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef TYPE_PROTOBUF_CONVERTER_IMPL_HPP_
#define TYPE_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Type/Bool.pb.h"
#include "Protobuf/Gen/Type/Float.pb.h"
#include "Protobuf/Gen/Type/Integer.pb.h"
#include "Protobuf/Gen/Type/Pointer.pb.h"
#include "Protobuf/Gen/Type/TypeError.pb.h"
#include "Protobuf/Gen/Type/UnknownType.pb.h"

#include "Factory/Nest.h"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<Type> {
    typedef Type normal_t;
    typedef proto::Type proto_t;
    typedef borealis::FactoryNest context_t;

    static Type::ProtoPtr toProtobuf(const normal_t& t);
    static Type::Ptr fromProtobuf(const context_t& fn, const proto_t& t);
};

#define MK_EMPTY_TYPE_PB_IMPL(NS, T) \
    template<> \
    struct protobuf_traits_impl<NS::T> { \
        typedef NS::T normal_t; \
        typedef NS::proto::T proto_t; \
        typedef FactoryNest context_t; \
\
        static std::unique_ptr<proto_t> toProtobuf(const normal_t&) { \
            return util::uniq(new proto_t()); \
        } \
\
        static normal_t::Ptr fromProtobuf(const context_t& fn, const proto_t&) { \
            return fn.Type->get##T(); \
        } \
    }; \

MK_EMPTY_TYPE_PB_IMPL(type, Bool);
MK_EMPTY_TYPE_PB_IMPL(type, Float);
MK_EMPTY_TYPE_PB_IMPL(type, Integer);
MK_EMPTY_TYPE_PB_IMPL(type, UnknownType);

#undef MK_EMPTY_TYPE_PB_IMPL

template<>
struct protobuf_traits_impl<type::Pointer> {
    typedef type::Pointer normal_t;
    typedef type::proto::Pointer proto_t;
    typedef FactoryNest context_t;

    static std::unique_ptr<proto_t> toProtobuf(const type::Pointer& p) {
        auto res = util::uniq(new type::proto::Pointer());

        Type::ProtoPtr pointed = protobuf_traits<Type>::toProtobuf(*p.getPointed());
        res->set_allocated_pointed(pointed.release());

        return std::move(res);
    }

    static Type::Ptr fromProtobuf(const context_t& fn, const type::proto::Pointer& p) {
        Type::Ptr pointed = protobuf_traits<Type>::fromProtobuf(fn, p.pointed());
        return fn.Type->getPointer(pointed);
    }
};

template<>
struct protobuf_traits_impl<type::TypeError> {
    typedef type::TypeError normal_t;
    typedef type::proto::TypeError proto_t;
    typedef FactoryNest context_t;

    static std::unique_ptr<proto_t> toProtobuf(const type::TypeError& e) {
        auto res = util::uniq(new type::proto::TypeError());
        res->set_message(e.getMessage());
        return res;
    }

    static Type::Ptr fromProtobuf(const context_t& fn, const type::proto::TypeError& p) {
        const auto& msg = p.message();
        return fn.Type->getTypeError(msg);
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* TYPE_PROTOBUF_CONVERTER_IMPL_HPP_ */
