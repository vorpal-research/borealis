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
#include "Protobuf/ConverterUtil.h"
#include "Protobuf/Gen/Predicate/Predicate.pb.h"
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

// Forward declaration
template<class SubClass> class Transformer;

namespace proto { class Predicate; }
// End of forward declaration

/** protobuf -> Predicate/Predicate.proto
import "Predicate/PredicateType.proto";

package borealis.proto;

message Predicate {
    optional PredicateType type = 1;

    extensions 16 to 64;
}

**/
class Predicate : public ClassTag {

public:

    typedef std::shared_ptr<const Predicate> Ptr;
    typedef std::unique_ptr<proto::Predicate> ProtoPtr;

protected:

    Predicate(id_t classTag);
    Predicate(id_t classTag, PredicateType type);
    Predicate(const Predicate&) = default;

public:

    virtual ~Predicate() {};

    PredicateType getType() const {
        return type;
    }

    Predicate* setType(PredicateType type) {
        this->type = type;
        return this;
    }



    std::string toString() const {
        switch (type) {
        case PredicateType::REQUIRES: return "@R " + asString;
        case PredicateType::ENSURES: return "@E " + asString;
        case PredicateType::ASSERT: return "@A " + asString;
        case PredicateType::ASSUME: return "@U " + asString;
        case PredicateType::PATH: return "@P " + asString;
        default: return asString;
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

    virtual Predicate* clone() const = 0;

protected:

    PredicateType type;
    // Must be set in subclasses
    std::string asString;

};

std::ostream& operator<<(std::ostream& s, Predicate::Ptr p);

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
    template<class B> friend struct protobuf_traits_impl; \
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

#endif /* PREDICATE_H_ */
