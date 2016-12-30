//
// Created by belyaev on 12/20/16.
//

#ifndef HAMT_HPP
#define HAMT_HPP

#include <cstddef>
#include <bitset>
#include <memory>

#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

template<class T, size_t MaxSize = 32>
class compacting_array {
    std::bitset<MaxSize> bitmask;
    std::unique_ptr<T[]> data = nullptr;

    void allocate(size_t sz) {
        data.reset(new T[sz]);
    }

    inline size_t realIx(size_t ix) const {
        auto bit = 1ULL << ix;
        auto realIx = (bitmask & std::bitset<MaxSize>(bit - 1)).count();
        return realIx;
    }

public:
    std::bitset<MaxSize> mask() const {
        return bitmask;
    }

    size_t size() const {
        return bitmask.count();
    }

    friend void swap(compacting_array& lhv, compacting_array& rhv) {
        std::swap(lhv.bitmask, rhv.bitmask);
        std::swap(lhv.data, rhv.data);
    }

    void clear() {
        bitmask.reset();
        data.reset();
    }

    compacting_array() = default;
    compacting_array(compacting_array&& that):
            bitmask{}, data{} {
        swap(*this, that);
    }
    compacting_array(const compacting_array& that):
            bitmask(that.bitmask), data(new T[that.size()]) {
        std::copy(
            that.begin(),
            that.end(),
            begin()
        );
    }
    explicit compacting_array(const std::bitset<MaxSize>& mask):
            bitmask(mask), data(new T[mask.count()]){}

    compacting_array& operator= (compacting_array&& that) {
        clear();
        swap(*this, that);
        return *this;
    }
    compacting_array& operator= (const compacting_array& that) {
        auto copy = that;
        swap(*this, copy);
        return *this;
    }

    T& at(size_t ix) {
        return data[realIx(ix)];
    }

    const T& at(size_t ix) const {
        return data[realIx(ix)];
    }

    bool count(size_t ix) const {
        return bitmask.test(ix);
    }

    compacting_array imm_set(size_t ix, const T& value) const {
        auto newBm = bitmask;
        if(newBm.test(ix)) {
            auto copy = *this;
            copy.at(ix) = value;
            return std::move(copy);
        } else {
            compacting_array result;
            result.bitmask = bitmask;
            result.bitmask.set(ix);
            result.allocate(size() + 1);
            auto real = realIx(ix);
            std::copy(begin(), begin() + real, result.begin());
            if(real < size()) std::copy(begin() + real, end(), result.begin() + real + 1);
            result.data[real] = value;
            return std::move(result);
        }
    }

    compacting_array imm_remove(size_t ix) const {
        auto newBm = bitmask;
        if(newBm.test(ix)) {
            compacting_array result;
            result.bitmask = bitmask;
            result.bitmask.reset(ix);
            result.allocate(size() - 1);
            auto real = realIx(ix);
            std::copy(begin(), begin() + real, result.begin());
            auto offset = real + 1;
            if(offset < size()) std::copy(begin() + offset, end(), result.begin() + real);
            return std::move(result);
        } else return *this;
    }

    T* begin() { return data.get(); }
    T* end() { return data.get() + size(); }
    const T* begin() const { return data.get(); }
    const T* end() const { return data.get() + size(); }

    void dump() const {
        std::cerr << "[";
        for(auto&& e : *this) {
            std::cerr << e << " ";
        }
        std::cerr << "]";
    }

    ~compacting_array() {}
};

template<class T>
struct hamt_node {
    using Ptr = std::shared_ptr<hamt_node>;

    enum class Kind { Leaf, Branch };

    Kind kind;

    explicit hamt_node(Kind kind): kind(kind) {}
    hamt_node() = delete;
    hamt_node(const hamt_node&) = delete;
};
template<class T>
struct hamt_leaf: hamt_node<T> {
    using base = hamt_node<T>;
    using base_ptr = typename base::Ptr;

    hamt_leaf(): base(base::Kind::Leaf), value{} {}
    template<class U>
    hamt_leaf(U&& arg, const std::shared_ptr<hamt_leaf>& next = nullptr): base(base::Kind::Leaf), value(std::forward<U>(arg)), next(next) {}

