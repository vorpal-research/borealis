/*
 * TypeFactory.cpp
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

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

Type::Ptr TypeFactory::getPointer(Type::Ptr to) const {
    if (TypeUtils::isInvalid(to)) return to;
    if (auto existing = util::at(pointers, to)) return existing.getUnsafe();
    else return pointers[to] = Type::Ptr(new type::Pointer(to));
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

Type::Ptr TypeFactory::getRecord(const std::string& name, const llvm::StructType* st) const {
    static std::unordered_set<const llvm::StructType*> visitedRecordTypes;

    if (auto existing = util::at(records, name)) return existing.getUnsafe();
    else {
        if (not recordBodies->count(name) && not visitedRecordTypes.count(st)) {
            visitedRecordTypes.insert(st);

            type::RecordBody rb;
            size_t i = 0;
            for (auto&& tp : util::view(st->element_begin(), st->element_end())) {
                rb.push_back(type::RecordField{cast(tp), i++});
            }
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
    DebugInfoFinder dfi;
    dfi.processModule(mit.getModule());

    for (auto&& var : mit.getVars()) {
        // FIXME: this is generally fucked up...
        //        LLVM funks up MDNodes for undefs
        if (llvm::isa<llvm::UndefValue>(var.first)) continue;

        auto&& llvmType = var.first->getType();
        if (var.second.treatment == VarInfo::Allocated) llvmType = llvmType->getPointerElementType();
        embedType(dfi, mit.getModule().getDataLayout(), llvmType, var.second.type);
    }
}

Type::Ptr TypeFactory::cast(const llvm::Type* type, llvm::Signedness sign) const {
    if (type->isVoidTy())
        return getUnknownType(); // FIXME
    else if (type->isIntegerTy())
        return (type->getIntegerBitWidth() == 1) ? getBool() : getInteger(type->getIntegerBitWidth(), sign);
    else if (type->isFloatingPointTy())
        return getFloat();
    else if (type->isPointerTy())
        return getPointer(cast(type->getPointerElementType()));
    else if (type->isArrayTy())
        return getArray(cast(type->getArrayElementType()), type->getArrayNumElements());
    else if (auto* str = llvm::dyn_cast<llvm::StructType>(type)) {
        auto&& name = str->hasName() ? str->getStructName().str() : util::toString(*str);
        return getRecord(name, str);
    } else if (type->isMetadataTy()) // we use metadata for unknown stuff
        return getUnknownType();
    else if (auto* func = llvm::dyn_cast<llvm::FunctionType>(type)) {
        return getFunction(
            cast(func->getReturnType()),
            util::view(func->param_begin(), func->param_end())
                .map([this](const llvm::Type* argty) { return cast(argty); })
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

    for (auto&& pr : util::viewContainer(lhv) ^ util::viewContainer(rhv)) {
        auto&& lfield = pr.first;
        auto&& rfield = pr.second;
        for (auto&& id : rfield.getIds()) {
            lfield.pushId(id);
        }
    }
}

Type::Ptr TypeFactory::embedRecordBodyNoRecursion(llvm::Type* type, DIType meta) const {
    if (type->isStructTy()) {
        DIStructType str = meta;
        ASSERTC(!!str);

        auto members = str.getMembers();
        using Members = decltype(members);

        type::RecordBody body;

        if (str.isUnion()) {
            ASSERTC(1 == type->getStructNumElements());

            body.push_back(type::RecordField{
                cast(type->getStructElementType(0)),
                util::range(0U, members.getNumElements())
                    .map(std::bind(&Members::getElement, members, std::placeholders::_1))
                    .fold(std::unordered_set<std::string>{}, [](auto&& a, auto&& e) {
                        a.insert(e.getName());
                        return a;
                    })
            });

        } else {
            ASSERTC(members.getNumElements() == type->getStructNumElements());

            for (auto&& mem :
                util::view(type->subtype_begin(), type->subtype_end())
                ^
                util::range(0U, members.getNumElements())
                    .map(std::bind(&Members::getElement, members, std::placeholders::_1))
            ) {
                body.push_back(type::RecordField{
                    cast(mem.first),
                    std::unordered_set<std::string>{ mem.second.getName() }
                });
            }
        }

        return embedRecordBodyNoRecursion(type->getStructName(), body);
    }

    return cast(type, meta.getSignedness());
}

Type::Ptr TypeFactory::embedType(const DebugInfoFinder& dfi, const llvm::DataLayout* DL, llvm::Type* type, DIType meta) const {
    // FIXME: huh?
    for (auto&& expanded : flattenTypeTree(dfi, DL, {type, meta})) {
        embedRecordBodyNoRecursion(expanded.first, stripAliases(dfi, expanded.second)); // just for the side effects
    }
    return cast(type);
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
