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
    T* data = nullptr;

    void allocate(size_t sz) {
        if(data) delete[] data;
        data = new T[sz];
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
        if(data) delete[] data;
        data = nullptr;
    }

    compacting_array() = default;
    compacting_array(compacting_array&& that):
            bitmask{}, data{} {
        swap(*this, that);
    }
    compacting_array(const compacting_array& that):
            bitmask(that.bitmask), data(new T[that.size()]) {
        std::copy(that.data, that.data + size(), data);
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
            std::copy(data, data + real, result.data);
            if(real < size()) std::copy(data + real, data + size(), result.data + real + 1);
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
            std::copy(data, data + real, result.data);
            auto offset = real + 1;
            if(offset < size()) std::copy(data + offset, data + size(), result.data + real);
            return std::move(result);
        } else return *this;
    }

    T* begin() { return data; }
    T* end() { return data + size(); }
    const T* begin() const { return data; }
    const T* end() const { return data + size(); }

    void dump() const {
        std::cerr << "[";
        for(auto&& e : *this) {
            std::cerr << e << " ";
        }
        std::cerr << "]";
    }

    ~compacting_array() { clear(); }
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
auto leaf_spread(const std::shared_ptr<hamt_leaf<T>>& ptr) {
    return util::view(
        essentials::adapt_simple_iterator(bucket_simple_iterator<T>(ptr), std::forward_iterator_tag{}),
        essentials::adapt_simple_iterator(bucket_simple_iterator<T>(), std::forward_iterator_tag{})
    );
}

template<class T>
auto branch_bfs(const std::shared_ptr<hamt_node<T>>& ptr) {
    return view(&ptr, &ptr + 1).bfs(&branch_spread<T>);
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

    const leaf* seek_bucket(const node& node, uint32_t hash) const {
        if(auto leaf = as_leaf(node)) return leaf;
        if(auto branch = as_branch(node)) {
            auto ix = hash & 0x1F;
            if(not branch->data.count(ix)) return nullptr;
            return seek_bucket(*branch->data.at(ix), hash >> 5);
        }
    }

    inline static size_t digit(size_t at, uint32_t hash) {
        return (hash >> (at * 5)) & 0x1f;
    }

    node_ptr insert(const node_ptr& nnode, uint8_t level, uint32_t hash, const T& value) const {
        // empty node, just make a leaf
        if(not nnode) return std::make_shared<leaf>(value);

        Equal eq;

        // leaf node - need to clarify
        if(as_leaf(*nnode)) {
            auto leaff = std::static_pointer_cast<leaf>(nnode);
            auto oldHash = this->hash(leaff->value); // recalc hash for existing leaf
            if(hash == oldHash) { // equal hashes = hash miss or equal items
                while(leaff) { // iterate the bucket
                    if(eq(leaff->value, value)) return nnode; // equal item found
                    leaff = leaff->next;
                }
                // hash miss, add this element to bucket front
                return std::make_shared<leaf>(value, std::static_pointer_cast<leaf>(nnode));
            } else { // local miss, need new branches (1 in good case, but potentially many)
                //++level;
                // we don't do a "not modified" check here, because it must be modified
                // XXX: we may recalc hashes numerous times down the line, which is not really a good thing
                return insert(insert(std::make_shared<branch>(), level, oldHash, leaff->value), level, hash, value);
            }
        }
        // branch node - the usual case
        if(as_branch(*nnode)) {
            auto branchh = std::static_pointer_cast<branch>(nnode);
            auto ix = digit(level, hash);
            if(not branchh->data.count(ix)){ // branch has no child in this position, good, put us
                return std::make_shared<branch>(branchh->data.imm_set(ix, std::make_shared<leaf>(value)));
            } else { // branch has children in this position, go deeper
                auto existing = branchh->data.at(ix);
                ++level;
                auto replaced = insert(existing, level, hash, value);
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
                ++level;
                auto newNext = erase(leaff->next, level, hash, value);
                if(leaff->next == newNext) return leaff; // value not found in tail, do nothing
                else if (newNext == nullptr) return std::make_shared<leaf>(leaff->value);
                else return std::make_shared<leaf>(leaff->value, std::static_pointer_cast<leaf>(newNext)); // value found in tail, remake the bucket
            } else return nnode; // this is not the right leaf
        }

        // branch node - the usual case
        if(as_branch(*nnode)) {
            auto branchh = std::static_pointer_cast<branch>(nnode);
            auto ix = digit(level, hash);
            if(not branchh->data.count(ix)) return nnode; // node not found
            else { // node may exist, go deeper
                auto existing = branchh->data.at(ix);
                ++level;
                auto erased = erase(existing, level, hash, value);
                if(not erased) {
                    auto newData = branchh->data.imm_remove(ix);
                    if(newData.size() == 0) return nullptr;
                    return std::make_shared<branch>(std::move(newData));
                }
                if(existing == erased) return nnode;
                else return std::make_shared<branch>(branchh->data.imm_set(ix, erased));
            }
        }

        UNREACHABLE("Incorrect hamt_node encountered");
    }

    node_ptr merge(const node_ptr& lnode, const node_ptr & rnode, uint8_t level) const {
        if(lnode == rnode) return lnode;
        if(lnode == nullptr) return rnode;
        if(rnode == nullptr) return lnode;

        if(!!as_branch(*lnode) && !!as_branch(*rnode)) {
            auto l = as_branch(*lnode);
            auto r = as_branch(*rnode);

            auto mask = l->data.mask() | r->data.mask();
            compacting_array<node_ptr, 32> newData{mask};

            for(int i = 0; i < 32; ++i) { // XXX: can we do better?
                if(l->data.count(i) && r->data.count(i)) {
                    newData.at(i) = merge(l->data.at(i), r->data.at(i), ++level);
                }
                if(l->data.count(i)) {
                    newData.at(i) = l->data.at(i);
                }
                if(r->data.count(i)) {
                    newData.at(i) = r->data.at(i);
                }
            }
            return std::make_shared<branch>(std::move(newData));
        }
        if(auto leaf = as_leaf(*lnode)) {
            return insert(rnode, level, this->hash(leaf->value), leaf->value);
        }
        if(auto leaf = as_leaf(*rnode)) {
            return insert(lnode, level, this->hash(leaf->value), leaf->value);
        }

        UNREACHABLE("Incorrect hamt_node encountered");
    }

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
        if(auto leaf = seek_bucket(*root_, hash(value))) {
            while(leaf) {
                if(leaf->value == value) return leaf;
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

    auto view() const {
        return impl_::branch_bfs(root_)
              .filter()
              .filter(LAM(t, !!impl_::as_leaf(*t)))
              .map(LAM(t, std::static_pointer_cast<leaf>(t)))
              .map(&impl_::leaf_spread<T>)
              .flatten();
    }

    auto begin() const {
        return view().begin();
    }

    auto end() const {
        return view().end();
    }

    template<class F>
    void foreach(F f) const {
        foreach(root_, f);
    }

    void dump() const {
        dump(root_, 0);
        std::cerr << std::endl;
    }

    friend hamt_set unite(const hamt_set& l, const hamt_set& r) {
        return { l.merge(l.root_, r.root_, 0) };
    }

};

} /* namespace util */
} /* namespace borealis */

#include "Util/unmacros.h"

#endif //HAMT_HPP