    T value;
    std::shared_ptr<hamt_leaf> next = nullptr;
};
template<class T>
struct hamt_branch: hamt_node<T> {
    using base = hamt_node<T>;
    using base_ptr = typename base::Ptr;

    hamt_branch(): base(base::Kind::Branch), data{} {}
    hamt_branch(const compacting_array<base_ptr>& data): base(base::Kind::Branch), data(data) {}
    hamt_branch(compacting_array<base_ptr>&& data): base(base::Kind::Branch), data(std::move(data)) {}

    compacting_array<base_ptr> data;
};

namespace impl_ {

template<class T>
const hamt_leaf<T>* as_leaf(const hamt_node<T>& node) {
    if(node.kind == hamt_node<T>::Kind::Leaf) {
        return static_cast< const hamt_leaf<T>* >(&node);
    } else return nullptr;
}

template<class T>
const hamt_branch<T>* as_branch(const hamt_node<T>& node) {
    if(node.kind == hamt_node<T>::Kind::Branch) {
        return static_cast< const hamt_branch<T>* >(&node);
    } else return nullptr;
}

template<class T>
using hamt_node_ptr = typename hamt_node<T>::Ptr;

template<class T>
auto branch_spread(const hamt_node_ptr<T>& ptr) -> util::iterator_view< const hamt_node_ptr<T>* > {
    if(ptr == nullptr) return { nullptr, nullptr };
    if(auto branch = as_branch(*ptr)) {
        return { branch->data.begin(), branch->data.end() };
    }
    return { nullptr, nullptr };
}

template<class T>
struct bucket_simple_iterator {
    using leaf_ptr = std::shared_ptr<hamt_leaf<T>>;
    leaf_ptr ll;
    
    bucket_simple_iterator(const leaf_ptr& leaf = nullptr): ll(leaf) {}
    
    void next() {
        ll = ll->next;
    }
    
    auto value() const { return ll->value; }
    
    bool equals(const bucket_simple_iterator& that) {
        return ll == that.ll;
    }
};

template<class T>
auto leaf_range(const std::shared_ptr<hamt_leaf<T>>& ptr, const std::shared_ptr<hamt_leaf<T>>& end) {
    return util::view(
        essentials::adapt_simple_iterator(bucket_simple_iterator<T>(ptr), std::forward_iterator_tag{}),
        essentials::adapt_simple_iterator(bucket_simple_iterator<T>(end), std::forward_iterator_tag{})
    );
}

template<class T>
auto leaf_spread(const std::shared_ptr<hamt_leaf<T>>& ptr) {
    return leaf_range<T>(ptr, nullptr);
}

template<class T>
auto branch_bfs(const std::shared_ptr<hamt_node<T>>& ptr) {
    return view(&ptr, &ptr + 1).bfs(&branch_spread<T>);
}

template<class T>
auto node_view(const std::shared_ptr<hamt_node<T>>& root) {
    return impl_::branch_bfs(root)
            .filter(LAM(t, t && !!as_leaf(*t)))
            .map(LAM(t, std::static_pointer_cast<hamt_leaf<T>>(t)))
            .map(&impl_::leaf_spread<T>)
            .flatten();
}

} /* namespace impl_ */

template <class T, class Hash = std::hash<T>, class Equal = std::equal_to<T>>
class hamt_set {
    static uint32_t hash(const T& value) {
        auto stdHash = Hash{}(value);
        if(sizeof(size_t) == sizeof(uint32_t)) {
            stdHash += ~(stdHash << 15);
            stdHash ^= (stdHash >> 10);
            stdHash += (stdHash << 3);
            stdHash ^= (stdHash >> 6);
            stdHash += ~(stdHash << 11);
            stdHash ^= (stdHash >> 16);
            return static_cast<uint32_t>(stdHash);
        }

        if(sizeof(size_t) == sizeof(uint32_t) * 2) {
            stdHash += ~(stdHash << 32);
            stdHash ^= (stdHash >> 22);
            stdHash += ~(stdHash << 13);
            stdHash ^= (stdHash >> 8);
            stdHash += (stdHash << 3);
            stdHash ^= (stdHash >> 15);
            stdHash += ~(stdHash << 27);
            stdHash ^= (stdHash >> 31);
            return static_cast<uint32_t>(stdHash);
        }

        return static_cast<uint32_t>(stdHash);
    }

