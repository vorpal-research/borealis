//
// Created by belyaev on 5/12/15.
//

#ifndef C_TYPE_REF_H 
#define C_TYPE_REF_H 

#include <string>

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeContext.h"

#include "Util/generate_macros.h"

namespace borealis {

class CTypeRef {

public:
    CTypeRef(const std::string& name, CTypeContext::Ptr context) : name(name), context(context) { }

private:
    std::string name;
    CTypeContext::WeakPtr context;

public:
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(CTypeRef);

    const std::string getName() const {
        return name;
    }

    CType::Ptr get() const {
        return context.lock()->get(name);
    }

    CType::Ptr operator->() const {
        return get();
    }

    /* explicit CType::Ptr */ operator CType::Ptr () const{
        return get();
    }
};

} /* namespace borealis */

#include "Util/generate_unmacros.h"

#endif /* C_TYPE_REF_H */

