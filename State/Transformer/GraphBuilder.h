#ifndef GRAPH_BUILDER_H
#define GRAPH_BUILDER_H

#include <unordered_set>
#include <llvm/Support/DOTGraphTraits.h>

#include "State/Transformer/Transformer.hpp"

namespace borealis {

namespace psgraph_impl {

template<class Container>
auto access_view(const Container& c) {
    using namespace borealis::util;
    return viewContainer(c).map([&](auto&& ptr){ return ptr.get(); });
}

} /* namespace psgraph_impl */

struct PSGraphNode {
    using Ptr = std::shared_ptr<PSGraphNode>;
    using ChildSet = std::unordered_set<Ptr>;

    PredicateState::Ptr data;
    std::unordered_set<Ptr> children;

    PSGraphNode(PredicateState::Ptr data): data(data), children{} {}

    static Ptr make(PredicateState::Ptr data) {
        return std::make_shared<PSGraphNode>( data );
    }

    inline auto childView() {
        return psgraph_impl::access_view(children);
    }
    inline auto child_begin() {
        return childView().begin();
    }
    inline auto child_end() {
        return childView().end();
    }
};

struct PSGraph {
    PSGraphNode::ChildSet allNodes;
    PSGraphNode::Ptr entry;
};

class GraphBuilder: public Transformer<GraphBuilder> {

    using base = Transformer<GraphBuilder>;

    PSGraphNode::Ptr header;
    PSGraphNode::Ptr footer;
    PSGraph result;

public:
    GraphBuilder() : Transformer(FactoryNest{}) { }
    GraphBuilder(const GraphBuilder&) = default;

    template<class AnyPtr>
    static GraphBuilder apply(AnyPtr ptr) {
        GraphBuilder g;
        g.transform(ptr);
        return g;
    }

    PSGraph& getGraph() { return result; }

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic) {
        auto node = PSGraphNode::make(basic);
        header = node;
        footer = node;
        result.allNodes.insert(node);
        result.entry = node;
        return basic;
    }
    PredicateState::Ptr transformChain(PredicateStateChainPtr chain) {
        auto base = apply(chain->getBase());
        auto tail = apply(chain->getCurr());

        header = base.header;
        footer = tail.footer;
        base.footer->children.insert(tail.header);
        result.allNodes.insert(base.result.allNodes.begin(), base.result.allNodes.end());
        result.allNodes.insert(tail.result.allNodes.begin(), tail.result.allNodes.end());
        result.entry = base.result.entry;
        return chain;
    }
    PredicateState::Ptr transformChoice(PredicateStateChoicePtr choice) {
        std::vector<GraphBuilder> choices = util::viewContainer(choice->getChoices())
                       .map([](auto&& ps){ return apply(ps);  })
                       .toVector();

        header = PSGraphNode::make(nullptr);
        footer = PSGraphNode::make(nullptr);
        result.allNodes.insert(header);
        result.allNodes.insert(footer);

        for(auto&& ch: choices) {
            header->children.insert(ch.header);
            ch.footer->children.insert(footer);
            result.allNodes.insert(ch.result.allNodes.begin(), ch.result.allNodes.end());
        }
        result.entry = header;

        return choice;
    }
};

} /* namespace borealis */

namespace llvm {

template<class GraphType> struct GraphTraits;
template<class GraphType> struct DOTGraphTraits;

template<>
struct GraphTraits<borealis::PSGraph*> {
    typedef borealis::PSGraphNode NodeType;

    static inline NodeType* getEntryNode(borealis::PSGraph* T) { return T->entry.get(); }

    static inline auto child_begin(NodeType* N) {
        return N->child_begin();
    }
    static inline auto child_end(NodeType *N) {
        return N->child_end();
    }

    using ChildIteratorType = decltype(std::declval<NodeType>().child_begin());
    using nodes_iterator = ChildIteratorType;

    static auto nodes_view(borealis::PSGraph* T) {
        return borealis::psgraph_impl::access_view(T->allNodes);
    }

    static nodes_iterator nodes_begin(borealis::PSGraph* T) {
        return nodes_view(T).begin();
    }

    static nodes_iterator nodes_end(borealis::PSGraph* T) {
        return nodes_view(T).end();
    }
};

template<>
struct DOTGraphTraits<borealis::PSGraph*>: DefaultDOTGraphTraits {
    DOTGraphTraits(){}
    DOTGraphTraits(bool ignore){}

    static bool isNodeHidden(const borealis::PSGraphNode* node) {
        return false; //node->data == nullptr;
    }

    template<typename GraphType>
    std::string getNodeLabel(const borealis::PSGraphNode* node, GraphType &) {
        if(node->data == nullptr) return "<>";
        return node->data->toString();
    }
};

} /* namespace llvm */

#endif // GRAPH_BUILDER_H

