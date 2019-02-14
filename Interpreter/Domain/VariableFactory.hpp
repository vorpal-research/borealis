//
// Created by abdullin on 2/11/19.
//

#ifndef BOREALIS_VARIABLEFACTORY_HPP
#define BOREALIS_VARIABLEFACTORY_HPP

#include <unordered_map>

#include "AbstractDomain.hpp"
#include "Type/TypeFactory.h"

namespace borealis {
namespace absint {

class GlobalManager;
class AbstractFactory;

class VariableFactory {
public:

    VariableFactory(const llvm::DataLayout* dl, GlobalManager* gm);
    VariableFactory(const VariableFactory&) = default;
    VariableFactory(VariableFactory&&) = default;
    VariableFactory& operator=(const VariableFactory&) = default;
    VariableFactory& operator=(VariableFactory&&) = default;

    Type::Ptr cast(const llvm::Type* type) const;
    AbstractDomain::Ptr top(Type::Ptr type) const;
    AbstractDomain::Ptr bottom(Type::Ptr type) const;

    AbstractDomain::Ptr top(const llvm::Type* type) const {
        return top(cast(type));
    }
    AbstractDomain::Ptr bottom(const llvm::Type* type) const {
        return bottom(cast(type));
    }

    AbstractDomain::Ptr get(const llvm::Value* val) const;
    AbstractDomain::Ptr get(const llvm::Constant* constant) const;
    AbstractDomain::Ptr get(const llvm::GlobalVariable* global) const;

    AbstractDomain::Ptr findGlobal(const llvm::Value* val) const;

private:

    AbstractDomain::Ptr getConstOperand(const llvm::Constant* c) const;
    AbstractDomain::Ptr interpretConstantExpr(const llvm::ConstantExpr* ce) const;
    AbstractDomain::Ptr handleGEPConstantExpr(const llvm::ConstantExpr* ce) const;

private:

    AbstractFactory* af_;
    const llvm::DataLayout* dl_;
    GlobalManager* gm_;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_VARIABLEFACTORY_HPP
