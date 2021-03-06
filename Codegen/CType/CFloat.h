/*
 * Codegen/CType/CFloat.h
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

#ifndef CFLOAT_H
#define CFLOAT_H

#include "Util/util.hpp"

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeRef.h"
#include "Codegen/CType/CStructMember.h"

#include <cstddef>

namespace borealis {

class CTypeFactory;

/** protobuf -> Codegen/CType/CFloat.proto
import "Codegen/CType/CType.proto";
import "Codegen/CType/CStructMember.proto";
import "Codegen/CType/CQualifier.proto";
import "Codegen/CType/CTypeRef.proto";



package borealis.proto;

message CFloat {
    extend borealis.proto.CType {
        optional CFloat ext = $COUNTER_CTYPE;
    }


    optional uint32 bitsize = 1;
}

**/

class CFloat : public CType {

    typedef CFloat Self;
    typedef CType Base;

    CFloat(const std::string& name, size_t bitsize): CType(class_tag(*this), name), bitsize(bitsize) {}

public:

    friend class ::borealis::CTypeFactory;
    friend class util::enable_special_make_shared<CFloat, CTypeFactory>; // enable factory-construction only

    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

private:
    size_t bitsize;

public:
    size_t getBitsize() const { return this->bitsize; }

};

} // namespace borealis

#endif // CFLOAT_H
