/*
 * Converter.cpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include "Protobuf/Converter.hpp"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t) {
    return Converter<Type, proto::Type>::toProtobuf(t);
}

Type::Ptr deprotobuffy(FactoryNest FN, const proto::Type& t) {
    return Converter<Type, proto::Type>::fromProtobuf(FN, t);
}

} // namespace borealis
