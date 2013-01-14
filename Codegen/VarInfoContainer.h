/*
 * VarInfoContainer.h
 *
 *  Created on: Jan 14, 2013
 *      Author: belyaev
 */

#ifndef VARINFOCONTAINER_H_
#define VARINFOCONTAINER_H_

#include <string>
#include <map>
#include <unordered_map>

#include <llvm/Value.h>

#include "VarInfo.h"
#include "Util/key_ptr.hpp"
#include "Util/util.h"

namespace borealis {

// time optimized multidirectional container from Value* to VarInfo
class VarInfoContainer {
    typedef std::unordered_map<llvm::Value*, VarInfo> v2vi_t;
    typedef std::unordered_map<util::key_ptr<std::string>, llvm::Value*> str2v_t;
    typedef std::multimap<util::key_ptr<Locus>, llvm::Value*> loc2v_t;
    typedef std::unordered_map<clang::Decl*, llvm::Value*> clang2v_t;

    v2vi_t fwd;
    str2v_t bwd_names;
    loc2v_t bwd_locs;
    clang2v_t bwd_clang;

public:
    typedef loc2v_t::const_iterator loc_value_iterator;
    typedef std::pair<loc_value_iterator, loc_value_iterator> loc_value_range;

    typedef v2vi_t::const_iterator const_iterator;

    VarInfoContainer();
    ~VarInfoContainer();

    void put(llvm::Value* val, const VarInfo& vi) {
        using std::make_pair;
        using util::key_ptr;

        if(fwd.count(val)) {
            auto& old_vi = fwd[val];
            for(std::string& viname: old_vi.originalName)
                bwd_names.erase(viname);
            for(Locus& viloc: old_vi.originalLocus)
                util::removeFromMultimap(bwd_locs, key_ptr<Locus>(viloc), val);
            if(old_vi.ast && bwd_clang.count(old_vi.ast))
                bwd_clang.erase(old_vi.ast);
        }

        fwd[val] = vi;
        auto& new_vi = fwd[val];
        for(const auto& name: new_vi.originalName) {
            bwd_names.insert(make_pair(key_ptr<std::string>(name), val));
        }
        for(const auto& loc: new_vi.originalLocus) {
            bwd_locs.insert(make_pair(key_ptr<Locus>(loc), val));
        }
        if(new_vi.ast) bwd_clang.insert(make_pair(new_vi.ast, val));
    }

    const VarInfo& get(llvm::Value* val) const { return fwd.at(val); }
    llvm::Value* byName(const std::string& str) const {
        return bwd_names.at(str);
    }

    loc_value_range byLoc(const Locus& loc) const {
        auto start = bwd_locs.upper_bound(loc);
        auto end = start;
        while (end != bwd_locs.end() && end->first == start->first) ++end;
        return std::make_pair(start, end);
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
