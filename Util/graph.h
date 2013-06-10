/*
 * graph.h
 *
 *  Created on: May 27, 2013
 *      Author: ice-phoenix
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/Support/CFG.h>

#include <unordered_map>

#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {

class TopologicalSorter {

    enum class Color {
        NONE,
        GREY,
        BLACK
    };
    typedef std::unordered_map<const llvm::BasicBlock*, Color> Marks;

public:

    typedef std::list<llvm::BasicBlock*> Ordered;
    typedef borealis::util::option<Ordered> Result;

    void clear() {
        marks.clear();
        ordered.clear();
    }

    Result doit(llvm::Function& F) {
        clear();

        for (llvm::BasicBlock& BB : F) {
            auto* pBB = &BB;

            if (isBlack(pBB)) continue;
            if (!visit(pBB)) {
                return util::nothing();
            }
        }

        return util::just(Ordered(ordered));
    }

private:

    Marks marks;
    Ordered ordered;

    bool visit(llvm::BasicBlock* BB) {
        using namespace llvm;
        using borealis::util::view;

        if (isGrey(BB)) return false;
        if (isBlack(BB)) return true;

        markGrey(BB);
        for (llvm::BasicBlock* pred : view(pred_begin(BB), pred_end(BB))) {
            if (!visit(pred)) {
                return false;
            }
        }
        markBlack(BB);

        ordered.push_back(BB);

        return true;
    }

    bool isBlack(const llvm::BasicBlock* BB) const {
        return borealis::util::containsKey(marks, BB)
               && marks.at(BB) == Color::BLACK;
    }

    bool isGrey(const llvm::BasicBlock* BB) const {
        return borealis::util::containsKey(marks, BB)
               && marks.at(BB) == Color::GREY;
    }

    void markBlack(const llvm::BasicBlock* BB) {
        marks[BB] = Color::BLACK;
    }

    void markGrey(const llvm::BasicBlock* BB) {
        marks[BB] = Color::GREY;
    }

};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* GRAPH_H_ */
