/*
 * Unlogic.cpp
 *
 *  Created on: Aug 19, 2013
 *      Author: sam
 */

#include <stack>

#include "Predicate/PredicateFactory.h"
#include "SMT/MathSAT/Unlogic/Symbols.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "State/PredicateStateBuilder.h"
#include "Util/util.h"
#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {
namespace unlogic {


mathsat::VISIT_STATUS callback(mathsat::Expr expr, void* data) {
    std::stack<AbstractSymbol::Ptr> *symbolStack = static_cast<std::stack<AbstractSymbol::Ptr> *>(data);
    symbolStack->push(SymbolFactory(expr));
    return mathsat::VISIT_STATUS::PROCESS;
}


AbstractSymbol::Ptr makeSymbol(std::stack<AbstractSymbol::Ptr>& symbolStack) {
    std::stack<AbstractSymbol::Ptr> terminalStack;
    while (!symbolStack.empty()) {
        auto& symbol = symbolStack.top();
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


PredicateState::Ptr undoThat(Dynamic& dyn) {
    mathsat::Expr expr = mathsat_::logic::msatimpl::getExpr(dyn);
    std::stack<AbstractSymbol::Ptr> symbolStack;
    expr.visit(callback, &symbolStack);

    auto TF = TermFactory::get(nullptr, TypeFactory::get());
    auto PSF = PredicateStateFactory::get();
    auto PF = PredicateFactory::get();

    PredicateStateBuilder PSB(PSF, PSF->Basic());

    auto jointSymbol = makeSymbol(symbolStack);
    auto jointTerm = jointSymbol->undoThat(PSB);

    auto predicate = (
            PSB +=
            PF->getEqualityPredicate(
                    jointTerm,
                    TF->getBooleanTerm(true)
            )
    )();
    return predicate;
}

} // namespace unlogic
} // namespace mathsat_
} // namespace borealis


#include "Util/unmacros.h"
