/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef ANNOTATION_PROTOBUF_CONVERTER_IMPL_HPP_
#define ANNOTATION_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Annotation/Annotation.def"
#include "Annotation/AnnotationContainer.h"

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Annotation/AssertAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/AssignsAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/AssumeAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/EndMaskAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/EnsuresAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/IgnoreAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/InlineAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/MaskAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/SkipAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/StackDepthAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/RequiresAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/UnrollAnnotation.pb.h"
#include "Protobuf/Gen/Annotation/AnnotationContainer.pb.h"

#include "Term/ProtobufConverterImpl.hpp"

#include "Factory/Nest.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<Annotation> {
    typedef Annotation normal_t;
    typedef proto::Annotation proto_t;
    typedef borealis::FactoryNest context_t;

    typedef protobuf_traits<Locus> LocusConverter;

    static Annotation::ProtoPtr toProtobuf(const normal_t& t);
    static Annotation::Ptr fromProtobuf(const context_t& fn, const proto_t& t);
};

#define MAKE_EMPTY_ANNOTATION_PB_IMPL(ANNO) \
    template<> \
    struct protobuf_traits_impl<ANNO> { \
        typedef protobuf_traits<Annotation> AnnotationConverter; \
\
        static std::unique_ptr<proto::ANNO> toProtobuf(const ANNO&) { \
            return util::uniq(new proto::ANNO()); \
        } \
\
        static Annotation::Ptr fromProtobuf( \
                const FactoryNest&, \
                const Locus& locus, \
                const proto::ANNO&) { \
            return Annotation::Ptr{ new ANNO(locus) }; \
        } \
    };

MAKE_EMPTY_ANNOTATION_PB_IMPL(EndMaskAnnotation)
MAKE_EMPTY_ANNOTATION_PB_IMPL(IgnoreAnnotation)
MAKE_EMPTY_ANNOTATION_PB_IMPL(InlineAnnotation)
MAKE_EMPTY_ANNOTATION_PB_IMPL(SkipAnnotation)

#undef MAKE_EMPTY_ANNOTATION_PB_IMPL

template<>
struct protobuf_traits_impl<MaskAnnotation> {
    typedef protobuf_traits<Annotation> AnnotationConverter;
    typedef protobuf_traits<Term> TermConverter;

    static std::unique_ptr<proto::MaskAnnotation> toProtobuf(const MaskAnnotation& a) {
        auto res = util::uniq(new proto::MaskAnnotation());
        for(auto maskPtr : a.getMasks()) {
            res->mutable_masks()->AddAllocated(TermConverter::toProtobuf(*maskPtr).release());
        }
        return std::move(res);
    }

    static Annotation::Ptr fromProtobuf(
            const FactoryNest& fn,
            const Locus& locus,
            const proto::MaskAnnotation& t) {
        std::vector<Term::Ptr> ret;
        for (int i = 0; i < t.masks_size(); ++i) {
            ret.push_back(TermConverter::fromProtobuf(fn, t.masks(i)));
        }
        return Annotation::Ptr{ new MaskAnnotation(locus, ret) };
    }
};

template<>
struct protobuf_traits_impl<StackDepthAnnotation> {
    static std::unique_ptr<proto::StackDepthAnnotation> toProtobuf(const StackDepthAnnotation& a) {
        auto res = util::uniq(new proto::StackDepthAnnotation());
        res->set_depth(a.getDepth());
        return std::move(res);
    }

    static Annotation::Ptr fromProtobuf(
            const FactoryNest&,
            const Locus& locus,
            const proto::StackDepthAnnotation& t) {
        return Annotation::Ptr{ new StackDepthAnnotation(locus, t.depth()) };
    }
};

template<>
struct protobuf_traits_impl<UnrollAnnotation> {
    static std::unique_ptr<proto::UnrollAnnotation> toProtobuf(const UnrollAnnotation& a) {
        auto res = util::uniq(new proto::UnrollAnnotation());
        res->set_level(a.getLevel());
        return std::move(res);
    }

    static Annotation::Ptr fromProtobuf(
            const FactoryNest&,
            const Locus& locus,
            const proto::UnrollAnnotation& t) {
        return Annotation::Ptr{ new UnrollAnnotation(locus, t.level()) };
    }
};

// all logic annotations right now are marker classes with no contents
#define MAKE_EMPTY_LOGIC_ANNOTATION_PB_IMPL(ANNO) \
    template<> \
    struct protobuf_traits_impl<ANNO> { \
        static std::unique_ptr<proto::ANNO> toProtobuf(const ANNO&) { \
            return util::uniq(new proto::ANNO()); \
        } \
\
        static Annotation::Ptr fromProtobuf( \
                const FactoryNest&, \
                const Locus& locus, \
                const Term::Ptr& term, \
                const proto::ANNO&) { \
            return Annotation::Ptr{ new ANNO(locus, term) }; \
        } \
    };

