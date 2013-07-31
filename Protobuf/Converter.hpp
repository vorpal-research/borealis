/*
 * Converter.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef PROTOBUF_CONVERTER_HPP_
#define PROTOBUF_CONVERTER_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Predicate/Predicate.h"
#include "Predicate/Predicate.def"
#include "Term/Term.h"
#include "Term/Term.def"
#include "Type/Type.h"
#include "Type/Type.def"
#include "Type/ConverterImpl.hpp"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t);
Type::Ptr    deprotobuffy(FactoryNest FN, const proto::Type& t);

Term::ProtoPtr protobuffy(Term::Ptr t);
Term::Ptr    deprotobuffy(FactoryNest FN, const proto::Term& t);

Predicate::ProtoPtr protobuffy(Predicate::Ptr p);
Predicate::Ptr    deprotobuffy(FactoryNest FN, const proto::Predicate& p);



////////////////////////////////////////////////////////////////////////////////
// Type::Ptr
////////////////////////////////////////////////////////////////////////////////
template<class FN>
struct Converter<Type, proto::Type, FN> {

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

////////////////////////////////////////////////////////////////////////////////
// Term::Ptr
////////////////////////////////////////////////////////////////////////////////
template<class FN>
struct Converter<Term, proto::Term, FN> {

    static Term::ProtoPtr toProtobuf(Term::Ptr t) {
        auto res = util::uniq(new proto::Term());

        res->set_allocated_type(
            Converter<Type, proto::Type, FN>::toProtobuf(t->getType()).release()
        );
        res->set_name(t->getName());

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

////////////////////////////////////////////////////////////////////////////////
// Predicate::Ptr
////////////////////////////////////////////////////////////////////////////////
template<class FN>
struct Converter<Predicate, proto::Predicate, FN> {

    static Predicate::ProtoPtr toProtobuf(Predicate::Ptr p) {
        auto res = util::uniq(new proto::Predicate());

        res->set_type(static_cast<proto::PredicateType>(p->getType()));

        if (false) {}
#define HANDLE_PREDICATE(NAME, CLASS) \
        else if (auto* pp = llvm::dyn_cast<CLASS>(p)) { \
            auto* proto = ConverterImpl<CLASS, proto::CLASS, FN> \
                          ::toProtobuf(pp); \
            res->SetAllocatedExtension( \
                proto::CLASS::ext, \
                proto \
            ); \
        }
#include "Predicate/Predicate.def"
        else BYE_BYE(Predicate::ProtoPtr, "Should not happen!");

        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(FN fn, const proto::Predicate& p) {

        auto type = static_cast<PredicateType>(p.type());

#define HANDLE_PREDICATE(NAME, CLASS) \
        if (p.HasExtension(proto::CLASS::ext)) { \
            const auto& ext = p.GetExtension(proto::CLASS::ext); \
            return ConverterImpl<CLASS, proto::CLASS, FN> \
                   ::fromProtobuf(fn, type, ext); \
        }
#include "Predicate/Predicate.def"
        BYE_BYE(Predicate::Ptr, "Should not happen!");
    }

};

////////////////////////////////////////////////////////////////////////////////
// PredicateState::Ptr
////////////////////////////////////////////////////////////////////////////////
template<class FN>
struct Converter<PredicateState, proto::PredicateState, FN> {

    static PredicateState::ProtoPtr toProtobuf(PredicateState::Ptr ps) {
        auto res = util::uniq(new proto::PredicateState());

        if (false) {}
#define HANDLE_STATE(NAME, CLASS) \
        else if (auto* pps = llvm::dyn_cast<CLASS>(ps)) { \
            auto* proto = ConverterImpl<CLASS, proto::CLASS, FN> \
                          ::toProtobuf(pps); \
            res->SetAllocatedExtension( \
                proto::CLASS::ext, \
                proto \
            ); \
        }
#include "State/PredicateState.def"
        else BYE_BYE(PredicateState::ProtoPtr, "Should not happen!");

        return std::move(res);
    }

    static PredicateState::Ptr fromProtobuf(FN fn, const proto::PredicateState& ps) {
#define HANDLE_STATE(NAME, CLASS) \
        if (ps.HasExtension(proto::CLASS::ext)) { \
            const auto& ext = ps.GetExtension(proto::CLASS::ext); \
            return ConverterImpl<CLASS, proto::CLASS, FN> \
                   ::fromProtobuf(fn, ext); \
        }
#include "State/PredicateState.def"
        BYE_BYE(PredicateState::Ptr, "Should not happen!");
    }

};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PROTOBUF_CONVERTER_HPP_ */
