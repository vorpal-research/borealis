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

enum class PredicateType {
    PATH,
    STATE,
    REQUIRES,
    ENSURES,
    ASSERT,
    ASSUME
};

PredicateType predicateType(const Annotation* a);

// Forward declaration
template<class SubClass> class Transformer;
// End of forward declaration

class Predicate : public ClassTag {

public:

    typedef std::shared_ptr<const Predicate> Ptr;

    Predicate(id_t classTag);
    Predicate(id_t classTag, PredicateType type);
    Predicate(const Predicate&) = default;
    virtual ~Predicate() {};

    PredicateType getType() const {
        return type;
    }

    Predicate* setType(PredicateType type) {
        this->type = type;
        return this;
    }

    const llvm::Instruction* getLocation() const {
        return location;
    }

    Predicate* setLocation(const llvm::Instruction* location) {
        this->location = location;
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
        return type == other->type &&
                location == other->location;
    }

    bool operator==(const Predicate& other) const {
        if (this == &other) return true;
        return this->equals(&other);
    }

    virtual size_t hashCode() const {
        return util::hash::defaultHasher()(classTag, type, location);
    }

    virtual Predicate* clone() const = 0;

protected:

    PredicateType type;
    const llvm::Instruction* location;

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
