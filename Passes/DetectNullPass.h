/*
 * DetectNullPass.h
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#ifndef DETECTNULLPASS_H_
#define DETECTNULLPASS_H_

#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>
#include <tuple>
#include <utility>

#include "util.h"

namespace borealis {

using util::contains;
using util::containsKey;
using util::sayonara;

enum class NullStatus {
	Null, Maybe_Null, Not_Null
};

enum class NullType {
    VALUE, DEREF
};

struct NullInfo {

	typedef std::map<std::vector<unsigned>, NullStatus> OffsetInfoMap;
	typedef std::pair<std::vector<unsigned>, NullStatus> OffsetInfoMapEntry;

	NullType type = NullType::VALUE;
	OffsetInfoMap offsetInfoMap;

	inline NullInfo& setType(
	        const NullType& type) {
	    this->type = type;
	    return *this;
	}

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
		if (one == NullStatus::Null && two == NullStatus::Null) {
			return NullStatus::Null;
		} else if (one == NullStatus::Not_Null && two == NullStatus::Not_Null) {
			return NullStatus::Not_Null;
		} else {
			return NullStatus::Maybe_Null;
		}
	}

	NullInfo& merge(const NullInfo& other) {
		if (type != other.type) {
		    sayonara(__FILE__, __LINE__,
		            "Different NullInfo types in merge");
		}

		for (const auto& entry : other.offsetInfoMap){
			std::vector<unsigned> idxs = entry.first;
			NullStatus status = entry.second;

			if (!containsKey(offsetInfoMap, idxs)) {
				offsetInfoMap[idxs] = status;
			} else {
				offsetInfoMap[idxs] = mergeStatus(offsetInfoMap[idxs], status);
			}
		}

		return *this;
	}

	NullInfo& merge(const NullStatus& status) {
		for (const auto& entry : offsetInfoMap) {
			std::vector<unsigned> idxs = entry.first;
			offsetInfoMap[idxs] = mergeStatus(offsetInfoMap[idxs], status);
		}
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
		return data;
	}

	NullPtrSet getNullSet(const NullType& type) {
		NullPtrSet res = NullPtrSet();
		for (const auto& entry : data) {
			const NullInfo info = entry.second;
			if (info.type == type) {
			    res.insert(entry.first);
			}
		}
		return res;
	}

private:

	NullInfoMap data;

	void init() {
	    data.clear();
	}
};

llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const NullInfo& info);

} /* namespace borealis */

#endif /* DETECTNULLPASS_H_ */
