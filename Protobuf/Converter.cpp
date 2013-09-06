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
    return protobuf_traits<Type>::toProtobuf(*t);
}

Type::Ptr deprotobuffy(FactoryNest fn, const proto::Type& t) {
    return protobuf_traits<Type>::fromProtobuf(fn, t);
}

Term::ProtoPtr protobuffy(Term::Ptr t) {
    return protobuf_traits<Term>::toProtobuf(*t);
}

Term::Ptr deprotobuffy(FactoryNest fn, const proto::Term& t) {
    return protobuf_traits<Term>::fromProtobuf(fn, t);
}

Predicate::ProtoPtr protobuffy(Predicate::Ptr p) {
    return protobuf_traits<Predicate>::toProtobuf(*p);
}

Predicate::Ptr deprotobuffy(FactoryNest fn, const proto::Predicate& p) {
    return protobuf_traits<Predicate>::fromProtobuf(fn, p);
}

Annotation::ProtoPtr protobuffy(Annotation::Ptr p) {
    return protobuf_traits<Annotation>::toProtobuf(*p);
}
Annotation::Ptr deprotobuffy(FactoryNest fn, const proto::Annotation& p) {
    return protobuf_traits<Annotation>::fromProtobuf(fn, p);
}

AnnotationContainer::ProtoPtr protobuffy(AnnotationContainer::Ptr p) {
    return protobuf_traits<AnnotationContainer>::toProtobuf(*p);
}
AnnotationContainer::Ptr deprotobuffy(FactoryNest fn, const proto::AnnotationContainer& p) {
    return protobuf_traits<AnnotationContainer>::fromProtobuf(fn, p);
}


PredicateState::ProtoPtr protobuffy(PredicateState::Ptr ps) {
    return protobuf_traits<PredicateState>::toProtobuf(*ps);
}

PredicateState::Ptr deprotobuffy(FactoryNest fn, const proto::PredicateState& ps) {
    return protobuf_traits<PredicateState>::fromProtobuf(fn, ps);
}

std::unique_ptr<proto::LocalLocus> protobuffy(const LocalLocus& p) {
    return protobuf_traits<LocalLocus>::toProtobuf(p);
}

std::unique_ptr<LocalLocus> deprotobuffy(const proto::LocalLocus& p) {
    return protobuf_traits<LocalLocus>::fromProtobuf(nullptr, p);
}

std::unique_ptr<proto::Locus> protobuffy(const Locus& p) {
    return protobuf_traits<Locus>::toProtobuf(p);
}

std::unique_ptr<Locus> deprotobuffy(const proto::Locus& p) {
    return protobuf_traits<Locus>::fromProtobuf(nullptr, p);
}

std::unique_ptr<proto::LocusRange> protobuffy(const LocusRange& p) {
    return protobuf_traits<LocusRange>::toProtobuf(p);
}

#define RAWSTR(...) R ## #__VA_ARGS__
#define PUN1(...) RAWSTR((__VA_ARGS__))
#define PUN(...) ((void*)(PUN1(__VA_ARGS__)))

std::unique_ptr<LocusRange> deprotobuffy(const proto::LocusRange& p) {
    constexpr auto cat = PUN(

                You know, this is even not funny at all.
                This type could be used to actually do something.
                And you waste it like this.
                You should be ashamed of yourself.

                ...

                No, seriously.

                ...

                Remove this crap.
                Let`s use this cat as a type instead.


                .               ,.
               T.`-._..---.._,-`/|
               l|`-.  _.v._   {` |
               [l /.`_ \  _~`-.`-t
               Y ` _(o} _{o)._ ^.|
               j  T  ,-<v>-.  T  ]
               \  l ( /-^-\ ) !  !
                \  \.  `~`  ./  /c-..,__
                  ^r- .._ .- .-`  `- .  ~`--.
                   > \.                      \
                   ]   ^.                     \
                   3  .  `>            .       Y
      ,.__.--._   _j   \ ~   .         ;       |
     (    ~`-._~`^._\   ^.    ^._      I     . l
      `-._ ___ ~`-,_7    .Z-._   7`   Y      ;  \        _
         /`   `~-(r r  _/_--._~-/    /      /,.--^-._   / Y
         `-._    ``~~~>-._~]>--^---./____,.^~        ^.^  !
             ~--._    `   Y---.                        \ /
                  ~~--._  l_   )                        \
                        ~-._~~~---._,____..---           )
                            ~----`~       \
                                           \


                ...

                This note is left here for historical reasons.
                Now we pass the cat as a parameter!

                                         No joking, right here
                                                     V
    );
    return protobuf_traits<LocusRange>::fromProtobuf(cat, p);
}

#undef PUN
#undef PUN1
#undef RAWSTR

} // namespace borealis
