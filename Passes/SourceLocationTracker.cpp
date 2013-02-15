/*
 * SourceLocationTracker.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#include "SourceLocationTracker.h"

#include "Logging/tracer.hpp"

namespace borealis {

bool SourceLocationTracker::runOnModule(llvm::Module& M) {
	using namespace::llvm;

	TRACE_FUNC;

	// Get metadata about functions (subprograms)
	llvm::DebugInfoFinder dif;
	dif.processModule(M);

	for (auto it = dif.subprogram_begin(); it != dif.subprogram_end(); ++it) {
		MDNode *mdnode = *it;
		DIDescriptor diDesc(mdnode);
		if (!diDesc.isSubprogram())
			continue;

		DISubprogram subProg(mdnode);
		valueDebugInfo.put(Locus(subProg.getFilename().str(), subProg.getLineNumber(), 0U), subProg.getFunction());
	}

	for (auto it = dif.global_variable_begin(); it != dif.global_variable_end(); ++it) {
		MDNode *mdnode = *it;
		DIDescriptor diDesc(mdnode);
		if (!diDesc.isGlobalVariable())
			continue;

		DIGlobalVariable glob(mdnode);
		if(glob.getGlobal()) valueDebugInfo.put(Locus(glob.getFilename().str(), glob.getLineNumber(), 0U), glob.getGlobal());
	}

	using borealis::util::containsKey;

	for (auto& F : M) {
		for (auto& BB : F) {
			for (auto& Inst : BB) {
				std::string fname;
				Value* func = Inst.getParent()->getParent();
				if (valueDebugInfo.contains(func)) {
					fname = valueDebugInfo[Inst.getParent()->getParent()].filename;

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

					dbgs() << llvm::valueSummary(Inst) << " ===> " << retloc << endl;
                    valueDebugInfo.put(retloc, &Inst);
				}
			}
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

void SourceLocationTracker::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	Info.setPreservesAll();
}

void SourceLocationTracker::print(llvm::raw_ostream &ost, const llvm::Module*) const {
	using borealis::util::streams::endl;

	typedef std::pair<Locus, llvm::Value*> valueDebugMapEntry;

	for (const valueDebugMapEntry& val : valueDebugInfo.getFrom()) {
		ost << " " << llvm::valueSummary(val.second) << " ==> " << getLocFor(val.second) << endl;
	}
}

char SourceLocationTracker::ID;
static llvm::RegisterPass<SourceLocationTracker>
X("source-location-tracker", "Clang metadata extractor to provide values with their source locations");

} // namespace borealis
