/*
 * TypeVisitor.h
 *
 *  Created on: Oct 21, 2013
 *      Author: belyaev
 */

#ifndef TYPEVISITOR_HPP_
#define TYPEVISITOR_HPP_

#include "Type/Type.h"
#include "Type/Type.def"

namespace borealis {

template<class Derived, typename RetTy = void>
class TypeVisitor {
public:
    TypeVisitor() {}

    Derived* self() { return static_cast<Derived*>(this); }
    const Derived* self() const { return static_cast<const Derived*>(this); }

    RetTy visit(Type::Ptr theType) {
        if(false) {}
#define HANDLE_TYPE(NAME, CLASS) \
        else if(auto resolved = llvm::dyn_cast<type::CLASS>(theType)) { \
            return self()->visit ## NAME (*resolved); \
        }
#include "Type/Type.def"
#include "Util/macros.h"
        BYE_BYE(RetTy, "Unknown type in TypeVisitor");
#include "Util/unmacros.h"
    }

    RetTy visit(const Type& theType) {
        if(false) {}
#define HANDLE_TYPE(NAME, CLASS) \
        else if(auto resolved = llvm::dyn_cast<type::CLASS>(&theType)) { \
            return self()->visit ## NAME (*resolved); \
        }
#include "Type/Type.def"
#include "Util/macros.h"
        BYE_BYE(RetTy, "Unknown type in TypeVisitor");
#include "Util/unmacros.h"
    }

#define HANDLE_TYPE(NAME, CLASS) \
    RetTy visit ## NAME (const type::CLASS&) { return (RetTy)0; }
#include "Type/Type.def"

};

template<class Derived>
class ExhaustiveTypeVisitor: public TypeVisitor<ExhaustiveTypeVisitor<Derived>> {
public:
    ExhaustiveTypeVisitor() {}

    Derived* self() { return static_cast<Derived*>(this); }
    const Derived* self() const { return static_cast<const Derived*>(this); }

#define HANDLE_TYPE(NAME, CLASS) \
    bool preVisit ## NAME (const type::CLASS&) { \
        return true; \
    }
#include "Type/Type.def"
#define HANDLE_TYPE(NAME, CLASS) \
    void postVisit ## NAME (const type::CLASS&) {}
#include "Type/Type.def"

#define SIMPLE_VISIT(NAME, CLASS) \
    void visit ## NAME(const type::CLASS& tp) { \
        if(!self() -> preVisit ## NAME (tp)) return; \
        self() -> postVisit ## NAME (tp); \
    }
    SIMPLE_VISIT(Bool, Bool);
    SIMPLE_VISIT(Integer, Integer);
    SIMPLE_VISIT(Float, Float);
    SIMPLE_VISIT(UnknownType, UnknownType);
    SIMPLE_VISIT(TypeError, TypeError);
#undef SIMPLE_VISIT

    void visitPointer(const type::Pointer& ptr) {
        if(!self() -> preVisitPointer(ptr)) return;
        self() -> visit(ptr.getPointed());
        self() -> postVisitPointer(ptr);
    }
    void visitArray(const type::Array& arr) {
        if(!self() -> preVisitArray(arr)) return;
        self() -> visit(arr.getElement());
        self() -> postVisitArray(arr);
    }
    void visitRecord(const type::Record& rec) {
        if(!self() -> preVisitRecord(rec)) return;
        for(const auto& fld : rec.getBody()->get()) {
            self() -> visit(fld.getType());
        }
        self() -> postVisitRecord(rec);
    }
    void visitFunction(const type::Function& func) {
        if(!self() -> preVisitFunction(func)) return;
        self() -> visit(func.getRetty());
        for(const auto& arg : func.getArgs()) {
            self() -> visit(arg);
        }
        self() -> postVisitFunction(func);
    }

};

class RecordNameCollector : public ExhaustiveTypeVisitor<RecordNameCollector> {
    std::set<std::string> names;
public:
    const std::set<std::string>& getNames() const { return names; }

    bool preVisitRecord(const type::Record& rec) {
        if(names.count(rec.getName())) return false;
        names.insert(rec.getName());
        return true;
    }
};

class TypeSizer : public TypeVisitor<TypeSizer, unsigned long long> {

    using RetTy = unsigned long long;

public:
#define SIMPLE_VISIT(NAME, CLASS) \
    RetTy visit ## NAME(const type::CLASS&) { \
        return 1; \
    }
    SIMPLE_VISIT(Bool, Bool);
    SIMPLE_VISIT(Integer, Integer);
    SIMPLE_VISIT(Float, Float);
    SIMPLE_VISIT(Pointer, Pointer);
    SIMPLE_VISIT(UnknownType, UnknownType);
    SIMPLE_VISIT(TypeError, TypeError);
    SIMPLE_VISIT(Function, Function);
#undef SIMPLE_VISIT

    RetTy visitArray(const type::Array& arr) {
        auto res = self()->visit(arr.getElement());
        res *= arr.getSize().getOrElse(0ULL);
        return res;
    }
    RetTy visitRecord(const type::Record& rec) {
        auto res = 0ULL;
        for(const auto& fld : rec.getBody()->get()) {
            res += self()->visit(fld.getType());
        }
        return res != 0 ? res : 1;
    }
};

} /* namespace borealis */

#endif /* TYPEVISITOR_HPP_ */
