////
//// Created by abdullin on 11/1/17.
////
//
//#include "OutOfBoundsVisitor.h"
//
//#include "Util/collections.hpp"
//#include "Util/macros.h"
//
//namespace borealis {
//namespace absint {
//
//OutOfBoundsVisitor::RetTy OutOfBoundsVisitor::visit(Domain::Ptr domain, const std::vector<Domain::Ptr>& indices) {
//    if (false) { }
//#define HANDLE_DOMAIN(NAME, CLASS) \
//        else if (auto&& resolved = llvm::dyn_cast<CLASS>(domain.get())) { \
//            return visit##NAME(*resolved, indices); \
//        }
//#include "Interpreter/Domain/Domain.def"
//    UNREACHABLE("Unknown type in OutOfBoundVisitor");
//}
//
//OutOfBoundsVisitor::RetTy OutOfBoundsVisitor::visitAggregate(const AggregateDomain& aggregate,
//                                                             const std::vector<Domain::Ptr>& indices) {
//    if (not aggregate.isValue()) return true;
//
//    auto length = aggregate.getMaxLength();
//    auto idx_interval = llvm::cast<IntegerIntervalDomain>(indices.begin()->get());
//    auto idx_begin = idx_interval->lb()->getRawValue();
//    auto idx_end = idx_interval->ub()->getRawValue();
//
//    if (idx_end > length || idx_begin > length) {
//        return true;
//    }
//
//    std::vector<Domain::Ptr> sub_idx(indices.begin() + 1, indices.end());
//    for (auto i = idx_begin; i <= idx_end && i < length; ++i) {
//        if ((not sub_idx.empty()) && util::at(aggregate.getElements(), i) &&
//            visit(aggregate.getElements().at(i), sub_idx)) {
//            return true;
//        }
//    }
//
//    return false;
//}
//
//OutOfBoundsVisitor::RetTy OutOfBoundsVisitor::visitPointer(const PointerDomain& ptr,
//                                                           const std::vector<Domain::Ptr>& indices) {
//    if (not ptr.isValue()) return true;
//
//    std::vector<Domain::Ptr> subOffsets(indices.begin(), indices.end());
//    auto zeroElement = subOffsets[0];
//
//    for (auto&& it : ptr.getLocations()) {
//        for (auto&& cur_offset : it.offsets_) {
//            subOffsets[0] = zeroElement->add(cur_offset);
//            if (visit(it.location_, subOffsets)) return true;
//        }
//    }
//
//    return false;
//}
//
//OutOfBoundsVisitor::RetTy OutOfBoundsVisitor::visitNullptr(const NullptrDomain&, const std::vector<Domain::Ptr>&) {
//    return true;
//}
//OutOfBoundsVisitor::RetTy OutOfBoundsVisitor::visitFloat(const FloatIntervalDomain&, const std::vector<Domain::Ptr>&) {
//    return false;
//}
//OutOfBoundsVisitor::RetTy OutOfBoundsVisitor::visitFunction(const FunctionDomain&, const std::vector<Domain::Ptr>&) {
//    return false;
//}
//OutOfBoundsVisitor::RetTy OutOfBoundsVisitor::visitInteger(const IntegerIntervalDomain&, const std::vector<Domain::Ptr>&) {
//    return false;
//}
//
//} // namespace absint
//} // namespace borealis
//
//#include "Util/unmacros.h"