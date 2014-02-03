/*
 * ExprFactory.h
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3_EXPRFACTORY_H_
#define Z3_EXPRFACTORY_H_

#include <llvm/Target/TargetData.h>
#include <z3/z3++.h>

#include "SMT/Z3/Z3.h"
#include "Util/util.h"

namespace borealis {
namespace z3_ {

class ExprFactory {

    USING_SMT_LOGIC(Z3);

public:

    static size_t sizeForType(Type::Ptr type) {
        using llvm::isa;
        return isa<type::Integer>(type) ? Integer::bitsize :
               isa<type::Pointer>(type) ? Pointer::bitsize :
               isa<type::Array>(type)   ? Pointer::bitsize : // FIXME: ???
               isa<type::Float>(type)   ? Real::bitsize :
               util::sayonara<size_t>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                       "Cannot acquire bitsize for type " + util::toString(*type));
    }

    ExprFactory();

    z3::context& unwrap() {
        return *ctx;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Pointers
    Pointer getPtrVar(const std::string& name, bool fresh = false);
    Pointer getPtrConst(int ptr);
    Pointer getNullPtr();
    // Bools
    Bool getBoolVar(const std::string& name, bool fresh = false);
    Bool getBoolConst(bool b);
    Bool getTrue();
    Bool getFalse();
    // Integers
    Integer getIntVar(const std::string& name, bool fresh = false);
    Integer getIntConst(int v);
    // Reals
    Real getRealVar(const std::string& name, bool fresh = false);
    Real getRealConst(int v);
    Real getRealConst(double v);
    // Memory
    MemArray getNoMemoryArray(const std::string& id);
    MemArray getEmptyMemoryArray(const std::string& id);
    MemArray getDefaultMemoryArray(const std::string& id, int def);

    // Generic functions
    Dynamic getVarByTypeAndName(
            Type::Ptr type,
            const std::string& name,
            bool fresh = false);

    // Valid/invalid pointers
    Pointer getInvalidPtr();
    Bool isInvalidPtrExpr(Pointer ptr);
    // Misc pointer stuff
    Bool getDistinct(const std::vector<Pointer>& exprs);

#include "Util/macros.h"
    auto if_(Bool cond) QUICK_RETURN(logic::if_(cond))
#include "Util/unmacros.h"

    template<class T, class U>
    T switch_(
            U val,
            const std::vector<std::pair<U, T>>& cases,
            T default_) {
        return logic::switch_(val, cases, default_);
    }

    template<class T>
    T switch_(
            const std::vector<std::pair<Bool, T>>& cases,
            T default_) {
        return logic::switch_(cases, default_);
    }

    template<class ...Args>
    Bool forAll(std::function<Bool(Args...)> func) {
        return logic::forAll(*ctx, func);
    }

    Bool implies(Bool from, Bool to) {
        return logic::implies(from, to);
    }

    static void initialize(llvm::TargetData* TD);

private:

    std::unique_ptr<z3::context> ctx;

    static unsigned int pointerSize;

};

} // namespace z3_
} // namespace borealis

#endif /* Z3_EXPRFACTORY_H_ */
