/*
 * Annotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATION_H_
#define ANNOTATION_H_

#include <llvm/Instructions.h>

#include <memory>
#include <string>

#include "Codegen/llvm.h"
#include "Term/TermFactory.h"
#include "Util/locations.h"
#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

class Annotation : public std::enable_shared_from_this<Annotation> {
private:
    typedef Annotation self;

protected:
    typedef const char* const keyword_t;
    borealis::id_t annotation_type_id;
    keyword_t keyword;
    Locus locus;

public:
    typedef std::shared_ptr<self> Ptr;

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

    const Locus& getLocus() const { return locus; }
    keyword_t getKeyword() const { return keyword; }

    static bool classof(const Annotation*) {
        return true;
    }

    static const Annotation::Ptr fromIntrinsic(const llvm::CallInst& CI) {
        return static_cast<Annotation*>(MDNode2Ptr(CI.getMetadata("anno.ptr")))
                ->shared_from_this();
    }
};

template<class Streamer>
Streamer& operator<<(Streamer& str, const Annotation& a) {
    // this is generally fucked up
    return static_cast<Streamer&>(str << a.toString());
}

} /* namespace borealis */

#endif /* ANNOTATION_H_ */
