//
// Created by abdullin on 7/6/17.
//

#include "OutOfBoundsChecker.h"

#include "Interpreter/Domain/AggregateDomain.h"
#include "Interpreter/Domain/FloatIntervalDomain.h"
#include "Interpreter/Domain/FunctionDomain.h"
#include "Interpreter/Domain/IntegerIntervalDomain.h"
#include "Interpreter/Domain/PointerDomain.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

static config::BoolConfigEntry enableLogging("absint", "checker-logging");

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
            if ((not sub_idx.empty()) && util::at(aggregate.getElements(), i) &&
                    visit(aggregate.getElements().at(i), sub_idx)) {
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
            for (auto&& cur_offset : it.offsets_) {
                subOffsets[0] = zeroElement->add(cur_offset);
                if (visit(it.location_, subOffsets)) return true;
            }
        }

        return false;
    }

    RetTy visitNullptr(const NullptrDomain&, const std::vector<Domain::Ptr>&) { return true; }

    RetTy visitFloat(const FloatIntervalDomain&, const std::vector<Domain::Ptr>&) { return false; }
    RetTy visitFunction(const FunctionDomain&, const std::vector<Domain::Ptr>&) { return false; }
    RetTy visitInteger(const IntegerIntervalDomain&, const std::vector<Domain::Ptr>&) { return false; }
};



OutOfBoundsChecker::OutOfBoundsChecker(Module* module, DefectManager* DM, FuncInfoProvider* FIP)
        : ObjectLevelLogging("interpreter"),
          module_(module),
          DM_(DM),
          FIP_(FIP) {
    ST_ = module_->getSlotTracker();
}

void OutOfBoundsChecker::visitGEPOperator(llvm::Instruction& loc, llvm::GEPOperator& GI) {
    if(visited_.count(&GI) > 0) return;
    visited_.insert(&GI);

    auto&& info = infos();

    auto di = DM_->getDefect(DefectType::BUF_01, &loc);
    if (enableLogging.get(true)) {
        info << "Checking: " << ST_->toString(&loc) << endl;
        info << "Gep operand: " << ST_->toString(&GI) << endl;
        info << "Defect: " << di << endl;
    }

    if (not module_->checkVisited(&loc) || not module_->checkVisited(&GI)) {
        if (enableLogging.get(true)) info << "Instruction not visited" << endl;
        defects_[di] |= false;

    } else {
        auto ptrOperand = GI.getPointerOperand();
        auto ptr = module_->getDomainFor(ptrOperand, loc.getParent());

        std::vector<Domain::Ptr> offsets;
        for (auto j = GI.idx_begin(); j != GI.idx_end(); ++j) {
            auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(j);
            Domain::Ptr indx = intConstant ?
                               module_->getDomainFactory()->getIndex(*intConstant->getValue().getRawData()) :
                               module_->getDomainFor(llvm::cast<llvm::Value>(j), loc.getParent());

            offsets.emplace_back(indx);
        }
        auto bug = OutOfBoundVisitor().visit(ptr, offsets);
        defects_[di] |= bug;

        if (enableLogging.get(true)) {
            info << "Pointer operand: " << ptr << endl;
            for (auto&& indx : offsets)
                info << "Shift: " << indx << endl;
            info << "Result: " << bug << endl;
        }
    }
}

void OutOfBoundsChecker::visitInstruction(llvm::Instruction& I) {
    for (auto&& op : util::viewContainer(I.operands())
            .map(llvm::dyn_caster<llvm::GEPOperator>())
            .filter()) {
        visitGEPOperator(I, *op);
    }
}

void OutOfBoundsChecker::visitCallInst(llvm::CallInst& CI) {
    visitInstruction(CI);

    auto&& info = infos();
    auto di = DM_->getDefect(DefectType::BUF_01, &CI);

    if (enableLogging.get(true)) {
        info << "Checking: " << ST_->toString(&CI) << endl;
        info << "Defect: " << di << endl;
    }

    if (not module_->checkVisited(&CI)) {
        if (enableLogging.get(true)) info << "Instruction not visited" << endl;
        defects_[di] |= false;
        return;
    }

    if (CI.isInlineAsm() || (not CI.getCalledFunction())) {
        if (enableLogging.get(true)) info << "Unknown function" << endl;
        defects_[di] = true;
        return;
    }

    try {
        auto funcData = FIP_->getInfo(CI.getCalledFunction());
        for (auto i = 0U; i < funcData.argInfo.size(); ++i) {
            auto&& argInfo = funcData.argInfo[i];
            if (argInfo.isArray != func_info::ArrayTag::IsArray) continue;

            if (not argInfo.sizeArgument) {
                defects_[di] = true;

            } else {
                auto&& size = argInfo.sizeArgument.getUnsafe();
                auto&& ptr = module_->getDomainFor(CI.getArgOperand(i), CI.getParent());

                std::vector<Domain::Ptr> offsets = {module_->getDomainFactory()->getIndex(0),
                                                    module_->getDomainFor(CI.getArgOperand(size), CI.getParent())};

                auto bug = OutOfBoundVisitor().visit(ptr, offsets);
                defects_[di] |= bug;

                if (enableLogging.get(true)) {
                    info << "Pointer operand: " << ptr << endl;
                    for (auto&& indx : offsets)
                        info << "Shift: " << indx << endl;
                    info << "Result: " << bug << endl;
                }
            }
        }

    } catch (std::out_of_range&) {
        if (enableLogging.get(true)) info << "Unknown function" << endl;
        defects_[di] = true;
    }
}

void OutOfBoundsChecker::visitGetElementPtrInst(llvm::GetElementPtrInst& GI) {
    visitGEPOperator(GI, llvm::cast<llvm::GEPOperator>(GI));
}

void OutOfBoundsChecker::run() {
    visit(const_cast<llvm::Module*>(module_->getInstance()));

    util::viewContainer(defects_)
            .filter(LAM(a, not a.second))
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"