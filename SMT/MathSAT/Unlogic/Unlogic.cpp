/*
 * Unlogic.cpp
 *
 *  Created on: Aug 19, 2013
 *      Author: sam
 */

#include <stack>

#include "SMT/MathSAT/Unlogic/Symbols.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "State/PredicateStateBuilder.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {
namespace unlogic {

USING_SMT_LOGIC(MathSAT);

mathsat::VISIT_STATUS callback(mathsat::Expr expr, void* data) {
    std::stack<AbstractSymbol::Ptr>* symbolStack =
        static_cast<std::stack<AbstractSymbol::Ptr>*>(data);
    symbolStack->push(SymbolFactory(expr));
    return mathsat::VISIT_STATUS::PROCESS;
}

AbstractSymbol::Ptr makeSymbol(std::stack<AbstractSymbol::Ptr>& symbolStack) {
    std::stack<AbstractSymbol::Ptr> terminalStack;
    while (!symbolStack.empty()) {
        auto symbol = symbolStack.top();
        if (symbol->isTerminal()) {
            terminalStack.push(symbol);
        } else {
            ASSERTC(terminalStack.size() >= symbol->numArgs());
            for (unsigned i = 0; i < symbol->numArgs(); ++i) {
                symbol->setArg(i, terminalStack.top());
                terminalStack.pop();
            }
            terminalStack.push(symbol);
        }
        symbolStack.pop();
    }
    ASSERTC(terminalStack.size() == 1);
    return terminalStack.top();
}

Term::Ptr undoThat(Dynamic dyn) {
    auto expr = borealis::mathsat_::logic::msatimpl::getExpr(dyn);
    std::stack<AbstractSymbol::Ptr> symbolStack;
    expr.visit(callback, &symbolStack);

    FactoryNest FN(nullptr);

    auto jointSymbol = makeSymbol(symbolStack);
    return jointSymbol->undoThat(FN);
}

} // namespace unlogic
} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"
