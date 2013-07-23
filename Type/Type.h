#ifndef TYPE_H
#define TYPE_H

#include <memory>
#include <string>

#include "Util/typeindex.hpp"

namespace borealis {

/** protobuf -> Type/Type.proto
package borealis.proto;

message Type {
    extensions 1 to 15;
}

**/
class Type : public ClassTag {
public:
    Type(id_t id) : ClassTag(id) {};

    typedef std::shared_ptr<const Type> Ptr;

};

} // namespace borealis

namespace std {
template<>
struct hash<borealis::Type::Ptr> {
    size_t operator()(const borealis::Type::Ptr& t) const {
        return reinterpret_cast<size_t>(t.get());
    }
};
template<>
struct hash<const borealis::Type::Ptr> {
    size_t operator()(const borealis::Type::Ptr& t) const {
        return reinterpret_cast<size_t>(t.get());
    }
};
} // namespace std

#endif // TYPE_H
