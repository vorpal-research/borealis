/*
 * TypeFactory.h
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#ifndef TYPEFACTORY_H_
#define TYPEFACTORY_H_

#include "Passes/Tracker/VariableInfoTracker.h"
#include "Type/Type.def"
#include "Type/TypeUtils.h"

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

    static TypeFactory::Ptr get();

    static constexpr size_t defaultTypeSize = 64;

    Type::Ptr getBool() const;
    Type::Ptr getInteger(size_t bitsize = defaultTypeSize, llvm::Signedness sign = llvm::Signedness::Unknown) const;
    Type::Ptr getFloat() const;
    Type::Ptr getUnknownType() const;
    Type::Ptr getPointer(Type::Ptr to) const;
    Type::Ptr getArray(Type::Ptr elem, size_t size = 0U) const;
    Type::Ptr getRecord(const std::string& name, const llvm::StructType* st = nullptr, const llvm::DataLayout* DL = nullptr) const;
    Type::Ptr getTypeError(const std::string& message) const;
    Type::Ptr getFunction(Type::Ptr retty, const std::vector<Type::Ptr>& args) const;

    void initialize(const VariableInfoTracker& mit);

    Type::Ptr cast(const llvm::Type* type, const llvm::DataLayout* dl, llvm::Signedness sign = llvm::Signedness::Unknown) const;

    Type::Ptr embedRecordBodyNoRecursion(const std::string& name, const type::RecordBody& body) const;

private:

    void mergeRecordBodyInto(type::RecordBody& lhv, const type::RecordBody& rhv) const;

    Type::Ptr embedRecordBodyNoRecursion(llvm::Type* type, const llvm::DataLayout* DL, CType::Ptr meta) const;

    Type::Ptr embedType(const DebugInfoFinder& dfi, const llvm::DataLayout* DL, llvm::Type* type, CType::Ptr meta) const;

public:

    Type::Ptr merge(Type::Ptr one, Type::Ptr two);

};

std::ostream& operator<<(std::ostream& ost, const Type& tp);

} /* namespace borealis */

#endif /* TYPEFACTORY_H_ */
