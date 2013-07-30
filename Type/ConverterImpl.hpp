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

template<class FN>
struct ConverterImpl<type::Bool, type::proto::Bool, FN> {
    static type::proto::Bool* toProtobuf(const type::Bool*) {
        return util::uniq(new type::proto::Bool()).release();
    }

    static Type::Ptr fromProtobuf(FN fn, const type::proto::Bool&) {
        return fn.Type->getBool();
    }
};

template<class FN>
struct ConverterImpl<type::Float, type::proto::Float, FN> {
    static type::proto::Float* toProtobuf(const type::Float*) {
        return util::uniq(new type::proto::Float()).release();
    }

    static Type::Ptr fromProtobuf(FN fn, const type::proto::Float&) {
        return fn.Type->getFloat();
    }
};

template<class FN>
struct ConverterImpl<type::Integer, type::proto::Integer, FN> {
    static type::proto::Integer* toProtobuf(const type::Integer*) {
        return util::uniq(new type::proto::Integer()).release();
    }

    static Type::Ptr fromProtobuf(FN fn, const type::proto::Integer&) {
        return fn.Type->getInteger();
    }
};

template<class FN>
struct ConverterImpl<type::UnknownType, type::proto::UnknownType, FN> {
    static type::proto::UnknownType* toProtobuf(const type::UnknownType*) {
        return util::uniq(new type::proto::UnknownType()).release();
    }

    static Type::Ptr fromProtobuf(FN fn, const type::proto::UnknownType&) {
        return fn.Type->getUnknown();
    }
};



template<class FN>
struct ConverterImpl<type::Pointer, type::proto::Pointer, FN> {
    static type::proto::Pointer* toProtobuf(const type::Pointer* p) {
        auto res = util::uniq(new type::proto::Pointer());

        Type::ProtoPtr pointed = Converter<Type, proto::Type, FN>
                                 ::toProtobuf(p->getPointed());
        res->set_allocated_pointed(pointed.release());

        return res.release();
    }

    static Type::Ptr fromProtobuf(FN fn, const type::proto::Pointer& p) {
        Type::Ptr pointed = Converter<Type, proto::Type, FN>
                            ::fromProtobuf(fn, p.pointed());
        return fn.Type->getPointer(pointed);
    }
};

template<class FN>
struct ConverterImpl<type::TypeError, type::proto::TypeError, FN> {
    static type::proto::TypeError* toProtobuf(const type::TypeError* e) {
        auto res = util::uniq(new type::proto::TypeError());
        res->set_message(e->getMessage());
        return res.release();
    }

    static Type::Ptr fromProtobuf(FN fn, const type::proto::TypeError& p) {
        const auto& msg = p.message();
        return fn.Type->getTypeError(msg);
    }
};

} // namespace borealis

#endif /* TYPE_CONVERTERIMPL_HPP_ */