MAKE_EMPTY_LOGIC_ANNOTATION_PB_IMPL(AssertAnnotation)
MAKE_EMPTY_LOGIC_ANNOTATION_PB_IMPL(AssignsAnnotation)
MAKE_EMPTY_LOGIC_ANNOTATION_PB_IMPL(AssumeAnnotation)
MAKE_EMPTY_LOGIC_ANNOTATION_PB_IMPL(EnsuresAnnotation)
MAKE_EMPTY_LOGIC_ANNOTATION_PB_IMPL(RequiresAnnotation)

#undef MAKE_EMPTY_LOGIC_ANNOTATION_PB_IMPL

template<>
struct protobuf_traits_impl<LogicAnnotation> {
    typedef protobuf_traits<Term> TermConverter;

    typedef LogicAnnotation normal_t;
    typedef proto::LogicAnnotation proto_t;
    typedef borealis::FactoryNest context_t;

    static std::unique_ptr<proto::LogicAnnotation> toProtobuf(const normal_t& a) {
        auto res = util::uniq(new proto_t());

        res->set_allocated_term(TermConverter::toProtobuf(*a.getTerm()).release());

        if (false) {}
// here we rely on the fact that only logic guys have bases
// if we put up other annotations, we'll need to explicitly ignore them here:
// #define HANDLE_SomeOtherMiddleBaseAnnotation(CLASS)
#define HANDLE_LogicAnnotation(CLASS) \
        else if (auto* tt = llvm::dyn_cast<CLASS>(&a)) { \
            auto proto = protobuf_traits_impl<CLASS> \
                         ::toProtobuf(*tt); \
            res->SetAllocatedExtension( \
                proto::CLASS::ext, \
                proto.release() \
            ); \
        }
#define HANDLE_ANNOTATION(KW, CLASS)
#define HANDLE_ANNOTATION_WITH_BASE(KW, BASE, CLASS) HANDLE_##BASE(CLASS)
#include "Annotation/Annotation.def"
#undef HANDLE_LogicAnnotation
        else BYE_BYE(std::unique_ptr<proto::LogicAnnotation>, "Should not happen!");

        return std::move(res);
    }

    static Annotation::Ptr fromProtobuf(
            const context_t& fn,
            const Locus& locus,
            const proto_t& t) {
        auto term = TermConverter::fromProtobuf(fn, t.term());
// here we rely on the fact that only logic guys have bases
// if we put up other annotations, we'll need to explicitly ignore them here:
// #define HANDLE_SomeOtherMiddleBaseAnnotation(CLASS)
#define HANDLE_LogicAnnotation(CLASS) \
        if (t.HasExtension(proto::CLASS::ext)) { \
            const auto& ext = t.GetExtension(proto::CLASS::ext); \
            return protobuf_traits_impl<CLASS> \
                   ::fromProtobuf(fn, locus, term, ext); \
        }
#define HANDLE_ANNOTATION(KW, CLASS)
#define HANDLE_ANNOTATION_WITH_BASE(KW, BASE, CLASS) HANDLE_##BASE(CLASS)
#include "Annotation/Annotation.def"
#undef HANDLE_LogicAnnotation
        BYE_BYE(Annotation::Ptr, "Should not happen!");
    }
};

template<>
struct protobuf_traits<AnnotationContainer> {
    typedef AnnotationContainer normal_t;
    typedef proto::AnnotationContainer proto_t;
    typedef borealis::FactoryNest context_t;

    typedef protobuf_traits<Annotation> AnnotationConverter;

    static AnnotationContainer::ProtoPtr toProtobuf(const normal_t& t) {
        auto res = util::uniq(new proto_t());
        for (const auto& a: t) {
            res->mutable_data()->AddAllocated(AnnotationConverter::toProtobuf(*a).release());
        }
        return std::move(res);
    }
    static AnnotationContainer::Ptr fromProtobuf(const context_t& fn, const proto_t& t) {
        AnnotationContainer::Ptr res { new AnnotationContainer() };
        for (auto i = 0; i < t.data_size(); ++i) {
            res->push_back(AnnotationConverter::fromProtobuf(fn, t.data(i)));
        }
        return std::move(res);
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* ANNOTATION_PROTOBUF_CONVERTER_IMPL_HPP_ */
