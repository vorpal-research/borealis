/*
 * SourceLocationTracker.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#include "Logging/tracer.hpp"
#include "Passes/SourceLocationTracker.h"

namespace borealis {

bool SourceLocationTracker::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::view;
    using borealis::util::viewContainer;

    TRACE_FUNC;

    // Get metadata about functions (subprograms)
    llvm::DebugInfoFinder dif;
    dif.processModule(M);

    for (auto* mdnode : view(dif.subprogram_begin(), dif.subprogram_end())) {
        DIDescriptor diDesc(mdnode);
        if (!diDesc.isSubprogram()) continue;

        DISubprogram subProg(mdnode);
        valueDebugInfo.put(Locus(subProg.getFilename().str(), subProg.getLineNumber(), 0U), subProg.getFunction());
    }

    for (auto* mdnode : view(dif.global_variable_begin(), dif.global_variable_end())) {
        DIDescriptor diDesc(mdnode);
        if (!diDesc.isGlobalVariable()) continue;

        DIGlobalVariable glob(mdnode);
        if (glob.getGlobal()) valueDebugInfo.put(Locus(glob.getFilename().str(), glob.getLineNumber(), 0U), glob.getGlobal());
    }


    for(auto& Inst : viewContainer(M).flatten().flatten()) {
        Value* func = Inst.getParent()->getParent();
        if (valueDebugInfo.contains(func)) {
            std::string fname = valueDebugInfo[func].filename;

            Locus retloc;
            retloc.filename = fname;
            const auto& dbgloc = Inst.getDebugLoc();

            if (!dbgloc.isUnknown()) {
                retloc.loc = dbgloc;
            }

            if(auto* locmd = Inst.getMetadata("dbg")) {
                DILocation diloc(locmd);
                retloc = diloc;
            }

            valueDebugInfo.put(retloc, &Inst);
        }
    }

    return false;
}

const std::string& SourceLocationTracker::getFilenameFor(llvm::Value* val) const {
    static std::string empty_string = "";

    if(val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val].filename;
    } else return empty_string;
}

unsigned SourceLocationTracker::getLineFor(llvm::Value* val) const {
    if(val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val].loc.line;
    } else return unsigned(-1);
}

unsigned SourceLocationTracker::getColumnFor(llvm::Value* val) const {
    if(val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val].loc.col;
    } else return unsigned(-1);
}

const Locus& SourceLocationTracker::getLocFor(llvm::Value* val) const {
    const static Locus empty;

    if(val && valueDebugInfo.contains(val)) {
        return valueDebugInfo[val];
    } else return empty;
}

SourceLocationTracker::valueDebugMap::const_range SourceLocationTracker::getRangeFor(const Locus& loc) const {
    return valueDebugInfo.range_after(loc);
}

void SourceLocationTracker::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

void SourceLocationTracker::print(llvm::raw_ostream &ost, const llvm::Module*) const {
    using borealis::util::streams::endl;

    typedef std::pair<Locus, llvm::Value*> valueDebugMapEntry;

    for (const valueDebugMapEntry& val : valueDebugInfo.getFrom()) {
        ost << " " << llvm::valueSummary(val.second) << " ==> " << getLocFor(val.second) << endl;
    }
}

char SourceLocationTracker::ID;
static RegisterPass<SourceLocationTracker>
X("source-location-tracker", "Clang metadata extractor to provide values with their source locations");

} // namespace borealis
