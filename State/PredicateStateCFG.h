#ifndef PREDICATE_STATE_CFG_H
#define PREDICATE_STATE_CFG_H

#include <memory>
#include "State/PredicateState.h"
#include "State/BasicPredicateState.h"

namespace borealis {

class PredicateStateCFG {

    using BasicPtr = std::pointer_traits<PredicateState::Ptr>::template rebind<BasicPredicateState>;

    BasicPtr entryNode_;
    std::vector<BasicPtr> nodes_;
    std::unordered_map<BasicPtr, size_t> rnodes_;
    std::vector<std::vector<bool>> adj_;

public:

    void add_node(const BasicPtr& node) {
        if(rnodes_.count(node)) return;

        nodes_.push_back(node);
        rnodes_[node] = nodes_.size() - 1;
    }

    void add_edge(const BasicPtr& from, const BasicPtr& to) {
        add_node(from);
        add_node(to);

        auto fromIx = rnodes_[from];
        auto toIx = rnodes_[to];

        if(adj_.size() <= fromIx) adj_.resize(fromIx + 1);
        if(adj_[fromIx].size() <= toIx) adj_[fromIx].resize(toIx + 1);

        adj_[fromIx][toIx] = true;
    }



};

} /* namespace borealis */

#endif // PREDICATE_STATE_CFG_H
