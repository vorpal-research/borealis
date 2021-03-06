/*
 * Type/UnknownType.h
 * This file is generated from the following haskell datatype representation:
 * 
 * data Type = 
    Integer { bitsize :: UInt, signedness :: LLVMSignedness } |
    Bool |
    Float |
    UnknownType |
    Pointer { pointed :: Type, memspace :: Size } |
    Array { element :: Type, size :: Maybe Size } |
    Record { name :: String, body :: RecordBodyRef } |
    TypeError { message :: String } |
    Function { retty :: Type, args :: [Type] }
      deriving (Show, Eq, Data, Typeable)

 * 
 * stored in Type/Type.datatype
 * using the template file Type/derived.h.hst
 * 
 * DO NOT EDIT THIS FILE DIRECTLY
 */

#ifndef UNKNOWNTYPE_H
#define UNKNOWNTYPE_H

#include "Type/Type.h"
#include "Type/RecordBody.h" // including this is generally fucked up



namespace borealis {

class TypeFactory;

namespace type {

/** protobuf -> Type/UnknownType.proto
import "Type/Type.proto";
import "Type/RecordBodyRef.proto";



package borealis.type.proto;

message UnknownType {
    extend borealis.proto.Type {
        optional UnknownType ext = 4;
    }

}

**/
class UnknownType : public Type {

    typedef UnknownType Self;
    typedef Type Base;

    UnknownType(): Type(class_tag(*this)) {}

public:
    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

};

} // namespace type
} // namespace borealis

#endif // UNKNOWNTYPE_H
