//
// Created by abdullin on 2/3/17.
//

#include "Domain.h"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

AbstractDomain::Ptr AbstractDomain::apply(BinaryOperator, AbstractDomain::ConstPtr) const {
    UNREACHABLE("Unsupported operation");
}

AbstractDomain::Ptr AbstractDomain::apply(CmpOperator, AbstractDomain::ConstPtr) const {
    UNREACHABLE("Unsupported operation");
}

AbstractDomain::Ptr AbstractDomain::load(Type::Ptr, Ptr) const {
    UNREACHABLE("Unsupported operation");
}
void AbstractDomain::store(Ptr, Ptr) {
    UNREACHABLE("Unsupported operation");
}

AbstractDomain::Ptr AbstractDomain::gep(Type::Ptr, const std::vector<AbstractDomain::Ptr>&) {
    UNREACHABLE("Unsupported operation");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void Domain::setTop() {
    ASSERT(isMutable(), "changing immutable domain");
    value_ = TOP;
}

void Domain::setBottom() {
    ASSERT(isMutable(), "changing immutable domain");
    value_ = BOTTOM;
}

void Domain::setValue() {
    ASSERT(isMutable(), "changing immutable domain");
    value_ = VALUE;
}

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
MK_BINOP_IMPL(implies);

#undef MK_BINOP_IMPL

Domain::Ptr Domain::neg() const {
    UNREACHABLE("Unimplemented unary operation");
}

Domain::Ptr Domain::bNot() const {
    UNREACHABLE("Unimplemented unary operation");
}

Domain::Ptr Domain::extractElement(const std::vector<Domain::Ptr>&) {
    UNREACHABLE("Unimplemented vector operation");
}

void Domain::insertElement(Domain::Ptr, const std::vector<Domain::Ptr>&) {
    UNREACHABLE("Unimplemented vector operation");
}

Domain::Ptr Domain::extractValue(Type::Ptr, const std::vector<Domain::Ptr>&) {
    UNREACHABLE("Unimplemented aggregate operation");
}

void Domain::insertValue(Domain::Ptr, const std::vector<Domain::Ptr>&) {
    UNREACHABLE("Unimplemented aggregate operation");
}

Domain::Ptr Domain::load(Type::Ptr, Domain::Ptr) {
    UNREACHABLE("Unimplemented memory operation");
}

void Domain::store(Domain::Ptr, Domain::Ptr) {
    UNREACHABLE("Unimplemented memory operation");
}

Domain::Ptr Domain::gep(Type::Ptr, const std::vector<Domain::Ptr>&) {
    UNREACHABLE("Unimplemented memory operation");
}


#define MK_CONST_CAST_OP_IMPL(inst) Domain::Ptr Domain::inst(Type::Ptr) const { \
    UNREACHABLE("Unimplemented cast operation"); \
}

#define MK_CAST_OP_IMPL(inst) Domain::Ptr Domain::inst(Type::Ptr) { \
    UNREACHABLE("Unimplemented cast operation"); \
}

MK_CONST_CAST_OP_IMPL(trunc);
MK_CONST_CAST_OP_IMPL(zext);
MK_CONST_CAST_OP_IMPL(sext);
MK_CONST_CAST_OP_IMPL(fptrunc);
MK_CONST_CAST_OP_IMPL(fpext);
MK_CONST_CAST_OP_IMPL(fptoui);
MK_CONST_CAST_OP_IMPL(fptosi);
MK_CONST_CAST_OP_IMPL(uitofp);
MK_CONST_CAST_OP_IMPL(sitofp);
MK_CONST_CAST_OP_IMPL(inttoptr);

MK_CAST_OP_IMPL(ptrtoint);
MK_CAST_OP_IMPL(bitcast);

#undef MK_CONST_CAST_OP_IMPL
#undef MK_CAST_OP_IMPL


Domain::Ptr Domain::icmp(Domain::Ptr, llvm::CmpInst::Predicate) const {
    UNREACHABLE("Unimplemented cmp operation");
}

Domain::Ptr Domain::fcmp(Domain::Ptr, llvm::CmpInst::Predicate) const {
    UNREACHABLE("Unimplemented cmp operation");
}

bool Domain::isAggregate() const {
    return this->type_ == DomainType::AGGREGATE;
}

bool Domain::isPointer() const {
    return this->type_ == DomainType::POINTER;
}

bool Domain::isSimple() const {
    return this->type_ == DomainType::INTEGER_INTERVAL ||
            this->type_ == DomainType::FLOAT_INTERVAL;
}

bool Domain::isInteger() const {
    return this->type_ == DomainType::INTEGER_INTERVAL;
}

bool Domain::isFloat() const {
    return this->type_ == DomainType::FLOAT_INTERVAL;
}

bool Domain::isFunction() const {
    return this->type_ == DomainType::FUNCTION;
}

bool Domain::isNullptr() const {
    return this->type_ == DomainType::NULLPTR;
}

bool Domain::isMutable() const {
    return this->type_ == DomainType::POINTER ||
            this->type_ == DomainType::AGGREGATE ||
            this->type_ == DomainType::FUNCTION;
}

Split Domain::splitByEq(Domain::Ptr) {
    UNREACHABLE("Unimplemented split operation");
}

Split Domain::splitByLess(Domain::Ptr) {
    UNREACHABLE("Unimplemented split operation");
}

Split Domain::splitBySLess(Domain::Ptr) {
    UNREACHABLE("Unimplemented split operation");
}

void Domain::moveToTop() {
    setTop();
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
