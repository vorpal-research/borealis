#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <memory>

#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

class Type {
    id_t theId;
public:
    Type(id_t id): theId(id){};
    id_t getId() const { return theId; }	

    typedef std::shared_ptr<const Type> Ptr;
};

}

#endif // TYPE_H
