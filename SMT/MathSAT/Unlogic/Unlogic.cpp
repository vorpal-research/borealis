/*
 * Unlogic.cpp
 *
 *  Created on: Aug 19, 2013
 *      Author: sam
 */

#include <stack>

#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "State/PredicateStateBuilder.h"

namespace borealis {
namespace mathsat {


struct callback_data {
    std::stack<msat_term>* termStack;
    size_t* term_id;

    callback_data(std::stack<msat_term>* stack, size_t *id) : termStack(stack), term_id(id) {}
} typedef callback_data;

msat_visit_status callback(msat_env env, msat_term term, int preorder, void* data) {
    callback_data* cd = static_cast< callback_data *>(data);
    std::stack<msat_term>* termStack = cd->termStack;
    if((1 == preorder) && ( cd->term_id == nullptr || (*cd->term_id != msat_term_id(term)))) {
        termStack->push(term);
        if (0 != msat_decl_get_arity(msat_term_get_decl(term))) {
        // FIXME sam Не знаю насколько нужна эта проверка.
        // Она позволяет уменьшить количество рекурсивных вызовов: для терминальных
        // узлов не будет вызываться msat_visit_term. Без этой проверки также работает,
        // но окончание рекурсии определяется в предыдущей проверке с дополнительным
        // рекурсивным вызовом.
            auto id = msat_term_id(term);
            callback_data pcd(termStack, &id);
            msat_visit_term(env, term, callback, &pcd);
            return MSAT_VISIT_SKIP;
        }
    }
    return MSAT_VISIT_PROCESS;
}


PredicateState::Ptr undoThat(Expr& expr) {
    std::stack<msat_term> termStack;
    callback_data pcd(&termStack, nullptr);
    expr.visit(callback, &pcd);
    std::cout << termStack.size() << std::endl;
    while(!termStack.empty()) {
        std::cout << msat_term_repr(termStack.top()) << std::endl;
        termStack.pop();
    }
    auto PSF = PredicateStateFactory::get();
    auto basic = PSF->Basic();
    return PredicateStateBuilder(PSF, basic)();
}


////////////////////////////////////////////////////////////////////////////////


class AbstractSymbol {
protected:
    msat_symbol_tag symbolTag_;
    std::vector<AbstractSymbol *> args_;
    Expr expr_;


public:
    AbstractSymbol(msat_symbol_tag symbolTag, unsigned numArgs, const Expr& expr)
                    : symbolTag_(symbolTag), args_(std::vector<AbstractSymbol *>()),
                      expr_(expr) {
        args_.reserve(numArgs);
    }

    virtual ~AbstractSymbol() {}

    msat_symbol_tag symbolTag() const { return symbolTag_; };

    unsigned numArgs() const { return args_.size(); }

    bool isTerminal() const { return args_.size() == 0; }

    const Expr& expr() const { return expr_; }

    const std::vector<AbstractSymbol *>& args() const { return args_; }

    void setArg(unsigned num, AbstractSymbol* symbol) { args_[num] = symbol; }

    virtual borealis::Term::Ptr undoThat() const = 0;
};

} //namespace mathsat
} //namespace borealis
