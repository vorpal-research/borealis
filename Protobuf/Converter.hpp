/*
 * Converter.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef PROTOBUF_CONVERTER_HPP_
#define PROTOBUF_CONVERTER_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Type/ProtobufConverterImpl.hpp"
#include "Term/ProtobufConverterImpl.hpp"
#include "Predicate/ProtobufConverterImpl.hpp"
#include "Annotation/ProtobufConverterImpl.hpp"
#include "State/ProtobufConverterImpl.hpp"
#include "Util/ProtobufConverterImpl.hpp"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

Type::ProtoPtr protobuffy(Type::Ptr t);
Type::Ptr    deprotobuffy(FactoryNest FN, const proto::Type& t);

Term::ProtoPtr protobuffy(Term::Ptr t);
Term::Ptr    deprotobuffy(FactoryNest FN, const proto::Term& t);

Predicate::ProtoPtr protobuffy(Predicate::Ptr p);
Predicate::Ptr    deprotobuffy(FactoryNest FN, const proto::Predicate& p);

Annotation::ProtoPtr protobuffy(Annotation::Ptr p);
Annotation::Ptr    deprotobuffy(FactoryNest FN, const proto::Annotation& p);

PredicateState::ProtoPtr protobuffy(PredicateState::Ptr p);
PredicateState::Ptr    deprotobuffy(FactoryNest FN, const proto::PredicateState& p);

std::unique_ptr<proto::LocalLocus> protobuffy(const LocalLocus& p);
std::unique_ptr<LocalLocus> deprotobuffy(const proto::LocalLocus& p);

std::unique_ptr<proto::Locus> protobuffy(const Locus& p);
std::unique_ptr<Locus> deprotobuffy(const proto::Locus& p);

std::unique_ptr<proto::LocusRange> protobuffy(const LocusRange& p);
std::unique_ptr<LocusRange> deprotobuffy(const proto::LocusRange& p);

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PROTOBUF_CONVERTER_HPP_ */
