//
// Created by abdullin on 2/7/17.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "IntervalDomain.h"
#include "DomainFactory.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Domain::Ptr DomainFactory::get(const llvm::Type& type) const {
    if (auto&& intType = llvm::dyn_cast<llvm::IntegerType>(&type)) {
        return getInteger(intType->getBitWidth());
    } else {
        errs() << "Creating domain of unknown type " << type << endl;
        return nullptr;
    }
}

Domain::Ptr borealis::absint::DomainFactory::get(const llvm::Value* val) const {
    return get(*val->getType());
}

Domain::Ptr DomainFactory::get(const llvm::Constant* constant) const {
    if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
        return getInteger(llvm::APSInt(intConstant->getValue()));
    } else {
        return get(llvm::cast<llvm::Value>(constant));
    }
}

Domain::Ptr DomainFactory::getInteger(unsigned width, bool isSigned) const {
    return Domain::Ptr{ new IntervalDomain(width, isSigned) };
}

Domain::Ptr DomainFactory::getInteger(const llvm::APSInt& val) const {
    return Domain::Ptr{ new IntervalDomain(val) };
}

Domain::Ptr DomainFactory::getInteger(const llvm::APSInt& from, const llvm::APSInt& to) const {
    return Domain::Ptr{ new IntervalDomain(from, to) };
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"