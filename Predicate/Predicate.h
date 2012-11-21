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

enum class DependeeType {
    NONE = 3,
    VALUE = 5,
    DEREF_VALUE = 7
};

class Predicate {

public:

    typedef std::shared_ptr<const Predicate> Ptr;

    typedef std::pair<size_t, Term::id_t> Key;
    struct KeyHash {
    public:
        static size_t hash(const Key& k) {
            return k.first ^ (size_t)k.second;
        }
        size_t operator()(const Key& k) const {
            return hash(k);
        }
    };

    typedef std::pair<DependeeType, Term::id_t> Dependee;
    struct DependeeHash {
    public:
        static size_t hash(const Dependee& k) {
            return static_cast<size_t>(k.first) ^ (size_t)k.second;
        }
        size_t operator()(const Dependee& k) const {
            return hash(k);
        }
    };
    typedef std::unordered_set<Dependee, DependeeHash> DependeeSet;

    Predicate(borealis::id_t predicate_type_id);
    Predicate(borealis::id_t predicate_type_id, PredicateType type);
    virtual ~Predicate() = 0;
    virtual Key getKey() const = 0;

    virtual Dependee getDependee() const = 0;
    virtual DependeeSet getDependees() const = 0;

    std::string toString() const {
        return asString;
    }

    PredicateType getType() const {
        return type;
    }

    virtual z3::expr toZ3(Z3ExprFactory& z3ef) const = 0;

    static bool classof(const Predicate* /* t */) {
        return true;
    }

    inline borealis::id_t getPredicateTypeId() const {
        return predicate_type_id;
    }

protected:

    borealis::id_t predicate_type_id;
    PredicateType type;
    std::string asString;

};

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Predicate& p);
logging::stream_t& operator<<(logging::stream_t& s, const borealis::Predicate& p);

} /* namespace borealis */

#endif /* PREDICATE_H_ */
