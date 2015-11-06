/*
 * Codegen/CType/CPointer.h
 * This file is generated from the following haskell datatype representation:
 * 
 * data CType =
    CVoid |
    CInteger { bitsize :: Size, signedness :: LLVMSignedness } |
    CFloat { bitsize:: Size } |
    CPointer { element :: Param CTypeRef } |
    CAlias { original :: Param CTypeRef, qualifier :: Exact CQualifier } |
    CArray { element :: Param CTypeRef, size :: Maybe Size } |
    CStruct { elements :: [Exact CStructMember], opaque :: Bool } |
    CFunction { resultType :: Param CTypeRef, argumentTypes :: [Param CTypeRef] }
      deriving (Show, Eq, Data, Typeable)

 * 
 * stored in Codegen/CType/CType.datatype
 * using the template file Codegen/CType/derived.h.hst
 * 
 * DO NOT EDIT THIS FILE DIRECTLY
 */

#ifndef CPOINTER_H
#define CPOINTER_H

#include "Util/util.hpp"

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeRef.h"
#include "Codegen/CType/CStructMember.h"



namespace borealis {

class CTypeFactory;

/** protobuf -> Codegen/CType/CPointer.proto
import "Codegen/CType/CType.proto";
import "Codegen/CType/CStructMember.proto";
import "Codegen/CType/CQualifier.proto";
import "Codegen/CType/CTypeRef.proto";



package borealis.proto;

message CPointer {
    extend borealis.proto.CType {
        optional CPointer ext = $COUNTER_CTYPE;
    }


    optional CTypeRef element = 1;
}

**/

class CPointer : public CType {

    typedef CPointer Self;
    typedef CType Base;

    CPointer(const std::string& name, const CTypeRef& element): CType(class_tag(*this), name), element(element) {}

public:

    friend class ::borealis::CTypeFactory;
    friend class util::enable_special_make_shared<CPointer, CTypeFactory>; // enable factory-construction only

    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

private:
    CTypeRef element;

public:
    const CTypeRef& getElement() const { return this->element; }

};

} // namespace borealis

#endif // CPOINTER_H
