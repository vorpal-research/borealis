/*
 * Predicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATE_H_
#define PREDICATE_H_

#include <llvm/Instruction.h>

#include <memory>

#include "Annotation/Annotation.h"
#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"
#include "SMT/SMTUtil.h"
#include "Term/Term.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

/** protobuf -> Predicate/PredicateType.proto
package borealis.proto;

enum PredicateType {
    PATH     = 0;
    STATE    = 1;
    REQUIRES = 2;
    ENSURES  = 3;
    ASSERT   = 4;
    ASSUME   = 5;
}
**/
enum class PredicateType {
    PATH     = 0,
    STATE    = 1,
    REQUIRES = 2,
    ENSURES  = 3,
    ASSERT   = 4,
    ASSUME   = 5
};

PredicateType predicateType(const Annotation* a);

template<class SubClass> class Transformer;
template<class T> struct protobuf_traits;
template<class T> struct protobuf_traits_impl;

namespace proto { class Predicate; }
/** protobuf -> Predicate/Predicate.proto
import "Predicate/PredicateType.proto";
import "Util/locations.proto";

package borealis.proto;

message Predicate {
    optional PredicateType type = 1;
    optional Locus location = 2;

    extensions 16 to 64;
}

**/
class Predicate : public ClassTag, public std::enable_shared_from_this<Predicate> {

public:

    typedef std::shared_ptr<const Predicate> Ptr;
    typedef std::unique_ptr<proto::Predicate> ProtoPtr;

protected:

    Predicate(id_t classTag);
    Predicate(id_t classTag, PredicateType type);
    Predicate(id_t classTag, PredicateType type, const Locus& loc);
    Predicate(const Predicate&) = default;

    friend struct protobuf_traits<Predicate>;

public:
    virtual ~Predicate() {};

    PredicateType getType() const {
        return type;
    }

    Predicate* setType(PredicateType type) {
        this->type = type;
        return this;
    }

    const Locus& getLocation() const {
        return location;
    }

    Predicate* setLocations(const Locus& loc) {
        this->location = loc;
        return this;
    }

    std::string toString() const {
        switch (type) {
        case PredicateType::REQUIRES: return "@R " + asString;
        case PredicateType::ENSURES:  return "@E " + asString;
        case PredicateType::ASSERT:   return "@A " + asString;
        case PredicateType::ASSUME:   return "@U " + asString;
        case PredicateType::PATH:     return "@P " + asString;
        case PredicateType::STATE:    return asString;
        default:                      return "@?" + asString;
        }
    }

    static bool classof(const Predicate*) {
        return true;
    }

    virtual bool equals(const Predicate* other) const {
        if (other == nullptr) return false;
        return classTag == other->classTag &&
                type == other->type;
    }

    bool operator==(const Predicate& other) const {
        if (this == &other) return true;
        return this->equals(&other);
    }

    virtual size_t hashCode() const {
        return util::hash::defaultHasher()(classTag, type);
    }

    virtual Predicate* clone() const {
#include "Util/macros.h"
        BYE_BYE(Predicate*, "Should not be called!");
#include "Util/unmacros.h"
    }

protected:

    PredicateType type;
    Locus location;
    // Must be set in subclasses
    std::string asString;

};

std::ostream& operator<<(std::ostream& s, Predicate::Ptr p);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Predicate::Ptr p);

} /* namespace borealis */

namespace std {
template<>
struct hash<borealis::PredicateType> : public borealis::util::enums::enum_hash<borealis::PredicateType> {};
template<>
struct hash<const borealis::PredicateType> : public borealis::util::enums::enum_hash<borealis::PredicateType> {};
}

#define MK_COMMON_PREDICATE_IMPL(CLASS) \
private: \
    typedef CLASS Self; \
    CLASS(const Self&) = default; \
public: \
    friend class PredicateFactory; \
    friend struct protobuf_traits_impl<CLASS>; \
    virtual ~CLASS() {}; \
    static bool classof(const Self*) { \
        return true; \
    } \
    static bool classof(const Predicate* p) { \
        return p->getClassTag() == class_tag<Self>(); \
    } \
    virtual Predicate* clone() const override { \
        return new Self{ *this }; \
    }

#define PREDICATE_ON_CHANGED(COND, CTOR) \
    if (COND) return Predicate::Ptr{ CTOR }; \
    else return this->shared_from_this();

#endif /* PREDICATE_H_ */
