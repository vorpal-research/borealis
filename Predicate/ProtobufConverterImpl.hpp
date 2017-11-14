/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATE_PROTOBUF_CONVERTER_IMPL_HPP_
#define PREDICATE_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Predicate/Predicate.pb.h"

#include "Protobuf/Gen/Predicate/AllocaPredicate.pb.h"
#include "Protobuf/Gen/Predicate/CallPredicate.pb.h"
#include "Protobuf/Gen/Predicate/DefaultSwitchCasePredicate.pb.h"
#include "Protobuf/Gen/Predicate/EqualityPredicate.pb.h"
#include "Protobuf/Gen/Predicate/GlobalsPredicate.pb.h"
#include "Protobuf/Gen/Predicate/InequalityPredicate.pb.h"
#include "Protobuf/Gen/Predicate/MallocPredicate.pb.h"
#include "Protobuf/Gen/Predicate/SeqDataPredicate.pb.h"
#include "Protobuf/Gen/Predicate/SeqDataZeroPredicate.pb.h"
#include "Protobuf/Gen/Predicate/StorePredicate.pb.h"
#include "Protobuf/Gen/Predicate/WritePropertyPredicate.pb.h"
#include "Protobuf/Gen/Predicate/WriteBoundPredicate.pb.h"
#include "Protobuf/Gen/Predicate/MarkPredicate.pb.h"

#include "Term/ProtobufConverterImpl.hpp"
#include "Util/ProtobufConverterImpl.hpp"

#include "Factory/Nest.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<Predicate> {
    typedef Predicate normal_t;
    typedef proto::Predicate proto_t;
    typedef borealis::FactoryNest context_t;

    static Predicate::ProtoPtr toProtobuf(const normal_t& p);
    static Predicate::Ptr fromProtobuf(const context_t& fn, const proto_t& p);
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
        res->set_allocated_orignumelements(
            TermConverter::toProtobuf(*p.getOrigNumElems()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            Predicate::Ptr base,
            const proto::AllocaPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto numElems = TermConverter::fromProtobuf(fn, p.numelements());
        auto origNumElems = TermConverter::fromProtobuf(fn, p.orignumelements());
        return Predicate::Ptr{
            new AllocaPredicate(lhv, numElems, origNumElems, base->getLocation(), base->getType())
        };
    }
};

