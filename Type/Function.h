/*
 * Type/Function.h
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

#ifndef FUNCTION_H
#define FUNCTION_H

#include "Type/Type.h"
#include "Type/RecordBody.h" // including this is generally fucked up

#include <vector>

namespace borealis {

class TypeFactory;

namespace type {

/** protobuf -> Type/Function.proto
import "Type/Type.proto";
import "Type/RecordBodyRef.proto";



package borealis.type.proto;

message Function {
    extend borealis.proto.Type {
        optional Function ext = 9;
    }

    optional borealis.proto.Type retty = 1;
    repeated borealis.proto.Type args = 2;
}

**/
class Function : public Type {

    typedef Function Self;
    typedef Type Base;

    Function(Type::Ptr retty, const std::vector<Type::Ptr>& args): Type(class_tag(*this)), retty(retty), args(args) {}

public:
    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

private:
    Type::Ptr retty;
    std::vector<Type::Ptr> args;

public:
    Type::Ptr getRetty() const { return this->retty; }
    const std::vector<Type::Ptr>& getArgs() const { return this->args; }

};

} // namespace type
} // namespace borealis

#endif // FUNCTION_H
