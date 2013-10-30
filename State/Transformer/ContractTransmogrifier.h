/*
 * ContractTransmogrifier.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef CONTRACTTRANSMOGRIFIER_H_
#define CONTRACTTRANSMOGRIFIER_H_

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class ContractTransmogrifier : public borealis::Transformer<ContractTransmogrifier> {

    typedef borealis::Transformer<ContractTransmogrifier> Base;

public:

    ContractTransmogrifier(FactoryNest FN) : Base(FN) {}

    Term::Ptr transformCmpTerm(CmpTermPtr t) {
        using llvm::isa;

        auto op = t->getOpcode();

        if (op == llvm::ConditionType::EQ) {
            if (isa<BoundTerm>(t->getLhv())) {
                return FN.Term->getCmpTerm(
                    llvm::ConditionType::UGE,
                    t->getLhv(),
                    t->getRhv()
                );
            } else if (isa<BoundTerm>(t->getRhv())) {
                return FN.Term->getCmpTerm(
                    llvm::ConditionType::UGE,
                    t->getRhv(),
                    t->getLhv()
                );
            }
        }

        return t;
    }

};

} /* namespace borealis */

#endif /* CONTRACTTRANSMOGRIFIER_H_ */
