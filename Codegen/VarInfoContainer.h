/*
 * VarInfoContainer.h
 *
 *  Created on: Jan 14, 2013
 *      Author: belyaev
 */

#ifndef VARINFOCONTAINER_H_
#define VARINFOCONTAINER_H_

#include <llvm/IR/Value.h>

#include <map>
#include <string>
#include <unordered_map>

#include "Codegen/VarInfo.h"
#include "Util/key_ptr.hpp"
#include "Util/util.h"

namespace borealis {

// time optimized multidirectional container from Value* to VarInfo
class VarInfoContainer {
    typedef std::unordered_multimap<llvm::Value*, VarInfo> v2vi_t;
    typedef std::unordered_multimap<util::key_ptr<std::string>, llvm::Value*> str2v_t;
    typedef std::multimap<util::key_ptr<Locus>, llvm::Value*> loc2v_t;

    // fwd keeps the actual data
    v2vi_t fwd;

    // all other containers operate on pointers into fwd through key_ptrs
    // (or just plain pointers into llvm or clang inner memory)
    str2v_t bwd_names;
    loc2v_t bwd_locs;

public:
    typedef loc2v_t::const_iterator loc_value_iterator;
    typedef std::reverse_iterator<loc_value_iterator> reverse_loc_value_iterator;
    typedef std::pair<loc_value_iterator, loc_value_iterator> loc_value_range;

    typedef str2v_t::const_iterator str_value_iterator;
    typedef std::pair<str_value_iterator, str_value_iterator> str_value_range;

    typedef v2vi_t::const_iterator const_iterator;
    typedef std::pair<const_iterator, const_iterator> value_range;

    VarInfoContainer();
    ~VarInfoContainer();

    void put(llvm::Value* val, const VarInfo& vi) {
        using borealis::util::key_ptr;

        auto new_it = fwd.insert({val, vi});

        const auto& new_vi = new_it->second;

        bwd_names.insert({ new_vi.name, val });
        bwd_locs.insert({ new_vi.locus,  val });
    }

    value_range get(llvm::Value* val) const {
        return fwd.equal_range(val);
    }

    str_value_range byName(const std::string& str) const {
        return bwd_names.equal_range(str);
    }

    str_value_iterator byNameEnd() const {
        return bwd_names.end();
    }

    loc_value_iterator byLocBegin() const {
        return bwd_locs.begin();
    }

    loc_value_iterator byLocFwd(const Locus& loc) const {
        return bwd_locs.lower_bound(loc); // FIXME: fwd or bwd???
    }

    loc_value_iterator byLocBwd(const Locus& loc) const {
        return bwd_locs.lower_bound(loc); // FIXME: fwd or bwd???
    }

    loc_value_iterator byLocEnd() const {
        return bwd_locs.end();
    }

    reverse_loc_value_iterator byLocReverse(const Locus& loc) const {
        return reverse_loc_value_iterator(byLocBwd(loc));
    }

    reverse_loc_value_iterator byLocReverseEnd() const {
        return bwd_locs.rend();
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
