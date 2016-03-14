/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef TERM_PROTOBUF_CONVERTER_IMPL_HPP_
#define TERM_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Term/Term.pb.h"

#include "Protobuf/Gen/Term/ArgumentKind.pb.h"
#include "Protobuf/Gen/Term/ArgumentTerm.pb.h"
#include "Protobuf/Gen/Term/ArgumentCountTerm.pb.h"
#include "Protobuf/Gen/Term/AxiomTerm.pb.h"
#include "Protobuf/Gen/Term/BinaryTerm.pb.h"
#include "Protobuf/Gen/Term/BoundTerm.pb.h"
#include "Protobuf/Gen/Term/CastTerm.pb.h"
#include "Protobuf/Gen/Term/CmpTerm.pb.h"
#include "Protobuf/Gen/Term/ConstTerm.pb.h"
#include "Protobuf/Gen/Term/GepTerm.pb.h"
#include "Protobuf/Gen/Term/LoadTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueBoolConstantTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueBuiltinTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueNamedConstantTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueCallTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueFloatingConstantTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueIndexingTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueMemberAccessTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueInvalidPtrTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueIntConstantTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueBigIntConstantTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueNullPtrTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueStringConstantTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueUndefTerm.pb.h"
#include "Protobuf/Gen/Term/OpaqueVarTerm.pb.h"
#include "Protobuf/Gen/Term/ReadPropertyTerm.pb.h"
#include "Protobuf/Gen/Term/ReturnValueTerm.pb.h"
#include "Protobuf/Gen/Term/ReturnPtrTerm.pb.h"
#include "Protobuf/Gen/Term/SignTerm.pb.h"
#include "Protobuf/Gen/Term/TernaryTerm.pb.h"
#include "Protobuf/Gen/Term/UnaryTerm.pb.h"
#include "Protobuf/Gen/Term/ValueTerm.pb.h"
#include "Protobuf/Gen/Term/VarArgumentTerm.pb.h"

#include "Type/ProtobufConverterImpl.hpp"
#include "Util/ProtobufConverterImpl.hpp"

#include "Factory/Nest.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<Term> {
    typedef Term normal_t;
    typedef proto::Term proto_t;
    typedef borealis::FactoryNest context_t;

    static Term::ProtoPtr toProtobuf(const normal_t& t);
    static Term::Ptr fromProtobuf(const context_t& fn, const proto_t& t);
};

template<>
struct protobuf_traits_impl<ArgumentTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::ArgumentTerm> toProtobuf(const ArgumentTerm& t) {
        auto res = util::uniq(new proto::ArgumentTerm());
        res->set_idx(t.getIdx());
        res->set_kind(static_cast<proto::ArgumentKind>(t.getKind()));
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::ArgumentTerm& t) {
        return Term::Ptr{
            new ArgumentTerm(
                base->getType(),
                t.idx(),
                base->getName(),
                static_cast<ArgumentKind>(t.kind())
            )
        };
    }
};

template<>
struct protobuf_traits_impl<ArgumentCountTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::ArgumentCountTerm> toProtobuf(const ArgumentCountTerm&) {
        return util::uniq(new proto::ArgumentCountTerm());
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::ArgumentCountTerm&) {
        return Term::Ptr{
            new ArgumentCountTerm(
                base->getType()
            )
        };
    }
};

template<>
struct protobuf_traits_impl<AxiomTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::AxiomTerm> toProtobuf(const AxiomTerm& t) {
        auto res = util::uniq(new proto::AxiomTerm());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*t.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::AxiomTerm& t) {
        auto lhv = TermConverter::fromProtobuf(fn, t.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new AxiomTerm(base->getType(), lhv, rhv) };
    }
};

template<>
struct protobuf_traits_impl<BinaryTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::BinaryTerm> toProtobuf(const BinaryTerm& t) {
        auto res = util::uniq(new proto::BinaryTerm());
        res->set_opcode(static_cast<proto::ArithType>(t.getOpcode()));
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*t.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::BinaryTerm& t) {
        auto opcode = static_cast<llvm::ArithType>(t.opcode());
        auto lhv = TermConverter::fromProtobuf(fn, t.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new BinaryTerm(base->getType(), opcode, lhv, rhv) };
    }
};

