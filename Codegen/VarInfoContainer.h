/*
 * VarInfoContainer.h
 *
 *  Created on: Jan 14, 2013
 *      Author: belyaev
 */

#ifndef VARINFOCONTAINER_H_
#define VARINFOCONTAINER_H_

#include <llvm/Value.h>

#include <map>
#include <string>
#include <unordered_map>

#include "Codegen/VarInfo.h"
#include "Util/key_ptr.hpp"
#include "Util/util.h"

namespace borealis {

// time optimized multidirectional container from Value* to VarInfo
class VarInfoContainer {
    typedef std::unordered_map<llvm::Value*, VarInfo> v2vi_t;
    typedef std::unordered_multimap<util::key_ptr<std::string>, llvm::Value*> str2v_t;
    typedef std::multimap<util::key_ptr<Locus>, llvm::Value*> loc2v_t;
    typedef std::unordered_map<clang::Decl*, llvm::Value*> clang2v_t;

    // fwd keeps the actual data
    v2vi_t fwd;

    // all other containers operate on pointers into fwd through key_ptrs
    // (or just plain pointers into llvm or clang inner memory)
    str2v_t bwd_names;
    loc2v_t bwd_locs;
    clang2v_t bwd_clang;

public:
    typedef loc2v_t::const_iterator loc_value_iterator;
    typedef std::pair<loc_value_iterator, loc_value_iterator> loc_value_range;

    typedef str2v_t::const_iterator str_value_iterator;
    typedef std::pair<str_value_iterator, str_value_iterator> str_value_range;

    typedef v2vi_t::const_iterator const_iterator;

    VarInfoContainer();
    ~VarInfoContainer();

    void put(llvm::Value* val, const VarInfo& vi) {
        using util::key_ptr;

        if (fwd.count(val)) {
            auto& old_vi = fwd[val];
            for (std::string& viname: old_vi.originalName)
                util::removeFromMultimap(bwd_names, key_ptr<std::string>(viname), val);
            for (Locus& viloc: old_vi.originalLocus)
                util::removeFromMultimap(bwd_locs, key_ptr<Locus>(viloc), val);
            if (old_vi.ast && bwd_clang.count(old_vi.ast))
                bwd_clang.erase(old_vi.ast);
        }

        fwd[val] = vi;
        // ▲ that was a copy assignment
        // and this ▼ is taking a reference
        auto& new_vi = fwd[val]; // vi and new_vi are NOT the same

        for (const auto& name: new_vi.originalName) {
            bwd_names.insert({ key_ptr<std::string>(name), val });
        }
        for (const auto& loc: new_vi.originalLocus) {
            bwd_locs.insert({ key_ptr<Locus>(loc), val });
        }
        if (new_vi.ast) {
            bwd_clang.insert({new_vi.ast, val});
        }
    }

    const VarInfo& get(llvm::Value* val) const { return fwd.at(val); }
    str_value_range byName(const std::string& str) const {
        return bwd_names.equal_range(str);
    }

    loc_value_iterator byLocIterFwd(const Locus& loc){
        return bwd_locs.lower_bound(loc);
    }

    loc_value_iterator byLocIterBwd(const Locus& loc){
        return --bwd_locs.lower_bound(loc);
    }

    loc_value_range byLoc(const Locus& loc, DiscoveryDirection dp = DiscoveryDirection::Next) const {
        if (dp == DiscoveryDirection::Next) {
            auto start = bwd_locs.lower_bound(loc);
            auto end = start;
            while (end != bwd_locs.end() && end->first == start->first) ++end;
            return std::make_pair(start, end);
        } else { // DiscoveryDirection::Previous
            auto end = bwd_locs.lower_bound(loc);
            --end;
            auto start = end;
            while (start != bwd_locs.begin() && start->first == end->first) --start;
            return std::make_pair(start, end);
        }
    }

    llvm::Value* byClang(clang::Decl* dcl) const {
        return bwd_clang.at(dcl);
    }

    const_iterator begin() const {
        return fwd.begin();
    }

    const_iterator end() const {
        return fwd.end();
    }
};

} /* namespace borealis */

#endif /* VARINFOCONTAINER_H_ */
