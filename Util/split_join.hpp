#ifndef SPLIT_JOIN_HPP
#define SPLIT_JOIN_HPP

#include <unordered_set>

#include "Util/generate_macros.h"

namespace borealis{
namespace util{
namespace impl_ {

    template<class T>
    struct sj_node {
        bool isLeaf;

    protected:
        sj_node(bool isLeaf): isLeaf(isLeaf) {}
        virtual ~sj_node() {}
    };

    template<class T>
    struct sj_leaf: sj_node<T> {
        T value;

        sj_leaf(const T& value):
            sj_node<T>(true), value(value) {}
    };

    template<class T>
    struct sj_branch: sj_node<T> {
        std::shared_ptr<sj_node<T>> left;
        std::shared_ptr<sj_node<T>> right;

        sj_branch(const std::shared_ptr<sj_node<T>>& left,
                  const std::shared_ptr<sj_node<T>>& right):
            sj_node<T>(false), left(left), right(right) {}
    };

} /* namespace impl_ */

template<class T>
class split_join {
    using node_t = std::shared_ptr<impl_::sj_node<T>>;

    node_t root;

    static node_t mkLeaf(const T& val) {
        using namespace impl_;
        return std::make_shared<sj_leaf<T>>(val);
    }

    static node_t mkBranch(const node_t& l, const node_t& r) {
        using namespace impl_;
        return std::make_shared<sj_branch<T>>(l, r);
    }

    split_join(const node_t& node): root(node) {}

public:
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(split_join);

    void insert(const T& val) {
        using namespace impl_;
        auto wrap = mkLeaf(val);
        if(!root) {
            root = wrap;
        } else {
            auto branch = std::make_shared<sj_branch<T>>(root, wrap);
            root = branch;
        }
    }

    static split_join join(const split_join& lhv, const split_join& rhv) {
        if(lhv.root == rhv.root || rhv.root == nullptr) return lhv;
        if(lhv.root == nullptr) return rhv;

        return split_join{ mkBranch(lhv.root, rhv.root) };
    }

    // TODO: remake this into an iterator-based interface?
    template<class F>
    void foreach(F f) const {
        using namespace impl_;
        std::unordered_set<node_t> nodes;
        std::queue<node_t> que;
        if(!root) return;
        que.push(root);
        
        while(!que.empty()) {
            auto node = que.front();
            que.pop();
            bool inserted = false;
            std::tie(std::ignore, inserted) = nodes.insert(node);
            if(inserted && !node->isLeaf) {
                auto branch = std::static_pointer_cast<sj_branch<T>>(node);
                if(branch->left) que.push(branch->left);
                if(branch->right) que.push(branch->right);
            }
        }
        
        for(auto&& node : nodes) if(node && node->isLeaf) {
            auto leaf = std::static_pointer_cast<sj_leaf<T>>(node);
            f(leaf->value);
        }
        
    }
};

} /* namespace util */
} /* namespace borealis */

#include "Util/generate_unmacros.h"

#endif // SPLIT_JOIN_HPP
