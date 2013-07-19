/*
 * Term.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef TERM_H_
#define TERM_H_

#include <string>

#include "SMT/SMTUtil.h"
#include "Type/TypeFactory.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

// Forward declarations
template<class SubClass> class Transformer;
// End of forward declarations

class Term {

public:

    typedef std::shared_ptr<const Term> Ptr;

protected:

    Term(id_t id, const std::string& name, borealis::id_t term_type_id) :
        id(id), term_type_id(term_type_id), name(name) {};
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
        return this->id == other->id &&
                this->term_type_id == other->term_type_id &&
                this->name == other->name;
    }

    bool operator==(const Term& other) const {
        if (this == &other) return true;
        return this->equals(&other);
    }

    size_t hashCode() const {
        return static_cast<size_t>(id) ^
               static_cast<size_t>(term_type_id) ^
               std::hash<std::string>()(name);
    }

    static bool classof(const Term*) {
        return true;
    }

    virtual Type::Ptr getTermType() const = 0;

private:

    const id_t id;
    const borealis::id_t term_type_id;

protected:

    std::string name;

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
