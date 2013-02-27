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

#include "Annotation/EnsuresAnnotation.h"
#include "Annotation/RequiresAnnotation.h"
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
    STATE,
    REQUIRES,
    ENSURES
};

PredicateType predicateType(const Annotation* a);

// Forward declaration
template<class SubClass>
class Transformer;

class Predicate {

public:

    typedef std::shared_ptr<const Predicate> Ptr;

    Predicate(borealis::id_t predicate_type_id);
    Predicate(borealis::id_t predicate_type_id, PredicateType type);
    virtual ~Predicate() = 0;

    inline borealis::id_t getPredicateTypeId() const {
        return predicate_type_id;
    }

    inline PredicateType getType() const {
        return type;
    }

    inline Predicate* setType(PredicateType type) {
        this->type = type;
        return this;
    }

    inline std::string toString() const {
        return asString;
    }

    virtual logic::Bool toZ3(Z3ExprFactory&, ExecutionContext* = nullptr) const = 0;

    static bool classof(const Predicate* /* t */) {
        return true;
    }

    virtual bool equals(const Predicate* other) const = 0;
    bool operator==(const Predicate& other) const {
        return this->equals(&other);
    }

    virtual size_t hashCode() const = 0;

    inline const llvm::Instruction* getLocation() const {
        return location;
    }

    inline Predicate* setLocation(const llvm::Instruction* location) {
        this->location = location;
        return this;
    }

protected:

    const borealis::id_t predicate_type_id;

    PredicateType type;
    const llvm::Instruction* location;

    // Must be set in subclasses
    std::string asString;

};

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Predicate& p);
std::ostream& operator<<(std::ostream& s, const borealis::Predicate& p);

} /* namespace borealis */

#endif /* PREDICATE_H_ */
