////
//// Created by abdullin on 11/1/17.
////
//
//#ifndef BOREALIS_OUTOFBOUNDSVISITOR_H
//#define BOREALIS_OUTOFBOUNDSVISITOR_H
//
//#include "Interpreter/Domain/Domain.h"
//#include "Interpreter/Domain/AggregateDomain.h"
//#include "Interpreter/Domain/FloatIntervalDomain.h"
//#include "Interpreter/Domain/FunctionDomain.h"
//#include "Interpreter/Domain/IntegerIntervalDomain.h"
//#include "Interpreter/Domain/PointerDomain.h"
//
//namespace borealis {
//namespace absint {
//
//class OutOfBoundsVisitor {
//public:
//
//    using RetTy = bool;
//
//    RetTy visit(Domain::Ptr domain, const std::vector<Domain::Ptr>& indices);
//
//    RetTy visitAggregate(const AggregateDomain& aggregate, const std::vector<Domain::Ptr>& indices);
//    RetTy visitPointer(const PointerDomain& ptr, const std::vector<Domain::Ptr>& indices);
//    RetTy visitNullptr(const NullptrDomain&, const std::vector<Domain::Ptr>&);
//    RetTy visitFloat(const FloatIntervalDomain&, const std::vector<Domain::Ptr>&);
//    RetTy visitFunction(const FunctionDomain&, const std::vector<Domain::Ptr>&);
//    RetTy visitInteger(const IntegerIntervalDomain&, const std::vector<Domain::Ptr>&);
//};
//
//} // namespace absint
//} // namespace borealis
//
//
//#endif //BOREALIS_OUTOFBOUNDSVISITOR_H