    using leaf = hamt_leaf<T>;
    using node = hamt_node<T>;
    using node_ptr = typename node::Ptr;
    using branch = hamt_branch<T>;

    node_ptr root_ = nullptr;

    const leaf* as_leaf(const node& node) const {
        if(node.kind == node::Kind::Leaf) {
            return static_cast<const leaf*>(&node);
        } else return nullptr;
    }

    const branch* as_branch(const node& node) const {
        if(node.kind == node::Kind::Branch) {
            return static_cast<const branch*>(&node);
        } else return nullptr;
    }

    // better than (value + 1) in that it does not autocast to int
    template<class U>
    static constexpr U inc(U value) { return ++value; }

    // 32-nary digit number `at` in `hash`, 0 <= at <= 6 
    inline static size_t digit(size_t at, uint32_t hash) {
        return (hash >> (at * 5)) & 0x1f;
    }

    // find top leaf in `node` on `level` containing `hash`
    const leaf* seek_bucket(const node& node, uint8_t level, uint32_t hash) const {
        if(auto leaf = as_leaf(node)) return leaf;
        if(auto branch = as_branch(node)) {
            auto ix = digit(level, hash);
            if(not branch->data.count(ix)) return nullptr;
            return seek_bucket(*branch->data.at(ix), inc(level), hash);
        }
        UNREACHABLE("Incorrect hamt_node encountered");
    }

    std::shared_ptr<leaf> merge_buckets(const std::shared_ptr<leaf>& lb, const std::shared_ptr<leaf>& rb) const {
        Equal eq;
        auto lit = lb;
        auto rit = rb;
        std::shared_ptr<leaf> common = nullptr;
        // find common tail, if any
        while(lit) {
            while(rit) {
                rit = rit->next;
                if(lit == rit) {
                    common = lit;
                    break;
                }
            }
            if(lit == rit) {
                break;
            }
            lit = lit->next;
        }

        rit = rb;
        lit = lb;

        if(lit == common) return rit;
        if(rit == common) return lit;

        while(rit != common) {
            if(impl_::leaf_range(lb, common).all_of([&](auto&& v){ return not eq(v, rit->value); })) {
                lit = std::make_shared<leaf>(rit->value, lit);
            }
            rit = rit->next;
        }
        return lit;
    }

    // insert `value` into `nnode` on `level`. `hash(value)` == hash.
    node_ptr insert_leaf(const node_ptr& nnode, uint8_t level, uint32_t hash, const std::shared_ptr<leaf>& ins) const {
        // empty node, just return a leaf
        if(not nnode) return ins;
        if(nnode == ins) return ins; // happens

        // leaf node - need to clarify
        if(as_leaf(*nnode)) {
            auto leaff = std::static_pointer_cast<leaf>(nnode);
            auto oldHash = this->hash(leaff->value); // recalc hash for existing leaf
            if(hash == oldHash) { // equal hashes = hash miss or equal items
                return merge_buckets(leaff, ins);
            } else {
                // local miss, need new branches (1 in good case, but potentially many)
                auto divert_level = level;

                while(digit(divert_level, hash) == digit(divert_level, oldHash)) ++divert_level;
                auto digit0 = digit(divert_level, hash);
                auto digit1 = digit(divert_level, oldHash);

                compacting_array<node_ptr, 32> newData{ std::bitset<32>().set(digit0).set(digit1) };
                newData.at(digit0) = ins;
                newData.at(digit1) = leaff;

                auto branchh = std::make_shared<branch>(newData);
                while(divert_level != level) {
                    divert_level--;
                    branchh = std::make_shared<branch>(
                        compacting_array<node_ptr, 32>().imm_set(digit(divert_level, hash), branchh)
                    );
                }

                return branchh;
            }
        }
        // branch node - the usual case
        if(as_branch(*nnode)) {
            auto branchh = std::static_pointer_cast<branch>(nnode);
            auto ix = digit(level, hash);
            if(not branchh->data.count(ix)){ // branch has no child in this position, good, put us there
                return std::make_shared<branch>(
                    branchh->data.imm_set(ix, ins)
                );
            } else { // branch has children in this position, go deeper
                auto existing = branchh->data.at(ix);
                auto replaced = insert_leaf(existing, inc(level), hash, ins);
                if(existing == replaced) return nnode; // element already inside
                else return std::make_shared<branch>(branchh->data.imm_set(ix, replaced));
            }
        }

        UNREACHABLE("Incorrect hamt_node encountered");
    }

