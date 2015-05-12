//
// Created by belyaev on 5/12/15.
//

#ifndef C_TYPE_REF_H 
#define C_TYPE_REF_H 

#include <string>

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeContext.h"

namespace borealis {

class CTypeRef {

    std::string name;
    CTypeContext::WeakPtr context;

public:
    CType::Ptr get() const {
        return context.lock()->get(name);
    }

    CType::Ptr operator->() const {
        return get();
    }

};

} /* namespace borealis */

#endif /* C_TYPE_REF_H */

