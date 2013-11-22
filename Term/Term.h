/*
 * Term.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef TERM_H_
#define TERM_H_

#include <memory>
#include <string>

#include "SMT/SMTUtil.h"
#include "Type/TypeFactory.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

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
    optional bool retypable = 3;

    extensions 16 to 64;
}

**/
class Term : public ClassTag, public std::enable_shared_from_this<Term> {

public:

    typedef std::shared_ptr<const Term> Ptr;
    typedef std::unique_ptr<proto::Term> ProtoPtr;

protected:

    Term(id_t classTag, Type::Ptr type, const std::string& name, bool retypable = true) :
        ClassTag(classTag), type(type), name(name), retypable(retypable) {};
    Term(const Term&) = default;

    friend struct protobuf_traits<Term>;

public:
    virtual ~Term() {};

    Type::Ptr getType() const {
        return type;
    }

    const std::string& getName() const {
        return name;
    }

    bool isRetypable() const {
        return retypable;
    }

    virtual bool equals(const Term* other) const {
        if (other == nullptr) return false;
        return classTag == other->classTag &&
                type == other->type &&
                name == other->name &&
                retypable == other->retypable;
    }

    bool operator==(const Term& other) const {
        if (this == &other) return true;
        return this->equals(&other);
    }

    virtual size_t hashCode() const {
        return util::hash::defaultHasher()(classTag, type, name, retypable);
    }

    static bool classof(const Term*) {
        return true;
    }

protected:

    Type::Ptr type;
    std::string name;
    bool retypable;

};

} /* namespace borealis */

namespace std {
template<>
struct hash<borealis::Term::Ptr> {
    size_t operator()(const borealis::Term::Ptr& t) const {
        return t->hashCode();
    }
};
template<>
struct hash<const borealis::Term::Ptr> {
    size_t operator()(const borealis::Term::Ptr& t) const {
        return t->hashCode();
    }
};
} // namespace std

#define MK_COMMON_TERM_IMPL(CLASS) \
private: \
    typedef CLASS Self; \
    CLASS(const CLASS&) = default; \
public: \
    friend class TermFactory; \
    friend struct protobuf_traits_impl<CLASS>; \
    virtual ~CLASS() {}; \
    static bool classof(const Self*) { \
        return true; \
    } \
    static bool classof(const Term* t) { \
        return t->getClassTag() == class_tag<Self>(); \
    }

#define TERM_ON_CHANGED(COND, CTOR) \
    if (COND) return Term::Ptr{ CTOR }; \
    else return this->shared_from_this();

#endif /* TERM_H_ */
