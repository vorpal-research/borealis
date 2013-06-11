/*
 * Term.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef TERM_H_
#define TERM_H_

#include <string>

#include "Solver/ExecutionContext.h"
#include "Solver/Z3ExprFactory.h"
#include "Type/TypeFactory.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

// Forward declaration
template<class SubClass>
class Transformer;

class Term {

public:

    typedef std::shared_ptr<const Term> Ptr;

protected:

    Term(id_t id, const std::string& name, borealis::id_t term_type_id) :
        id(id), name(name), term_type_id(term_type_id) {};
    Term(const Term&) = default;
    virtual ~Term() {};

public:

    id_t getId() const {
        return id;
    }

    const std::string& getName() const {
        return name;
    }

    borealis::id_t getTermTypeId() const {
        return term_type_id;
    }

    virtual bool equals(const Term* other) const {
        if (other == nullptr) return false;
        if (this == other) return true;
        return this->id == other->id &&
                this->term_type_id == other->term_type_id;
    }

    bool operator==(const Term& other) const {
        return this->equals(&other);
    }

    size_t hashCode() const {
        return static_cast<size_t>(id) ^ static_cast<size_t>(term_type_id);
    }

    static bool classof(const Term* /* t */) {
        return true;
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory&, ExecutionContext*) const {
        BYE_BYE(Z3ExprFactory::Dynamic, "Should not be called!");
    }
#include "Util/unmacros.h"

    virtual Type::Ptr getTermType() const = 0;

private:

    const id_t id;
    const std::string name;
    const borealis::id_t term_type_id;

};

} /* namespace borealis */

namespace std {
template<>
struct hash<borealis::Term::Ptr> {
    size_t operator()(const borealis::Term::Ptr& t) const {
        return t->hashCode();
    }
};
template<>
struct hash<const borealis::Term::Ptr> {
    size_t operator()(const borealis::Term::Ptr& t) const {
        return t->hashCode();
    }
};
} // namespace std

#endif /* TERM_H_ */
