/*
 * ExprFactory.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#include "Config/config.h"
#include "SMT/Z3/ExprFactory.h"

#include "Util/macros.h"

namespace borealis {
namespace z3_ {

ExprFactory::ExprFactory() {
    z3::config cfg;
    cfg.set("model", true);
    cfg.set("proof", false);
    cfg.set("unsat_core", true);

    ctx = std::unique_ptr<z3::context>(new z3::context(cfg));

    Z3_set_ast_print_mode(*ctx, Z3_PRINT_SMTLIB2_COMPLIANT);
}

unsigned int ExprFactory::pointerSize = 32;

////////////////////////////////////////////////////////////////////////////////

Z3::Pointer ExprFactory::getPtrVar(const std::string& name, bool fresh) {
    return fresh ? Pointer::mkFreshVar(*ctx, name) : Pointer::mkVar(*ctx, name);
}

Z3::Pointer ExprFactory::getPtrConst(int ptr) {
    return Pointer::mkConst(*ctx, ptr);
}

Z3::Pointer ExprFactory::getNullPtr() {
    return Pointer::mkConst(*ctx, 0);
}

////////////////////////////////////////////////////////////////////////////////

Z3::Bool ExprFactory::getBoolVar(const std::string& name, bool fresh) {
    return fresh ? Bool::mkFreshVar(*ctx, name) : Bool::mkVar(*ctx, name);
}

Z3::Bool ExprFactory::getBoolConst(bool v) {
    return Bool::mkConst(*ctx, v);
}

Z3::Bool ExprFactory::getTrue() {
    return getBoolConst(true);
}

Z3::Bool ExprFactory::getFalse() {
    return getBoolConst(false);
}

////////////////////////////////////////////////////////////////////////////////

Z3::Integer ExprFactory::getIntVar(const std::string& name, bool fresh) {
    return fresh ? Integer::mkFreshVar(*ctx, name) : Integer::mkVar(*ctx, name);
}

Z3::Integer ExprFactory::getIntConst(int v) {
    return Integer::mkConst(*ctx, v);
}

Z3::Real ExprFactory::getRealVar(const std::string& name, bool fresh) {
    return fresh ? Real::mkFreshVar(*ctx, name) : Real::mkVar(*ctx, name);
}

////////////////////////////////////////////////////////////////////////////////

Z3::Real ExprFactory::getRealConst(int v) {
    return Real::mkConst(*ctx, v);
}

Z3::Real ExprFactory::getRealConst(double v) {
    return Real::mkConst(*ctx, (long long int)v);
}

////////////////////////////////////////////////////////////////////////////////

Z3::MemArray ExprFactory::getNoMemoryArray(const std::string& id) {
    static config::ConfigEntry<bool> DefaultsToUnknown("analysis", "memory-defaults-to-unknown");

    if (DefaultsToUnknown.get(false)) {
        return getEmptyMemoryArray(id);
    } else {
        return getDefaultMemoryArray(id, 0xff);
    }
}

Z3::MemArray ExprFactory::getEmptyMemoryArray(const std::string& id) {
    return MemArray::mkFree(*ctx, id);
}

Z3::MemArray ExprFactory::getDefaultMemoryArray(const std::string& id, int def) {
    return MemArray::mkDefault(*ctx, id, Byte::mkConst(*ctx, def));
}

Z3::Pointer ExprFactory::getInvalidPtr() {
    return getPtrConst(~0U);
}

Z3::Bool ExprFactory::isInvalidPtrExpr(Z3::Pointer ptr) {
    return (ptr == getInvalidPtr() || ptr == getNullPtr());
}

Z3::Bool ExprFactory::getDistinct(const std::vector<Z3::Pointer>& exprs) {
    return z3_::logic::distinct(*ctx, exprs);
}

////////////////////////////////////////////////////////////////////////////////

Z3::Dynamic ExprFactory::getVarByTypeAndName(
        Type::Ptr type,
        const std::string& name,
        bool fresh) {
    using llvm::isa;

    if (isa<type::Integer>(type))
        return getIntVar(name, fresh);
    else if (isa<type::Float>(type))
        return getRealVar(name, fresh);
    else if (isa<type::Bool>(type))
        return getBoolVar(name, fresh);
    else if (isa<type::Pointer>(type))
        return getPtrVar(name, fresh);
    else if (isa<type::UnknownType>(type))
        BYE_BYE(Dynamic, "Unknown var type in Z3 conversion");
    else if (isa<type::TypeError>(type))
        BYE_BYE(Dynamic, "Encountered type error in Z3 conversion: " + util::toString(*type));

    BYE_BYE(Dynamic, "Unreachable!");
}

////////////////////////////////////////////////////////////////////////////////

void ExprFactory::initialize(const llvm::DataLayout* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"
