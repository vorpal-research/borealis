//
// Created by abdullin on 5/19/17.
//

#ifndef BOREALIS_GRAPHTRAITS_HPP
#define BOREALIS_GRAPHTRAITS_HPP

#include <algorithm>
#include <regex>

#include <llvm/Support/GraphWriter.h>

#include "BasicBlock.h"
#include "Function.h"

namespace llvm {
//
//template <>
//struct GraphTraits<borealis::absint::Function*> {
//    typedef borealis::absint::BasicBlock NodeType;
//
//    static inline NodeType* getEntryNode(borealis::absint::Function* T) {
//        return T->getEntryNode();
//    }
//
//    static inline auto child_begin(NodeType* Node) {
//        return Node->succ_begin();
//    }
//    static inline auto child_end(NodeType* Node) {
//        return Node->succ_end();
//    }
//
//    using nodes_iterator = decltype(std::declval<NodeType>().succ_begin());
//    using ChildIteratorType = nodes_iterator;
//
//    static nodes_iterator nodes_begin(borealis::absint::Function* T) {
//        return T->begin();
//    }
//    static nodes_iterator nodes_end(borealis::absint::Function* T) {
//        return T->end();
//    }
//};
//
//template<>
//struct DOTGraphTraits<borealis::absint::Function*>: DefaultDOTGraphTraits {
//    DOTGraphTraits(){}
//    DOTGraphTraits(bool){}
//
//    template<typename GraphType>
//    std::string getNodeLabel(borealis::absint::BasicBlock* node, GraphType &) {
//        std::string nodeStr = node->toString();
//        return std::regex_replace(nodeStr, std::regex("\n"), "\\l");
//    }
//
//};

}   /* namespace llvm */

#endif //BOREALIS_GRAPHTRAITS_HPP