    // insert `value` into `nnode` on `level`. `hash(value)` == hash.
    node_ptr insert(const node_ptr& nnode, uint8_t level, uint32_t hash, const T& value) const {
        // empty node, just make a leaf
        if(not nnode) return std::make_shared<leaf>(value);

        Equal eq;

        // leaf node - need to clarify
        if(as_leaf(*nnode)) {
            auto leaff = std::static_pointer_cast<leaf>(nnode);
            auto oldHash = this->hash(leaff->value); // recalc hash for existing leaf
            if(hash == oldHash) { // equal hashes = hash miss or equal items
                for(auto&& e : impl_::leaf_spread(leaff)) { // iterate the bucket
                    if(eq(e, value)) return nnode;// equal item found
                }
                // hash miss, add this element to bucket front
                return std::make_shared<leaf>(value, std::static_pointer_cast<leaf>(nnode));
            } else { // local miss, need new branches (1 in good case, but potentially many)
                // we don't do a "not modified" check here, because it is always modified
                // XXX: we may recalc hashes numerous times down the line, which is not really a good thing
                return insert(
                    insert_leaf(std::make_shared<branch>(), level, oldHash, leaff),
                    level, hash, value
                );
            }
        }
        // branch node - the usual case
        if(as_branch(*nnode)) {
            auto branchh = std::static_pointer_cast<branch>(nnode);
            auto ix = digit(level, hash);
            if(not branchh->data.count(ix)){ // branch has no child in this position, good, put us there
                return std::make_shared<branch>(
                    branchh->data.imm_set(ix, std::make_shared<leaf>(value))
                );
            } else { // branch has children in this position, go deeper
                auto existing = branchh->data.at(ix);
                auto replaced = insert(existing, inc(level), hash, value);
                if(existing == replaced) return nnode; // element already inside
                else return std::make_shared<branch>(branchh->data.imm_set(ix, replaced));
            }
        }

        UNREACHABLE("Incorrect hamt_node encountered");
    }

    node_ptr erase(const node_ptr& nnode, uint8_t level, uint32_t hash, const T& value) const {
        // empty node, nothing to delete
        if(not nnode) return nullptr;

        Equal eq;

        // leaf node - need to clarify
        if(as_leaf(*nnode)) {
            auto leaff = std::static_pointer_cast<leaf>(nnode);
            auto oldHash = this->hash(leaff->value); // recalc hash for existing leaf
            if(hash == oldHash) { // either an existing value or a hash miss
                // iterate the leaf to find the value
                if(eq(leaff->value, value)) return leaff->next; // value found, remove it
                //XXX: using recursion instead of iteration here is not the best idea, as buckets may be 
                //     very long, leading to potential SO, but it is much easier
                //     Anyway, SO is only possible with a VERY VERY BAD hash function
                auto newNext = erase(leaff->next, level, hash, value);
                if(leaff->next == newNext) return nnode; // value not found in tail, do nothing
                else if (newNext == nullptr) {// value removed the whole tail, make a new leaf
                    return std::make_shared<leaf>(leaff->value); 
                } else {
                    // value found in tail, remake the bucket
                    return std::make_shared<leaf>(leaff->value, std::static_pointer_cast<leaf>(newNext)); 
                }
            } else return nnode; // these are not the droids we are looking for
        }

        // branch node - the usual case
        if(as_branch(*nnode)) {
            auto branchh = std::static_pointer_cast<branch>(nnode);
            auto ix = digit(level, hash);
            if(not branchh->data.count(ix)) return nnode; // node not found, shortcut
            else { // node may exist, go deeper
                auto existing = branchh->data.at(ix);
                auto erased = erase(existing, inc(level), hash, value);
                if(not erased) { // we removed the whole branch
                    auto newData = branchh->data.imm_remove(ix); // remove it upward
                    if(newData.size() == 0) return nullptr; // we again removed the whole branch
                    if(newData.size() == 1) { // only one successor left
                        auto succ = *newData.begin();
                        if(as_leaf(*succ)) return succ; // branch with single leaf is redundant
                        // branch with single branch is not (potentially many branches deeper down)
                    }
                    // XXX: it may seem we are very inefficient here in these cases, but
                    //      sparse arrays of both size 0 and 1 are very efficient
                    //      (aside from additional heap allocations)
                    return std::make_shared<branch>(std::move(newData));
                }
                if(existing == erased) return nnode; // nothing changed
                else { // standard case: something changed, not sure what
                    return std::make_shared<branch>(branchh->data.imm_set(ix, erased));
                }
            }
        }

        UNREACHABLE("Incorrect hamt_node encountered");
    }

