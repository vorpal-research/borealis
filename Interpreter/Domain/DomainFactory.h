//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_DOMAINFACTORY_H
#define BOREALIS_DOMAINFACTORY_H

#include <unordered_map>

#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Value.h>

#include "AggregateDomain.h"
#include "Domain.h"
#include "FloatIntervalDomain.h"
#include "FunctionDomain.h"
#include "IntegerIntervalDomain.h"
#include "Interpreter/IR/Function.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "PointerDomain.h"

namespace borealis {
namespace absint {

class Module;

class DomainFactory: public logging::ObjectLevelLogging<DomainFactory> {
public:

    using IntCache = std::unordered_map<IntegerIntervalDomain::ID,
            Domain::Ptr,
            IntegerIntervalDomain::IDHash,
            IntegerIntervalDomain::IDEquals>;

    using FloatCache = std::unordered_map<FloatIntervalDomain::ID,
            Domain::Ptr,
            FloatIntervalDomain::IDHash,
            FloatIntervalDomain::IDEquals>;

    DomainFactory(Module* module);
    ~DomainFactory();

    SlotTrackerPass& getSlotTracker() const {
        return *ST_;
    }

    Domain::Ptr getTop(const llvm::Type& type);
    Domain::Ptr getBottom(const llvm::Type& type);

    /// simply create domain of given type
    Domain::Ptr get(const llvm::Value* val);
    Domain::Ptr get(const llvm::Constant* constant);
    /// allocates value of given type, like it's a memory object
    Domain::Ptr allocate(const llvm::Type& type);

    Integer::Ptr toInteger(uint64_t val, size_t width);
    Integer::Ptr toInteger(const llvm::APInt& val);
    /// create IntegerDomain for given index (for arrays and structs)
    Domain::Ptr getIndex(uint64_t indx);

    Domain::Ptr getInteger(size_t width);
    Domain::Ptr getInteger(Domain::Value value, size_t width);
    Domain::Ptr getInteger(Integer::Ptr val);
    Domain::Ptr getInteger(Integer::Ptr from, Integer::Ptr to);

    Domain::Ptr getFloat(const llvm::fltSemantics& semantics);
    Domain::Ptr getFloat(Domain::Value value, const llvm::fltSemantics& semantics);
    Domain::Ptr getFloat(const llvm::APFloat& val);
    Domain::Ptr getFloat(const llvm::APFloat& from, const llvm::APFloat& to);

    Domain::Ptr getAggregate(Domain::Value value, const llvm::Type& type);
    Domain::Ptr getAggregate(const llvm::Type& type);
    Domain::Ptr getAggregate(const llvm::Type& type, std::vector<Domain::Ptr> elements);

    Domain::Ptr getPointer(Domain::Value value, const llvm::Type& elementType);
    Domain::Ptr getPointer(const llvm::Type& elementType);
    Domain::Ptr getPointer(const llvm::Type& elementType, const PointerDomain::Locations& locations);
    Domain::Ptr getNullptr(const llvm::Type& elementType);
    Domain::Ptr getNullptrLocation();

    Domain::Ptr getFunction(const llvm::Type& type);
    Domain::Ptr getFunction(const llvm::Type& type, Function::Ptr function);
    Domain::Ptr getFunction(const llvm::Type& type, const FunctionDomain::FunctionSet& functions);

private:

    Domain::Ptr cached(const IntegerIntervalDomain::ID& key);
    Domain::Ptr cached(const FloatIntervalDomain::ID& key);

    Domain::Ptr getConstOperand(const llvm::Constant* c);
    Domain::Ptr interpretConstantExpr(const llvm::ConstantExpr* ce);
    Domain::Ptr handleGEPConstantExpr(const llvm::ConstantExpr* ce);

    Module* module_;
    SlotTrackerPass* ST_;
    IntCache ints_;
    FloatCache floats_;
    Domain::Ptr nullptr_;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_DOMAINFACTORY_H
