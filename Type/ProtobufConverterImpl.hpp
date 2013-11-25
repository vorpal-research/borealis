/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef TYPE_PROTOBUF_CONVERTER_IMPL_HPP_
#define TYPE_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Type/Array.pb.h"
#include "Protobuf/Gen/Type/Bool.pb.h"
#include "Protobuf/Gen/Type/Float.pb.h"
#include "Protobuf/Gen/Type/Integer.pb.h"
#include "Protobuf/Gen/Type/Pointer.pb.h"
#include "Protobuf/Gen/Type/Record.pb.h"
#include "Protobuf/Gen/Type/RecordBodyRef.pb.h"
#include "Protobuf/Gen/Type/TypeError.pb.h"
#include "Protobuf/Gen/Type/UnknownType.pb.h"

#include "Type/TypeVisitor.hpp"

#include "Factory/Nest.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits_impl<Type> {
    typedef Type normal_t;
    typedef proto::Type proto_t;
    typedef borealis::FactoryNest context_t;

    struct HandlingContext {};

    static Type::ProtoPtr toProtobuf(const normal_t& t);
    static Type::Ptr fromProtobuf(const context_t& fn, const proto_t& t);
};

template<>
struct protobuf_traits<Type> {
    typedef Type normal_t;
    typedef proto::Type proto_t;
    typedef borealis::FactoryNest context_t;

    static Type::ProtoPtr toProtobuf(const normal_t& t) {
        return protobuf_traits_impl<Type>::toProtobuf(t);
    }
    static Type::Ptr fromProtobuf(const context_t& fn, const proto_t& t) {
        return protobuf_traits_impl<Type>::fromProtobuf(fn, t);
    }
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
    };

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

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());

        auto pointed = protobuf_traits_impl<Type>::toProtobuf(*p.getPointed());
        res->set_allocated_pointed(pointed.release());

        return std::move(res);
    }

    static Type::Ptr fromProtobuf(const context_t& fn, const proto_t& p) {
        auto pointed = protobuf_traits<Type>::fromProtobuf(fn, p.pointed());
        return fn.Type->getPointer(pointed);
    }
};

template<>
struct protobuf_traits_impl<type::Array> {
    typedef type::Array normal_t;
    typedef type::proto::Array proto_t;
    typedef FactoryNest context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());

        auto element = protobuf_traits_impl<Type>::toProtobuf(*p.getElement());
        res->set_allocated_element(element.release());

        auto optsize = p.getSize();
        for(auto size : optsize) res->set_size(size);

        return std::move(res);
    }

    static Type::Ptr fromProtobuf(const context_t& fn, const proto_t& p) {
        auto elem = protobuf_traits<Type>::fromProtobuf(fn, p.element());
        return p.has_size() ?
            fn.Type->getArray(elem, p.size()) :
            fn.Type->getArray(elem);
    }
};

template<>
struct protobuf_traits_impl<type::Record> {
    typedef type::Record normal_t;
    typedef type::proto::Record proto_t;
    typedef FactoryNest context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        res->set_name(p.getName());

        // Crazy shit starts here
        // XXX: make this a thread-local static
        //      if we ever decide to do threads and shit
        static bool discardAllBodies = false;
        if(discardAllBodies) return res;
        discardAllBodies = true;
        ON_SCOPE_EXIT(discardAllBodies = false)
        // Crazy shit ends here

        auto pbodyref = util::uniq(new proto::RecordBodyRef());
        pbodyref->set_id(p.getName());

        auto registry = p.getBody()->getRegistry().lock();

        RecordNameCollector names;
        names.visit(p);

        for(const auto& name : names.getNames()) {
            auto pbody = util::uniq(new proto::RecordBodyRef_RecordBody());
            pbody->set_id(name);

            auto fields = registry->at(name);
            ASSERTC(!fields.empty());

            for (const auto& field : fields.getUnsafe()) {
                auto pfield = util::uniq(new proto::RecordBodyRef_RecordField());

                auto fieldType = protobuf_traits_impl<Type>::toProtobuf(*field.getType());
                pfield->set_allocated_type(fieldType.release());

                for(const auto& id : field.getIds()) {
                    pfield->mutable_ids()->AddAllocated(new std::string{id});
                }

                pbody->mutable_fields()->AddAllocated(pfield.release());
            }

            pbodyref->mutable_bodytable()->AddAllocated(pbody.release());
        }

        res->set_allocated_body(pbodyref.release());

        return std::move(res);
    }

    static Type::Ptr fromProtobuf(const context_t& fn, const proto_t& p) {
        // Crazy shit starts here
        // XXX: make this a thread-local static
        //      if we ever decide to do threads and shit
        static bool discardAllBodies = false;
        if(discardAllBodies) return fn.Type->getRecord(p.name());
        discardAllBodies = true;
        ON_SCOPE_EXIT(discardAllBodies = false)
        // Crazy shit ends here

        for(const auto& pbody : p.body().bodytable()) {
            type::RecordBody body;
            for (const auto& pfield : pbody.fields()) {
                type::RecordField field {
                    protobuf_traits_impl<Type>::fromProtobuf(fn, pfield.type()),
                    std::vector<std::string>{ pfield.ids().begin(), pfield.ids().end() }
                };
                body.push_back(field);
            }
            fn.Type->getRecord(pbody.id(), body); // just for the side effects
        }

        return fn.Type->getRecord(p.name());
    }
};

template<>
struct protobuf_traits_impl<type::TypeError> {
    typedef type::TypeError normal_t;
    typedef type::proto::TypeError proto_t;
    typedef FactoryNest context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        res->set_message(p.getMessage());
        return res;
    }

    static Type::Ptr fromProtobuf(const context_t& fn, const proto_t& p) {
        return fn.Type->getTypeError(p.message());
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* TYPE_PROTOBUF_CONVERTER_IMPL_HPP_ */
