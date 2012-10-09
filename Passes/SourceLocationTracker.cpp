/*
 * SourceLocationTracker.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#include "SourceLocationTracker.h"

namespace borealis {

char SourceLocationTracker::ID;
static llvm::RegisterPass<SourceLocationTracker>
X("source-location-tracker", "Clang metadata extracter to provide values with their source locations");


bool SourceLocationTracker::runOnModule(llvm::Module& M) {
	using namespace ::llvm;
	using std::make_tuple;

	// Get metadata about functions (subprograms)
	llvm::DebugInfoFinder dif;
	dif.processModule(M);

	for (auto it = dif.subprogram_begin(); it != dif.subprogram_end(); ++it) {
		MDNode *mdnode = *it;
		DIDescriptor diDesc(mdnode);
		if (!diDesc.isSubprogram())
			continue;

		DISubprogram subProg(mdnode);
		valueDebugInfo[subProg.getFunction()] = make_tuple(subProg.getFilename().str(), subProg.getLineNumber(), 0U);
	}

	for (auto it = dif.global_variable_begin(); it != dif.global_variable_end(); ++it) {
		MDNode *mdnode = *it;
		DIDescriptor diDesc(mdnode);
		if (!diDesc.isGlobalVariable())
			continue;

		DIGlobalVariable glob(mdnode);
		valueDebugInfo[glob.getGlobal()] = make_tuple(glob.getFilename().str(), glob.getLineNumber(), 0U);
	}

	using namespace llvm;
	using borealis::util::for_each;
	using borealis::util::containsKey;
	using std::tie;
	using std::string;

	for_each(M,[this](Function& F){
		for_each(F, [this](BasicBlock& BB) {
			for_each(BB, [this](Instruction& Inst) {
				string fname;
				auto& _ = std::ignore;
				const Value* func = Inst.getParent()->getParent();
				if(containsKey(valueDebugInfo, func)) {
					tie(fname, _, _) = valueDebugInfo[Inst.getParent()->getParent()];

					const auto& loc = Inst.getDebugLoc();

					if(!loc.isUnknown()) {
						valueDebugInfo[&Inst] = make_tuple(
													fname,
													Inst.getDebugLoc().getLine(),
													Inst.getDebugLoc().getCol()
												);
					}
				}
			});
		});
	});



	return false;
}

const std::string& SourceLocationTracker::getFilenameFor(const llvm::Value* val) const {
	using namespace borealis::util;
	using namespace std;

	static std::string empty_string = "";

	if(val && containsKey(valueDebugInfo,val)) {
		return get<0>(valueDebugInfo.at(val));
	} else return empty_string;
}

unsigned SourceLocationTracker::getLineFor(const llvm::Value* val) const {
	using namespace borealis::util;
	using namespace std;

	if(val && containsKey(valueDebugInfo,val)) {
		return get<1>(valueDebugInfo.at(val));
	} else return unsigned(-1);
}

unsigned SourceLocationTracker::getColumnFor(const llvm::Value* val) const {
	using namespace borealis::util;
	using namespace std;

	if(val && containsKey(valueDebugInfo,val)) {
		return get<2>(valueDebugInfo.at(val));
	} else return unsigned(-1);
}


void SourceLocationTracker::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	Info.setPreservesAll();
}


void SourceLocationTracker::print(llvm::raw_ostream &ost, const llvm::Module *M) const {
	using borealis::util::for_each;
	using borealis::util::streams::endl;

	for_each(valueDebugInfo, [&](const valueDebugMapEntry& val){
		ost << *val.first << endl;
		ost.indent(2) << getFilenameFor(val.first)
			   << ":" << getLineFor(val.first)
			   << ":" << getColumnFor(val.first)
			   << endl;
	});
}

}
