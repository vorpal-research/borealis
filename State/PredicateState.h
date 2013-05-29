/*
 * PredicateState.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATE_H_
#define PREDICATESTATE_H_

#include <functional>
#include <initializer_list>
#include <memory>

#include "Logging/logstream.hpp"
#include "Predicate/Predicate.h"
#include "Solver/ExecutionContext.h"
#include "Solver/Z3ExprFactory.h"

namespace borealis {

class PredicateState :
    public std::enable_shared_from_this<const PredicateState> {

    typedef borealis::logging::logstream logstream;

public:

    typedef std::shared_ptr<const PredicateState> Ptr;
    typedef std::function<Predicate::Ptr(Predicate::Ptr)> Mapper;
    typedef std::function<bool(Predicate::Ptr)> Filterer;

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const = 0;
    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx = nullptr) const = 0;

    virtual PredicateState::Ptr addVisited(const llvm::Value* loc) const = 0;
    virtual bool hasVisited(std::initializer_list<const llvm::Value*> locs) const = 0;

    virtual PredicateState::Ptr map(Mapper m) const = 0;
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const = 0;
    virtual PredicateState::Ptr filter(Filterer f) const = 0;
    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const = 0;

    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr base) const = 0;

    virtual PredicateState::Ptr simplify() const = 0;

    bool isUnreachable() const;

    static bool classof(const PredicateState* /* ps */) {
        return true;
    }

    virtual bool isEmpty() const = 0;

    virtual bool equals(const PredicateState* other) const = 0;
    bool operator==(const PredicateState& other) const {
        return this->equals(&other);
    }

    virtual std::string toString() const = 0;
    virtual logstream& dump(logstream& s) const {
        return s << this->toString();
    }

    PredicateState(borealis::id_t predicate_state_type_id);
    virtual ~PredicateState();

    borealis::id_t getPredicateStateTypeId() const {
        return predicate_state_type_id;
    }

protected:

    const borealis::id_t predicate_state_type_id;

    static PredicateState::Ptr Simplified(const PredicateState* s) {
        return PredicateState::Ptr(s)->simplify();
    }
};

std::ostream& operator<<(std::ostream& s, PredicateState::Ptr state);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, PredicateState::Ptr state);

////////////////////////////////////////////////////////////////////////////////
//
// PredicateState operators
//
////////////////////////////////////////////////////////////////////////////////

PredicateState::Ptr operator+ (PredicateState::Ptr state, Predicate::Ptr p);
PredicateState::Ptr operator<<(PredicateState::Ptr state, const llvm::Value* loc);
PredicateState::Ptr operator<<(PredicateState::Ptr state, const llvm::Value& loc);

} /* namespace borealis */

#endif /* PREDICATESTATE_H_ */
