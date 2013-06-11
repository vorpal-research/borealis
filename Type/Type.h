#ifndef TYPE_H
#define TYPE_H

#include <memory>
#include <string>

#include "Util/typeindex.hpp"

namespace borealis {

class Type {
    id_t theId;
public:
    Type(id_t id) : theId(id) {};
    id_t getId() const { return theId; }	

    typedef std::shared_ptr<const Type> Ptr;
};

} // namespace borealis

#endif // TYPE_H
