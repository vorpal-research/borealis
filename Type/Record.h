/*
 * Type/Record.h
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

#ifndef RECORD_H
#define RECORD_H

#include "Type/Type.h"
#include "Type/RecordBody.h" // including this is generally fucked up

#include <string>

namespace borealis {

class TypeFactory;

namespace type {

/** protobuf -> Type/Record.proto
import "Type/Type.proto";
import "Type/RecordBodyRef.proto";



package borealis.type.proto;

message Record {
    extend borealis.proto.Type {
        optional Record ext = 7;
    }

    optional string name = 1;
    optional borealis.proto.RecordBodyRef body = 2;
}

**/
class Record : public Type {

    typedef Record Self;
    typedef Type Base;

    Record(const std::string& name, RecordBodyRef::Ptr body): Type(class_tag(*this)), name(name), body(body) {}

public:
    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

private:
    std::string name;
    RecordBodyRef::Ptr body;

public:
    const std::string& getName() const { return this->name; }
    RecordBodyRef::Ptr getBody() const { return this->body; }

};

} // namespace type
} // namespace borealis

#endif // RECORD_H