    // union of `lnode` and `rnode` treated as sets, both being on `level` in their respective HAMTs
    node_ptr merge(const node_ptr& lnode, const node_ptr & rnode, uint8_t level) const {
        if(lnode == rnode) return lnode;
        if(lnode == nullptr) return rnode;
        if(rnode == nullptr) return lnode;

        if(!!as_branch(*lnode) && !!as_branch(*rnode)) {
            auto l = as_branch(*lnode);
            auto r = as_branch(*rnode);

            // make a sparse array with all positions filled from both l and r
            // fill it with `nullptr`s
            auto mask = l->data.mask() | r->data.mask();
            compacting_array<node_ptr, 32> newData{mask};

            // XXX: we can do better using clz, but std::bitset does not support it :-(
            for(int i = 0; i < 32; ++i) if(newData.count(i)) {
                auto left = l->data.count(i) ? l->data.at(i) : nullptr;
                auto right = r->data.count(i) ? r->data.at(i) : nullptr;
                newData.at(i) = merge(left, right, inc(level));
            }
            return std::make_shared<branch>(std::move(newData));
        }
        // nodes are different and one of them is leaf, just insert the leaf into the other node
        if(as_leaf(*lnode)) {
            auto leaff = std::static_pointer_cast<leaf>(lnode);
            return insert_leaf(rnode, level, this->hash(leaff->value), leaff);
        }
        // the mirror case
        if(as_leaf(*rnode)) {
            auto leaff = std::static_pointer_cast<leaf>(rnode);
            return insert_leaf(lnode, level, this->hash(leaff->value), leaff);
        }

        UNREACHABLE("Incorrect hamt_node encountered");
    }
    
    bool equals(const node_ptr& l, const node_ptr& r, uint8_t level) {
        if(l == r) return true;
        if(l == nullptr || r == nullptr) return false;
        
        if(as_leaf(*l) && as_leaf(*r)) {
            auto ll = std::static_pointer_cast<leaf>(l);
            auto rl = std::static_pointer_cast<leaf>(r);
            // now, there is no _good_ way of doing this
            auto lview = impl_::leaf_spread(ll);
            auto rview = impl_::leaf_spread(rl);
            Equal eq;
            // std::is_permutation is a good shot, as it implements basic optimisations, like
            // skipping common prefix
            return std::is_permutation(lview.begin(), lview.end(), rview.begin(), rview.end(), eq);
        }
        
        if(as_branch(*l) && as_branch(*r)) {
            auto lb = std::static_pointer_cast<branch>(l);
            auto rb = std::static_pointer_cast<branch>(r);
            
            if(lb.data.mask() != rb.data.mask()) return false;
            auto lit = lb.data.begin();
            auto rit = rb.data.begin();
            auto lend = lb.data.end();
            auto rend = rb.data.end();
            for(; lit != lend && rit != rend; ++lit, ++rit) {
                if(not equals(*lit, *rit, inc(level))) return false;
            }
            return true;
        }
        
        return false;
    }

