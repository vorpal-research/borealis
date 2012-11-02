/*
 * SourceLocationTracker.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#include "SourceLocationTracker.h"

namespace borealis {

bool SourceLocationTracker::runOnModule(llvm::Module& M) {
	using namespace::llvm;

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
		valueDebugInfo.put(Locus(glob.getFilename().str(), glob.getLineNumber(), 0U), glob.getGlobal());
	}

	using borealis::util::containsKey;

	for (auto& F : M) {
		for (auto& BB : F) {
			for (auto& Inst : BB) {
				std::string fname;
				Value* func = Inst.getParent()->getParent();
				if (valueDebugInfo.contains(func)) {
					fname = valueDebugInfo[Inst.getParent()->getParent()].filename;

					const auto& loc = Inst.getDebugLoc();

					if (!loc.isUnknown()) {
						valueDebugInfo.put(Locus(
								fname,
								Inst.getDebugLoc().getLine(),
								Inst.getDebugLoc().getCol()
							), &Inst);
					}
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

	typedef std::pair<llvm::Value*, Locus> valueDebugMapEntry;

	for (const valueDebugMapEntry& val : valueDebugInfo.getTo()) {
		ost << *val.first << endl;
		ost.indent(2) << getFilenameFor(val.first)
			   << ":" << getLineFor(val.first)
			   << ":" << getColumnFor(val.first)
			   << endl;
	}
}

char SourceLocationTracker::ID;
static llvm::RegisterPass<SourceLocationTracker>
X("source-location-tracker", "Clang metadata extractor to provide values with their source locations");

} // namespace borealis
