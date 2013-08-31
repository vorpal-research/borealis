/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef STATE_PROTOBUF_CONVERTER_IMPL_HPP_
#define STATE_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/State/BasicPredicateState.pb.h"
#include "Protobuf/Gen/State/PredicateStateChain.pb.h"
#include "Protobuf/Gen/State/PredicateStateChoice.pb.h"

#include "Predicate/ProtobufConverterImpl.hpp"

#include "Factory/Nest.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<PredicateState> {
    typedef PredicateState normal_t;
    typedef proto::PredicateState proto_t;
    typedef borealis::FactoryNest context_t;

    static PredicateState::ProtoPtr toProtobuf(const normal_t& ps);
    static PredicateState::Ptr fromProtobuf(const context_t& fn, const proto_t& ps);
};

template<>
struct protobuf_traits_impl<BasicPredicateState> {

    typedef protobuf_traits<Predicate> PredicateConverter;

    static std::unique_ptr<proto::BasicPredicateState> toProtobuf(const BasicPredicateState& ps) {
        auto res = util::uniq(new proto::BasicPredicateState());
        for (const auto& p : ps.getData()) {
            res->mutable_data()->AddAllocated(
                PredicateConverter::toProtobuf(*p).release()
            );
        }
        return std::move(res);
    }

    static PredicateState::Ptr fromProtobuf(
            const FactoryNest& fn,
            const proto::BasicPredicateState& ps) {
        auto res = util::uniq(new BasicPredicateState());
        for (const auto& p : ps.data()) {
            res->addPredicateInPlace(
                PredicateConverter::fromProtobuf(fn, p)
            );
        }
        return PredicateState::Ptr{ std::move(res) };
    }
};

template<>
struct protobuf_traits_impl<PredicateStateChain> {

    typedef protobuf_traits<PredicateState> PredicateStateConverter;

    static std::unique_ptr<proto::PredicateStateChain> toProtobuf(const PredicateStateChain& ps) {
        auto res = util::uniq(new proto::PredicateStateChain());
        res->set_allocated_base(
            PredicateStateConverter::toProtobuf(*ps.getBase()).release()
        );
        res->set_allocated_curr(
            PredicateStateConverter::toProtobuf(*ps.getCurr()).release()
        );
        return std::move(res);
    }

    static PredicateState::Ptr fromProtobuf(
            const FactoryNest& fn,
            const proto::PredicateStateChain& ps) {
        auto base = PredicateStateConverter::fromProtobuf(fn, ps.base());
        auto curr = PredicateStateConverter::fromProtobuf(fn, ps.curr());
        return PredicateState::Ptr{ new PredicateStateChain(base, curr) };
    }
};

template<>
struct protobuf_traits_impl<PredicateStateChoice> {

    typedef protobuf_traits<PredicateState> PredicateStateConverter;

    static std::unique_ptr<proto::PredicateStateChoice> toProtobuf(const PredicateStateChoice& ps) {
        auto res = util::uniq(new proto::PredicateStateChoice());
        for (const auto& c : ps.getChoices()) {
            res->mutable_choices()->AddAllocated(
                PredicateStateConverter::toProtobuf(*c).release()
            );
        }
        return std::move(res);
    }

    static PredicateState::Ptr fromProtobuf(
            const FactoryNest& fn,
            const proto::PredicateStateChoice& ps) {
        std::vector<PredicateState::Ptr> choices;
        choices.reserve(ps.choices_size());
        for (const auto& c : ps.choices()) {
            choices.push_back(
                PredicateStateConverter::fromProtobuf(fn, c)
            );
        }
        return PredicateState::Ptr{ new PredicateStateChoice(choices) };
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* STATE_PROTOBUF_CONVERTER_IMPL_HPP_ */
