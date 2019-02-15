//
// Created by abdullin on 2/11/19.
//

#include "AbstractFactory.hpp"

#include "Numerical/Interval.hpp"
#include "Numerical/DoubleInterval.hpp"
#include "Interpreter/Domain/Memory/PointerDomain.hpp"
#include "Memory/ArrayDomain.hpp"
#include "Memory/StructDomain.hpp"
#include "Interpreter/Domain/Memory/FunctionDomain.hpp"
#include "Memory/MemoryLocation.hpp"
#include "util/casts.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

AbstractDomain::Ptr AbstractFactory::top(Type::Ptr type) const {
    if (llvm::isa<type::UnknownType>(type.get())) {
        return nullptr;
    } else if (llvm::isa<type::Bool>(type.get())) {
        return getBool(TOP);
    } else if (llvm::isa<type::Integer>(type.get())) {
        return getInteger(type, TOP);
    } else if (llvm::isa<type::Float>(type.get())) {
        return getFloat(TOP);
    } else if (llvm::isa<type::Array>(type.get())) {
        return getArray(type, TOP);
    } else if (llvm::isa<type::Record>(type.get())) {
        return getStruct(type, TOP);
    } else if (llvm::isa<type::Function>(type.get())) {
        return getFunction(type, TOP);
    } else if (llvm::isa<type::Pointer>(type.get())) {
        return getPointer(type, TOP);
    } else {
        errs() << "Creating domain of unknown type <" << type << ">" << endl;
        return nullptr;
    }
}

AbstractDomain::Ptr AbstractFactory::bottom(Type::Ptr type) const {
    if (llvm::isa<type::UnknownType>(type.get())) {
        return nullptr;
    } else if (llvm::isa<type::Bool>(type.get())) {
        return getBool(BOTTOM);
    } else if (llvm::isa<type::Integer>(type.get())) {
        return getInteger(type, BOTTOM);
    } else if (llvm::isa<type::Float>(type.get())) {
        return getFloat(BOTTOM);
    } else if (llvm::isa<type::Array>(type.get())) {
        return getArray(type, BOTTOM);
    } else if (llvm::isa<type::Record>(type.get())) {
        return getStruct(type, BOTTOM);
    } else if (llvm::isa<type::Function>(type.get())) {
        return getFunction(type, BOTTOM);
    } else if (llvm::isa<type::Pointer>(type.get())) {
        return getPointer(type, BOTTOM);
    } else {
        errs() << "Creating domain of unknown type <" << type << ">" << endl;
        return nullptr;
    }
}

AbstractDomain::Ptr AbstractFactory::allocate(Type::Ptr type) const {
    if (llvm::isa<type::UnknownType>(type.get())) {
        return nullptr;
    } else if (llvm::isa<type::Bool>(type.get())) {
        auto intTy = tf_->getInteger(1);
        auto arrayTy = tf_->getArray(intTy, 1);
        return allocate(arrayTy);
    } else if (llvm::is_one_of<type::Integer, type::Float>(type.get())) {
        auto arrayTy = tf_->getArray(type, 1);
        return allocate(arrayTy);
    } else if (llvm::isa<type::Array>(type.get())) {
        return getArray(type);
    } else if (llvm::isa<type::Record>(type.get())) {
        return getStruct(type);
    } else if (llvm::isa<type::Function>(type.get())) {
        return getFunction(type);
    } else if (auto ptr = llvm::dyn_cast<type::Pointer>(type.get())) {
        auto location = allocate(ptr->getPointed());
        return getPointer(type, location, getMachineInt(0));
    } else {
        errs() << "Creating domain of unknown type <" << type << ">" << endl;
        return nullptr;
    }
}


