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

#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"
#include "Predicate/Predicate.h"
#include "SMT/SMTUtil.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<class T> struct protobuf_traits;
template<class T> struct protobuf_traits_impl;

namespace proto { class PredicateState; }
/** protobuf -> State/PredicateState.proto

package borealis.proto;

message PredicateState {
    extensions 16 to 64;
}

**/
class PredicateState :
    public ClassTag,
    public std::enable_shared_from_this<const PredicateState> {

public:

    using Ptr = std::shared_ptr<const PredicateState>;
    using ProtoPtr = std::unique_ptr<proto::PredicateState>;
    using Loci = std::unordered_set<Locus>;

    using FMapper = std::function<PredicateState::Ptr(PredicateState::Ptr)>;
    using Mapper = std::function<Predicate::Ptr(Predicate::Ptr)>;
    using Filterer = std::function<bool(Predicate::Ptr)>;

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const = 0;

    virtual PredicateState::Ptr addVisited(const Locus& locus) const = 0;
    virtual bool hasVisited(std::initializer_list<Locus> loci) const = 0;
    virtual bool hasVisitedFrom(Loci& visited) const = 0;

    virtual Loci getVisited() const = 0;

    virtual PredicateState::Ptr fmap(FMapper) const;

    virtual PredicateState::Ptr map(Mapper m) const;
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const;
    virtual PredicateState::Ptr filter(Filterer f) const;
    virtual PredicateState::Ptr reverse() const;

    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(
            std::initializer_list<PredicateType> types) const = 0;
    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr on) const = 0;
    virtual PredicateState::Ptr simplify() const = 0;

    bool isUnreachableIn(unsigned long long memoryStart) const;

    static bool classof(const PredicateState*) {
        return true;
    }

    virtual bool isEmpty() const = 0;

    virtual bool equals(const PredicateState* other) const;

    virtual std::string toString() const = 0;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const = 0;

    PredicateState(id_t classTag);
    virtual ~PredicateState() = default;

protected:

    static PredicateState::Ptr Simplified(const PredicateState* s) {
        return PredicateState::Ptr(s)->simplify();
    }

    template<typename T, typename ...Args>
    static PredicateState::Ptr Simplified(Args&&... args) {
        return PredicateState::Ptr(new T{ std::forward<Args>(args)... })->simplify();
    }

};

bool operator==(const PredicateState& a, const PredicateState& b);

std::ostream& operator<<(std::ostream& s, PredicateState::Ptr state);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, PredicateState::Ptr state);

PredicateState::Ptr operator+ (PredicateState::Ptr state, Predicate::Ptr pred);
PredicateState::Ptr operator<<(PredicateState::Ptr state, const Locus& locus);

} /* namespace borealis */

#define MK_COMMON_STATE_IMPL(CLASS) \
private: \
    using Self = CLASS; \
    using SelfPtr = std::unique_ptr<Self>; \
    CLASS(const Self&) = default; \
    CLASS(Self&&) = default; \
public: \
    friend class PredicateState; \
    friend class PredicateStateFactory; \
    friend struct protobuf_traits_impl<CLASS>; \
    virtual ~CLASS() = default; \
    static bool classof(const Self*) { \
        return true; \
    } \
    static bool classof(const PredicateState* s) { \
        return s->getClassTag() == class_tag<Self>(); \
    } \
    template<typename ...Args> \
    static SelfPtr Uniquified(Args&&... args) { \
        return SelfPtr(new Self{ std::forward<Args>(args)... }); \
    }

#include "Util/unmacros.h"

#endif /* PREDICATESTATE_H_ */
