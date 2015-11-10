//
// Created by belyaev on 10/20/15.
//

#ifndef CTYPEUTILS_H
#define CTYPEUTILS_H

#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeFactory.h"
#include "Codegen/CType/CType.def"
#include "Util/util.h"

namespace borealis {

struct CTypeUtils {

    static CType::Ptr stripAllAliasing(CType::Ptr type);
    static llvm::Signedness getSignedness(CType::Ptr type);
    static CType::Ptr loadType(CType::Ptr base);
    static CType::Ptr indexType(CType::Ptr base);
    static CStructMember::Ptr getField(CType::Ptr struct_, const std::string& fieldName);
    static CType::Ptr decayType(CTypeFactory& ctx, CType::Ptr tp);
    static CType::Ptr commonType(CTypeFactory& ctx, CType::Ptr lhv, CType::Ptr rhv);
};

} /* namespace borealis */

#endif //CTYPEUTILS_H
