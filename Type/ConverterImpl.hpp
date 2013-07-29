/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef TYPE_CONVERTERIMPL_HPP_
#define TYPE_CONVERTERIMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Type/Bool.pb.h"
#include "Protobuf/Gen/Type/Float.pb.h"
#include "Protobuf/Gen/Type/Integer.pb.h"
#include "Protobuf/Gen/Type/Pointer.pb.h"
#include "Protobuf/Gen/Type/TypeError.pb.h"
#include "Protobuf/Gen/Type/UnknownType.pb.h"

#include "Factory/Nest.h"

#include "Util/util.h"

namespace borealis {

template<class _>
struct ConverterImpl<type::Bool, type::proto::Bool, _> {
    static type::proto::Bool* toProtobuf(const type::Bool*) {
        return util::uniq(new type::proto::Bool()).release();
    }

    static Type::Ptr fromProtobuf(FactoryNest FN, const type::proto::Bool&) {
        return FN.Type->getBool();
    }
};

template<class _>
struct ConverterImpl<type::Float, type::proto::Float, _> {
    static type::proto::Float* toProtobuf(const type::Float*) {
        return util::uniq(new type::proto::Float()).release();
    }

    static Type::Ptr fromProtobuf(FactoryNest FN, const type::proto::Float&) {
        return FN.Type->getFloat();
    }
};

template<class _>
struct ConverterImpl<type::Integer, type::proto::Integer, _> {
    static type::proto::Integer* toProtobuf(const type::Integer*) {
        return util::uniq(new type::proto::Integer()).release();
    }

    static Type::Ptr fromProtobuf(FactoryNest FN, const type::proto::Integer&) {
        return FN.Type->getInteger();
    }
};

template<class _>
struct ConverterImpl<type::UnknownType, type::proto::UnknownType, _> {
    static type::proto::UnknownType* toProtobuf(const type::UnknownType*) {
        return util::uniq(new type::proto::UnknownType()).release();
    }

    static Type::Ptr fromProtobuf(FactoryNest FN, const type::proto::UnknownType&) {
        return FN.Type->getUnknown();
    }
};



template<class _>
struct ConverterImpl<type::Pointer, type::proto::Pointer, _> {
    static type::proto::Pointer* toProtobuf(const type::Pointer* p) {
        auto res = util::uniq(new type::proto::Pointer());

        Type::ProtoPtr pointed = Converter<Type, proto::Type, _>
                                 ::toProtobuf(p->getPointed());
        res->set_allocated_pointed(pointed.release());

        return res.release();
    }

    static Type::Ptr fromProtobuf(FactoryNest FN, const type::proto::Pointer& p) {
        Type::Ptr pointed = Converter<Type, proto::Type, _>
                            ::fromProtobuf(FN, p.pointed());
        return FN.Type->getPointer(pointed);
    }
};

template<class _>
struct ConverterImpl<type::TypeError, type::proto::TypeError, _> {
    static type::proto::TypeError* toProtobuf(const type::TypeError* e) {
        auto res = util::uniq(new type::proto::TypeError());
        res->set_message(e->getMessage());
        return res.release();
    }

    static Type::Ptr fromProtobuf(FactoryNest FN, const type::proto::TypeError& p) {
        const auto& msg = p.message();
        return FN.Type->getTypeError(msg);
    }
};

} // namespace borealis

#endif /* TYPE_CONVERTERIMPL_HPP_ */
