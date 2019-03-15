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

util::Adapter<AbstractFactory::UInt>* uintAdapter(size_t width) {
    return util::Adapter<AbstractFactory::UInt>::get(width);
}

util::Adapter<AbstractFactory::SInt>* sintAdapter(size_t width) {
    return util::Adapter<AbstractFactory::SInt>::get(width);
}

util::Adapter<Float>* floatAdapter() {
    return util::Adapter<Float>::get();
}

util::Adapter<size_t>* sizetAdapter() {
    return util::Adapter<size_t>::get();
}

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
        return BoolT::top(uintAdapter(1));
    } else if (kind == BOTTOM) {
        return BoolT::bottom(uintAdapter(1));
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getBool(bool value) const {
    return BoolT::constant(UInt(1, (int) value), uintAdapter(1));
}

AbstractDomain::Ptr AbstractFactory::getMachineInt(Kind kind) const {
    if (kind == TOP) {
        return Interval<MachineInt>::top(uintAdapter(defaultSize));
    } else if (kind == BOTTOM) {
        return Interval<MachineInt>::bottom(uintAdapter(defaultSize));
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getMachineInt(size_t value) const {
    return Interval<MachineInt>::constant(BitInt<false>(defaultSize, value), uintAdapter(defaultSize));
}

AbstractDomain::Ptr AbstractFactory::getInteger(Type::Ptr type, AbstractFactory::Kind kind) const {
    auto* integer = llvm::dyn_cast<type::Integer>(type.get());
    ASSERTC(integer);

    if (kind == TOP) {
        return IntT::top(sintAdapter(integer->getBitsize()), uintAdapter(integer->getBitsize()));
    } else if (kind == BOTTOM) {
        return IntT::bottom(sintAdapter(integer->getBitsize()), uintAdapter(integer->getBitsize()));
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getInteger(unsigned long long n, unsigned width) const {
    return IntT::constant(BitInt<true>(width, n), BitInt<false>(width, n), sintAdapter(width), uintAdapter(width));
}

AbstractDomain::Ptr AbstractFactory::getFloat(AbstractFactory::Kind kind) const {
    if (kind == TOP) {
        return FloatT::top(floatAdapter());
    } else if (kind == BOTTOM) {
        return FloatT::bottom(floatAdapter());
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getFloat(double n) const {
    return FloatT::constant(n, floatAdapter());
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

    return ArrayT::constant(array->getElement(), elements);
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

    return getPointer(base, offset);
}

AbstractDomain::Ptr AbstractFactory::getPointer(AbstractDomain::Ptr base, AbstractDomain::Ptr offset) const {
    if (llvm::isa<ArrayT>(base.get())) {
        return std::make_shared<PointerT>(makeArrayLocation(base, offset));
    } else if (llvm::isa<StructT>(base.get())) {
        return std::make_shared<PointerT>(makeStructLocation(base, {offset}));
    } else if (llvm::isa<FunctionT>(base.get())) {
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
                                                        const StructLocationT::OffsetSet& offsets) const {
    return std::make_shared<StructLocationT>(base, offsets);
}

AbstractDomain::Ptr AbstractFactory::cast(CastOperator op, Type::Ptr target, AbstractDomain::Ptr domain) const {
    auto* integer = llvm::dyn_cast<type::Integer>(target.get());
    if (not integer && llvm::isa<type::Bool>(target.get()))
        integer = llvm::cast<type::Integer>(tf_->getInteger(1, llvm::Signedness::Unsigned).get());

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
                        return sint->clone();
                    } else {
                        auto&& casted = util::cast<SInt, UInt>()(*sint);
                        return UInt::interval(casted.lb(), casted.ub());
                    }
                } else if (auto* uint = llvm::dyn_cast<UInt>(domain.get())) {
                    if (targetSigned) {
                        auto&& casted = util::cast<UInt, SInt>()(*uint);
                        return SInt::interval(casted.lb(), casted.ub());
                    } else {
                        return uint->clone();
                    }
                } else {
                    UNREACHABLE("Unknown interval");
                }
            }
        case TRUNC:
        case EXT:

            if (auto* sint = llvm::dyn_cast<SInt>(domain.get())) {
                ASSERTC(integer);
                auto&& converted = util::convert<SInt>()(*sint, integer->getBitsize());
                return SInt::interval(converted.lb(), converted.ub());
            } else if (auto* uint = llvm::dyn_cast<UInt>(domain.get())) {
                ASSERTC(integer);
                // if we are trying to extend boolean, we need to return IntT
                if (uint->lb().caster()->width() == 1) {
                    if (uint->isTop())
                        return getInteger(target, TOP);
                    else if (uint->isBottom())
                        return getInteger(target, BOTTOM);
                    else if (uint->isConstant())
                        return getInteger((size_t) uint->asConstant(), integer->getBitsize());
                    else {
                        auto&& converted = util::convert<UInt>()(*uint, integer->getBitsize());
                        auto&& casted = UInt::interval(converted.lb(), converted.ub());
                        auto&& scasted = cast(SIGN, tf_->getInteger(integer->getBitsize(), llvm::Signedness::Signed), casted);
                        return IntT::interval(scasted, casted);
                    }
                } else {
                    auto&& converted = util::convert<UInt>()(*uint, integer->getBitsize());
                    return UInt::interval(converted.lb(), converted.ub());
                }
            } else if (auto* dint = llvm::dyn_cast<IntT>(domain.get())) {
                ASSERTC(integer);
                auto&& first = cast(op, target, dint->first());
                auto&& second = cast(op, target, dint->second());
                // if we are trying to trunc integer to boolean, we just need to return uint part
                if (integer->getBitsize() == 1) {
                    return second;
                } else {
                    return IntT::interval(first, second);
                }
            } else if (llvm::isa<FloatT>(domain.get())) {
                warns() << "Trying to cast float types, not supported" << endl;
                return domain->clone();
            } else {
                UNREACHABLE("Unknown interval");
            }
        case SEXT:
            ASSERTC(integer);

            if (auto* sint = llvm::dyn_cast<SInt>(domain.get())) {
                auto&& converted = util::convert<SInt>()(*sint, integer->getBitsize());
                return SInt::interval(converted.lb(), converted.ub());
            } else if (llvm::isa<UInt>(domain.get())) {
                auto* signd = llvm::cast<SInt>(cast(SIGN, tf_->getInteger(integer->getBitsize(), llvm::Signedness::Signed), domain).get());
                auto&& converted = util::convert<SInt>()(*signd, integer->getBitsize());
                auto&& sres = SInt::interval(converted.lb(), converted.ub());
                return cast(SIGN, tf_->getInteger(integer->getBitsize(), llvm::Signedness::Unsigned), sres);
            } else if (auto* dint = llvm::dyn_cast<IntT>(domain.get())) {
                auto&& first = cast(op, target, dint->first());
                auto&& second = cast(op, target, dint->second());
                return IntT::interval(first, second);
            } else {
                UNREACHABLE("Unknown interval");
            }
        case FPTOI:
            {
                ASSERTC(integer);

                if (auto* fp = llvm::dyn_cast<FloatT>(domain.get())) {
                    auto&& casted = util::cast<FloatT, SInt>()(*fp);
                    auto&& sint = SInt::interval(casted.lb(), casted.ub());
                    auto&& first = cast(SEXT, target, sint);
                    auto&& castedU = util::cast<FloatT, UInt>()(*fp);
                    auto&& uint = UInt::interval(castedU.lb(), castedU.ub());
                    auto&& second = cast(EXT, target, uint);
                    return IntT::interval(first, second);
                } else {
                    UNREACHABLE("Unknown interval");
                }
            }
        case ITOFP:
            if (auto* sint = llvm::dyn_cast<SInt>(domain.get())) {
                auto&& casted = util::cast<SInt, FloatT>()(*sint);
                return FloatT::interval(casted.lb(), casted.ub());
            } else if (auto* uint = llvm::dyn_cast<UInt>(domain.get())) {
                auto&& casted = util::cast<UInt, FloatT>()(*uint);
                return FloatT::interval(casted.lb(), casted.ub());
            } else if (auto* dint = llvm::dyn_cast<IntT>(domain.get())) {
                return cast(op, target, dint->first());
            } else {
                UNREACHABLE("Unknown interval");
            }
        case ITOPTR:
            return PointerT::top();
        case PTRTOI:
            domain->setTop();
            return IntT::top(sintAdapter(integer->getBitsize()), uintAdapter(integer->getBitsize()));
        case BITCAST:
            return top(target);
    }
    UNREACHABLE("Unknown cast operator");
}

AbstractDomain::Ptr AbstractFactory::machineIntInterval(AbstractDomain::Ptr domain) const {
    if (auto* dint = llvm::dyn_cast<IntT>(domain.get())) {
        return cast(EXT, tf_->getInteger(defaultSize, llvm::Signedness::Unsigned), dint->second());
    } else if (llvm::isa<Interval<MachineInt>>(domain.get())) {
        return domain;
    } else {
        UNREACHABLE("Unknown numerical domain");
    }
}

std::pair<Bound<size_t>, Bound<size_t>> AbstractFactory::unsignedBounds(AbstractDomain::Ptr domain) const {
    using UBound = Bound<size_t>;

    UnsignedInterval* unsignedInterval = nullptr;
    if (auto* dint = llvm::dyn_cast<IntT>(domain.get())) {

        unsignedInterval = llvm::dyn_cast<UnsignedInterval>(dint->second().get());

    } else if (auto* uint = llvm::dyn_cast<UnsignedInterval>(domain.get())) {
        unsignedInterval = uint;
    } else {
        UNREACHABLE("Unknown numerical domain");
    }

    ASSERTC(unsignedInterval);
    auto lb = unsignedInterval->lb();
    auto ub = unsignedInterval->ub();

    auto first = (lb.isFinite()) ? UBound(sizetAdapter(), (size_t) lb) : UBound(sizetAdapter(), 0);
    auto second = (ub.isFinite()) ? UBound(sizetAdapter(), (size_t) lb) : UBound::plusInfinity(sizetAdapter());

    return std::make_pair(first, second);
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"