//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_DOMAINFACTORY_H
#define BOREALIS_DOMAINFACTORY_H

#include <unordered_map>

#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Value.h>
#include <Util/cache.hpp>

#include "Interpreter/Domain/Domain.h"
#include "Interpreter/Domain/FloatIntervalDomain.h"
#include "Interpreter/Domain/FunctionDomain.h"
#include "Interpreter/Domain/IntegerIntervalDomain.h"
#include "Interpreter/Domain/PointerDomain.h"
#include "Interpreter/IR/GlobalVariableManager.h"
#include "Interpreter/IR/Function.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Type/TypeFactory.h"

namespace borealis {
namespace absint {

class GlobalVariableManager;

class DomainFactory: public logging::ObjectLevelLogging<DomainFactory> {
public:

    static const size_t defaultSize = 64;

    template <class Key, class Value>
    using IntCacheImpl = std::unordered_map<Key, Value, IntegerIntervalDomain::IDHash, IntegerIntervalDomain::IDEquals>;
    template <class Key, class Value>
    using FloatCacheImpl = std::unordered_map<Key, Value, FloatIntervalDomain::IDHash, FloatIntervalDomain::IDEquals>;

    DomainFactory(SlotTrackerPass* ST, GlobalVariableManager* GM, const llvm::DataLayout* DL);
    ~DomainFactory() = default;

    SlotTrackerPass& getSlotTracker() const {
        return *ST_;
    }

    TypeFactory::Ptr getTypeFactory() const {
        return TF_;
    }

    GlobalVariableManager* getGlobalVariableManager() const {
        return GVM_;
    }

    Type::Ptr cast(const llvm::Type* type);

    Domain::Ptr getTop(Type::Ptr type);
    Domain::Ptr getBottom(Type::Ptr type);

    /// simply create domain of given type
    Domain::Ptr get(const llvm::Value* val);
    Domain::Ptr get(const llvm::Constant* constant);
    Domain::Ptr get(const llvm::GlobalVariable* global);
    /// allocates value of given type, like it's a memory object
    Domain::Ptr allocate(Type::Ptr type);

    Integer::Ptr toInteger(uint64_t val, size_t width, bool isSigned = false);
    Integer::Ptr toInteger(const llvm::APInt& val);
    /// create IntegerDomain for given index (for arrays and structs)
    Domain::Ptr getIndex(uint64_t indx);

    Domain::Ptr getBool(bool value);
    Domain::Ptr getInteger(Type::Ptr type);
    Domain::Ptr getInteger(Domain::Value value, Type::Ptr type);
    Domain::Ptr getInteger(Integer::Ptr val);
    Domain::Ptr getInteger(Integer::Ptr from, Integer::Ptr to);
    Domain::Ptr getInteger(Integer::Ptr from, Integer::Ptr to, Integer::Ptr sfrom, Integer::Ptr sto);

    Domain::Ptr getFloat();
    Domain::Ptr getFloat(Domain::Value value);
    Domain::Ptr getFloat(double val);
    Domain::Ptr getFloat(const llvm::APFloat& val);
    Domain::Ptr getFloat(const llvm::APFloat& from, const llvm::APFloat& to);

    Domain::Ptr getAggregate(Domain::Value value, Type::Ptr type);
    Domain::Ptr getAggregate(Type::Ptr type);
    Domain::Ptr getAggregate(Type::Ptr type, std::vector<Domain::Ptr> elements);

    Domain::Ptr getPointer(Domain::Value value, Type::Ptr elementType);
    Domain::Ptr getPointer(Type::Ptr elementType);
    Domain::Ptr getPointer(Type::Ptr elementType, const PointerDomain::Locations& locations);
    Domain::Ptr getNullptr(Type::Ptr elementType);
    Domain::Ptr getNullptrLocation();

    Domain::Ptr getFunction(Type::Ptr type);
    Domain::Ptr getFunction(Type::Ptr type, Function::Ptr function);
    Domain::Ptr getFunction(Type::Ptr type, const FunctionDomain::FunctionSet& functions);

private:

    Domain::Ptr getConstOperand(const llvm::Constant* c);
    Domain::Ptr interpretConstantExpr(const llvm::ConstantExpr* ce);
    Domain::Ptr handleGEPConstantExpr(const llvm::ConstantExpr* ce);

    SlotTrackerPass* ST_;
    GlobalVariableManager* GVM_;
    const llvm::DataLayout* DL_;
    TypeFactory::Ptr TF_;
    util::cache<IntegerIntervalDomain::ID, Domain::Ptr, IntCacheImpl> int_cache_;
    util::cache<FloatIntervalDomain::ID, Domain::Ptr, FloatCacheImpl> float_cache_;
    Domain::Ptr nullptr_;
    PointerLocation nullptrLocation_;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_DOMAINFACTORY_H
