/*
 * ExprFactory.h
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#ifndef MATHSAT_EXPRFACTORY_H_
#define MATHSAT_EXPRFACTORY_H_

#include <llvm/IR/DataLayout.h>

#include "SMT/MathSAT/MathSAT.h"
#include "SMT/MathSAT/MathSatTypes.h"
#include "Util/util.h"

namespace borealis {
namespace mathsat_ {

class ExprFactory {

    USING_SMT_LOGIC(MathSAT);

public:

    static size_t sizeForType(Type::Ptr type) {
        using llvm::isa;
        using llvm::cast;
        return isa<type::Integer>(type) ? cast<type::Integer>(type)->getBitsize() :
               isa<type::Pointer>(type) ? Pointer::bitsize :
               isa<type::Array>(type)   ? Pointer::bitsize : // FIXME: ???
               isa<type::Float>(type)   ? Real::bitsize :
               isa<type::Bool>(type)    ? 1 :
               util::sayonara<size_t>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                        "Cannot acquire bitsize for type " + util::toString(*type));
    }

    ExprFactory();
    ExprFactory(const ExprFactory&) = delete;
    ExprFactory(ExprFactory&&) = delete;

    mathsat::Env& unwrap() {
        return *env;
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
    Integer getIntVar(const std::string& name, unsigned int size = Byte::bitsize, bool fresh = false);
    Integer getIntConst(int v, unsigned int size = Byte::bitsize);
    Integer getIntConst(const std::string& value, unsigned int size = Byte::bitsize);
    // Reals
    Real getRealVar(const std::string& name, bool fresh = false);
    Real getRealConst(int v);
    Real getRealConst(double v);
    // Memory
    MemArray getNoMemoryArray(const std::string& id);

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

    Bool implies(Bool lhv, Bool rhv) {
        return logic::implies(lhv, rhv);
    }

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

    static void initialize(llvm::DataLayout* TD);

private:

    std::unique_ptr<mathsat::Env> env;

    static unsigned int pointerSize;

};

} // namespace mathsat_
} // namespace borealis

#endif /* MATHSAT_EXPRFACTORY_H_ */
