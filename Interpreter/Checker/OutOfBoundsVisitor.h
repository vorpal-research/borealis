//
// Created by abdullin on 11/1/17.
//

#ifndef BOREALIS_OUTOFBOUNDSVISITOR_H
#define BOREALIS_OUTOFBOUNDSVISITOR_H

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/Memory/ArrayDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

class OutOfBoundsVisitor {
private:

    AbstractFactory* af_;

public:

    OutOfBoundsVisitor() : af_(AbstractFactory::get()) {}

    using RetTy = bool;

    RetTy visit(AbstractDomain::Ptr domain, const std::vector<AbstractDomain::Ptr>& indices);

    RetTy visitArray(const AbstractFactory::ArrayT& array, const std::vector<AbstractDomain::Ptr>& indices);
    RetTy visitFunction(const AbstractFactory::FunctionT&, const std::vector<AbstractDomain::Ptr>&);
    RetTy visitPointer(const AbstractFactory::PointerT& ptr, const std::vector<AbstractDomain::Ptr>& indices);
    RetTy visitStruct(const AbstractFactory::StructT& structure, const std::vector<AbstractDomain::Ptr>& indices);
    RetTy visitArrayLocation(const AbstractFactory::ArrayLocationT& arrayLoc, const std::vector<AbstractDomain::Ptr>& indices);
    RetTy visitStructLocation(const AbstractFactory::StructLocationT& structLoc, const std::vector<AbstractDomain::Ptr>& indices);
    RetTy visitFunctionLocation(const AbstractFactory::FunctionLocationT&, const std::vector<AbstractDomain::Ptr>&);
    RetTy visitNullLocation(const AbstractFactory::NullLocationT&, const std::vector<AbstractDomain::Ptr>&);

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_OUTOFBOUNDSVISITOR_H
