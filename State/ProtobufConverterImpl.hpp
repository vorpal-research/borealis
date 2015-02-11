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
    using normal_t = PredicateState;
    using proto_t = proto::PredicateState;
    using context_t = FactoryNest;

    static PredicateState::ProtoPtr toProtobuf(const normal_t& ps);
    static PredicateState::Ptr fromProtobuf(const context_t& fn, const proto_t& ps);
};

template<>
struct protobuf_traits_impl<BasicPredicateState> {

    using PredicateConverter = protobuf_traits<Predicate>;

    static std::unique_ptr<proto::BasicPredicateState> toProtobuf(const BasicPredicateState& ps) {
        auto&& res = std::make_unique<proto::BasicPredicateState>();
        for (auto&& p : ps.getData()) {
            res->mutable_data()->AddAllocated(
                PredicateConverter::toProtobuf(*p).release()
            );
        }
        return std::move(res);
    }

    static PredicateState::Ptr fromProtobuf(
            const FactoryNest& fn,
            const proto::BasicPredicateState& ps) {
        auto&& res = BasicPredicateState::Uniquified();
        for (auto&& p : ps.data()) {
            res->addPredicateInPlace(
                PredicateConverter::fromProtobuf(fn, p)
            );
        }
        return PredicateState::Ptr{ std::move(res) };
    }
};

template<>
struct protobuf_traits_impl<PredicateStateChain> {

    using PredicateStateConverter = protobuf_traits<PredicateState>;

    static std::unique_ptr<proto::PredicateStateChain> toProtobuf(const PredicateStateChain& ps) {
        auto&& res = std::make_unique<proto::PredicateStateChain>();
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
        auto&& base = PredicateStateConverter::fromProtobuf(fn, ps.base());
        auto&& curr = PredicateStateConverter::fromProtobuf(fn, ps.curr());
        return PredicateState::Ptr{ PredicateStateChain::Uniquified(base, curr) };
    }
};

template<>
struct protobuf_traits_impl<PredicateStateChoice> {

    using PredicateStateConverter = protobuf_traits<PredicateState>;

    static std::unique_ptr<proto::PredicateStateChoice> toProtobuf(const PredicateStateChoice& ps) {
        auto&& res = std::make_unique<proto::PredicateStateChoice>();
        for (auto&& c : ps.getChoices()) {
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
        for (auto&& c : ps.choices()) {
            choices.push_back(
                PredicateStateConverter::fromProtobuf(fn, c)
            );
        }
        return PredicateState::Ptr{ PredicateStateChoice::Uniquified(choices) };
    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* STATE_PROTOBUF_CONVERTER_IMPL_HPP_ */
