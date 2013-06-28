/*
 * SourceLocationTracker.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#include <llvm/ADT/DepthFirstIterator.h>

#include "Passes/Tracker/SourceLocationTracker.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

bool SourceLocationTracker::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::view;
    using borealis::util::viewContainer;

    // Get metadata about functions (subprograms)
    llvm::DebugInfoFinder dif;
    dif.processModule(M);

    for (auto* mdnode : view(dif.subprogram_begin(), dif.subprogram_end())) {
        DIDescriptor diDesc(mdnode);
        if (!diDesc.isSubprogram()) continue;

        DISubprogram subProg(mdnode);
        if (subProg.getFunction())
            valueDebugInfo.put(Locus(subProg.getFilename().str(), subProg.getLineNumber(), 0U), subProg.getFunction());
    }

    for (auto* mdnode : view(dif.global_variable_begin(), dif.global_variable_end())) {
        DIDescriptor diDesc(mdnode);
        if (!diDesc.isGlobalVariable()) continue;

        DIGlobalVariable glob(mdnode);
        if (glob.getGlobal())
            valueDebugInfo.put(Locus(glob.getFilename().str(), glob.getLineNumber(), 0U), glob.getGlobal());
    }


    for (auto& I : viewContainer(M).flatten().flatten()) {
        Value* func = I.getParent()->getParent();
        if (valueDebugInfo.contains(func)) {
            Locus retloc;
            retloc.filename = valueDebugInfo[func].filename;
            const auto& dbgloc = I.getDebugLoc();

            if (!dbgloc.isUnknown()) {
                retloc.loc = dbgloc;
            }

            if (auto* locmd = I.getMetadata("dbg")) {
                DILocation diloc(locmd);
                retloc = diloc;
            }

            valueDebugInfo.put(retloc, &I);
        }
    }

    for (auto& F : M) {
        if (F.isDeclaration()) continue;

        auto& loops = GetAnalysis<LoopInfo>::doit(this, F);

        for (auto* root : loops) {
            for (auto* loop : view(df_begin(root), df_end(root))) {
                Locus minBound;

                for (BasicBlock* bb : view(loop->block_begin(), loop->block_end())) {
                    for (Instruction& i : *bb) {
                        if (minBound.isUnknown()) minBound = getLocFor(&i);
                        else minBound = std::min(minBound, getLocFor(&i));
                    }
                }

                loopDebugInfo.put(minBound, loop->getBlocks());
            }
        }
    }

    return false;
}

const std::string& SourceLocationTracker::getFilenameFor(llvm::Value* val) const {
    static std::string empty_string = "";

    if (val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val].filename;
    } else return empty_string;
}

unsigned SourceLocationTracker::getLineFor(llvm::Value* val) const {
    if (val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val].loc.line;
    } else return unsigned(-1);
}

unsigned SourceLocationTracker::getColumnFor(llvm::Value* val) const {
    if (val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val].loc.col;
    } else return unsigned(-1);
}

const Locus& SourceLocationTracker::getLocFor(llvm::Value* val) const {
    const static Locus empty;

    if (val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val];
    } else return empty;
}

const Locus& SourceLocationTracker::getLocFor(llvm::Loop* loop) const {
    const static Locus empty;

    if (loop && loopDebugInfo.contains(loop->getBlocks())) {
        return loopDebugInfo[loop->getBlocks()];
    } else return empty;
}

SourceLocationTracker::valueDebugMap::const_range SourceLocationTracker::getRangeFor(const Locus& loc) const {
    return valueDebugInfo.range_after(loc);
}

const std::vector<llvm::BasicBlock*>& SourceLocationTracker::getLoopFor(const Locus& loc) const {
	const static std::vector<llvm::BasicBlock*> empty;

    auto range = loopDebugInfo.range_after(loc);
    if (range.first == range.second) return empty;
    else return range.first->second;
}

void SourceLocationTracker::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<llvm::LoopInfo>::addRequired(AU);
}

void SourceLocationTracker::print(llvm::raw_ostream&, const llvm::Module*) const {

    typedef std::pair<Locus, llvm::Value*> valueDebugMapEntry;
    typedef std::pair<Locus, std::vector<llvm::BasicBlock*>> loopDebugMapEntry;

    auto info = infos();

    for (const valueDebugMapEntry& val : valueDebugInfo.getFrom()) {
        info << " " << llvm::valueSummary(val.second) << " ==> " << getLocFor(val.second) << endl;
    }

    info << "loops:" << endl;
    for (const loopDebugMapEntry& val : loopDebugInfo.getFrom()) {
        info << " " << val.second << " ==> " << loopDebugInfo[val.second] << endl;
    }
}

char SourceLocationTracker::ID;
static RegisterPass<SourceLocationTracker>
X("source-location-tracker", "Clang metadata extractor to provide values with their source locations");

} // namespace borealis
