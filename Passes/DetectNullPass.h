/*
 * DetectNullPass.h
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#ifndef DETECTNULLPASS_H_
#define DETECTNULLPASS_H_

#include <llvm/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>
#include <tuple>
#include <utility>

#include "../util.h"
#include "../util.hpp"

using util::contains;
using util::containsKey;
using util::for_each;

namespace borealis {

enum NullStatus {
	Null, Maybe_Null, Not_Null
};

struct NullInfo {

	typedef std::map<std::vector<unsigned>, NullStatus> OffsetInfoMap;
	typedef std::pair<std::vector<unsigned>, NullStatus> OffsetInfoMapEntry;

	OffsetInfoMap offsetInfoMap;

	inline NullInfo& setStatus(
			const NullStatus& status) {
		return setStatus(0, status);
	}

	inline NullInfo& setStatus(
			unsigned idx,
			const NullStatus& status) {
		return setStatus(std::vector<unsigned> { idx }, status);
	}

	inline NullInfo& setStatus(
			const std::vector<unsigned>& v,
			const NullStatus& status) {
		offsetInfoMap[v] = status;
		return *this;
	}

	static NullStatus mergeStatus(
			const NullStatus& one,
			const NullStatus& two) {
		if (one == Null && two == Null) {
			return Null;
		} else if (one == Not_Null && two == Not_Null) {
			return Not_Null;
		} else {
			return Maybe_Null;
		}
	}

	NullInfo& merge(const NullInfo& other) {
		using namespace::std;

		for_each(other.offsetInfoMap, [this](const OffsetInfoMapEntry& pair){
			vector<unsigned> idxs = pair.first;
			NullStatus status = pair.second;

			if (!containsKey(offsetInfoMap, idxs)) {
				offsetInfoMap[idxs] = status;
			} else {
				offsetInfoMap[idxs] = mergeStatus(offsetInfoMap[idxs], status);
			}
		});

		return *this;
	}

	NullInfo& merge(const NullStatus& status) {
		using namespace::std;

		for_each(offsetInfoMap, [this, &status](const OffsetInfoMapEntry& pair){
			vector<unsigned> idxs = pair.first;
			offsetInfoMap[idxs] = mergeStatus(offsetInfoMap[idxs], status);
		});

		return *this;
	}
};

class DetectNullPass: public llvm::FunctionPass {

public:

	typedef std::set<const llvm::Value*> NullPtrSet;
	typedef std::map<const llvm::Value*, NullInfo> NullInfoMap;
	typedef std::pair<const llvm::Value*, NullInfo> NullInfoMapEntry;

	static char ID;

	DetectNullPass();
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~DetectNullPass();

	const NullInfoMap& getNullInfoMap() {
		return nullInfoMap;
	}

	NullPtrSet getNullSet() {
		using namespace::std;
		using namespace::llvm;

		NullPtrSet res = NullPtrSet();
		for_each(nullInfoMap, [&res](const NullInfoMapEntry& pair){
			const Value* value = pair.first;
			res.insert(value);
		});
		return res;
	}

private:

	NullInfoMap nullInfoMap;

	void processInst(const llvm::Instruction& I);

	void process(const llvm::CallInst& I);
	void process(const llvm::InsertValueInst& I);
	void process(const llvm::PHINode& I);
	void process(const llvm::StoreInst& I);

	bool containsKey(const llvm::Value& value);
};

llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const NullInfo& info);

} /* namespace borealis */

#endif /* DETECTNULLPASS_H_ */