template<>
struct protobuf_traits_impl<CallPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::CallPredicate> toProtobuf(const CallPredicate& p) {
        auto res = util::uniq(new proto::CallPredicate());
        res->set_allocated_function(
                TermConverter::toProtobuf(*p.getFunctionName()).release()
        );
        res->set_allocated_lhv(
                TermConverter::toProtobuf(*p.getLhv()).release()
        );
        for (const auto& d : p.getArgs()) {
            res->mutable_args()->AddAllocated(
                    TermConverter::toProtobuf(*d).release()
            );
        }
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            Predicate::Ptr base,
            const proto::CallPredicate& p) {
        auto funName = TermConverter::fromProtobuf(fn, p.function());
        auto result = TermConverter::fromProtobuf(fn, p.lhv());
        std::vector<Term::Ptr> data;
        data.reserve(p.args_size());
        for (const auto& d : p.args()) {
            data.push_back(
                    TermConverter::fromProtobuf(fn, d)
            );
        }
        return Predicate::Ptr{
                new CallPredicate(funName, result, data, base->getLocation(), base->getType())
        };
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
            Predicate::Ptr base,
            const proto::DefaultSwitchCasePredicate& p) {
        auto cond = TermConverter::fromProtobuf(fn, p.cond());

        std::vector<Term::Ptr> cases;
        cases.reserve(p.cases_size());
        for (const auto& c : p.cases()) {
            cases.push_back(
                TermConverter::fromProtobuf(fn, c)
            );
        }

        return Predicate::Ptr{
            new DefaultSwitchCasePredicate(cond, cases, base->getLocation(), base->getType())
        };
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
            Predicate::Ptr base,
            const proto::EqualityPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{
            new EqualityPredicate(lhv, rhv, base->getLocation(), base->getType())
        };
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
            Predicate::Ptr base,
            const proto::GlobalsPredicate& p) {
        std::vector<Term::Ptr> globals;
        globals.reserve(p.globals_size());
        for (const auto& g : p.globals()) {
            globals.push_back(
                TermConverter::fromProtobuf(fn, g)
            );
        }
        return Predicate::Ptr{
            new GlobalsPredicate(globals, base->getLocation(), base->getType())
        };
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
            Predicate::Ptr base,
            const proto::InequalityPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{
            new InequalityPredicate(lhv, rhv, base->getLocation(), base->getType())
        };
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
        res->set_allocated_orignumelements(
            TermConverter::toProtobuf(*p.getOrigNumElems()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            Predicate::Ptr base,
            const proto::MallocPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto numElems = TermConverter::fromProtobuf(fn, p.numelements());
        auto origNumElems = TermConverter::fromProtobuf(fn, p.orignumelements());
        return Predicate::Ptr{
            new MallocPredicate(lhv, numElems, origNumElems, base->getLocation(), base->getType())
        };
    }
};

template<>
struct protobuf_traits_impl<SeqDataPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::SeqDataPredicate> toProtobuf(const SeqDataPredicate& p) {
        auto res = util::uniq(new proto::SeqDataPredicate());
        res->set_allocated_base(
            TermConverter::toProtobuf(*p.getBase()).release()
        );
        for (const auto& d : p.getData()) {
            res->mutable_data()->AddAllocated(
                TermConverter::toProtobuf(*d).release()
            );
        }
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            Predicate::Ptr base,
            const proto::SeqDataPredicate& p) {
        auto b = TermConverter::fromProtobuf(fn, p.base());

        std::vector<Term::Ptr> data;
        data.reserve(p.data_size());
        for (const auto& d : p.data()) {
            data.push_back(
                TermConverter::fromProtobuf(fn, d)
            );
        }

        return Predicate::Ptr{
            new SeqDataPredicate(b, data, base->getLocation(), base->getType())
        };
    }
};

template<>
struct protobuf_traits_impl<SeqDataZeroPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::SeqDataZeroPredicate> toProtobuf(const SeqDataZeroPredicate& p) {
        auto res = util::uniq(new proto::SeqDataZeroPredicate());
        res->set_allocated_base(
            TermConverter::toProtobuf(*p.getBase()).release()
        );
        res->set_size(p.getSize());
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
            const FactoryNest& fn,
            Predicate::Ptr base,
            const proto::SeqDataZeroPredicate& p) {
        auto b = TermConverter::fromProtobuf(fn, p.base());

        return Predicate::Ptr{
            new SeqDataZeroPredicate(b, p.size(), base->getLocation(), base->getType())
        };
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
            Predicate::Ptr base,
            const proto::StorePredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{
            new StorePredicate(lhv, rhv, base->getLocation(), base->getType())
        };
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
            Predicate::Ptr base,
            const proto::WritePropertyPredicate& p) {
        auto propName = TermConverter::fromProtobuf(fn, p.propname());
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{
            new WritePropertyPredicate(propName, lhv, rhv, base->getLocation(), base->getType())
        };
    }
};

template<>
struct protobuf_traits_impl<WriteBoundPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::WriteBoundPredicate> toProtobuf(const WriteBoundPredicate& p) {
        auto res = util::uniq(new proto::WriteBoundPredicate());
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
            Predicate::Ptr base,
            const proto::WriteBoundPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{
            new WriteBoundPredicate(lhv, rhv, base->getLocation(), base->getType())
        };
    }
};

template<>
struct protobuf_traits_impl<MarkPredicate> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::MarkPredicate> toProtobuf(const MarkPredicate& p) {
        auto res = util::uniq(new proto::MarkPredicate());
        res->set_allocated_id(
            TermConverter::toProtobuf(*p.getId()).release()
        );
        return std::move(res);
    }

    static Predicate::Ptr fromProtobuf(
        const FactoryNest& fn,
        Predicate::Ptr base,
        const proto::MarkPredicate& p) {
        auto id = TermConverter::fromProtobuf(fn, p.id());
        return Predicate::Ptr{
            new MarkPredicate(id, base->getLocation(), base->getType())
        };
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* TYPE_PROTOBUF_CONVERTER_IMPL_HPP_ */
