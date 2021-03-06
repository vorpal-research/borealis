/*
 * Codegen/CType/CInteger.h
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

#ifndef CINTEGER_H
#define CINTEGER_H

#include "Util/util.hpp"

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeRef.h"
#include "Codegen/CType/CStructMember.h"

#include <cstddef>
#include "Util/util.h"

namespace borealis {

class CTypeFactory;

/** protobuf -> Codegen/CType/CInteger.proto
import "Codegen/CType/CType.proto";
import "Codegen/CType/CStructMember.proto";
import "Codegen/CType/CQualifier.proto";
import "Codegen/CType/CTypeRef.proto";

import "Util/Signedness.proto";

package borealis.proto;

message CInteger {
    extend borealis.proto.CType {
        optional CInteger ext = $COUNTER_CTYPE;
    }


    optional uint32 bitsize = 1;
    optional borealis.proto.Signedness signedness = 2;
}

**/

class CInteger : public CType {

    typedef CInteger Self;
    typedef CType Base;

    CInteger(const std::string& name, size_t bitsize, llvm::Signedness signedness): CType(class_tag(*this), name), bitsize(bitsize), signedness(signedness) {}

public:

    friend class ::borealis::CTypeFactory;
    friend class util::enable_special_make_shared<CInteger, CTypeFactory>; // enable factory-construction only

    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

private:
    size_t bitsize;
    llvm::Signedness signedness;

public:
    size_t getBitsize() const { return this->bitsize; }
    llvm::Signedness getSignedness() const { return this->signedness; }

};

} // namespace borealis

#endif // CINTEGER_H
