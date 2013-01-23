/*
 * Annotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATION_H_
#define ANNOTATION_H_

#include <memory>
#include <string>

#include "Util/util.h"
#include "Util/locations.h"
#include "Util/typeindex.hpp"

#include "Term/TermFactory.h"

namespace borealis {

class Annotation {
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
        locus(locus){}

    id_t getTypeId() const { return annotation_type_id; }

    virtual ~Annotation() = 0;
    virtual std::string argToString() const { return ""; }
    virtual std::string toString() const {
        return "@" + std::string(keyword) + argToString() +
                " at " + util::toString(locus);
    }

    keyword_t getKeyword() { return keyword; }

    static bool classof(const Annotation*) {
        return true;
    }
};

template<class Streamer>
Streamer& operator<<(Streamer& str, const Annotation& a) {
    return str << a.toString();
}

} /* namespace borealis */
#endif /* ANNOTATION_H_ */
