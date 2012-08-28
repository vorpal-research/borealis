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
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "../util.hpp"

namespace borealis {

enum NullStatus {
	Null, Maybe_Null, Not_Null
};

struct NullInfo {

	typedef std::map<std::vector<unsigned>, NullStatus> OffsetInfoMap;
	typedef std::pair<std::vector<unsigned>, NullStatus> OffsetInfoMapEntry;

	OffsetInfoMap offsetInfoMap;

	inline NullInfo& setStatus(const NullStatus& status) {
		return setStatus(0, status);
	}

	inline NullInfo& setStatus(unsigned idx, const NullStatus& status) {
		return setStatus(std::vector<unsigned> { idx }, status);
	}

	inline NullInfo& setStatus(const std::vector<unsigned>& v, const NullStatus& status) {
		this->offsetInfoMap[v] = status;
		return *this;
	}

	static NullStatus mergeStatus(const NullStatus& one, const NullStatus& two) {
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

		for_each(other.offsetInfoMap.begin(), other.offsetInfoMap.end(), [this](const OffsetInfoMapEntry& pair){
			vector<unsigned> idxs;
			NullStatus status;
			tie(idxs, status) = pair;

			if (!util::contains(this->offsetInfoMap, idxs)) {
				this->offsetInfoMap[idxs] = status;
			} else {
				this->offsetInfoMap[idxs] = mergeStatus(this->offsetInfoMap[idxs], status);
			}
		});

		return *this;
	}

	NullInfo& merge(const NullStatus& status) {
		using namespace::std;

		for_each(this->offsetInfoMap.begin(), this->offsetInfoMap.end(), [this, &status](const OffsetInfoMapEntry& pair){
			vector<unsigned> idxs;
			tie(idxs, ignore) = pair;
			this->offsetInfoMap[idxs] = mergeStatus(this->offsetInfoMap[idxs], status);
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
		return this->nullInfoMap;
	}

	std::auto_ptr<NullPtrSet> getNullSet() {
		using namespace::std;
		using namespace::llvm;

		auto res = new NullPtrSet();
		for_each(this->nullInfoMap.begin(), this->nullInfoMap.end(), [&res](const NullInfoMapEntry& pair){
			const Value* value;
			tie(value, ignore) = pair;
			res->insert(value);
		});
		return std::auto_ptr<NullPtrSet>(res);
	}

private:
	NullInfoMap nullInfoMap;

	void processInst(const llvm::Instruction& I);

	void process(const llvm::CallInst& I);
	void process(const llvm::StoreInst& I);
	void process(const llvm::PHINode& I);
	void process(const llvm::InsertValueInst& I);

	bool containsInfoForValue(const llvm::Value& value);
};

llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const NullInfo& info);

} /* namespace borealis */
#endif /* DETECTNULLPASS_H_ */
