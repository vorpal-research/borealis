/*
 * VarInfo.h
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#ifndef VARINFO_H_
#define VARINFO_H_

#include <clang/AST/DeclBase.h>
#include <llvm/Analysis/DebugInfo.h>

#include <string>

#include "Logging/logger.hpp"
#include "Util/util.h"

namespace borealis {

enum class DiscoveryDirection {
    Next,
    Previous
};

enum class DiscoveryPolicy {
    NextVal,
    PreviousVal,
    NextFunction,
    PreviousFunction,
    NextArgument,
    PreviousArgument,
    Loop
};

struct VarInfo {
    borealis::util::option<std::string> originalName;
    borealis::util::option<borealis::Locus> originalLocus;
    enum { Plain, Allocated } treatment;
    llvm::Signedness signedness;
    clang::Decl* ast;

    const VarInfo& overwriteBy(const VarInfo& vi) {

        if (!vi.originalName.empty()) {
            originalName = vi.originalName;
        }
        if (!vi.originalLocus.empty()) {
            originalLocus = vi.originalLocus;
        }
        if (vi.ast) {
            ast = vi.ast;
        }

        treatment = vi.treatment;
        signedness = vi.signedness;

        return *this;
    }
};

inline llvm::Signedness meta2sign(llvm::DIType type) {
    return type.isValid()
        ? (type.isUnsignedDIType() ? llvm::Signedness::Unsigned : llvm::Signedness::Signed)
        : (llvm::Signedness::Unknown);
}

inline VarInfo meta2vi(const llvm::DIVariable& dd, clang::Decl* ast = nullptr) {
    using borealis::util::just;
    using borealis::Locus;

    return VarInfo{
        just(dd.getName().str()),
        just(
            Locus{
                dd.getContext().getFilename(),
                LocalLocus{ dd.getLineNumber(), 0U }
            }
        ),
        VarInfo::Plain,
        meta2sign(dd.getType()),
        ast
    };
}

template<class Streamer>
Streamer& operator<<(Streamer& ost, const VarInfo& vi) {
    ost << vi.originalName.getOrElse("<unknown-variable>")
        << " defined at ";

    if (vi.originalLocus.empty()) ost << "<unknown-location>";
    else ost << vi.originalLocus.getUnsafe();

    if (vi.treatment == VarInfo::Allocated) ost << " (alloca)";

    switch (vi.signedness) {
    case llvm::Signedness::Signed:   ost << " (signed)";   break;
    case llvm::Signedness::Unsigned: ost << " (unsigned)"; break;
    case llvm::Signedness::Unknown:  ost << " (unknown)";  break;
    }

    if (vi.ast) ost << " : " << *vi.ast;
    return ost;
}

} /* namespace borealis */

#endif /* VARINFO_H_ */
