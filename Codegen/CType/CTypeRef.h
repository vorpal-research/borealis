//
// Created by belyaev on 5/12/15.
//

#ifndef C_TYPE_REF_H 
#define C_TYPE_REF_H 

#include <string>

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeContext.h"
#include "Util/json.hpp"
#include "Util/hash.hpp"

/** protobuf -> Codegen/CType/CTypeRef.proto
package borealis.proto;

message CTypeRef {
    optional string name = 1;
}

**/

#include "Util/generate_macros.h"

namespace borealis {

class CTypeRef {

public:
    CTypeRef(const std::string& name, CTypeContext::Ptr context) : name(name), context(context) { }
    explicit CTypeRef(const std::string& name) : name(name), context() { }

    std::string name;
    CTypeContext::WeakPtr context;

public:
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(CTypeRef);
    GENERATE_PRINT(CTypeRef, name)
    GENERATE_EQ(CTypeRef, name)
    GENERATE_LESS(CTypeRef, name)

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

    friend struct util::json_traits<CTypeRef, void>;
};

} /* namespace borealis */

GENERATE_OUTLINE_JSON_TRAITS(borealis::CTypeRef, name)
GENERATE_OUTLINE_HASH(borealis::CTypeRef, name)

#include "Util/generate_unmacros.h"

#endif /* C_TYPE_REF_H */

