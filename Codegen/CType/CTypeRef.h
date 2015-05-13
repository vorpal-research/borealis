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

public:
    CTypeRef(const std::string& name, CTypeContext::Ptr context) : name(name), context(context) { }

private:
    std::string name;
    CTypeContext::WeakPtr context;

public:


    const std::string getName() const {
        return name;
    }

    CType::Ptr get() const {
        return context.lock()->get(name);
    }

    CType::Ptr operator->() const {
        return get();
    }

};

} /* namespace borealis */

#endif /* C_TYPE_REF_H */

