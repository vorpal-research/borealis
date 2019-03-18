/*
 * Term.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef TERM_H_
#define TERM_H_

#include <llvm/Support/Casting.h>

#include <memory>
#include <string>
#include <unordered_set>

#include "Logging/tracer.hpp"
#include "SMT/SMTUtil.h"
#include "Type/TypeFactory.h"
#include "Util/typeindex.hpp"
#include "Util/irf_ptr.hpp"
#include "Util/unions.hpp"
#include "Util/util.h"

#include "functional-hell/matchers_aux.hpp"

namespace borealis {

template<class SubClass> class Transformer;
template<class T> struct protobuf_traits;
template<class T> struct protobuf_traits_impl;

namespace proto { class Term; }
/** protobuf -> Term/Term.proto
import "Type/Type.proto";

package borealis.proto;

message Term {
    optional Type type = 1;
    optional string name = 2;

    extensions 16 to 64;
}

**/

class TermFactory;

template<class T>
struct PoolDeleter {
    void* poolPtr = nullptr;
    std::add_pointer_t<void(void*, T*)> deleter = nullptr;

    void operator()(T* ptr) {
        if(poolPtr && deleter) deleter(poolPtr, ptr);
        else delete ptr;
    }
};

class Term : public ClassTag, public util::irfd_base<const Term, PoolDeleter<const Term>> {

    friend bool operator==(const Term& a, const Term& b);

public:

    using Ptr = util::irfd_ptr<const Term, PoolDeleter<const Term>>;
    using ProtoPtr = std::unique_ptr<proto::Term>;
    using Subterms = std::vector<Term::Ptr>;

    // FIXME: akhin Maybe specialize std::equal_to ???
    struct DerefEqualsTo {
        bool operator()(Term::Ptr a, Term::Ptr b) const { return ops::deref_equals_to(a, b); }
    };
    using Set = std::unordered_set<Term::Ptr, std::hash<Term::Ptr>, Term::DerefEqualsTo>;

protected:

    Term(id_t classTag, Type::Ptr type, const std::string& name);
    Term(id_t classTag, Type::Ptr type, util::indexed_string name);
    Term(id_t classTag, Type::Ptr type, const char* name); // ambiguity resolution
    Term(const Term&) = default;

    void update();

    friend struct protobuf_traits<Term>;

public:

    virtual ~Term() = default;

    Type::Ptr getType() const;
    const std::string& getName() const;

    size_t getNumSubterms() const;
    const Subterms& getSubterms() const;

    virtual bool equals(const Term* other) const;
    virtual size_t hashCode() const;

    friend class ::borealis::TermFactory;
    Term::Ptr setType(TermFactory* TF, Type::Ptr newtype) const;

    static bool classof(const Term*) {
        return true;
    }

    static Set getFullTermSet(Term::Ptr term);

protected:

    Type::Ptr type;
    util::indexed_string name;

    Subterms subterms;

};

template<class Sub> struct AllocationPoint; // needed in TermFactory


std::ostream& operator<<(std::ostream& s, Term::Ptr t);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Term::Ptr t);

struct TermHash {
    size_t operator()(Term::Ptr trm) const noexcept {
        return trm->hashCode();
    }
};

struct TermShallowHash {
    size_t operator()(Term::Ptr trm) const noexcept {
        return std::hash<const void*>{}(trm.get());
    }
};

struct TermEquals {
    bool operator()(Term::Ptr lhv, Term::Ptr rhv) const noexcept {
        if(!lhv) return !rhv;
        return lhv->equals(rhv.get());
    }
};

struct TermCompare {
    bool operator()(Term::Ptr lhv, Term::Ptr rhv) const noexcept {
        return lhv->getName() < rhv->getName();
    }
};

// need for util::toString() to work
namespace util {

template <>
struct Stringifier<borealis::Term> {
    static std::string toString(const Term& t) {
        return t.getName();
    }
};

} // namespace util

} /* namespace borealis */

namespace std {
template<>
struct hash<borealis::Term::Ptr> {
    size_t operator()(const borealis::Term::Ptr& t) const noexcept {
        return t->hashCode();
    }
};
template<>
struct hash<const borealis::Term::Ptr> : hash<borealis::Term::Ptr> {};
} // namespace std

namespace functional_hell {
namespace matchers {

template<>
struct compare_trait<borealis::Term::Ptr> {
    bool operator()(const borealis::Term::Ptr& lhv, const borealis::Term::Ptr& rhv) const {
        return lhv->equals(rhv.get());
    }
};

} /* namespace matchers */
} /* namespace functional_hell */

struct EmplacePoint; // needed for pool allocation

#define MK_COMMON_TERM_IMPL(CLASS) \
private: \
    using Self = CLASS; \
    CLASS(const Self&) = default; \
public: \
    friend class TermFactory; \
    friend struct protobuf_traits_impl<Self>; \
    virtual ~CLASS() = default; \
    static bool classof(const Self*) { \
        return true; \
    } \
    static bool classof(const Term* t) { \
        return t->getClassTag() == class_tag<Self>(); \
    } \
    friend struct AllocationPoint<Self>; \
    friend struct ::EmplacePoint; \

#define TERM_ON_CHANGED(COND, CTOR) \
    if (COND) return Term::Ptr{ CTOR }; \
    else return this->shared_from_this();

#endif /* TERM_H_ */
