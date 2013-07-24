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

#include "Type/TypeFactory.h"

#include "Util/util.h"

namespace borealis {

template<class _>
struct ConverterImpl<type::Bool, type::proto::Bool, _> {
    static type::proto::Bool* toProtobuf(const type::Bool*) {
        return util::uniq(new type::proto::Bool()).release();
    }
};

template<class _>
struct ConverterImpl<type::Float, type::proto::Float, _> {
    static type::proto::Float* toProtobuf(const type::Float*) {
        return util::uniq(new type::proto::Float()).release();
    }
};

template<class _>
struct ConverterImpl<type::Integer, type::proto::Integer, _> {
    static type::proto::Integer* toProtobuf(const type::Integer*) {
        return util::uniq(new type::proto::Integer()).release();
    }
};

template<class _>
struct ConverterImpl<type::UnknownType, type::proto::UnknownType, _> {
    static type::proto::UnknownType* toProtobuf(const type::UnknownType*) {
        return util::uniq(new type::proto::UnknownType()).release();
    }
};



template<class _>
struct ConverterImpl<type::Pointer, type::proto::Pointer, _> {
    static type::proto::Pointer* toProtobuf(const type::Pointer* p) {
        auto res = util::uniq(new type::proto::Pointer());

        Type::ProtoPtr pointed = Converter<Type::Ptr, Type::ProtoPtr, _>
                                 ::toProtobuf(p->getPointed());
        res->set_allocated_pointed(pointed.release());

        return res.release();
    }
};

template<class _>
struct ConverterImpl<type::TypeError, type::proto::TypeError, _> {
    static type::proto::TypeError* toProtobuf(const type::TypeError* e) {
        auto res = util::uniq(new type::proto::TypeError());

        res->set_message(e->getMessage());

        return res.release();
    }
};

} // namespace borealis

#endif /* TYPE_CONVERTERIMPL_HPP_ */