template<>
struct protobuf_traits_impl<BoundTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::BoundTerm> toProtobuf(const BoundTerm& t) {
        auto res = util::uniq(new proto::BoundTerm());
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::BoundTerm& t) {
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new BoundTerm(base->getType(), rhv) };
    }
};

template<>
struct protobuf_traits_impl<CastTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::CastTerm> toProtobuf(const CastTerm& t) {
        auto res = util::uniq(new proto::CastTerm());
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        res->set_signextend(t.isSignExtend());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::CastTerm& t) {
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        auto signExtend = t.signextend();
        return Term::Ptr{ new CastTerm(base->getType(), signExtend, rhv) };
    }
};

template<>
struct protobuf_traits_impl<CmpTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::CmpTerm> toProtobuf(const CmpTerm& t) {
        auto res = util::uniq(new proto::CmpTerm());
        res->set_opcode(static_cast<proto::ConditionType>(t.getOpcode()));
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*t.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::CmpTerm& t) {
        auto opcode = static_cast<llvm::ConditionType>(t.opcode());
        auto lhv = TermConverter::fromProtobuf(fn, t.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new CmpTerm(base->getType(), opcode, lhv, rhv) };
    }
};

template<>
struct protobuf_traits_impl<ConstTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::ConstTerm> toProtobuf(const ConstTerm&) {
        return util::uniq(new proto::ConstTerm());
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::ConstTerm&) {
        return Term::Ptr{ new ConstTerm(base->getType(), base->getName()) };
    }
};

template<>
struct protobuf_traits_impl<GepTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::GepTerm> toProtobuf(const GepTerm& t) {
        auto res = util::uniq(new proto::GepTerm());

        res->set_allocated_base(
            TermConverter::toProtobuf(*t.getBase()).release()
        );

        res->set_triviallyinbounds(t.isTriviallyInbounds());

        for (const auto& shift : t.getShifts()) {
            res->mutable_by()->AddAllocated(
                TermConverter::toProtobuf(*shift).release()
            );
        }

        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::GepTerm& t) {

        auto ptr = TermConverter::fromProtobuf(fn, t.base());

        std::vector<Term::Ptr> shifts;
        shifts.reserve(t.by_size());
        for (int i = 0; i < t.by_size(); ++i) {
            auto by = TermConverter::fromProtobuf(fn, t.by(i));
            shifts.push_back(by);
        }

        bool inbounds = t.triviallyinbounds();

        return Term::Ptr{ new GepTerm(base->getType(), ptr, shifts, inbounds) };
    }
};


template<>
struct protobuf_traits_impl<OpaqueInvalidPtrTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueInvalidPtrTerm> toProtobuf(const OpaqueInvalidPtrTerm&) {
        return util::uniq(new proto::OpaqueInvalidPtrTerm());
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueInvalidPtrTerm&) {
        return Term::Ptr{ new OpaqueInvalidPtrTerm(base->getType()) };
    }
};

