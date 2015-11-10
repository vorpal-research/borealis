//
// Created by belyaev on 5/12/15.
//

#ifndef C_STRUCT_MEMBER_H
#define C_STRUCT_MEMBER_H

#include <memory>
#include <string>
#include "Codegen/CType/CTypeRef.h"

/** protobuf -> Codegen/CType/CStructMember.proto
import "Codegen/CType/CTypeRef.proto";

package borealis.proto;

message CStructMember {
    optional uint32 offset = 1;
    optional string name = 2;
    optional CTypeRef type = 3;
}

**/

namespace borealis {

class CStructMember {
public:
    CStructMember(size_t offset, const std::string& name, const CTypeRef& type) :
        offset(offset), name(name), type(type) { }
    CStructMember(const CStructMember&) = default;
    CStructMember(CStructMember&&) = default;

private:
    size_t offset;
    std::string name;
    CTypeRef type;

public:

    using Ptr = std::shared_ptr<const CStructMember>;

    size_t getOffset() const {
        return offset;
    }

    const std::string& getName() const {
        return name;
    }

    const CTypeRef& getType() const {
        return type;
    }
};

} /* namespace borealis */

#endif /* C_STRUCT_MEMBER_H */
