/*
 * ExprFactory.cpp
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#include "Config/config.h"
#include "SMT/MathSAT/ExprFactory.h"

#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {

ExprFactory::ExprFactory() {
    mathsat::Config cfg;
    env = std::unique_ptr<mathsat::Env>(new mathsat::Env(cfg));
}

unsigned int ExprFactory::pointerSize = 32;

////////////////////////////////////////////////////////////////////////////////

MathSAT::Pointer ExprFactory::getPtrVar(const std::string& name, bool fresh) {
    return fresh ? Pointer::mkFreshVar(*env, name) : Pointer::mkVar(*env, name);
}

MathSAT::Pointer ExprFactory::getPtrConst(int ptr) {
    return Pointer::mkConst(*env, ptr);
}

MathSAT::Pointer ExprFactory::getNullPtr() {
    return Pointer::mkConst(*env, 0);
}

////////////////////////////////////////////////////////////////////////////////

MathSAT::Bool ExprFactory::getBoolVar(const std::string& name, bool fresh) {
    return fresh ? Bool::mkFreshVar(*env, name) : Bool::mkVar(*env, name);
}

MathSAT::Bool ExprFactory::getBoolConst(bool v) {
    return Bool::mkConst(*env, v);
}

MathSAT::Bool ExprFactory::getTrue() {
    return getBoolConst(true);
}

MathSAT::Bool ExprFactory::getFalse() {
    return getBoolConst(false);
}

////////////////////////////////////////////////////////////////////////////////

MathSAT::Integer ExprFactory::getIntVar(const std::string& name, unsigned int size, bool fresh) {
    return fresh ? Integer::mkFreshSizedVar(*env, name, size) : Integer::mkSizedVar(*env, name, size);
}

MathSAT::Integer ExprFactory::getIntConst(int v, unsigned int size) {
    return Integer::mkSizedConst(*env, v, size);
}

MathSAT::Integer ExprFactory::getIntConst(const std::string& v, unsigned int size) {
    return Integer::mkSizedConst(*env, v, size);
}

////////////////////////////////////////////////////////////////////////////////

MathSAT::Real ExprFactory::getRealVar(const std::string& name, bool fresh) {
    return fresh ? Real::mkFreshVar(*env, name) : Real::mkVar(*env, name);
}

MathSAT::Real ExprFactory::getRealConst(int v) {
    return Real::mkConst(*env, v);
}

MathSAT::Real ExprFactory::getRealConst(double v) {
    return Real::mkConst(*env, (long long int)v);
}

////////////////////////////////////////////////////////////////////////////////

MathSAT::MemArray ExprFactory::getNoMemoryArray(const std::string& id) {
    static config::ConfigEntry<bool> DefaultsToUnknown("analysis", "memory-defaults-to-unknown");

    if (DefaultsToUnknown.get(false)) {
        return MemArray::mkFree(*env, id);
    } else {
        return MemArray::mkDefault(*env, id, Byte::mkConst(*env, 0xff));
    }
}

MathSAT::Pointer ExprFactory::getInvalidPtr() {
    return getPtrConst(~0U);
}

MathSAT::Bool ExprFactory::isInvalidPtrExpr(MathSAT::Pointer ptr) {
    return (ptr == getInvalidPtr() || ptr == getNullPtr());
}

MathSAT::Bool ExprFactory::getDistinct(const std::vector<MathSAT::Pointer>& exprs) {
    return mathsat_::logic::distinct(*env, exprs);
}

////////////////////////////////////////////////////////////////////////////////

MathSAT::Dynamic ExprFactory::getVarByTypeAndName(
        Type::Ptr type,
        const std::string& name,
        bool fresh) {
    using llvm::isa;
    using llvm::dyn_cast;

    if (auto ti = dyn_cast<type::Integer>(type))
        return getIntVar(name, ti->getBitsize(), fresh);
    else if (isa<type::Float>(type))
        return getRealVar(name, fresh);
    else if (isa<type::Bool>(type))
        return getBoolVar(name, fresh);
    else if (isa<type::Pointer>(type))
        return getPtrVar(name, fresh);
    else if (isa<type::UnknownType>(type))
        BYE_BYE(Dynamic, "Unknown var type in MathSAT conversion");
    else if (isa<type::TypeError>(type))
        BYE_BYE(Dynamic, "Encountered type error in MathSAT conversion");

    BYE_BYE(Dynamic, "Unreachable!");
}

////////////////////////////////////////////////////////////////////////////////

void ExprFactory::initialize(llvm::DataLayout* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"
