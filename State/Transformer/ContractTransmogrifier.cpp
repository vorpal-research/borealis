/*
 * ContractTransmogrifier.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#include "State/Transformer/ContractTransmogrifier.h"

namespace borealis {

ContractTransmogrifier::ContractTransmogrifier(FactoryNest FN) : Base(FN) {}

Term::Ptr ContractTransmogrifier::transformCmpTerm(CmpTermPtr t) {

    auto&& op = t->getOpcode();

    switch (op) {
        case llvm::ConditionType::EQ: {
            if (llvm::isa<BoundTerm>(t->getLhv())) {
                return FN.Term->getCmpTerm(
                    llvm::ConditionType::UGE,
                    t->getLhv(),
                    t->getRhv()
                );
            } else if (llvm::isa<BoundTerm>(t->getRhv())) {
                return FN.Term->getCmpTerm(
                    llvm::ConditionType::UGE,
                    t->getRhv(),
                    t->getLhv()
                );
            }
            break;
        };
    }

    return t;
}

} /* namespace borealis */
