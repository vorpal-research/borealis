/*
 * ExprFactory.h
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#ifndef MATHSAT_EXPRFACTORY_H_
#define MATHSAT_EXPRFACTORY_H_

#include <llvm/Target/TargetData.h>

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
        return isa<type::Integer>(type) ? Integer::bitsize :
               isa<type::Pointer>(type) ? Pointer::bitsize :
               isa<type::Float>(type) ? Real::bitsize :
               util::sayonara<size_t>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                       "Cannot acquire bitsize for type " + util::toString(type));
    }

    ExprFactory();

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
    Integer getIntVar(const std::string& name, bool fresh = false);
    Integer getIntConst(int v);
    // Reals
    Real getRealVar(const std::string& name, bool fresh = false);
    Real getRealConst(int v);
    Real getRealConst(double v);
    // Memory
    MemArray getNoMemoryArray();

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

    static void initialize(llvm::TargetData* TD);

private:

    std::unique_ptr<mathsat::Env> env;

    static unsigned int pointerSize;

};

} // namespace mathsat_
} // namespace borealis


#endif /* MATHSAT_EXPRFACTORY_H_ */
