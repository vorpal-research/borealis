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

    using Base = borealis::Transformer<ContractTransmogrifier>;

public:

    ContractTransmogrifier(FactoryNest FN);

    Term::Ptr transformCmpTerm(CmpTermPtr t);};

} /* namespace borealis */

#endif /* CONTRACTTRANSMOGRIFIER_H_ */
