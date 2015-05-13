/*
 * Codegen/CType/CFunction.h
 * This file is generated from the following haskell datatype representation:
 * 
 * data CType = 
    CInteger { bitsize :: Size, signedness :: LLVMSignedness } |
    CFloat { bitsize:: Size } |
    CPointer { element :: Param CTypeRef } |
    CAlias { original :: Param CTypeRef, qualifier :: Exact CQualifier } |
    CArray { element :: Param CTypeRef, size :: Maybe Size } |
    CStruct { elements :: [Exact CStructMember] } |
    CFunction { resultType :: Param CTypeRef, argumentTypes :: [Param CTypeRef] }
      deriving (Show, Eq, Data, Typeable)

 * 
 * stored in Codegen/CType/CType.datatype
 * using the template file Codegen/CType/derived.h.hst
 * 
 * DO NOT EDIT THIS FILE DIRECTLY
 */

#ifndef CFUNCTION_H
#define CFUNCTION_H

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeRef.h"
#include "Codegen/CType/CStructMember.h"

#include <vector>

namespace borealis {

class CTypeFactory;

class CFunction : public CType {

    typedef CFunction Self;
    typedef CType Base;

    CFunction(const std::string& name, const CTypeRef& resultType, const std::vector<CTypeRef>& argumentTypes): CType(class_tag(*this), name), resultType(resultType), argumentTypes(argumentTypes) {}

public:

    friend class ::borealis::CTypeFactory;

    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

private:
    CTypeRef resultType;
    std::vector<CTypeRef> argumentTypes;

public:
    const CTypeRef& getResultType() const { return this->resultType; }
    const std::vector<CTypeRef>& getArgumentTypes() const { return this->argumentTypes; }

};

} // namespace borealis

#endif // CFUNCTION_H