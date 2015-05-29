/*
 * Annotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATION_H_
#define ANNOTATION_H_

#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallSite.h>

#include <memory>
#include <string>

#include "Codegen/llvm.h"
#include "Term/TermFactory.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

template<class SubClass> class Transformer;

namespace proto { class Annotation; }
/** protobuf -> Annotation/Annotation.proto
import "Util/locations.proto";

package borealis.proto;

message Annotation {
    optional Locus locus = 1;

    extensions 16 to 64;
}

**/
class Annotation : public std::enable_shared_from_this<Annotation> {
private:
    typedef Annotation self;

protected:
    typedef const char* const keyword_t;

    borealis::id_t annotation_type_id;
    keyword_t keyword;
    Locus locus;

public:
    typedef std::shared_ptr<const self> Ptr;
    typedef std::unique_ptr<proto::Annotation> ProtoPtr;

    Annotation(borealis::id_t annotation_type_id, keyword_t keyword, Locus locus):
        annotation_type_id(annotation_type_id),
        keyword(keyword),
        locus(locus) {}

    id_t getTypeId() const { return annotation_type_id; }

    virtual ~Annotation() = 0;
    virtual std::string argToString() const { return ""; }
    virtual std::string toString() const {
        return "@" + std::string(keyword) + argToString() +
                " at " + util::toString(locus);
    }

    template<class SubClass>
    Annotation::Ptr accept(Transformer<SubClass>*) const {
        return shared_from_this();
    }

    keyword_t getKeyword() const { return keyword; }
    const Locus& getLocus() const { return locus; }

    static bool classof(const Annotation*) {
        return true;
    }

    static const Annotation::Ptr fromIntrinsic(const llvm::CallInst& CI) {
        return static_cast<Annotation*>(MDNode2Ptr(CI.getMetadata("anno.ptr")))
               ->shared_from_this();
    }

    static const Annotation::Ptr fromIntrinsic(llvm::CallSite CI) {
        return static_cast<Annotation*>(MDNode2Ptr(CI.getInstruction()->getMetadata("anno.ptr")))
               ->shared_from_this();
    }

    static void installOnIntrinsic(llvm::CallInst& CI, Annotation::Ptr anno) {
        CI.setMetadata("anno.ptr", ptr2MDNode(CI.getContext(), anno.get()));
        addTracking(&CI, anno);
    }

    friend std::ostream& operator<<(std::ostream& str, Annotation::Ptr a) {
        str << a->toString();
        return str;
    }

    friend std::ostream& operator<<(std::ostream& str, const Annotation& a) {
        str << a.toString();
        return str;
    }
};



} /* namespace borealis */

#endif /* ANNOTATION_H_ */
