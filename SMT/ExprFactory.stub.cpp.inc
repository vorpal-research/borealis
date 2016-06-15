#ifndef NAMESPACE
#define NAMESPACE stub_
#endif
#ifndef BACKEND
#define BACKEND NoSMT
#endif

#include "Util/macros.h"

namespace borealis {
namespace NAMESPACE {
using smt = typename BACKEND::Engine;

ExprFactory::ExprFactory() : ctx(smt::init()) {}

unsigned int ExprFactory::pointerSize = 64;

////////////////////////////////////////////////////////////////////////////////

ExprFactory::Pointer ExprFactory::getPtrVar(const std::string& name, bool fresh) {
    return fresh ? Pointer::mkFreshVar(*ctx, name) : Pointer::mkVar(*ctx, name);
}

ExprFactory::Pointer ExprFactory::getPtrConst(int ptr) {
    return Pointer::mkConst(*ctx, ptr);
}

ExprFactory::Pointer ExprFactory::getNullPtr() {
    return Pointer::mkConst(*ctx, 0);
}

////////////////////////////////////////////////////////////////////////////////

ExprFactory::Bool ExprFactory::getBoolVar(const std::string& name, bool fresh) {
    return fresh ? Bool::mkFreshVar(*ctx, name) : Bool::mkVar(*ctx, name);
}

ExprFactory::Bool ExprFactory::getBoolConst(bool v) {
    return Bool::mkConst(*ctx, v);
}

ExprFactory::Bool ExprFactory::getTrue() {
    return getBoolConst(true);
}

ExprFactory::Bool ExprFactory::getFalse() {
    return getBoolConst(false);
}

////////////////////////////////////////////////////////////////////////////////

ExprFactory::Integer ExprFactory::getIntVar(const std::string& name, unsigned int size, bool fresh) {
    return fresh ? Integer::mkFreshVar(*ctx, name, size) : Integer::mkVar(*ctx, name, size);
}

ExprFactory::Integer ExprFactory::getIntConst(int v, unsigned int size) {
    return Integer::mkConst(*ctx, v, size);
}

ExprFactory::Integer ExprFactory::getIntConst(const std::string& v, unsigned int size) {
    return Integer::mkConst(*ctx, v, size);
}


ExprFactory::Real ExprFactory::getRealVar(const std::string& name, bool fresh) {
    return fresh ? Real::mkFreshVar(*ctx, name) : Real::mkVar(*ctx, name);
}

////////////////////////////////////////////////////////////////////////////////

ExprFactory::Real ExprFactory::getRealConst(int v) {
    return Real::mkConst(*ctx, v);
}

ExprFactory::Real ExprFactory::getRealConst(double v) {
    return Real::mkConst(*ctx, (long long int)v);
}

////////////////////////////////////////////////////////////////////////////////

ExprFactory::MemArray ExprFactory::getNoMemoryArray(const std::string& id) {
    static config::ConfigEntry<bool> DefaultsToUnknown("analysis", "memory-defaults-to-unknown");

    if (DefaultsToUnknown.get(false)) {
        return getEmptyMemoryArray(id);
    } else {
        return getDefaultMemoryArray(id, 0xff);
    }
}

ExprFactory::MemArray ExprFactory::getEmptyMemoryArray(const std::string& id) {
    return MemArray::mkFree(*ctx, id);
}

ExprFactory::MemArray ExprFactory::getDefaultMemoryArray(const std::string& id, int def) {
    return MemArray::mkDefault(*ctx, id, Byte::mkConst(*ctx, def));
}

ExprFactory::Pointer ExprFactory::getInvalidPtr() {
    return getPtrConst(~0U);
}

ExprFactory::Bool ExprFactory::isInvalidPtrExpr(ExprFactory::Pointer ptr) {
    return (ptr == getInvalidPtr() || ptr == getNullPtr());
}

ExprFactory::Bool ExprFactory::getDistinct(const std::vector<ExprFactory::Pointer>& exprs) {
    return logic::distinct(*ctx, exprs);
}

////////////////////////////////////////////////////////////////////////////////

ExprFactory::Dynamic ExprFactory::getVarByTypeAndName(
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
        BYE_BYE(Dynamic, "Unknown var type in SMT conversion");
    else if (isa<type::TypeError>(type))
        BYE_BYE(Dynamic, "Encountered type error in SMT conversion: " + util::toString(*type));

    warns() << "getVarByTypeAndName: Unsupported type encountered: " << *type << endl;
    return getIntVar(name, sizeForType(type), fresh);
}

////////////////////////////////////////////////////////////////////////////////

void ExprFactory::initialize(const llvm::DataLayout* TD) {
    pointerSize = TD->getPointerSizeInBits();
}

} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"
