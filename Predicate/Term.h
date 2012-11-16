/*
 * Term.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef TERM_H_
#define TERM_H_

#include "Util/util.h"

namespace borealis {

class Term {

public:

    Term(llvm::ValueType type, std::string name) :
        type(type), name(name)
    {}

    virtual llvm::ValueType getType() const {
        return type;
    }

    virtual std::string getName() const {
        return name;
    }

    virtual ~Term() {};

private:

    const llvm::ValueType type;
    const std::string name;

};

} /* namespace borealis */

#endif /* TERM_H_ */
