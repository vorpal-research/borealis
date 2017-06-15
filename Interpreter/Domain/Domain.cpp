//
// Created by abdullin on 2/3/17.
//

#include "Domain.h"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

#define MK_BINOP_IMPL(inst) Domain::Ptr Domain::inst(Domain::Ptr) const { \
    UNREACHABLE("Unimplemented binary operation"); \
}

MK_BINOP_IMPL(add);
MK_BINOP_IMPL(fadd);
MK_BINOP_IMPL(sub);
MK_BINOP_IMPL(fsub);
MK_BINOP_IMPL(mul);
MK_BINOP_IMPL(fmul);
MK_BINOP_IMPL(udiv);
MK_BINOP_IMPL(sdiv);
MK_BINOP_IMPL(fdiv);
MK_BINOP_IMPL(urem);
MK_BINOP_IMPL(srem);
MK_BINOP_IMPL(frem);
MK_BINOP_IMPL(shl);
MK_BINOP_IMPL(lshr);
MK_BINOP_IMPL(ashr);
MK_BINOP_IMPL(bAnd);
MK_BINOP_IMPL(bOr);
MK_BINOP_IMPL(bXor);

#undef MK_BINOP_IMPL


Domain::Ptr Domain::extractElement(const std::vector<Domain::Ptr>&) const {
    UNREACHABLE("Unimplemented vector operation");
}

void Domain::insertElement(Domain::Ptr, const std::vector<Domain::Ptr>&) const {
    UNREACHABLE("Unimplemented vector operation");
}

Domain::Ptr Domain::extractValue(const llvm::Type&, const std::vector<Domain::Ptr>&) const {
    UNREACHABLE("Unimplemented aggregate operation");
}

void Domain::insertValue(Domain::Ptr, const std::vector<Domain::Ptr>&) const {
    UNREACHABLE("Unimplemented aggregate operation");
}

Domain::Ptr Domain::load(const llvm::Type&, Domain::Ptr) const {
    UNREACHABLE("Unimplemented memory operation");
}

void Domain::store(Domain::Ptr, Domain::Ptr) const {
    UNREACHABLE("Unimplemented memory operation");
}

Domain::Ptr Domain::gep(const llvm::Type&, const std::vector<Domain::Ptr>&) const {
    UNREACHABLE("Unimplemented memory operation");
}


#define MK_CAST_OP_IMPL(inst) Domain::Ptr Domain::inst(const llvm::Type&) const { \
    UNREACHABLE("Unimplemented cast operation"); \
}

MK_CAST_OP_IMPL(trunc);
MK_CAST_OP_IMPL(zext);
MK_CAST_OP_IMPL(sext);
MK_CAST_OP_IMPL(fptrunc);
MK_CAST_OP_IMPL(fpext);
MK_CAST_OP_IMPL(fptoui);
MK_CAST_OP_IMPL(fptosi);
MK_CAST_OP_IMPL(uitofp);
MK_CAST_OP_IMPL(sitofp);
MK_CAST_OP_IMPL(ptrtoint);
MK_CAST_OP_IMPL(inttoptr);
MK_CAST_OP_IMPL(bitcast);

#undef MK_CAST_OP_IMPL


Domain::Ptr Domain::icmp(Domain::Ptr, llvm::CmpInst::Predicate) const {
    UNREACHABLE("Unimplemented cmp operation");
}

Domain::Ptr Domain::fcmp(Domain::Ptr, llvm::CmpInst::Predicate) const {
    UNREACHABLE("Unimplemented cmp operation");
}

bool Domain::isAggregateType() const {
    return this->type_ == Type::AGGREGATE;
}

bool Domain::isPointerType() const {
    return this->type_ == Type::POINTER;
}

bool Domain::isSimpleType() const {
    return this->type_ == Type::INTEGER_INTERVAL ||
            this->type_ == Type::FLOAT_INTERVAL;
}

Split Domain::splitByEq(Domain::Ptr) const {
    UNREACHABLE("Unimplemented split operation");
}

Split Domain::splitByNeq(Domain::Ptr) const {
    UNREACHABLE("Unimplemented split operation");
}

Split Domain::splitByLess(Domain::Ptr) const {
    UNREACHABLE("Unimplemented split operation");
}

Split Domain::splitBySLess(Domain::Ptr) const {
    UNREACHABLE("Unimplemented split operation");
}

std::ostream& operator<<(std::ostream& s, Domain::Ptr d) {
    s << d->toString();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Domain::Ptr d) {
    s << d->toString();
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"