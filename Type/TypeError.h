/*
 * Type/TypeError.h
 * This file is generated from the following haskell datatype representation:
 * 
 * data Type = Integer | Bool | Float | UnknownType | Pointer { pointed :: Type } | TypeError { message :: String } deriving (Show, Eq, Data, Typeable)
 * 
 * stored in Type/Type.datatype
 * using the template file Type/derived.h.hst
 * 
 * DO NOT EDIT THIS FILE DIRECTLY
 */

#ifndef TYPEERROR_H
#define TYPEERROR_H

#include "Type/Type.h"

#include <string>

namespace borealis {

class TypeFactory;

namespace type {

/** protobuf -> Type/TypeError.proto
import "Type/Type.proto";

package borealis.type.proto;

message TypeError {
    extend borealis.proto.Type {
        optional TypeError ext = 6;
    }
    optional string message = 1;
}

**/
class TypeError : public Type {

    typedef TypeError Self;
    typedef Type Base;

    TypeError(const std::string& message): Type(class_tag(*this)), message(message) {}

public:
    friend class ::borealis::TypeFactory;
    
    static bool classof(const Self*) { return true; }
    static bool classof(const Base* b) { return b->getClassTag() == class_tag<Self>(); }

private:
    std::string message;

public:
    const std::string& getMessage() const { return this->message; }

};

} // namespace type
} // namespace borealis

#endif // TYPEERROR_H