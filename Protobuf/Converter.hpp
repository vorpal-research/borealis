/*
 * Converter.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef PROTOBUF_CONVERTER_HPP_
#define PROTOBUF_CONVERTER_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Annotation/ProtobufConverterImpl.hpp"
#include "Predicate/ProtobufConverterImpl.hpp"
#include "State/ProtobufConverterImpl.hpp"
#include "Term/ProtobufConverterImpl.hpp"
#include "Type/ProtobufConverterImpl.hpp"
#include "Util/ProtobufConverterImpl.hpp"

#include "Util/util.h"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t);
Type::Ptr    deprotobuffy(FactoryNest FN, const proto::Type& t);

Term::ProtoPtr protobuffy(Term::Ptr t);
Term::Ptr    deprotobuffy(FactoryNest FN, const proto::Term& t);

Predicate::ProtoPtr protobuffy(Predicate::Ptr p);
Predicate::Ptr    deprotobuffy(FactoryNest FN, const proto::Predicate& p);

Annotation::ProtoPtr protobuffy(Annotation::Ptr p);
Annotation::Ptr    deprotobuffy(FactoryNest FN, const proto::Annotation& p);

AnnotationContainer::ProtoPtr protobuffy(AnnotationContainer::Ptr p);
AnnotationContainer::Ptr    deprotobuffy(FactoryNest FN, const proto::AnnotationContainer& p);

PredicateState::ProtoPtr protobuffy(PredicateState::Ptr p);
PredicateState::Ptr    deprotobuffy(FactoryNest FN, const proto::PredicateState& p);

std::unique_ptr<proto::LocalLocus> protobuffy(const LocalLocus& p);
std::unique_ptr<LocalLocus>      deprotobuffy(const proto::LocalLocus& p);

std::unique_ptr<proto::Locus> protobuffy(const Locus& p);
std::unique_ptr<Locus>       deprotobuffy(const proto::Locus& p);

std::unique_ptr<proto::LocusRange> protobuffy(const LocusRange& p);
std::unique_ptr<LocusRange>      deprotobuffy(const proto::LocusRange& p);

namespace util {

template<class T, class Ctx>
auto read_as_protobuf(std::istream& str, Ctx&& ctx) -> decltype(auto) {
    typename protobuf_traits<T>::proto_t proto;
    proto.ParseFromIstream(&str);
    return protobuf_traits<T>::fromProtobuf(ctx, proto);
}

template<class T>
void write_as_protobuf(std::ostream& str, const T& rep) {
    auto&& proto = protobuf_traits<T>::toProtobuf(rep);
    proto->SerializeToOstream(&str);
};

} /* namespace util */


} // namespace borealis

#endif /* PROTOBUF_CONVERTER_HPP_ */
