/*
 * TypeFactory.cpp
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#include <Codegen/CType/CTypeUtils.h>
#include "Codegen/llvm.h"
#include "Type/RecordBody.h"
#include "Type/Type.h"
#include "Type/TypeFactory.h"
#include "Type/TypeVisitor.hpp"
#include "Util/cast.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

TypeFactory::TypeFactory() : recordBodies{ new type::RecordRegistry{} } {}

TypeFactory::Ptr TypeFactory::get() {
    static TypeFactory::Ptr instance(new TypeFactory());
    return instance;
}

Type::Ptr TypeFactory::getBool() const {
    if (!theBool) return theBool = Type::Ptr(new type::Bool());
    else return theBool;
}

Type::Ptr TypeFactory::getInteger(size_t bitsize, llvm::Signedness sign) const {
    auto&& pair = std::make_pair(bitsize, sign);
    if (auto existing = util::at(integers, pair)) return existing.getUnsafe();
    else return integers[pair] = Type::Ptr(new type::Integer(bitsize, sign));
}

Type::Ptr TypeFactory::getFloat() const {
    if (!theFloat) return theFloat = Type::Ptr(new type::Float());
    else return theFloat;
}

Type::Ptr TypeFactory::getUnknownType() const {
    if (!theUnknown) return theUnknown = Type::Ptr(new type::UnknownType());
    else return theUnknown;
}

Type::Ptr TypeFactory::getPointer(Type::Ptr to, size_t memspace) const {
    if (TypeUtils::isInvalid(to)) return to;
    auto key = std::make_pair(to, memspace);

    if (auto existing = util::at(pointers, key)) return existing.getUnsafe();
    else return pointers[key] = Type::Ptr(new type::Pointer(to, memspace));
}

Type::Ptr TypeFactory::getArray(Type::Ptr elem, size_t size) const {
    if (TypeUtils::isInvalid(elem)) return elem;
    auto&& pair = std::make_pair(elem, size);
    if (auto existing = util::at(arrays, pair)) return existing.getUnsafe();
    else return arrays[pair] = Type::Ptr(
            new type::Array(
                elem,
                size != 0U ? util::just(size) : util::nothing()
            )
        );
}

Type::Ptr TypeFactory::getRecord(const std::string& name, const llvm::StructType* st, const llvm::DataLayout* dl) const {
    static std::unordered_set<const llvm::StructType*> visitedRecordTypes;

    if (auto existing = util::at(records, name)) return existing.getUnsafe();
    else {
        if (not recordBodies->count(name) && st && not visitedRecordTypes.count(st)) {
            visitedRecordTypes.insert(st);

            if(st->isOpaque()) {
                return records[name] = getUnknownType();
            }

            auto sl = dl->getStructLayout(const_cast<llvm::StructType*>(st)); // why, llvm, why???

            type::RecordBody rb{
                util::range(0U, st->getNumElements())
                .map(LAM(ix, type::RecordField{this->cast(st->getElementType(ix), dl), sl->getElementOffsetInBits(ix)} ))
                .toVector()
            };

            embedRecordBodyNoRecursion(name, rb);
        }
        return records[name] = Type::Ptr(
            new type::Record(
                name,
                type::RecordBodyRef::Ptr(
                    new type::RecordBodyRef(
                        recordBodies,
                        name
                    )
                )
            )
        );
    }
}

Type::Ptr TypeFactory::getTypeError(const std::string& message) const {
    if (auto existing = util::at(errors, message)) return existing.getUnsafe();
    else return errors[message] = Type::Ptr(new type::TypeError(message));
}

Type::Ptr TypeFactory::getFunction(Type::Ptr retty, const std::vector<Type::Ptr>& args) const {
    std::vector<Type::Ptr> key;
    key.reserve(1 + args.size());
    key.push_back(retty);
    key.insert(key.end(), args.begin(), args.end());

    if (auto existing = util::at(functions, key)) return existing.getUnsafe();
    else return functions[std::move(key)] = Type::Ptr(new type::Function(retty, args));
}

void TypeFactory::initialize(const VariableInfoTracker& mit) {
    // FIXME: this is stupid as hell
    static const VariableInfoTracker* lastMIT;
    if(&mit == lastMIT) return;
    lastMIT = &mit;

    DebugInfoFinder dfi;
    dfi.processModule(mit.getModule());

    for (auto&& var : mit.getVars()) {
        // FIXME: this is generally fucked up...
        //        LLVM funks up MDNodes for undefs
        if (llvm::isa<llvm::UndefValue>(var.first)) continue;

        auto&& llvmType = var.first->getType();
        if (var.second.storage == StorageSpec::Memory) llvmType = llvmType->getPointerElementType();
        embedType(dfi, mit.getModule().getDataLayout(), llvmType, var.second.type);
    }
}

Type::Ptr TypeFactory::cast(const llvm::Type* type, const llvm::DataLayout* dl, llvm::Signedness sign) const {
    if (type->isVoidTy())
        return getUnknownType(); // FIXME
    else if (type->isIntegerTy())
        return (type->getIntegerBitWidth() == 1) ? getBool() : getInteger(type->getIntegerBitWidth(), sign);
    else if (type->isFloatingPointTy())
        return getFloat();
    else if (type->isPointerTy())
        return getPointer(cast(type->getPointerElementType(), dl));
    else if (type->isArrayTy())
        return getArray(cast(type->getArrayElementType(), dl), type->getArrayNumElements());
    else if (type->isVectorTy())
        return getArray(cast(type->getVectorElementType(), dl), type->getVectorNumElements());
    else if (auto* str = llvm::dyn_cast<llvm::StructType>(type)) {
        auto&& name = str->hasName() ? str->getStructName().str() : util::toString(*str);
        return getRecord(name, str, dl);
    } else if (type->isMetadataTy()) // we use metadata for unknown stuff
        return getUnknownType();
    else if (auto* func = llvm::dyn_cast<llvm::FunctionType>(type)) {
        return getFunction(
            cast(func->getReturnType(), dl),
            util::view(func->param_begin(), func->param_end())
                .map([this, dl](const llvm::Type* argty) { return cast(argty, dl); })
                .toVector()
        );
    } else return getTypeError("Unsupported llvm type: " + util::toString(*type));
}

Type::Ptr TypeFactory::embedRecordBodyNoRecursion(const std::string& name, const type::RecordBody& body) const {
    // FIXME: do something to check body compatibility if some already exists
    mergeRecordBodyInto((*recordBodies)[name], body);
    return getRecord(name);
}

void TypeFactory::mergeRecordBodyInto(type::RecordBody& lhv, const type::RecordBody& rhv) const {
    if (lhv.empty()) {
        lhv = rhv;
        return;
    }
}

Type::Ptr TypeFactory::embedRecordBodyNoRecursion(llvm::Type* type, const llvm::DataLayout* DL, CType::Ptr meta) const {
    return cast(type, DL, CTypeUtils::getSignedness(meta));
}

Type::Ptr TypeFactory::embedType(const DebugInfoFinder& /* dfi */, const llvm::DataLayout* DL, llvm::Type* type, CType::Ptr meta) const {
    return cast(type, DL, CTypeUtils::getSignedness(meta));
}

