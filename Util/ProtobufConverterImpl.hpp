/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef UTIL_PROTOBUF_CONVERTER_IMPL_HPP_
#define UTIL_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Util/locations.pb.h"

#include "Factory/Nest.h"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////
// Locations
////////////////////////////////////////////////////////////////////////////////

template<>
struct protobuf_traits<LocalLocus> {
    typedef LocalLocus normal_t;
    typedef std::unique_ptr<normal_t> normal_ptr;
    typedef proto::LocalLocus proto_t;
    typedef std::unique_ptr<proto_t> proto_ptr;
    typedef void* context_t;

    static std::unique_ptr<proto::LocalLocus> toProtobuf(const LocalLocus& t) {
        auto res = util::uniq(new proto::LocalLocus());
        if(!t.isUnknown()) {
            res->set_col(static_cast<google::protobuf::uint32>(t.col));
            res->set_line(static_cast<google::protobuf::uint32>(t.line));
        }
        return std::move(res);
    }

    static std::unique_ptr<LocalLocus> fromProtobuf(const context_t&, const proto::LocalLocus& t) {
        auto res = util::uniq(new LocalLocus());
        if(t.has_col()) res->col = static_cast<decltype(res->col)>(t.col());
        if(t.has_line()) res->line = static_cast<decltype(res->col)>(t.line());
        return std::move(res);
    }
};

template<>
struct protobuf_traits<Locus> {
    typedef Locus normal_t;
    typedef std::unique_ptr<normal_t> normal_ptr;
    typedef proto::Locus proto_t;
    typedef std::unique_ptr<proto_t> proto_ptr;
    typedef void* context_t;

    static protobuf_traits<LocalLocus> locals;

    static std::unique_ptr<proto::Locus> toProtobuf(const Locus& t) {
        auto res = util::uniq(new proto::Locus());
        if(!t.isUnknown()) {
            res->set_filename(t.filename);
            res->set_allocated_loc(locals.toProtobuf(t.loc).release());
        }
        return std::move(res);
    }

    static std::unique_ptr<Locus> fromProtobuf(const context_t& fn, const proto::Locus& t) {
        auto res = util::uniq(new Locus());
        if(t.has_filename()) res->filename = t.filename();
        if(t.has_loc()) res->loc = *locals.fromProtobuf(fn, t.loc());
        return std::move(res);
    }
};

template<>
struct protobuf_traits<LocusRange> {

    typedef LocusRange normal_t;
    typedef std::unique_ptr<normal_t> normal_ptr;
    typedef proto::LocusRange proto_t;
    typedef std::unique_ptr<proto_t> proto_ptr;
    typedef void* context_t;

    static protobuf_traits<Locus> locals;

    static std::unique_ptr<proto::LocusRange> toProtobuf(const LocusRange& t) {
        auto res = util::uniq(new proto::LocusRange());
        res->set_allocated_lhv(locals.toProtobuf(t.lhv).release());
        res->set_allocated_rhv(locals.toProtobuf(t.rhv).release());
        return std::move(res);
    }

    static std::unique_ptr<LocusRange> fromProtobuf(const context_t& fn, const proto::LocusRange& t) {
        auto res = util::uniq(new LocusRange());
        res->lhv = *locals.fromProtobuf(fn, t.lhv());
        res->rhv = *locals.fromProtobuf(fn, t.rhv());
        return std::move(res);
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* UTIL_PROTOBUF_CONVERTER_IMPL_HPP_ */
