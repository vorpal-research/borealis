#ifndef SERIAL_TEMPLATE_SPEC_HPP
#define SERIAL_TEMPLATE_SPEC_HPP

#include <Serializer.hpp>

#include <string>

#include "Protobuf/Converter.hpp"

namespace borealis {
namespace db {

using Buff = leveldb_mp::serializer::Buffer;

template <class T>
class ProtobufSerializer{
public:
    static Buff serialize(const T& obj){
        auto proto = protobuffy(obj.shared_from_this());
        std::shared_ptr<char> sp(new char[proto->ByteSize()], std::default_delete<char[]>());
        proto->SerializeToArray(sp.get(), proto->ByteSize());
        return {sp, (size_t) proto->ByteSize()};
    }
};

template <class T, class ProtoT, class Context>
class ProtobufDeserializer{
public:
    static T deserialize(const Buff& buf, Context& FN){
        std::unique_ptr<ProtoT> proto{new ProtoT{} };
        proto->ParseFromArray(buf.array.get(),buf.size);
        return deprotobuffy(FN, *proto);
    }

    static T notFound(){
        return nullptr;
    }
};

} /* namespace db */
} /* namespace borealis */

namespace leveldb_mp {
namespace serializer {

using borealis::db::ProtobufSerializer;
using borealis::db::ProtobufDeserializer;
using borealis::db::Buff;

/*Predicate State*/
template<>
struct leveldb_mp::serializer::serializer<borealis::PredicateState>: public ProtobufSerializer<borealis::PredicateState>{};

template<>
struct leveldb_mp::serializer::deserializer<borealis::PredicateState, Buff, borealis::FactoryNest>:
    public ProtobufDeserializer<borealis::PredicateState::Ptr, borealis::proto::PredicateState, borealis::FactoryNest>{};

/*Predicate*/
template<>
struct leveldb_mp::serializer::serializer<borealis::Predicate>: public ProtobufSerializer<borealis::Predicate>{};

template<>
struct leveldb_mp::serializer::deserializer<borealis::Predicate, Buff, borealis::FactoryNest>:
    public ProtobufDeserializer<borealis::Predicate::Ptr, borealis::proto::Predicate, borealis::FactoryNest>{};

/*Term*/
template<>
struct leveldb_mp::serializer::serializer<borealis::Term>: public ProtobufSerializer<borealis::Term>{};

template<>
struct leveldb_mp::serializer::deserializer<borealis::Term, Buff, borealis::FactoryNest>:
    public ProtobufDeserializer<borealis::Term::Ptr, borealis::proto::Term, borealis::FactoryNest>{};

/*Type*/
template<>
struct leveldb_mp::serializer::serializer<borealis::Type>: public ProtobufSerializer<borealis::Type>{};

template<>
struct leveldb_mp::serializer::deserializer<borealis::Type, Buff, borealis::FactoryNest>:
    public ProtobufDeserializer<borealis::Type::Ptr, borealis::proto::Type, borealis::FactoryNest>{};


/*Annotation*/
template<>
struct leveldb_mp::serializer::serializer<borealis::Annotation>: public ProtobufSerializer<borealis::Annotation>{};

template<>
struct leveldb_mp::serializer::deserializer<borealis::Annotation, Buff, borealis::FactoryNest>:
    public ProtobufDeserializer<borealis::Annotation::Ptr, borealis::proto::Annotation, borealis::FactoryNest>{};

/*AnnotationContainer*/
template<>
struct leveldb_mp::serializer::serializer<borealis::AnnotationContainer>: public ProtobufSerializer<borealis::AnnotationContainer>{};

template<>
struct leveldb_mp::serializer::deserializer<borealis::AnnotationContainer, Buff, borealis::FactoryNest>:
    public ProtobufDeserializer<borealis::AnnotationContainer::Ptr, borealis::proto::AnnotationContainer, borealis::FactoryNest>{};

} /* namespace serializer */
} /* namespace leveldb_mp */

#endif /* SERIAL_TEMPLATE_SPEC_HPP */
