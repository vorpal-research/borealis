/*
 * Term.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef TERM_H_
#define TERM_H_

#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

class Term {

public:

    typedef size_t id_t;
    typedef std::unique_ptr<Term> Ptr;

    Term(id_t id, llvm::ValueType type, const std::string& name, borealis::id_t term_type_id) :
        id(id), type(type), name(name), term_type_id(term_type_id)
    {}

    virtual id_t getId() const {
        return id;
    }

    virtual llvm::ValueType getType() const {
        return type;
    }

    virtual const std::string& getName() const {
        return name;
    }

    virtual ~Term() {};

private:

    const id_t id;
    const llvm::ValueType type;
    const std::string name;

protected:

    const borealis::id_t term_type_id;

};

} /* namespace borealis */

#endif /* TERM_H_ */
