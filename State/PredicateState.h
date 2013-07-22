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
#include "Logging/tracer.hpp"
#include "Predicate/Predicate.h"
#include "SMT/SMTUtil.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

class PredicateState :
    public std::enable_shared_from_this<const PredicateState> {

public:

    typedef std::shared_ptr<const PredicateState> Ptr;
    typedef std::function<PredicateState::Ptr(PredicateState::Ptr)> FMapper;
    typedef std::function<Predicate::Ptr(Predicate::Ptr)> Mapper;
    typedef std::function<bool(Predicate::Ptr)> Filterer;

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const = 0;

    virtual PredicateState::Ptr addVisited(const llvm::Value* loc) const = 0;
    virtual bool hasVisited(std::initializer_list<const llvm::Value*> locs) const = 0;
    virtual bool hasVisitedFrom(std::unordered_set<const llvm::Value*>& visited) const = 0;

    virtual PredicateState::Ptr fmap(FMapper) const {
        BYE_BYE(PredicateState::Ptr, "Should not be called!");
    }

    virtual PredicateState::Ptr map(Mapper m) const {
        return fmap([&](PredicateState::Ptr s) { return s->map(m); });
    }
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const {
        return fmap([&](PredicateState::Ptr s) { return s->filterByTypes(types); });
    }
    virtual PredicateState::Ptr filter(Filterer f) const {
        return fmap([&](PredicateState::Ptr s) { return s->filter(f); });
    }

    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const = 0;
    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr base) const = 0;
    virtual PredicateState::Ptr simplify() const = 0;

    bool isUnreachable() const;

    static bool classof(const PredicateState*) {
        return true;
    }

    virtual bool isEmpty() const = 0;

    virtual bool equals(const PredicateState* other) const = 0;
    bool operator==(const PredicateState& other) const {
        if (this == &other) return true;
        return this->equals(&other);
    }

    virtual std::string toString() const = 0;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const = 0;

    PredicateState(id_t predicate_state_type_id);
    virtual ~PredicateState() {};

    id_t getPredicateStateTypeId() const {
        return predicate_state_type_id;
    }

protected:

    const id_t predicate_state_type_id;

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

#define MK_COMMON_STATE_IMPL(CLASS) \
private: \
    typedef CLASS Self; \
    typedef std::unique_ptr<Self> SelfPtr; \
    CLASS(const Self& state) = default; \
    CLASS(Self&& state) = default; \
public: \
    friend class PredicateStateFactory; \
    virtual ~CLASS() {}; \
    static bool classof(const Self*) { \
        return true; \
    } \
    static bool classof(const PredicateState* ps) { \
        return ps->getPredicateStateTypeId() == type_id<Self>(); \
    }

#include "Util/unmacros.h"

#endif /* PREDICATESTATE_H_ */
