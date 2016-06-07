//
// Created by belyaev on 5/12/15.
//

#ifndef C_TYPE_CONTEXT_H
#define C_TYPE_CONTEXT_H

#include <string>
#include "Util/collections.hpp"
#include "Codegen/CType/CType.h"

/** protobuf -> Codegen/CType/CTypeContext.proto
import "Codegen/CType/CType.proto";

package borealis.proto;

message CTypeContext {
    repeated CType types = 1;
}

**/

namespace borealis {

class CTypeContext {
    std::unordered_map<std::string, CType::Ptr> types;

public:
    using Ptr = std::shared_ptr<CTypeContext>;
    using WeakPtr = std::weak_ptr<CTypeContext>;

    bool has(const std::string& name) const {
        return !!types.count(name);
    }

    CType::Ptr get(const std::string& name) const {
        if(auto&& res = util::at(types, name)) return res.getUnsafe();
        else return nullptr;
    }

    void put(CType::Ptr ptr) {
        types[ptr->getName()] = ptr;
    }

    auto begin() const { return types.begin(); }
    auto end() const { return types.end(); }

    void selfCheck() const;
};

} /* namespace borealis */

#endif /* C_TYPE_CONTEXT_H */