Type::Ptr TypeFactory::merge(Type::Ptr one, Type::Ptr two) {
    using borealis::util::match_pair;

    if (TypeUtils::isInvalid(one)) return one;
    if (TypeUtils::isInvalid(two)) return two;

    if (TypeUtils::isUnknown(one)) return two;
    if (TypeUtils::isUnknown(two)) return one;
    if (one == two) return one;

    if (auto match = match_pair<type::Integer, type::Integer>(one, two)) {
        // XXX: atm signedness is used only in AnnotationMaterializer,
        //      therefore we don't need to do any merge per se
        return getInteger(
            std::max(match->first->getBitsize(), match->second->getBitsize()),
            llvm::Signedness::Unknown
        );
    } else if (auto match = match_pair<type::Pointer, type::Integer>(one, two)) {
        return one;
    } else if (auto match = match_pair<type::Integer, type::Pointer>(one, two)) {
        return two;
    } else if (auto match = match_pair<type::Pointer, type::Pointer>(one, two)) {
        if (match->first->getPointed() == match->second->getPointed()) {
            return one;
        } else {
            return getPointer(
                getUnknownType() // FIXME: this is sooo fucked up...
            );
        }
    }

    return getTypeError(
        "Unmergeable types: " +
        TypeUtils::toString(*one) + " and " +
        TypeUtils::toString(*two)
    );
}

std::ostream& operator<<(std::ostream& ost, const Type& tp) {
    return ost << TypeUtils::toString(tp);
}

} /* namespace borealis */

#include "Util/unmacros.h"