AbstractDomain::Ptr AbstractFactory::getBool(AbstractFactory::Kind kind) const {
    if (kind == TOP) {
        return BoolT::top();
    } else if (kind == BOTTOM) {
        return BoolT::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getBool(bool value) const {
    return BoolT::constant(BitInt<false>(1, (int) value));
}

AbstractDomain::Ptr AbstractFactory::getMachineInt(Kind kind) const {
    if (kind == TOP) {
        return Interval<MachineInt>::top();
    } else if (kind == BOTTOM) {
        return Interval<MachineInt>::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getMachineInt(size_t value) const {
    return Interval<MachineInt>::constant(BitInt<false>(defaultSize, value));
}

AbstractDomain::Ptr AbstractFactory::getInteger(Type::Ptr type, AbstractFactory::Kind kind) const {
    auto* integer = llvm::dyn_cast<type::Integer>(type.get());
    ASSERTC(integer);

    if (kind == TOP) {
        return IntT::top();
    } else if (kind == BOTTOM) {
        return IntT::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getInteger(unsigned long long n, unsigned width) const {
    return IntT::constant(BitInt<true>(width, n), BitInt<false>(width, n));
}

AbstractDomain::Ptr AbstractFactory::getInt(AbstractFactory::Kind kind) const {
    if (kind == TOP) {
        return DoubleInterval<Int<32U, true>, Int<32U, false>>::top();
    } else if (kind == BOTTOM) {
        return DoubleInterval<Int<32U, true>, Int<32U, false>>::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getInt(int n) const {
    return DoubleInterval<Int<32U, true>, Int<32U, false>>::constant(n);
}

AbstractDomain::Ptr AbstractFactory::getLong(AbstractFactory::Kind kind) const {
    if (kind == TOP) {
        return DoubleInterval<Int<64U, true>, Int<64U, false>>::top();
    } else if (kind == BOTTOM) {
        return DoubleInterval<Int<64U, true>, Int<64U, false>>::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getLong(long n) const {
    return DoubleInterval<Int<64U, true>, Int<64U, false>>::constant(n);
}

AbstractDomain::Ptr AbstractFactory::getFloat(AbstractFactory::Kind kind) const {
    if (kind == TOP) {
        return FloatT::top();
    } else if (kind == BOTTOM) {
        return FloatT::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getFloat(double n) const {
    return FloatT::constant(n);
}

AbstractDomain::Ptr AbstractFactory::getArray(Type::Ptr type, AbstractFactory::Kind kind) const {
    auto* array = llvm::dyn_cast<type::Array>(type.get());
    ASSERTC(array);

    if (kind == TOP) {
        return ArrayT::top(array->getElement());
    } else if (kind == BOTTOM) {
        return ArrayT::bottom(array->getElement());
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getArray(Type::Ptr type) const {
    auto* array = llvm::dyn_cast<type::Array>(type.get());
    ASSERTC(array);

    auto&& length = (array->getSize()) ? getMachineInt(array->getSize().getUnsafe()) : getMachineInt(TOP);
    return ArrayT::value(array->getElement(), length);
}

AbstractDomain::Ptr AbstractFactory::getArray(Type::Ptr type, const std::vector<AbstractDomain::Ptr>& elements) const {
    auto* array = llvm::dyn_cast<type::Array>(type.get());
    ASSERTC(array);

    return ArrayT::constant(type, elements);
}

AbstractDomain::Ptr AbstractFactory::getStruct(Type::Ptr type, AbstractFactory::Kind kind) const {
    auto* record = llvm::dyn_cast<type::Record>(type.get());
    ASSERTC(record);

    auto& body = record->getBody()->get();
    StructT::Types types;
    for (auto i = 0U; i < body.getNumFields(); ++i)
        types.emplace_back(body.at(i).getType());

    if (kind == TOP) {
        return StructT::top(types);
    } else if (kind == BOTTOM) {
        return StructT::bottom(types);
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getStruct(Type::Ptr type) const {
    return getStruct(type, BOTTOM);
}

AbstractDomain::Ptr AbstractFactory::getStruct(Type::Ptr type, const std::vector<AbstractDomain::Ptr>& elements) const {
    auto* record = llvm::dyn_cast<type::Record>(type.get());
    ASSERTC(record);

    auto& body = record->getBody()->get();
    StructT::Types types;
    for (auto i = 0U; i < body.getNumFields(); ++i)
        types.emplace_back(body.at(i).getType());

    return StructT::constant(types, elements);
}

AbstractDomain::Ptr AbstractFactory::getFunction(Type::Ptr type, AbstractFactory::Kind kind) const {
    ASSERTC(llvm::isa<type::Function>(type.get()));

    if (kind == TOP) {
        return FunctionT::top(type);
    } else if (kind == BOTTOM) {
        return FunctionT::bottom(type);
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getFunction(Type::Ptr type) const {
    return getFunction(type, BOTTOM);
}

AbstractDomain::Ptr AbstractFactory::getFunction(ir::Function::Ptr function) const {
    return FunctionT::constant(function);
}

AbstractDomain::Ptr AbstractFactory::getPointer(Type::Ptr type, AbstractFactory::Kind kind) const {
    auto* ptr = llvm::dyn_cast<type::Pointer>(type.get());
    ASSERTC(ptr);

    if (kind == TOP) {
        return PointerT::top();
    } else if (kind == BOTTOM) {
        return PointerT::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getPointer(Type::Ptr type) const {
    auto* ptr = llvm::dyn_cast<type::Pointer>(type.get());
    ASSERTC(ptr);

    return getPointer(type, allocate(ptr->getPointed()), getMachineInt(0));
}

AbstractDomain::Ptr AbstractFactory::getPointer(Type::Ptr type, AbstractDomain::Ptr base, AbstractDomain::Ptr offset) const {
    auto* ptr = llvm::dyn_cast<type::Pointer>(type.get());
    ASSERTC(ptr);

    if (auto* array = llvm::dyn_cast<ArrayT>(base.get())) {
        return std::make_shared<PointerT>(makeArrayLocation(base, offset));
    } else if (auto* strct = llvm::dyn_cast<StructT>(base.get())) {
        return std::make_shared<PointerT>(makeStructLocation(base, {offset}));
    } else if (auto* func = llvm::dyn_cast<FunctionT>(base.get())) {
        return std::make_shared<PointerT>(makeFunctionLocation(base));
    } else {
        UNREACHABLE("Unknown base");
    }
}

AbstractDomain::Ptr AbstractFactory::getNullptr() const {
    static auto&& nullptrDomain = std::make_shared<PointerT>(makeNullLocation());
    return nullptrDomain;
}

AbstractDomain::Ptr AbstractFactory::makeNullLocation() const {
    return NullLocationT::get();
}

AbstractDomain::Ptr AbstractFactory::makeFunctionLocation(AbstractDomain::Ptr base) const {
    return std::make_shared<FunctionLocationT>(base);
}

AbstractDomain::Ptr AbstractFactory::makeArrayLocation(AbstractDomain::Ptr base, AbstractDomain::Ptr offset) const {
    return std::make_shared<ArrayLocationT>(base, offset);
}

AbstractDomain::Ptr AbstractFactory::makeStructLocation(AbstractDomain::Ptr base,
                                                        const std::unordered_set<AbstractDomain::Ptr>& offsets) const {
    return std::make_shared<StructLocationT>(base, offsets);
}

AbstractDomain::Ptr AbstractFactory::cast(CastOperator op, Type::Ptr target, AbstractDomain::Ptr domain) const {
    auto* integer = llvm::dyn_cast<type::Integer>(target.get());

    using SInt = Interval<BitInt<true>>;
    using UInt = Interval<BitInt<false>>;

    switch (op) {
        case SIGN:
            {
                ASSERTC(integer);
                bool targetSigned = false;
                if (integer->getSignedness() == llvm::Signedness::Signed)
                    targetSigned = true;

                if (auto* sint = llvm::dyn_cast<SInt>(domain.get())) {
                    if (targetSigned) {
                        return std::make_shared<SInt>(*sint);
                    } else {
                        return util::cast<SInt, UInt>()(*sint).shared_from_this();
                    }
                } else if (auto* uint = llvm::dyn_cast<UInt>(domain.get())) {
                    if (targetSigned) {
                        return util::cast<UInt, SInt>()(*uint).shared_from_this();
                    } else {
                        return std::make_shared<UInt>(*uint);
                    }
                } else {
                    UNREACHABLE("Unknown interval");
                }
            }
        case TRUNC:
        case EXT:
            ASSERTC(integer);

            if (auto* sint = llvm::dyn_cast<SInt>(domain.get())) {
                return util::convert<SInt>()(*sint, integer->getBitsize()).shared_from_this();
            } else if (auto* uint = llvm::dyn_cast<UInt>(domain.get())) {
                return util::convert<UInt>()(*uint, integer->getBitsize()).shared_from_this();
            } else {
                UNREACHABLE("Unknown interval");
            }
        case SEXT:
            ASSERTC(integer);

            if (auto* sint = llvm::dyn_cast<SInt>(domain.get())) {
                return util::convert<SInt>()(*sint, integer->getBitsize()).shared_from_this();
            } else if (llvm::isa<UInt>(domain.get())) {
                auto* signd = llvm::cast<SInt>(cast(SIGN, tf_->getInteger(integer->getBitsize(), llvm::Signedness::Signed), domain).get());
                return util::convert<SInt>()(*signd, integer->getBitsize()).shared_from_this();
            } else {
                UNREACHABLE("Unknown interval");
            }
        case FPTOI:
            {
                ASSERTC(integer);
                bool targetSigned = false;
                if (integer->getSignedness() == llvm::Signedness::Signed)
                    targetSigned = true;

                if (auto* fp = llvm::dyn_cast<FloatT>(domain.get())) {
                    if (targetSigned) {
                        auto&& sint = util::cast<FloatT, SInt>()(*fp).shared_from_this();
                        return cast(SEXT, target, sint);
                    } else {
                        auto&& uint = util::cast<FloatT, UInt>()(*fp).shared_from_this();
                        return cast(EXT, target, uint);
                    }
                } else {
                    UNREACHABLE("Unknown interval");
                }
            }
        case ITOFP:
            if (auto* sint = llvm::dyn_cast<SInt>(domain.get())) {
                return util::cast<SInt, FloatT>()(*sint).shared_from_this();
            } else if (auto* uint = llvm::dyn_cast<UInt>(domain.get())) {
                return util::cast<UInt, FloatT>()(*uint).shared_from_this();
            } else {
                UNREACHABLE("Unknown interval");
            }
        case ITOPTR:
            return PointerT::top();
        case PTRTOI:
            domain->setTop();
            return Interval<MachineInt>::top();
        case BITCAST:
            return top(target);
    }
    UNREACHABLE("Unknown cast operator");
}

std::pair<Bound<size_t>, Bound<size_t>> AbstractFactory::unsignedBounds(AbstractDomain::Ptr domain) const {
    if (auto* dint = llvm::dyn_cast<IntT>(domain.get())) {
        using UBound = Bound<size_t>;

        auto* unsignedInterval = llvm::dyn_cast<Interval<UInt>>(dint->second().get());
        ASSERTC(unsignedInterval);

        auto lb = unsignedInterval->lb();
        auto ub = unsignedInterval->ub();

        auto first = (lb.isFinite()) ? UBound((size_t) lb) : UBound(0);
        auto second = (ub.isFinite()) ? UBound((size_t) lb) : UBound::plusInfinity();

        return std::make_pair(first, second);
    } else {
        UNREACHABLE("Unknown numerical domain");
    }
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"