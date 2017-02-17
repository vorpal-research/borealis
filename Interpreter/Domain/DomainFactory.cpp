//
// Created by abdullin on 2/7/17.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "FloatInterval.h"
#include "Util.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Domain::Ptr DomainFactory::get(const llvm::Type& type) const {
    if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(intType->getBitWidth());
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(semantics);
    } else {
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr borealis::absint::DomainFactory::get(const llvm::Value* val) const {
    return get(*val->getType());
}

Domain::Ptr DomainFactory::get(const llvm::Constant* constant) const {
    if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
        return getInteger(llvm::APSInt(intConstant->getValue()));
    } else if (auto&& floatConstant = llvm::dyn_cast<llvm::ConstantFP>(constant)) {
        return getFloat(llvm::APFloat(floatConstant->getValueAPF()));
    } else {
        return get(llvm::cast<llvm::Value>(constant));
    }
}

Domain::Ptr DomainFactory::getInteger(unsigned width, bool isSigned) const {
    return Domain::Ptr{ new IntegerInterval(this, width, isSigned) };
}

Domain::Ptr DomainFactory::getInteger(Domain::Value value, unsigned width, bool isSigned) const {
    return Domain::Ptr{ new IntegerInterval(value, this, width, isSigned) };
}

Domain::Ptr DomainFactory::getInteger(const llvm::APSInt& val) const {
    return Domain::Ptr{ new IntegerInterval(this, val) };
}

Domain::Ptr DomainFactory::getInteger(const llvm::APSInt& from, const llvm::APSInt& to) const {
    return Domain::Ptr{ new IntegerInterval(this, from, to) };
}

Domain::Ptr DomainFactory::getFloat(const llvm::fltSemantics& semantics) const {
    return Domain::Ptr{ new FloatInterval(this, semantics) };
}

Domain::Ptr DomainFactory::getFloat(Domain::Value value, const llvm::fltSemantics& semantics) const {
    return Domain::Ptr{ new FloatInterval(value, this, semantics) };
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& val) const {
    return Domain::Ptr{ new FloatInterval(this, val) };
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& from, const llvm::APFloat& to) const {
    return Domain::Ptr{ new FloatInterval(this, from, to) };
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"