/*
 * Converter.cpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "Protobuf/Converter.hpp"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t) {
    return Converter<Type::Ptr, Type::ProtoPtr>::toProtobuf(t);
}

} // namespace borealis
