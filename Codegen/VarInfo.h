/*
 * VarInfo.h
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#ifndef VARINFO_H_
#define VARINFO_H_

#include <clang/AST/DeclBase.h>
#include <llvm/IR/DebugInfo.h>

#include <string>

#include "Codegen/llvm.h"
#include "Codegen/CType/CTypeRef.h"
#include "Logging/logger.hpp"
#include "Util/util.h"
#include "Util/enums.hpp"
#include "Util/hash.hpp"

#include "Util/generate_macros.h"

namespace borealis {

GENERATE_FANCY_ENUM(DiscoveryDirection,
    Next,
    Previous
);

GENERATE_FANCY_ENUM(DiscoveryPolicy,
    NextVal,
    PreviousVal,
    NextFunction,
    PreviousFunction,
    NextArgument,
    PreviousArgument,
    Loop,
    Global
);

GENERATE_FANCY_ENUM(StorageSpec,
    Register,
    Memory,
    Unknown
);

GENERATE_FANCY_ENUM(VariableKind,
    Local,
    Global,
    Static,
    Extern
);

} /* namespace borealis */

GENERATE_OUTLINE_ENUM_HASH(borealis::DiscoveryDirection)
GENERATE_OUTLINE_ENUM_HASH(borealis::DiscoveryPolicy)
GENERATE_OUTLINE_ENUM_HASH(borealis::StorageSpec)
GENERATE_OUTLINE_ENUM_HASH(borealis::VariableKind)

GENERATE_OUTLINE_ENUM_JSON_TRAITS(DiscoveryDirection,
    Next,
    Previous
)

GENERATE_OUTLINE_ENUM_JSON_TRAITS(DiscoveryPolicy,
    NextVal,
    PreviousVal,
    NextFunction,
    PreviousFunction,
    NextArgument,
    PreviousArgument,
    Loop,
    Global
)

GENERATE_OUTLINE_ENUM_JSON_TRAITS(StorageSpec,
    Register,
    Memory,
    Unknown
)

GENERATE_OUTLINE_ENUM_JSON_TRAITS(VariableKind,
    Local,
    Global,
    Static,
    Extern
)

namespace borealis{

struct VarInfo {
    std::string name;
    Locus locus;
    StorageSpec storage;
    CTypeRef type;
    VariableKind kind;

    GENERATE_DATA_TYPE_BP(VarInfo, name, locus, storage, type, kind);
};

} /* namespace borealis */

INLINE_TO_OUTLINE_HASH(borealis::VarInfo)

GENERATE_OUTLINE_JSON_TRAITS(borealis::VarInfo, name, locus, storage, type, kind)

#include "Util/generate_unmacros.h"

#endif /* VARINFO_H_ */
