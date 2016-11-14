//
// Created by belyaev on 11/2/16.
//

#ifndef SHARED_TREAP_HPP
#define SHARED_TREAP_HPP

#include "Util/generate_macros.h"

namespace borealis {
namespace util {

namespace shared_treap_impl {

template<class T>
struct node: std::enable_shared_from_this<node<T>> {

    static size_t rand() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<size_t> dis;
        return dis(gen);
    }

    using node_ptr = std::shared_ptr<node>;

    node_ptr left = nullptr;
    node_ptr right = nullptr;
    size_t key;
    T value;

    node(const T& value, size_t key = rand(), node_ptr left = nullptr, node_ptr right = nullptr):
        value(value), key(key), left(left), right(right) {}

    static node_ptr merge(node_ptr left, node_ptr right) {
        if(not left) return right;
        if(not right) return left;

        if(left.key > right.key) {
            auto newRight = merge(left->right, right);
            return std::make_shared<node>(left->value, left->key, left->left, newRight);
        } else {
            auto newLeft = merge(left, right->left);
            return std::make_shared<node>(right->value, right->key, newLeft, right->right);
        }
    }

    std::tuple<node_ptr, node_ptr, bool> split(const T& onValue) const {
        node_ptr L = nullptr, R = nullptr, newTree = nullptr;
        bool contains = false;
        if(value <= onValue) {
            if(not right) R = nullptr;
            else std::tie(newTree, R, contains) = right->split(onValue);
            L = std::make_shared<node>(value, key, left, newTree);
        } else {
            if(not left) L = nullptr;
            else std::tie(L, newTree, contains) = left->split(onValue);
            R = std::make_shared<node>(value, key, newTree, right);
        }
        return {L, R, (value == onValue) || contains};
    };

    node_ptr insert(const T& value) const {
        node_ptr l = nullptr, r = nullptr;
        bool found = false;
        std::tie(l, r, found) = split(value);
        if(found) return shared_from_this();

        auto m = std::make_shared<node>(value);
        return merge(merge(left, m), right);
    }

    node_ptr remove(const T& value) const noexcept {
        node_ptr l = nullptr, r = nullptr;
        bool found = false;
        std::tie(l, r, found) = split(value);

        if(not found) return shared_from_this();

        return merge(l, r);
    }

    bool contains(const T& needle) const noexcept {
        if(value == needle) return true;
        if(value < needle) return left && left->contains(needle);
        else return right && right->contains(needle);
    }

    static node_ptr unite(node_ptr left, node_ptr right) {
        if(left == right || not left) return right;
        if(not right) return left;

        if(left->key < right->key) std::swap(left, right);

        node_ptr lt, rt;
        bool contains = false;
        std::tie(lt, rt, contains) = right.split(left->value);

        return std::make_shared<node>(left->value, left->key, unite(left->left, lt), unite(left->right, rt));
    }

};

} /* namespace shared_treap_impl */

class shared_treap {

};

} /* namespace util */
} /* namespace borealis */

#include "Util/generate_unmacros.h"

#endif //SHARED_TREAP_HPP
