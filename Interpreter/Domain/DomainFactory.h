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

template <unsigned int width, bool sign>
class Int;

template <typename Number>
class Interval;

template <typename MachineInt>
class ArrayDomain;

template <typename MachineInt>
class ArrayLocation;

template <typename MachineInt>
class StructDomain;

template <typename MachineInt>
class StructLocation;

template <typename MachineInt>
class Pointer;

template <typename MachineInt>
class NullLocation;

class AbstractFactory {
public:

    static const size_t defaultSize = 64;

    using BoolT = Interval<Int<1U, false>>;
    using MachineIntT = Interval<Int<defaultSize, false>>;
    using ArrayT = ArrayDomain<MachineIntT>;
    using ArrayLocationT = ArrayLocation<MachineIntT>;
    using StructT = StructDomain<MachineIntT>;
    using StructLocationT = StructLocation<MachineIntT>;
    using PointerT = Pointer<MachineIntT>;
    using NullLocationT = NullLocation<MachineIntT>;

    enum Kind {
        TOP,
        BOTTOM
    };

private:

    TypeFactory::Ptr TF_;

private:

    AbstractFactory() : TF_(TypeFactory::get()) {}

public:

    AbstractFactory(const AbstractFactory&) = delete;
    AbstractFactory(AbstractFactory&&) = delete;
    AbstractFactory& operator=(const AbstractFactory&) = delete;
    AbstractFactory& operator=(AbstractFactory&&) = delete;

    static AbstractFactory* get() {
        static AbstractFactory* instance = new AbstractFactory();
        return instance;
    }

    AbstractDomain::Ptr top(Type::Ptr) const;
    AbstractDomain::Ptr bottom(Type::Ptr) const;

    AbstractDomain::Ptr getBool(Kind kind) const;
    AbstractDomain::Ptr getBool(bool value) const;

    AbstractDomain::Ptr getInteger(Type::Ptr, Kind kind, bool sign = false) const;
    AbstractDomain::Ptr getInteger(unsigned long long n, unsigned width, bool sign = false) const;

    AbstractDomain::Ptr getInt(Kind kind, bool sign = false) const;
    AbstractDomain::Ptr getInt(int n, bool sign = false) const;

    AbstractDomain::Ptr getLong(Kind kind, bool sign = false) const;
    AbstractDomain::Ptr getLong(long n, bool sign = false) const;
//
//    template <size_t width, bool sign = true>
//    AbstractDomain::Ptr getInt(const Int<width, sign>& n) const {
//        return Interval<Int<width, sign>>::constant(n);
//    }
//
//    AbstractDomain::Ptr getFloat(Type::Ptr, Kind kind) const;
//    AbstractDomain::Ptr getFloat(double n) const;
//    AbstractDomain::Ptr getFloat(const Float<>& n) const;
//
//    AbstractDomain::Ptr getArray(Type::Ptr, Kind kind) const;
//    AbstractDomain::Ptr getArray(Type::Ptr, const std::vector<AbstractDomain::Ptr>& elements) const;
//
//    AbstractDomain::Ptr getStruct(Type::Ptr, Kind kind) const;
//    AbstractDomain::Ptr getStruct(Type::Ptr, const std::vector<AbstractDomain::Ptr>& elements) const;
//
//    AbstractDomain::Ptr getPointer(Type::Ptr, Kind kind) const;
//    AbstractDomain::Ptr getPointer(Type::Ptr, const PointerT::PointsToSet& locations) const;
//
//    AbstractDomain::Ptr getNullptr(Type::Ptr) const;
//
//    AbstractDomain::Ptr getNullLocation() const;
//    AbstractDomain::Ptr makeLocation(AbstractDomain::Ptr base, AbstractDomain::Ptr offset) const;
//    AbstractDomain::Ptr makeLocation(AbstractDomain::Ptr base, const std::unordered_set<AbstractDomain::Ptr>& offsets) const;
};


namespace ir {
class GlobalVariableManager;
}   // namespace ir

class DomainFactory: public logging::ObjectLevelLogging<DomainFactory> {
public:

    static const size_t defaultSize = 64;

    template <class Key, class Value>
    using IntCacheImpl = std::unordered_map<Key, Value, IntegerIntervalDomain::IDHash, IntegerIntervalDomain::IDEquals>;
    template <class Key, class Value>
    using FloatCacheImpl = std::unordered_map<Key, Value, FloatIntervalDomain::IDHash, FloatIntervalDomain::IDEquals>;

    DomainFactory(SlotTrackerPass* ST, ir::GlobalVariableManager* GM, const llvm::DataLayout* DL);
    ~DomainFactory() = default;

    SlotTrackerPass& getSlotTracker() const {
        return *ST_;
    }

    TypeFactory::Ptr getTypeFactory() const {
        return TF_;
    }

    ir::GlobalVariableManager* getGlobalVariableManager() const {
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

    Domain::Ptr getPointer(Domain::Value value, Type::Ptr elementType, bool isGep = false);
    Domain::Ptr getPointer(Type::Ptr elementType, bool isGep = false);
    Domain::Ptr getPointer(Type::Ptr elementType, const PointerDomain::Locations& locations, bool isGep = false);
    Domain::Ptr getNullptr(Type::Ptr elementType);
    Domain::Ptr getNullptrLocation();

    Domain::Ptr getFunction(Type::Ptr type);
    Domain::Ptr getFunction(Type::Ptr type, ir::Function::Ptr function);
    Domain::Ptr getFunction(Type::Ptr type, const FunctionDomain::FunctionSet& functions);

private:

    Domain::Ptr getConstOperand(const llvm::Constant* c);
    Domain::Ptr interpretConstantExpr(const llvm::ConstantExpr* ce);
    Domain::Ptr handleGEPConstantExpr(const llvm::ConstantExpr* ce);

    SlotTrackerPass* ST_;
    ir::GlobalVariableManager* GVM_;
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
