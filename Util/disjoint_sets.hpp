#ifndef DISJOINT_SETS_HPP
#define DISJOINT_SETS_HPP

#include <list>
#include <cstddef>
#include <functional>
#include <unordered_map>

namespace borealis {
namespace util {

template<class T> class disjoint_set;

template<class T>
class subset {
    T data_;
    subset* parent_;
    size_t rank;

    friend class disjoint_set<T>;

    subset* parent() { return parent_; }
    void set_parent(subset* p) { parent_ = p; }

public:
    template<class ...Args>
    subset(Args&&... args): data_(std::forward<Args>(args)...), parent_(this), rank(0) {}
    subset(const subset&) = delete;
    subset(subset&&) = delete;

    T& data() { return data_; }
    const T& data() const { return data_; }

    bool isRoot() const { return parent_ == this; }

    subset* getRoot() {
        if(!isRoot()) {
            auto ancestor = parent()->getRoot();
            set_parent(ancestor);
            return ancestor;
        }
        return this;
    }

    const subset* getRootConst() const {
        if(!isRoot()) return parent_->getRootConst();
        else return this;
    }
};

template<class T>
class disjoint_set {
    // this is just an owner-storage
    // list is used to guarantee pointer liveness
    std::list<subset<T>> children_;
public:
    // copying DSs is a pain =(
    disjoint_set() = default;
    disjoint_set(const disjoint_set&) = delete;
    disjoint_set(disjoint_set&&) = delete;

    using iterator = typename std::list<subset<T>>::iterator;

    subset<T>* find(subset<T>* needle) {
        if(needle == nullptr) return nullptr;
        return needle->getRoot();
    }

    const subset<T>* findConst(const subset<T>* needle) const {
        if(needle == nullptr) return nullptr;
        return needle->getRootConst();
    }

    subset<T>* unite(subset<T>* left, subset<T>* right) {
        auto lRoot = find(left);
        auto rRoot = find(right);

        if(lRoot == rRoot) return lRoot;

        if(lRoot->rank < rRoot->rank) {
            lRoot->set_parent(rRoot);
            return rRoot;
        } else if(lRoot->rank > rRoot->rank) {
            rRoot->set_parent(lRoot);
            return lRoot;
        } else {
            rRoot->set_parent(lRoot);
            ++lRoot->rank;
            return lRoot;
        }
    }

    template<class ...Args>
    subset<T>* emplace(Args&&... args) {
        children_.emplace_back(std::forward<Args>(args)...);
        return &children_.back();
    }

    iterator begin() {
        return children_.begin();
    }

    iterator end() {
        return children_.end();
    }

};

} /* namespace util */
} /* namespace borealis */

#endif /* DISJOINT_SETS_HPP */