template<>
struct protobuf_traits_impl<LoadTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::LoadTerm> toProtobuf(const LoadTerm& t) {
        auto res = util::uniq(new proto::LoadTerm());
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::LoadTerm& t) {
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new LoadTerm(base->getType(), rhv) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueBoolConstantTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueBoolConstantTerm> toProtobuf(const OpaqueBoolConstantTerm& t) {
        auto res = util::uniq(new proto::OpaqueBoolConstantTerm());
        res->set_value(t.getValue());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueBoolConstantTerm& t) {
        auto value = t.value();
        return Term::Ptr{ new OpaqueBoolConstantTerm(base->getType(), value) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueBuiltinTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueBuiltinTerm> toProtobuf(const OpaqueBuiltinTerm& t) {
        auto res = util::uniq(new proto::OpaqueBuiltinTerm());
        res->set_vname(t.getVName());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueBuiltinTerm& t) {
        auto vname = t.vname();
        return Term::Ptr{ new OpaqueBuiltinTerm(base->getType(), vname) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueNamedConstantTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueNamedConstantTerm> toProtobuf(const OpaqueNamedConstantTerm& t) {
        auto res = util::uniq(new proto::OpaqueNamedConstantTerm());
        res->set_vname(t.getVName());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueNamedConstantTerm& t) {
        auto vname = t.vname();
        return Term::Ptr{ new OpaqueNamedConstantTerm(base->getType(), vname) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueFloatingConstantTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueFloatingConstantTerm> toProtobuf(const OpaqueFloatingConstantTerm& t) {
        auto res = util::uniq(new proto::OpaqueFloatingConstantTerm());
        res->set_value(t.getValue());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueFloatingConstantTerm& t) {
        auto value = t.value();
        return Term::Ptr{ new OpaqueFloatingConstantTerm(base->getType(), value) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueIndexingTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueIndexingTerm> toProtobuf(const OpaqueIndexingTerm& t) {
        auto res = util::uniq(new proto::OpaqueIndexingTerm());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*t.getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::OpaqueIndexingTerm& t) {
        auto lhv = TermConverter::fromProtobuf(fn, t.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new OpaqueIndexingTerm(base->getType(), lhv, rhv) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueMemberAccessTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueMemberAccessTerm> toProtobuf(const OpaqueMemberAccessTerm& t) {
        auto res = util::uniq(new proto::OpaqueMemberAccessTerm());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*t.getLhv()).release()
        );
        res->set_property(t.getProperty());
        res->set_indirect(t.isIndirect());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::OpaqueMemberAccessTerm& t) {
        auto lhv = TermConverter::fromProtobuf(fn, t.lhv());
        return Term::Ptr{ new OpaqueMemberAccessTerm(base->getType(), lhv, t.property(), t.indirect()) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueCallTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueCallTerm> toProtobuf(const OpaqueCallTerm& t) {
        auto res = util::uniq(new proto::OpaqueCallTerm());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(*t.getLhv()).release()
        );
        for(auto arg: t.getRhv()) {
            res->mutable_rhvs()->AddAllocated(TermConverter::toProtobuf(*arg).release());
        }
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::OpaqueCallTerm& t) {
        auto lhv = TermConverter::fromProtobuf(fn, t.lhv());
        auto rhv = std::vector<Term::Ptr>{};
        rhv.reserve(t.rhvs_size());
        for(auto arg: t.rhvs()) {
            rhv.push_back(TermConverter::fromProtobuf(fn, arg));
        }
        return Term::Ptr{ new OpaqueCallTerm(base->getType(), lhv, rhv) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueIntConstantTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueIntConstantTerm> toProtobuf(const OpaqueIntConstantTerm& t) {
        auto res = util::uniq(new proto::OpaqueIntConstantTerm());
        res->set_value(t.getValue());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueIntConstantTerm& t) {
        auto value = t.value();
        return Term::Ptr{ new OpaqueIntConstantTerm(base->getType(), value) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueBigIntConstantTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueBigIntConstantTerm> toProtobuf(const OpaqueBigIntConstantTerm& t) {
        auto res = util::uniq(new proto::OpaqueBigIntConstantTerm());
        res->set_representation(t.getRepresentation());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
        const FactoryNest&,
        Term::Ptr base,
        const proto::OpaqueBigIntConstantTerm& t) {
        auto value = t.representation();
        return Term::Ptr{ new OpaqueBigIntConstantTerm(base->getType(), value) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueStringConstantTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueStringConstantTerm> toProtobuf(const OpaqueStringConstantTerm& t) {
        auto res = util::uniq(new proto::OpaqueStringConstantTerm());
        res->set_value(t.getValue());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueStringConstantTerm& t) {
        auto value = t.value();
        return Term::Ptr{ new OpaqueStringConstantTerm(base->getType(), value) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueNullPtrTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueNullPtrTerm> toProtobuf(const OpaqueNullPtrTerm&) {
        return util::uniq(new proto::OpaqueNullPtrTerm());
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueNullPtrTerm&) {
        return Term::Ptr{ new OpaqueNullPtrTerm(base->getType()) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueUndefTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueUndefTerm> toProtobuf(const OpaqueUndefTerm&) {
        return util::uniq(new proto::OpaqueUndefTerm());
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueUndefTerm&) {
        return Term::Ptr{ new OpaqueUndefTerm(base->getType()) };
    }
};

template<>
struct protobuf_traits_impl<OpaqueVarTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::OpaqueVarTerm> toProtobuf(const OpaqueVarTerm& t) {
        auto res = util::uniq(new proto::OpaqueVarTerm());
        res->set_vname(t.getVName());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::OpaqueVarTerm& t) {
        auto vname = t.vname();
        return Term::Ptr{ new OpaqueVarTerm(base->getType(), vname) };
    }
};

template<>
struct protobuf_traits_impl<ReadPropertyTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::ReadPropertyTerm> toProtobuf(const ReadPropertyTerm& t) {
        auto res = util::uniq(new proto::ReadPropertyTerm());
        res->set_allocated_propname(
            TermConverter::toProtobuf(*t.getPropertyName()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::ReadPropertyTerm& t) {
        auto propName = TermConverter::fromProtobuf(fn, t.propname());
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new ReadPropertyTerm(base->getType(), propName, rhv) };
    }
};

template<>
struct protobuf_traits_impl<ReturnValueTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::ReturnValueTerm> toProtobuf(const ReturnValueTerm& t) {
        auto res = util::uniq(new proto::ReturnValueTerm());
        res->set_funcname(t.getFunctionName());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::ReturnValueTerm& t) {
        auto fName = t.funcname();
        return Term::Ptr{ new ReturnValueTerm(base->getType(), fName) };
    }
};

template<>
struct protobuf_traits_impl<ReturnPtrTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::ReturnPtrTerm> toProtobuf(const ReturnPtrTerm& t) {
        auto res = util::uniq(new proto::ReturnPtrTerm());
        res->set_funcname(t.getFunctionName());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::ReturnPtrTerm& t) {
        auto fName = t.funcname();
        return Term::Ptr{ new ReturnPtrTerm(base->getType(), fName) };
    }
};

template<>
struct protobuf_traits_impl<SignTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::SignTerm> toProtobuf(const SignTerm& t) {
        auto res = util::uniq(new proto::SignTerm());
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::SignTerm& t) {
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new SignTerm(base->getType(), rhv) };
    }
};

template<>
struct protobuf_traits_impl<TernaryTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::TernaryTerm> toProtobuf(const TernaryTerm& t) {
        auto res = util::uniq(new proto::TernaryTerm());
        res->set_allocated_cnd(
            TermConverter::toProtobuf(*t.getCnd()).release()
        );
        res->set_allocated_tru(
            TermConverter::toProtobuf(*t.getTru()).release()
        );
        res->set_allocated_fls(
            TermConverter::toProtobuf(*t.getFls()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::TernaryTerm& t) {
        auto cnd = TermConverter::fromProtobuf(fn, t.cnd());
        auto tru = TermConverter::fromProtobuf(fn, t.tru());
        auto fls = TermConverter::fromProtobuf(fn, t.fls());
        return Term::Ptr{ new TernaryTerm(base->getType(), cnd, tru, fls) };
    }
};

template<>
struct protobuf_traits_impl<UnaryTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::UnaryTerm> toProtobuf(const UnaryTerm& t) {
        auto res = util::uniq(new proto::UnaryTerm());
        res->set_opcode(static_cast<proto::UnaryArithType>(t.getOpcode()));
        res->set_allocated_rhv(
            TermConverter::toProtobuf(*t.getRhv()).release()
        );
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest& fn,
            Term::Ptr base,
            const proto::UnaryTerm& t) {
        auto opcode = static_cast<llvm::UnaryArithType>(t.opcode());
        auto rhv = TermConverter::fromProtobuf(fn, t.rhv());
        return Term::Ptr{ new UnaryTerm(base->getType(), opcode, rhv) };
    }
};

template<>
struct protobuf_traits_impl<ValueTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::ValueTerm> toProtobuf(const ValueTerm& t) {
        auto res = util::uniq(new proto::ValueTerm());
        res->set_global(t.isGlobal());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::ValueTerm& t) {
        return Term::Ptr{ new ValueTerm(base->getType(), base->getName(), t.global()) };
    }
};

template<>
struct protobuf_traits_impl<VarArgumentTerm> {

    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::VarArgumentTerm> toProtobuf(const VarArgumentTerm& t) {
        auto res = util::uniq(new proto::VarArgumentTerm());
        res->set_idx(t.getIdx());
        return std::move(res);
    }

    static Term::Ptr fromProtobuf(
            const FactoryNest&,
            Term::Ptr base,
            const proto::VarArgumentTerm& t) {
        return Term::Ptr{
            new VarArgumentTerm(
                base->getType(),
                t.idx()
            )
        };
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* TERM_PROTOBUF_CONVERTER_IMPL_HPP_ */
