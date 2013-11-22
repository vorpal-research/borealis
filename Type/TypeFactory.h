/*
 * TypeFactory.h
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#ifndef TYPEFACTORY_H_
#define TYPEFACTORY_H_

#include "Type/Type.h"
#include "Type/Type.def"
#include "Type/TypeVisitor.hpp"
#include "Type/RecordBody.h"
#include "Util/cast.hpp"
#include "Util/util.h"
#include "Codegen/llvm.h"

namespace borealis {

class TypeFactory {

    TypeFactory();

    mutable Type::Ptr theBool;
    mutable Type::Ptr theFloat;
    mutable Type::Ptr theUnknown;

    mutable std::map<llvm::Signedness,             Type::Ptr> integers;
    mutable std::map<Type::Ptr,                    Type::Ptr> pointers;
    mutable std::map<std::pair<Type::Ptr, size_t>, Type::Ptr> arrays;
    mutable std::map<std::string,                  Type::Ptr> errors;

    mutable std::map<std::string, Type::Ptr> records;
    mutable type::RecordRegistry::StrongPtr recordBodies;

public:

    typedef std::shared_ptr<TypeFactory> Ptr;

    static TypeFactory::Ptr get() {
        static TypeFactory::Ptr instance(new TypeFactory());
        return instance;
    }

    Type::Ptr getBool() const {
        if(!theBool) return theBool = Type::Ptr(new type::Bool());
        return theBool;
    }

    Type::Ptr getInteger(llvm::Signedness sign = llvm::Signedness::Unknown) const {
        if(!integers.count(sign)) return integers[sign] = Type::Ptr(new type::Integer(sign));
        return integers[sign];
    }

    Type::Ptr getFloat() const {
        if(!theFloat) return theFloat = Type::Ptr(new type::Float());
        return theFloat;
    }

    Type::Ptr getUnknownType() const {
        if(!theUnknown) return theUnknown = Type::Ptr(new type::UnknownType());
        return theUnknown;
    }

    Type::Ptr getPointer(Type::Ptr to) const {
        if(!isValid(to)) return to;

        if(!pointers.count(to)) return pointers[to] = Type::Ptr(new type::Pointer(to));
        return pointers[to];
    }

    Type::Ptr getArray(Type::Ptr elem, size_t size) const {
        if(!isValid(elem)) return elem;

        if(size == 0U) return getArray(elem);

        auto pair = make_pair(elem, size);
        if(auto existing = util::at(arrays, pair)) return existing.getUnsafe();
        else return arrays[pair] = Type::Ptr(new type::Array(elem, util::just(size)));
    }

    Type::Ptr getArray(Type::Ptr elem) const {
        if(!isValid(elem)) return elem;

        auto pair = make_pair(elem, 0U);
        if(auto existing = util::at(arrays, pair)) return existing.getUnsafe();
        else return arrays[pair] = Type::Ptr(new type::Array(elem, util::nothing()));
    }

    Type::Ptr getRecord(const std::string& name) const {
        if(auto existing = util::at(records, name)) {
            return existing.getUnsafe();
        } else {
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

#include "Util/macros.h"
    Type::Ptr getRecord(const std::string& name, const type::RecordBody& body) const {
        Type::Ptr ret = getRecord(name);

        // FIXME: do something to check body compatibility if exists
        (*recordBodies)[name] = body;
        return ret;
    }
#include "Util/unmacros.h"

    type::RecordRegistry::Ptr getRecordRegistry() const {
        return recordBodies;
    }

    Type::Ptr getTypeError(const std::string& message) const {
        if(!errors.count(message)) return errors[message] = Type::Ptr(new type::TypeError(message));
        return errors[message];
    }

    bool isValid(Type::Ptr type) const {
        return type->getClassTag() != class_tag<type::TypeError>();
    }

    bool isUnknown(Type::Ptr type) const {
        return type->getClassTag() == class_tag<type::UnknownType>();
    }

    Type::Ptr cast(const llvm::Type* type, llvm::Signedness sign = llvm::Signedness::Unknown) const {

        static std::set<std::string> visitedStructs {};
        static long literalStructs = 0;

        if(type->isIntegerTy())
            return (type->getIntegerBitWidth() == 1) ? getBool() : getInteger(sign);
        else if(type->isFloatingPointTy())
            return getFloat();
        else if(type->isPointerTy())
            return getPointer(cast(type->getPointerElementType()));
        else if (type->isArrayTy())
            return getArray(cast(type->getArrayElementType()), type->getArrayNumElements());
        else if (type->isStructTy()) {
#include "Util/macros.h"
            auto str = llvm::dyn_cast<llvm::StructType>(type);
            // FIXME: this is fucked up, literal (unnamed) structs are uniqued
            //        as structural types (they are rare though)
            auto name = !str->hasName() ?
                 ("bor.literalStruct." + util::toString(literalStructs++)) :
                 str->getName().str();

            if(recordBodies->at(name)) return getRecord(name);

            if(str->isOpaque() || visitedStructs.count(name)) return getRecord(name);
            else {
                visitedStructs.insert(name);
                ON_SCOPE_EXIT(visitedStructs.erase(name));

                type::RecordBody body;
                auto ix = 0U;
                for(auto* elem : util::view(str->element_begin(), str->element_end())) {
                    auto me = cast(elem);
                    body.push_back(type::RecordField{me, ix++});
                }
                return body.empty() ? getRecord(name) : getRecord(name, body);
            }
#include "Util/unmacros.h"
        }
        else if (type->isMetadataTy()) // we use metadata for unknown stuff
            return getUnknownType();
        else
            return getTypeError("Unsupported llvm type: " + toString(*type));
    }

    Type::Ptr castNoRecurse(llvm::Type* type, DIType meta) {
        if(type->isStructTy()) {
            using std::placeholders::_1;

            meta = stripAliases(meta);

            DIStructType str = meta;
            ASSERTC(!!str);

            auto members = str.getMembers();

            ASSERTC(members.getNumElements() == type->getStructNumElements());

            using Members = decltype(members);
            type::RecordBody body;

            auto ix = 0U;
            for(auto mem :
                util::view(type->subtype_begin(), type->subtype_end())
              ^ util::range(0U, members.getNumElements()).map(std::bind(&Members::getElement, members, _1))) {
                body.push_back(type::RecordField{ cast(mem.first), std::vector<std::string>{ mem.second.getName() }, ix++});
            }

            return getRecord(type->getStructName(), body);
        }
        return cast(type, meta.isUnsignedDIType() ? llvm::Signedness::Unsigned : llvm::Signedness::Signed );
    }

    Type::Ptr cast(llvm::Type* type, DIType meta) {
        auto expandedTypes = flattenTypeTree(std::make_pair(type, meta));
        for(const auto& expanded : expandedTypes) {
            castNoRecurse(expanded.first, expanded.second); // just for the side effects
        }
        return cast(type);
    }

#include "Util/macros.h"
    std::string toString(const Type& type) {
        using llvm::isa;
        using llvm::dyn_cast;

        if(isa<type::Integer>(type)) return "Integer";
        if(isa<type::Float>(type)) return "Float";
        if(isa<type::Bool>(type)) return "Bool";
        if(isa<type::UnknownType>(type)) return "Unknown";
        if(auto* Ptr = dyn_cast<type::Pointer>(&type)) return toString(*Ptr->getPointed()) + "*";
        if(auto* Err = dyn_cast<type::TypeError>(&type)) return "<Type Error>: " + Err->getMessage();
        if(auto* Arr = dyn_cast<type::Array>(&type)) {
            std::string ret = toString(*Arr->getElement()) + "[";
            for(const auto& size : Arr->getSize()) {
                ret += util::toString(size);
            }
            ret += "]";
            return std::move(ret);
        }
        if(auto* Rec = dyn_cast<type::Record>(&type)) {
            std::string ret = Rec->getName() + "{";
            ret += "}";
            return std::move(ret);
        }

        BYE_BYE(std::string, "Unknown type");
    }

    static Type::Ptr getGepChild(Type::Ptr parent, unsigned index) {

        if (auto structType = llvm::dyn_cast<type::Record>(parent)) {
            const auto& body = structType->getBody()->get();

            ASSERTC(index < body.getNumFields());

            return body.at(index).getType();
        } else if (auto* arrayType = llvm::dyn_cast<type::Array>(parent)) {
            const auto& sz = arrayType->getSize();
            // XXX: let's not try to replace defect detector with out petty logic
            // ASSERTC(!sz || (sz.getUnsafe() > index));
            if(!!sz && (sz.getUnsafe() <= index)) {
                dbgs() << "Should-be defect: array index overflow" << endl;
            }
            return arrayType->getElement();
        } else {
            return nullptr;
        }
    }

    Type::Ptr merge(Type::Ptr one, Type::Ptr two) {
        using borealis::util::match_pair;

        if(!isValid(one)) return one;
        if(!isValid(two)) return two;

        if(isUnknown(one)) return two;
        if(isUnknown(two)) return one;
        if(one == two) return one;

        if(auto match = match_pair<type::Integer, type::Integer>(one, two)) {
            // XXX: atm signedness is used only in AnnotationMaterializer,
            //      therefore we don't need to do any merge per se
            return getInteger(llvm::Signedness::Unknown);
        } else if (auto match = match_pair<type::Pointer, type::Integer>(one, two)) {
            return one;
        } else if (auto match = match_pair<type::Integer, type::Pointer>(one, two)) {
            return two;
        }

        return getTypeError(
            "Unmergeable types: " +
            toString(*one) + " and " +
            toString(*two)
        );
    }
#include "Util/unmacros.h"

};

std::ostream& operator<<(std::ostream& ost, Type::Ptr tp);

} /* namespace borealis */

#endif /* TYPEFACTORY_H_ */
