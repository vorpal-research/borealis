/*
 * Predicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATE_H_
#define PREDICATE_H_

#include <llvm/Support/raw_ostream.h>
#include <llvm/Value.h>
#include <z3/z3++.h>

#include <memory>
#include <tuple>
#include <unordered_set>

#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"
#include "Solver/ExecutionContext.h"
#include "Solver/Z3ExprFactory.h"
#include "Term/Term.h"
#include "Util/slottracker.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

enum class PredicateType {
    PATH,
    STATE
};

// Forward declaration
template<class SubClass>
class Transformer;

class Predicate {

public:

    typedef std::shared_ptr<const Predicate> Ptr;

    typedef std::pair<borealis::id_t, Term::id_t> Key;
    struct KeyHash {
    public:
        static size_t hash(const Key& k) {
            return (size_t)k.first ^ (size_t)k.second;
        }
        size_t operator()(const Key& k) const {
            return hash(k);
        }
    };

    Predicate(borealis::id_t predicate_type_id);
    Predicate(borealis::id_t predicate_type_id, PredicateType type);
    virtual ~Predicate() = 0;
    virtual Key getKey() const = 0;

    inline borealis::id_t getPredicateTypeId() const {
        return predicate_type_id;
    }

    inline PredicateType getType() const {
        return type;
    }

    inline std::string toString() const {
        return asString;
    }

    virtual z3::expr toZ3(Z3ExprFactory&, ExecutionContext* = nullptr) const = 0;

    static bool classof(const Predicate* /* t */) {
        return true;
    }

    virtual bool equals(const Predicate* other) const = 0;
    bool operator==(const Predicate& other) const {
        return this->equals(&other);
    }

    virtual size_t hashCode() const = 0;

protected:

    borealis::id_t predicate_type_id;
    PredicateType type;
    std::string asString;

};

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Predicate& p);
std::ostream& operator<<(std::ostream& s, const borealis::Predicate& p);

} /* namespace borealis */

#endif /* PREDICATE_H_ */
