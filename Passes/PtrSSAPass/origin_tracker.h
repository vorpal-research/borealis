/*
 * origin_tracker.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef ORIGIN_TRACKER_H_
#define ORIGIN_TRACKER_H_

#include <llvm/Value.h>
#include <unordered_map>

namespace borealis {

class origin_tracker {
public:
    typedef llvm::Value* value;
    typedef std::unordered_map<value, value> value_resolver;

    value getOrigin(value other) const;
    inline value getOriginOrSelf(value other) const {
        auto orig = getOrigin(other);
        return orig ? orig : other;
    }

    inline const value_resolver& getResolver() const { return origins; }

    void mergeOriginInfoFrom(const origin_tracker& other) ;

protected:
    void setOrigin(value k, value v);

    value_resolver origins;
};

} // namespace borealis

#endif /* ORIGIN_TRACKER_H_ */
