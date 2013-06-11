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

#include "Passes/ProxyFunctionPass.h"
#include "Util/passes.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

enum class NullStatus {
	Null, MaybeNull, NotNull
};

enum class NullType {
    VALUE, DEREF
};

struct NullInfo {

	typedef std::map<std::vector<unsigned>, NullStatus> OffsetInfoMap;
	typedef OffsetInfoMap::value_type OffsetInfoMapEntry;

	NullType type = NullType::VALUE;
	OffsetInfoMap offsetInfoMap;

	NullInfo& setType(
	        const NullType& type) {
	    this->type = type;
	    return *this;
	}

	NullInfo& setStatus(
			const NullStatus& status) {
		return setStatus(0, status);
	}

	NullInfo& setStatus(
			unsigned idx,
			const NullStatus& status) {
		return setStatus(std::vector<unsigned>{ idx }, status);
	}

	NullInfo& setStatus(
			const std::vector<unsigned>& v,
			const NullStatus& status) {
		offsetInfoMap[v] = status;
		return *this;
	}

	NullStatus getStatus() const {
	    using borealis::util::head;
	    using borealis::util::tail;

	    if (offsetInfoMap.empty()) return NullStatus::NotNull;

	    NullStatus res = head(offsetInfoMap).second;
	    for (const auto& e : tail(offsetInfoMap)) {
	        res = mergeStatus(res, e.second);
	    }

	    return res;
	}

	static NullStatus mergeStatus(
			const NullStatus& one,
			const NullStatus& two) {
		if (one == NullStatus::Null && two == NullStatus::Null) {
			return NullStatus::Null;
		} else if (one == NullStatus::NotNull && two == NullStatus::NotNull) {
			return NullStatus::NotNull;
		} else {
			return NullStatus::MaybeNull;
		}
	}

	NullInfo& merge(const NullInfo& other) {
	    using borealis::util::containsKey;

	    ASSERT(type == other.type, "Different NullInfo types in merge");

		for (const auto& entry : other.offsetInfoMap){
			const std::vector<unsigned>& idxs = entry.first;
			const NullStatus& status = entry.second;

			if (!containsKey(offsetInfoMap, idxs)) {
				offsetInfoMap[idxs] = status;
			} else {
				offsetInfoMap[idxs] = mergeStatus(offsetInfoMap[idxs], status);
			}
		}

		return *this;
	}

	NullInfo& merge(const NullStatus& status) {
	    if (offsetInfoMap.empty()) {
	        return setStatus(status);
	    } else {
            for (const auto& e : offsetInfoMap) {
                const std::vector<unsigned>& idxs = e.first;
                offsetInfoMap[idxs] = mergeStatus(offsetInfoMap[idxs], status);
            }
            return *this;
	    }
	}
};

class DetectNullPass:
        public ProxyFunctionPass,
        public ShouldBeModularized {

    friend class RegularDetectNullInstVisitor;
    friend class PHIDetectNullInstVisitor;

public:

	typedef std::set<llvm::Value*> NullPtrSet;

	typedef std::map<llvm::Value*, NullInfo> NullInfoMap;
	typedef NullInfoMap::value_type NullInfoMapEntry;

	static char ID;

	DetectNullPass();
	DetectNullPass(llvm::Pass*);
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
	virtual ~DetectNullPass();

	const NullInfoMap& getNullInfoMap() {
		return data;
	}

	NullPtrSet getNullSet(const NullType& type) {
		NullPtrSet res;
		for (const auto& e : data) {
			const NullInfo info = e.second;
			if (info.type == type && info.getStatus() != NullStatus::NotNull) {
                res.insert(e.first);
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

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* DETECTNULLPASS_H_ */
