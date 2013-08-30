/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATE_PROTOBUF_CONVERTER_IMPL_HPP_
#define PREDICATE_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Predicate/AllocaPredicate.pb.h"
#include "Protobuf/Gen/Predicate/DefaultSwitchCasePredicate.pb.h"
#include "Protobuf/Gen/Predicate/EqualityPredicate.pb.h"
#include "Protobuf/Gen/Predicate/GlobalsPredicate.pb.h"
#include "Protobuf/Gen/Predicate/InequalityPredicate.pb.h"
#include "Protobuf/Gen/Predicate/MallocPredicate.pb.h"
#include "Protobuf/Gen/Predicate/StorePredicate.pb.h"
#include "Protobuf/Gen/Predicate/WritePropertyPredicate.pb.h"

#include "Factory/Nest.h"

#include "Term/ProtobufConverterImpl.hpp"

#include "Util/util.h"
#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<Predicate> {
    typedef Predicate normal_t;
    typedef proto::Predicate proto_t;
    typedef borealis::FactoryNest context_t;

    static Predicate::ProtoPtr toProtobuf(const normal_t& p);
    static Predicate::Ptr fromProtobuf(const context_t& fn, const proto::Predicate& p);
};

template<>
struct protobuf_traits_impl<AllocaPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::AllocaPredicate> toProtobuf(const AllocaPredicate& p) {
        auto res = util::uniq(new proto::AllocaPredicate());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*p.getLhv()).release()
        );
        res->set_allocated_numelements(
            TermConverter::toProtobuf(*p.getNumElems()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::AllocaPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto numElems = TermConverter::fromProtobuf(fn, p.numelements());
        return Predicate::Ptr{ new AllocaPredicate(lhv, numElems, type) };
    }
};

template<>
struct protobuf_traits_impl<DefaultSwitchCasePredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::DefaultSwitchCasePredicate> toProtobuf(const DefaultSwitchCasePredicate& p) {
        auto res = util::uniq(new proto::DefaultSwitchCasePredicate());
        res->set_allocated_cond(
            TermConverter::toProtobuf(*p.getCond()).release()
        );
        for (const auto& c : p.getCases()) {
            res->mutable_cases()->AddAllocated(
                TermConverter::toProtobuf(*c).release()
            );
        }
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::DefaultSwitchCasePredicate& p) {
        auto cond = TermConverter::fromProtobuf(fn, p.cond());

        std::vector<Term::Ptr> cases;
        cases.reserve(p.cases_size());
        for (const auto& c : p.cases()) {
            cases.push_back(
                TermConverter::fromProtobuf(fn, c)
            );
        }

        return Predicate::Ptr{ new DefaultSwitchCasePredicate(cond, cases, type) };
    }
};

template<>
struct protobuf_traits_impl<EqualityPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::EqualityPredicate> toProtobuf(const EqualityPredicate& p) {
        auto res = util::uniq(new proto::EqualityPredicate());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*p.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*p.getRhv()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::EqualityPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{ new EqualityPredicate(lhv, rhv, type) };
    }
};

template<>
struct protobuf_traits_impl<GlobalsPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::GlobalsPredicate> toProtobuf(const GlobalsPredicate& p) {
        auto res = util::uniq(new proto::GlobalsPredicate());
        for (const auto& g : p.getGlobals()) {
            res->mutable_globals()->AddAllocated(
                TermConverter::toProtobuf(*g).release()
            );
        }
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::GlobalsPredicate& p) {
        std::vector<Term::Ptr> globals;
        globals.reserve(p.globals_size());
        for (const auto& g : p.globals()) {
            globals.push_back(
                TermConverter::fromProtobuf(fn, g)
            );
        }
        return Predicate::Ptr{ new GlobalsPredicate(globals, type) };
    }
};

template<>
struct protobuf_traits_impl<InequalityPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::InequalityPredicate> toProtobuf(const InequalityPredicate& p) {
        auto res = util::uniq(new proto::InequalityPredicate());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*p.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*p.getRhv()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::InequalityPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{ new InequalityPredicate(lhv, rhv, type) };
    }
};

template<>
struct protobuf_traits_impl<MallocPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::MallocPredicate> toProtobuf(const MallocPredicate& p) {
        auto res = util::uniq(new proto::MallocPredicate());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*p.getLhv()).release()
        );
        res->set_allocated_numelements(
            TermConverter::toProtobuf(*p.getNumElems()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::MallocPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto numElems = TermConverter::fromProtobuf(fn, p.numelements());
        return Predicate::Ptr{ new MallocPredicate(lhv, numElems, type) };
    }
};

template<>
struct protobuf_traits_impl<StorePredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::StorePredicate> toProtobuf(const StorePredicate& p) {
        auto res = util::uniq(new proto::StorePredicate());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*p.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*p.getRhv()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::StorePredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{ new StorePredicate(lhv, rhv, type) };
    }
};

template<>
struct protobuf_traits_impl<WritePropertyPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::WritePropertyPredicate> toProtobuf(const WritePropertyPredicate& p) {
        auto res = util::uniq(new proto::WritePropertyPredicate());
        res->set_allocated_propname(
            TermConverter::toProtobuf(*p.getPropertyName()).release()
        );
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*p.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*p.getRhv()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            PredicateType type,
            const proto::WritePropertyPredicate& p) {
        auto propName = TermConverter::fromProtobuf(fn, p.propname());
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{ new WritePropertyPredicate(propName, lhv, rhv, type) };
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* TYPE_PROTOBUF_CONVERTER_IMPL_HPP_ */