    // debugger-only function
    void dump(const node_ptr& what, size_t indent) const {
        std::cerr << "\n>";
        for(auto i = indent; i > 0; --i) std::cerr << " ";
        if(!what) {
            std::cerr << "[]";
            return;
        }
        if(auto leaf = as_leaf(*what)) {
            std::cerr << tfm::format("leaf(%s[%08x])->", leaf->value, this->hash(leaf->value));
            dump(leaf->next, indent + 8);
        }
        if(auto branch = as_branch(*what)) {
            std::cerr << "branch{";
            for(auto i = 0; i < 32; ++i) {
                if(branch->data.count(i)) {
                    std::cerr << "\n>";
                    for(auto i = indent + 4; i > 0; --i) std::cerr << " ";
                    std::cerr << tfm::format("[%02x]:", i);
                    dump(branch->data.at(i), indent + 6);
                }
            }
            std::cerr << "\n>";
            for(auto i = indent; i > 0; --i) std::cerr << " ";
            std::cerr << "}";
        }
    }

    // should be much faster than `iterations`-based iterators
    // NOTE: this is doing a DFS rather than BFS with the iterators
    //       so the order will be different
    template<class F>
    void foreach(const node_ptr& node, F f) const {
        if(!node) return;
        if(as_leaf(*node)) {
            auto leaff = std::static_pointer_cast<leaf>(node);
            while(leaff != nullptr) {
                f(leaff->value);
                leaff = leaff->next;
            }
            return;
        }
        if(auto branch = as_branch(*node)) {
            for(auto&& e : branch->data) {
                foreach(e, f);
            }
        }
    }

    hamt_set(const node_ptr& root): root_(root) {}
public:
    hamt_set() = default;
    hamt_set(const hamt_set&) = default;
    hamt_set& operator=(const hamt_set&) = default;

    const leaf* find_node(const T& value) const {
        if(root_ == nullptr) return nullptr;

        Equal eq;
        if(auto leaf = seek_bucket(*root_, 0, hash(value))) {
            while(leaf) {
                if(eq(leaf->value, value)) return leaf;
                leaf = leaf->next.get();
            }
            return nullptr;
        }
        return nullptr;
    }
    bool count(const T& value) const {
        return !!find_node(value);
    }

    hamt_set insert(const T& value) const {
        auto hvalue = hash(value);
        auto newRoot = insert(root_, 0, hvalue, value);
        return { newRoot };
    }

    hamt_set erase(const T& value) const {
        auto hvalue = hash(value);
        auto newRoot = erase(root_, 0, hvalue, value);
        return { newRoot };
    }

    // it's not easy to provide zero-cost std::iterator interface for this kind of data structure.
    // `iterations`-based approach is reasonable enough: it has the same O(*) time cost as a
    // normal bfs would have, follows everything in std::iterator and works.
    // If you need speed, use `foreach()` instead.
    auto view() const {
        return impl_::node_view(root_);
    }

    auto begin() const {
        return view().begin();
    }

    auto end() const {
        return view().end();
    }
    
    // this type is HUGE if you spell it out (zero-cost abstraction, remember?)
    using iterator = decltype(impl_::node_view(root_).begin());
    using const_iterator = iterator;

    template<class F>
    void foreach(F f) const {
        foreach(root_, f);
    }

    void dump() const {
        dump(root_, 0);
        std::cerr << std::endl;
    }

    friend bool operator==(const hamt_set& l, const hamt_set& r) {
        return l.equals(l.root_, r.root_, 0);
    }

    // this union is drastically more efficient than l.foreach(e -> r = r.insert(e))
    // in cases where l and r may have the same origin
    // (and even a little bit better if they don't)
    friend hamt_set unite(const hamt_set& l, const hamt_set& r) {
        return { l.merge(l.root_, r.root_, 0) };
    }

};

} /* namespace util */
} /* namespace borealis */

#include "Util/unmacros.h"

#endif //HAMT_HPP
