//
// Created by belyaev on 5/12/15.
//

#ifndef C_TYPE_CONTEXT_H
#define C_TYPE_CONTEXT_H

#include <string>
#include "Codegen/CType/CType.h"

namespace borealis {

class CTypeContext {
    std::unordered_map<std::string, CType::Ptr> types;

public:
    using Ptr = std::shared_ptr<CTypeContext>;
    using WeakPtr = std::weak_ptr<CTypeContext>;

    CType::Ptr get(const std::string& name) const {
        return types.at(name);
    }

    void put(CType::Ptr ptr) {
        types[ptr->getName()] = ptr;
    }
};

} /* namespace borealis */

#endif /* C_TYPE_CONTEXT_H */
