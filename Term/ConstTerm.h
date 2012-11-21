/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include "Term.h"

namespace borealis {

class TermFactory;
class BooleanPredicate;
class GEPPredicate;

class ConstTerm: public borealis::Term {

public:

    virtual ~ConstTerm() {};

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ConstTerm>();
    }

    static bool classof(const ConstTerm* /* t */) {
        return true;
    }

    llvm::ValueType getType() const {
        return type;
    }

    const std::string& getName() const {
        return name;
    }

    friend class TermFactory;
    friend class BooleanPredicate;
    friend class GEPPredicate;

private:

    ConstTerm(llvm::ValueType type, const std::string& name) :
        Term(0, type, name, type_id(*this)),
        name(name)
    { this->type = type; }

    llvm::ValueType type;
    const std::string name;

};

} /* namespace borealis */

#endif /* CONSTTERM_H_ */
