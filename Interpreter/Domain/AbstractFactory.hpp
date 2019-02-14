//
// Created by abdullin on 2/11/19.
//

#ifndef BOREALIS_ABSTRACTFACTORY_HPP
#define BOREALIS_ABSTRACTFACTORY_HPP

#include <unordered_map>

#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Value.h>

#include "AbstractDomain.hpp"
#include "Numerical/Number.hpp"
#include "Interpreter/IR/Function.h"
#include "Type/TypeFactory.h"

namespace borealis {
namespace absint {

template <typename Number>
class Interval;

template <typename MachineInt>
class ArrayDomain;

template <typename MachineInt>
class ArrayLocation;

template <typename MachineInt>
class StructDomain;

template <typename FunctionT, typename FHash, typename FEquals>
class FunctionDom;

template <typename MachineInt>
class StructLocation;

template <typename MachineInt>
class Pointer;

template <typename MachineInt>
class NullLocation;

template <typename MachineInt, typename FunctionT>
class FunctionLocation;

enum CastOperator {
    SIGN,
    TRUNC,
    EXT,
    SEXT,
    FPTOI,
    ITOFP,
    ITOPTR,
    PTRTOI,
    BITCAST
};

class AbstractFactory {
public:

    static const size_t defaultSize = 64;

    using BoolT = Interval<BitInt<false>>;
    using FloatT = Interval<Float>;
    using MachineInt = BitInt<false>;
    using ArrayT = ArrayDomain<MachineInt>;
    using ArrayLocationT = ArrayLocation<MachineInt>;
    using StructT = StructDomain<MachineInt>;
    using StructLocationT = StructLocation<MachineInt>;
    using FunctionT = FunctionDom<ir::Function::Ptr, ir::FunctionHash, ir::FunctionEquals>;
    using FunctionLocationT = FunctionLocation<MachineInt, FunctionT>;
    using NullLocationT = NullLocation<MachineInt>;
    using PointerT = Pointer<MachineInt>;

    enum Kind {
        TOP,
        BOTTOM
    };

private:

    TypeFactory::Ptr tf_;

private:

    AbstractFactory() : tf_(TypeFactory::get()) {}

public:

    AbstractFactory(const AbstractFactory&) = default;
    AbstractFactory(AbstractFactory&&) = default;
    AbstractFactory& operator=(const AbstractFactory&) = default;
    AbstractFactory& operator=(AbstractFactory&&) = default;

    static AbstractFactory* get() {
        static AbstractFactory* instance = new AbstractFactory();
        return instance;
    }

    TypeFactory::Ptr tf() const { return tf_; }

    AbstractDomain::Ptr top(Type::Ptr type) const;
    AbstractDomain::Ptr bottom(Type::Ptr type) const;
    AbstractDomain::Ptr allocate(Type::Ptr type) const;

    AbstractDomain::Ptr getBool(Kind kind) const;
    AbstractDomain::Ptr getBool(bool value) const;

    AbstractDomain::Ptr getMachineInt(Kind kind) const;
    AbstractDomain::Ptr getMachineInt(size_t value) const;

    AbstractDomain::Ptr getInteger(Type::Ptr type, Kind kind, bool sign = false) const;
    AbstractDomain::Ptr getInteger(unsigned long long n, unsigned width, bool sign = false) const;

    AbstractDomain::Ptr getInt(Kind kind, bool sign = false) const;
    AbstractDomain::Ptr getInt(int n, bool sign = false) const;

    AbstractDomain::Ptr getLong(Kind kind, bool sign = false) const;
    AbstractDomain::Ptr getLong(long n, bool sign = false) const;

    AbstractDomain::Ptr getFloat(Kind kind) const;
    AbstractDomain::Ptr getFloat(double n) const;

    AbstractDomain::Ptr getArray(Type::Ptr type, Kind kind) const;
    AbstractDomain::Ptr getArray(Type::Ptr type) const;
    AbstractDomain::Ptr getArray(Type::Ptr type, const std::vector<AbstractDomain::Ptr>& elements) const;

    AbstractDomain::Ptr getStruct(Type::Ptr type, Kind kind) const;
    AbstractDomain::Ptr getStruct(Type::Ptr type) const;
    AbstractDomain::Ptr getStruct(Type::Ptr type, const std::vector<AbstractDomain::Ptr>& elements) const;

    AbstractDomain::Ptr getFunction(Type::Ptr type, Kind kind) const;
    AbstractDomain::Ptr getFunction(Type::Ptr type) const;
    AbstractDomain::Ptr getFunction(ir::Function::Ptr function) const;

    AbstractDomain::Ptr getPointer(Type::Ptr type, Kind kind) const;
    AbstractDomain::Ptr getPointer(Type::Ptr type) const;
    AbstractDomain::Ptr getPointer(Type::Ptr type, AbstractDomain::Ptr base, AbstractDomain::Ptr offset) const;

    AbstractDomain::Ptr getNullptr() const;
    AbstractDomain::Ptr makeNullLocation() const;
    AbstractDomain::Ptr makeFunctionLocation(AbstractDomain::Ptr base) const;
    AbstractDomain::Ptr makeArrayLocation(AbstractDomain::Ptr base, AbstractDomain::Ptr offset) const;
    AbstractDomain::Ptr makeStructLocation(AbstractDomain::Ptr base, const std::unordered_set<AbstractDomain::Ptr>& offsets) const;

    AbstractDomain::Ptr cast(CastOperator op, Type::Ptr target, AbstractDomain::Ptr domain) const;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_ABSTRACTFACTORY_HPP
