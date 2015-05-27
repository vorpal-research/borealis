/*
 * Predicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATE_H_
#define PREDICATE_H_

#include <memory>

#include "Annotation/Annotation.h"
#include "Logging/logger.hpp"
#include "Logging/tracer.hpp"
#include "SMT/SMTUtil.h"
#include "Term/Term.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

#include "functional-hell/matchers_aux.hpp"

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
class Predicate : public ClassTag, public std::enable_shared_from_this<const Predicate> {

public:

    using Ptr = std::shared_ptr<const Predicate>;
    using ProtoPtr = std::unique_ptr<proto::Predicate>;
    using Operands = std::vector<Term::Ptr>;

protected:

    Predicate(id_t classTag);
    Predicate(id_t classTag, PredicateType type);
    Predicate(id_t classTag, PredicateType type, const Locus& loc);
    Predicate(const Predicate&) = default;

    friend struct protobuf_traits<Predicate>;

public:

    virtual ~Predicate() = default;

    PredicateType getType() const;
    Predicate* setType(PredicateType type);

    const Locus& getLocation() const;
    Predicate* setLocations(const Locus& loc);

    unsigned getNumOperands() const;
    const Operands& getOperands() const;

    std::string toString() const;

    static bool classof(const Predicate*) {
        return true;
    }

    virtual bool equals(const Predicate* other) const;
    virtual size_t hashCode() const;

    virtual Predicate* clone() const;

protected:

    PredicateType type;
    Locus location;

    // Must be set in subclasses
    std::string asString;
    Operands ops;

};

bool operator==(const Predicate& a, const Predicate& b);

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
    using Self = CLASS; \
    CLASS(const Self&) = default; \
public: \
    friend class PredicateFactory; \
    friend struct protobuf_traits_impl<CLASS>; \
    virtual ~CLASS() = default; \
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
