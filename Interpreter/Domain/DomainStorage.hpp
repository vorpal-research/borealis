//
// Created by abdullin on 2/12/19.
//

#ifndef BOREALIS_DOMAINSTORAGE_HPP
#define BOREALIS_DOMAINSTORAGE_HPP


#include "VariableFactory.hpp"
#include "Numerical/NumericalDomain.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {


class DomainStorage : public std::enable_shared_from_this<DomainStorage> {
private:

    using Variable = const llvm::Value*;
    using DomainMap = std::unordered_map<Variable, AbstractDomain::Ptr>;

    using SIntT = BitInt<true>;
    using UIntT = BitInt<false>;

private:

    NumericalDomain<Float, Variable>* unwrapFloat() {
        auto* floats = llvm::dyn_cast<NumericalDomain<Float, Variable>>(floats_.get());
        ASSERTC(floats);
        return floats;
    }

    NumericalDomain<SIntT, Variable>* unwrapSInt() {
        auto* sints = llvm::dyn_cast<NumericalDomain<SIntT, Variable>>(signedInts_.get());
        ASSERTC(sints);
        return sints;
    }

    NumericalDomain<UIntT, Variable>* unwrapUInt() {
        auto* uints = llvm::dyn_cast<NumericalDomain<UIntT, Variable>>(unsignedInts_.get());
        ASSERTC(uints);
        return uints;
    }

    AbstractDomain::Ptr createDomain(Variable value) {
        if (auto&& global = vf_->findGlobal(value)) {
            return global;

        } else if (auto&& local = context_->state->find(value)) {
            return local;

        } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
            return module_.getDomainFactory()->get(constant);

        } else if (value->getType()->isVoidTy() || value->getType()->isMetadataTy()) {
            return nullptr;
        }
        warns() << "Unknown value: " << ST_->toString(value) << endl;
        return nullptr;
    }

    void addVariable(Variable x) {
        auto&& type = vf_->cast(x->getType());

        if (llvm::isa<type::Integer>(type.get())) {
            auto* sint = unwrapSInt();
            auto* uint = unwrapUInt();

            auto&& sval = sint->getUnsafe(x);
            auto&& uval = uint->getUnsafe(x);

            if (sval == nullptr) sint->assign(x, )
        }

    }

//    AbstractDomain::Ptr getVariable(Variable value) {
//        if (auto&& global = vf_->findGlobal(value)) {
//            return global;
//
//        } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
//            return vf_->get(constant);
//
//        } else if (value->getType()->isVoidTy() || value->getType()->isMetadataTy()) {
//            return nullptr;
//
//        }
//        warns() << "Unknown value" << endl;
//        return nullptr;
//    }

public:

    DomainStorage(VariableFactory* vf) : vf_(vf) {}
    DomainStorage(const DomainStorage&) = default;
    DomainStorage(DomainStorage&&) = default;
    DomainStorage& operator=(const DomainStorage&) = default;
    DomainStorage& operator=(DomainStorage&&) = default;

    /// x = y op z
    void apply(llvm::BinaryOperator::BinaryOps op, Variable x, Variable y, Variable z) {
        typedef llvm::BinaryOperator::BinaryOps ops;
        auto aop = llvm::arithType(op);

        switch(op) {
            // sign-independent ops
            case ops::Add:
            case ops::Sub:
            case ops::Mul:
            case ops::Shl:
            case ops::LShr:
            case ops::AShr:
            case ops::And:
            case ops::Or:
            case ops::Xor: {
                auto* sints = unwrapSInt();
                auto* uints = unwrapUInt();

                sints->apply(aop, x, y, z);
                uints->apply(aop, x, y, z);
                break;
            }

            // unsigned operations
            case ops::UDiv:
            case ops::URem: {
                auto* uints = unwrapUInt();

                uints->apply(aop, x, y, z);
                break;
            }

            // signed operations
            case ops::SDiv:
            case ops::SRem: {
                auto* sints = unwrapSInt();

                sints->apply(aop, x, y, z);
                break;
            }

            // float operations
            case ops::FAdd:
            case ops::FSub:
            case ops::FMul:
            case ops::FDiv:
            case ops::FRem: {
                auto* fts = unwrapFloat();
                fts->apply(aop, x, y, z);
                break;
            }

            default: UNREACHABLE("Unreachable!");
        }
    }

    /// x = y op z
    void apply(llvm::CmpInst::Predicate op, Variable x, Variable y, Variable z) {
        typedef llvm::CmpInst::Predicate P;
        typedef llvm::ConditionType CT;

        auto cop = llvm::conditionType(op);

        switch (op) {
            case P::ICMP_EQ:
            case P::ICMP_NE: {
                auto* sints = unwrapSInt();
                auto* uints = unwrapUInt();

                auto xds = sints->apply(cop, y, z);
                auto xdu = uints->apply(cop, y, z);

                uints->assign(x, xds->join(xdu));
                break;
            }
            case P::ICMP_SGE:
            case P::ICMP_SGT:
            case P::ICMP_SLE:
            case P::ICMP_SLT: {
                auto* sints = unwrapSInt();
                auto* uints = unwrapUInt();

                auto xds = sints->apply(cop, y, z);
                uints->assign(x, xds);
                break;
            }

            case P::ICMP_UGT:
            case P::ICMP_ULE:
            case P::ICMP_ULT:
            case P::ICMP_UGE: {
                auto* uints = unwrapUInt();

                auto xdu = uints->apply(cop, y, z);
                uints->assign(x, xdu);
                break;
            }

            case P::FCMP_OEQ:
            case P::FCMP_UEQ:
            case P::FCMP_ONE:
            case P::FCMP_UNE:
            case P::FCMP_OGE:
            case P::FCMP_UGE:
            case P::FCMP_OGT:
            case P::FCMP_UGT:
            case P::FCMP_OLE:
            case P::FCMP_ULE:
            case P::FCMP_OLT:
            case P::FCMP_ULT:
            case P::FCMP_ORD:
            case P::FCMP_UNO:
            case P::FCMP_FALSE:
            case P::FCMP_TRUE: {
                auto* fts = unwrapFloat();
                auto* uints = unwrapUInt();

                auto xd = fts->apply(cop, y, z);
                uints->assign(x, xd);
                break;
            }

            default:
                UNREACHABLE("Unreachable!");
        }
    }

    /// x = *ptr
    void load(Variable x, Variable ptr) {

    }

    /// *ptr = x
    void store(Variable ptr, Variable x) {

    }

    /// x = gep(ptr, shifts)
    void gep(Variable x, Variable ptr, const std::vector<Variable>& shifts) {

    }

private:

    VariableFactory* vf_;
    AbstractDomain::Ptr signedInts_;
    AbstractDomain::Ptr unsignedInts_;
    AbstractDomain::Ptr floats_;
    DomainMap memory_;

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"


#endif //BOREALIS_DOMAINSTORAGE_HPP
