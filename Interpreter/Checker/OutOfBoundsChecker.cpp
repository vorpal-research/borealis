//
// Created by abdullin on 7/6/17.
//

#include "OutOfBoundsChecker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

// Returns true, if there might be OOB
class OutOfBoundVisitor {
public:

    using RetTy = bool;

    RetTy visit(Domain::Ptr domain, const std::vector<Domain::Ptr>& indices) {
        if (false) { }
#define HANDLE_DOMAIN(NAME, CLASS) \
        else if (auto&& resolved = llvm::dyn_cast<CLASS>(domain.get())) { \
            return visit##NAME(*resolved, indices); \
        }
#include "Interpreter/Domain/Domain.def"
        BYE_BYE(RetTy , "Unknown type in OutOfBoundVisitor");
    }

    RetTy visitAggregate(const AggregateDomain& aggregate, const std::vector<Domain::Ptr>& indices) {
        if (not aggregate.isValue()) return true;

        auto length = aggregate.getMaxLength();
        auto idx_interval = llvm::cast<IntegerIntervalDomain>(indices.begin()->get());
        auto idx_begin = idx_interval->lb()->getRawValue();
        auto idx_end = idx_interval->ub()->getRawValue();

        if (idx_end > length || idx_begin > length) {
            return true;
        }

        std::vector<Domain::Ptr> sub_idx(indices.begin() + 1, indices.end());
        for (auto i = idx_begin; i <= idx_end && i < length; ++i) {
            if ((not sub_idx.empty()) &&
                    visit(aggregate.getElements().at(i)->load(), sub_idx)) {
                return true;
            }
        }

        return false;
    }

    RetTy visitPointer(const PointerDomain& ptr, const std::vector<Domain::Ptr>& indices) {
        if (not ptr.isValue()) return true;

        std::vector<Domain::Ptr> subOffsets(indices.begin(), indices.end());
        auto zeroElement = subOffsets[0];

        for (auto&& it : ptr.getLocations()) {
            subOffsets[0] = zeroElement->add(it.offset_);
            if (visit(it.location_, subOffsets)) return true;
        }

        return false;
    }

    RetTy visitNullptr(const NullptrDomain&, const std::vector<Domain::Ptr>&) { return true; }

    RetTy visitFloat(const FloatIntervalDomain&, const std::vector<Domain::Ptr>&) { return false; }
    RetTy visitFunction(const FunctionDomain&, const std::vector<Domain::Ptr>&) { return false; }
    RetTy visitInteger(const IntegerIntervalDomain&, const std::vector<Domain::Ptr>&) { return false; }
};



OutOfBoundsChecker::OutOfBoundsChecker(Module* module, DefectManager* DM)
        : ObjectLevelLogging("interpreter"),
          module_(module),
          DM_(DM) {
    ST_ = module_->getSlotTracker();
}

void OutOfBoundsChecker::visitGEPOperator(llvm::Instruction& loc, llvm::GEPOperator& GI) {
    if(visited_.count(&GI) > 0) return;
    visited_.insert(&GI);

    auto&& info = infos();

    auto di = DM_->getDefect(DefectType::BUF_01, &loc);
    info << "Checking: " << ST_->toString(&loc) << endl;
    info << "Gep operand: " << ST_->toString(&GI) << endl;
    info << "Defect: " << di << endl;

    if (not module_->checkVisited(&loc) || not module_->checkVisited(&GI)) {
        info << "Instruction not visited" << endl;
        DM_->addNoAbsIntDefect(di);

    } else {
        auto ptr = module_->getDomainFor(GI.getPointerOperand(), loc.getParent());
        info << "Pointer operand: " << ptr << endl;

        std::vector<Domain::Ptr> offsets;
        for (auto j = GI.idx_begin(); j != GI.idx_end(); ++j) {
            auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(j);
            Domain::Ptr indx = intConstant ?
                               module_->getDomainFactory()->getIndex(*intConstant->getValue().getRawData()) :
                               module_->getDomainFor(llvm::cast<llvm::Value>(j), loc.getParent());

            info << "Shift: " << indx << endl;
            offsets.emplace_back(indx);
        }

        auto bug = OutOfBoundVisitor().visit(ptr, offsets);
        info << "Result: " << bug << endl;
        defects_[di] |= bug;
    }
    info << endl;
}

void OutOfBoundsChecker::visitInstruction(llvm::Instruction& I) {
    for (auto&& op : util::viewContainer(I.operands())
            .map(llvm::dyn_caster<llvm::GEPOperator>())
            .filter()) {
        visitGEPOperator(I, *op);
    }
}

void OutOfBoundsChecker::visitGetElementPtrInst(llvm::GetElementPtrInst& GI) {
    visitGEPOperator(GI, llvm::cast<llvm::GEPOperator>(GI));
}

void OutOfBoundsChecker::run() {
    visit(const_cast<llvm::Module*>(module_->getInstance()));

    util::viewContainer(defects_)
            .filter([&](auto&& it) -> bool { return not it.second; })
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"