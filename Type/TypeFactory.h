/*
 * TypeFactory.h
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#ifndef TYPEFACTORY_H_
#define TYPEFACTORY_H_

#include "Codegen/llvm.h"
#include "Type/RecordBody.h"
#include "Type/Type.h"
#include "Type/Type.def"
#include "Type/TypeVisitor.hpp"
#include "Type/TypeUtils.h"
#include "Util/cast.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

class TypeFactory {

    TypeFactory();

    mutable Type::Ptr theBool;
    mutable Type::Ptr theFloat;
    mutable Type::Ptr theUnknown;

    mutable std::map<std::pair<size_t, llvm::Signedness>, Type::Ptr> integers;
    mutable std::map<Type::Ptr,                           Type::Ptr> pointers;
    mutable std::map<std::pair<Type::Ptr, size_t>,        Type::Ptr> arrays;
    mutable std::map<std::string,                         Type::Ptr> errors;

    mutable std::map<std::string, Type::Ptr> records;
    mutable type::RecordRegistry::StrongPtr  recordBodies;

    mutable std::map<std::vector<Type::Ptr>, Type::Ptr> functions;

public:

    typedef std::shared_ptr<TypeFactory> Ptr;

    static TypeFactory::Ptr get() {
        static TypeFactory::Ptr instance(new TypeFactory());
        return instance;
    }

    Type::Ptr getBool() const {
        if(!theBool) return theBool = Type::Ptr(new type::Bool());
        else return theBool;
    }

    Type::Ptr getInteger(size_t bitsize = 32, llvm::Signedness sign = llvm::Signedness::Unknown) const {
        auto pair = std::make_pair(bitsize, sign);
        if(auto existing = util::at(integers, pair)) return existing.getUnsafe();
        else return integers[pair] = Type::Ptr(new type::Integer(bitsize, sign));
    }

    Type::Ptr getFloat() const {
        if(!theFloat) return theFloat = Type::Ptr(new type::Float());
        else return theFloat;
    }

    Type::Ptr getUnknownType() const {
        if(!theUnknown) return theUnknown = Type::Ptr(new type::UnknownType());
        else return theUnknown;
    }

    Type::Ptr getPointer(Type::Ptr to) const {
        if(TypeUtils::isInvalid(to)) return to;

        if(auto existing = util::at(pointers, to)) return existing.getUnsafe();
        else return pointers[to] = Type::Ptr(new type::Pointer(to));
    }

    Type::Ptr getArray(Type::Ptr elem, size_t size = 0U) const {
        if(TypeUtils::isInvalid(elem)) return elem;

        auto pair = std::make_pair(elem, size);
        if(auto existing = util::at(arrays, pair)) return existing.getUnsafe();
        else return arrays[pair] = Type::Ptr(
            new type::Array(
                elem,
                size != 0U ? util::just(size) : util::nothing()
            )
        );
    }

    Type::Ptr getRecord(const std::string& name) const {
        if(auto existing = util::at(records, name)) return existing.getUnsafe();
        else return records[name] = Type::Ptr(
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

    Type::Ptr getRecord(const std::string& name, const type::RecordBody& body) const {
        Type::Ptr ret = getRecord(name);
        // FIXME: do something to check body compatibility if exists
        (*recordBodies)[name] = body;
        return ret;
    }

    type::RecordRegistry::Ptr getRecordRegistry() const {
        return recordBodies;
    }

    Type::Ptr getTypeError(const std::string& message) const {
        if(auto existing = util::at(errors, message)) return existing.getUnsafe();
        else return errors[message] = Type::Ptr(new type::TypeError(message));
    }

    Type::Ptr getFunction(Type::Ptr retty, const std::vector<Type::Ptr>& args) const {
        std::vector<Type::Ptr> key;
        key.reserve(1 + args.size());
        key.push_back(retty);
        key.insert(key.end(), args.begin(), args.end());

        if(auto existing = util::at(functions, key)) return existing.getUnsafe();
        else return functions[std::move(key)] = Type::Ptr(new type::Function(retty, args));
    }

    Type::Ptr cast(const llvm::Type* type, llvm::Signedness sign = llvm::Signedness::Unknown) const {

        static std::set<std::string> visitedStructs;
        static std::unordered_map<const llvm::Type*,std::string> unnamedStructs;
        static long literalStructs = 0;

        if(type->isIntegerTy())
            return (type->getIntegerBitWidth() == 1) ? getBool() : getInteger(32, sign); // XXX: 32 -> ???
        else if(type->isFloatingPointTy())
            return getFloat();
        else if(type->isPointerTy())
            return getPointer(cast(type->getPointerElementType()));
        else if (type->isArrayTy())
            return getArray(cast(type->getArrayElementType()), type->getArrayNumElements());
        else if (auto* str = llvm::dyn_cast<llvm::StructType>(type)) {
            auto name = str->hasName() ? str->getName() : "";
            if(name.empty()) {
                if(auto unnamedName = util::at(unnamedStructs, type)) {
                    name = unnamedName.getUnsafe();
                } else {
                    name = unnamedStructs[type] = "bor.literalStruct." + util::toString(literalStructs++);
                }
            }
            if(recordBodies->at(name)) return getRecord(name);

            if(str->isOpaque() || visitedStructs.count(name)) {
                return getRecord(name);
            } else {
                visitedStructs.insert(name);
                ON_SCOPE_EXIT(visitedStructs.erase(name));

                type::RecordBody body;
                for(auto* elem : util::view(str->element_begin(), str->element_end())) {
                    auto me = cast(elem);
                    body.push_back(type::RecordField{me});
                }
                return getRecord(name, body);
            }

        } else if (type->isMetadataTy()) // we use metadata for unknown stuff
            return getUnknownType();
        else if (auto* func = llvm::dyn_cast<llvm::FunctionType>(type)) {
            return getFunction(
                cast(func->getReturnType()),
                util::view(func->param_begin(), func->param_end())
                .map([this](const llvm::Type* argty) { return cast(argty); })
                .toVector()
            );
        }
        else
            return getTypeError("Unsupported llvm type: " + util::toString(*type));
    }

private:
    Type::Ptr castNoRecursive(llvm::Type* type, DIType meta) {
        if(type->isStructTy()) {
            using std::placeholders::_1;

            DIStructType str = stripAliases(meta);
            ASSERTC(!!str);

            auto members = str.getMembers();
            ASSERTC(members.getNumElements() == type->getStructNumElements());

            using Members = decltype(members);
            type::RecordBody body;

            for(auto mem :
                util::view(type->subtype_begin(), type->subtype_end())
                ^
                util::range(0U, members.getNumElements())
                .map(std::bind(&Members::getElement, members, _1))
            ) {
                body.push_back(type::RecordField{
                    cast(mem.first),
                    std::vector<std::string>{ mem.second.getName() }
                });
            }

            return getRecord(type->getStructName(), body);
        }

        return cast(type, meta.getSignedness());
    }
public:

    Type::Ptr cast(llvm::Type* type, DIType meta) {
        // FIXME
//       for(const auto& expanded : flattenTypeTree({type, meta})) {
//           castNoRecursive(expanded.first, expanded.second); // just for the side effects
//       }
        return cast(type);
    }

    Type::Ptr merge(Type::Ptr one, Type::Ptr two) {
        using borealis::util::match_pair;

        if(TypeUtils::isInvalid(one)) return one;
        if(TypeUtils::isInvalid(two)) return two;

        if(TypeUtils::isUnknown(one)) return two;
        if(TypeUtils::isUnknown(two)) return one;
        if(one == two) return one;

        if(auto match = match_pair<type::Integer, type::Integer>(one, two)) {
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
        }

        return getTypeError(
            "Unmergeable types: " +
            TypeUtils::toString(*one) + " and " +
            TypeUtils::toString(*two)
        );
    }

};

std::ostream& operator<<(std::ostream& ost, const Type& tp);

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* TYPEFACTORY_H_ */
