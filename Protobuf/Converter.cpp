/*
 * Converter.cpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "Factory/Nest.h"

#include "Protobuf/Converter.hpp"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t) {
    return Converter<Type, proto::Type, FactoryNest>::toProtobuf(t);
}

Type::Ptr deprotobuffy(FactoryNest fn, const proto::Type& t) {
    return Converter<Type, proto::Type, FactoryNest>::fromProtobuf(fn, t);
}

} // namespace borealis
