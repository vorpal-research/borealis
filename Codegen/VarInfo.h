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
#include "Util/locations.h"
#include "Util/option.hpp"

namespace borealis {

enum class DiscoveryDirection {
    Next,
    Previous
};

enum class DiscoveryPolicy {
    NextVal,
    PreviousVal,
    NextFunction,
    Loop
};

struct VarInfo {
    borealis::util::option<std::string> originalName;
    borealis::util::option<borealis::Locus> originalLocus;
    clang::Decl* ast;

    const VarInfo& overwriteBy(const VarInfo& vi) {
        return *this;

        if (!vi.originalName.empty()) {
            originalName = vi.originalName;
        }
        if (!vi.originalLocus.empty()) {
            originalLocus = vi.originalLocus;
        }
        if (vi.ast) {
            ast = vi.ast;
        }

        return *this;
    }
};

inline VarInfo meta2vi(const llvm::DIVariable& dd, clang::Decl* ast = nullptr) {
    using borealis::util::just;
    using borealis::Locus;
    using borealis::infos;

    infos() << dd.getVersion() << endl;

    return VarInfo{
        just(dd.getName().str()),
        just(
            Locus {
                dd.getContext().getFilename(),
                LocalLocus { dd.getLineNumber(), 0U }
            }
        ),
        ast
    };
}


template<class Streamer>
Streamer& operator<<(Streamer& ost, const VarInfo& vi) {
    ost << vi.originalName.getOrElse("<unknown-variable>")
        << " defined at ";

    if (vi.originalLocus.empty()) ost << "<unknown-location>";
    else ost << vi.originalLocus.getUnsafe();

    if (vi.ast) ost << ": " << *vi.ast;
    return ost;
}

} /* namespace borealis */

#endif /* VARINFO_H_ */
