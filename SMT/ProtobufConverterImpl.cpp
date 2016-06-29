#include "SMT/ProtobufConverterImpl.hpp"

namespace borealis {

static proto::Term* convert(Term::Ptr t) {
    auto res = protobuf_traits<Term>::toProtobuf(*t);
    return res.release();
}

static Term::Ptr convert(const FactoryNest& FN, const proto::Term& t) {
    return protobuf_traits<Term>::fromProtobuf(FN, t);
}

std::unique_ptr<smt::proto::MemoryShape> protobuf_traits<smt::MemoryShape>::toProtobuf(const normal_t& t) {
    std::unique_ptr<smt::proto::MemoryShape> ret (new smt::proto::MemoryShape());

    auto&& start = t.getInitialMemoryShape();
    auto&& final = t.getFinalMemoryShape();

    for(auto&& kv : start) {
        auto&& tp = *ret->add_initialshape();
        tp.set_allocated_key(convert(kv.first));
        tp.set_allocated_value(convert(kv.second));
    }

    for(auto&& kv : final) {
        auto&& tp = *ret->add_finalshape();
        tp.set_allocated_key(convert(kv.first));
        tp.set_allocated_value(convert(kv.second));
    }

    return std::move(ret);
}

static smt::proto::MemoryShape* convert(const smt::MemoryShape& m) {
    return protobuf_traits<smt::MemoryShape>::toProtobuf(m).release();
}

static std::shared_ptr<smt::MemoryShape> convert(const FactoryNest& FN, const smt::proto::MemoryShape& m) {
    return protobuf_traits<smt::MemoryShape>::fromProtobuf(FN, m);
}

std::unique_ptr<smt::MemoryShape> protobuf_traits<smt::MemoryShape>::fromProtobuf(
        const FactoryNest& FN, const smt::proto::MemoryShape& m
    ) {
    std::unique_ptr<smt::MemoryShape> ret (new smt::MemoryShape());

    auto&& start = ret->getInitialMemoryShape();
    auto&& final = ret->getFinalMemoryShape();

    for(auto&& kv : m.initialshape()) {
        start.emplace(convert(FN, kv.key()), convert(FN, kv.value()));
    }

    for(auto&& kv : m.finalshape()) {
        final.emplace(convert(FN, kv.key()), convert(FN, kv.value()));
    }

    return ret;
}

std::unique_ptr<smt::proto::Model> protobuf_traits<smt::Model>::toProtobuf(const normal_t& t) {
    std::unique_ptr<smt::proto::Model> ret (new smt::proto::Model());

    for(auto&& kv : t.getAssignments()) {
        auto&& tp = *ret->add_assignments();
        tp.set_allocated_key(convert(kv.first));
        tp.set_allocated_value(convert(kv.second));
    }

    for(auto&& kv : t.getMemories()) {
        auto&& tp = *ret->add_memories();
        tp.set_key(kv.first);
        tp.set_allocated_value(convert(kv.second));
    }

    for(auto&& kv : t.getBounds()) {
        auto&& tp = *ret->add_bounds();
        tp.set_key(kv.first);
        tp.set_allocated_value(convert(kv.second));
    }

    for(auto&& kv : t.getProperties()) {
        auto&& tp = *ret->add_properties();
        tp.set_key(kv.first);
        tp.set_allocated_value(convert(kv.second));
    }

    return std::move(ret);
}

std::shared_ptr<smt::Model> protobuf_traits<smt::Model>::fromProtobuf(
    const FactoryNest& FN, const smt::proto::Model& m
) {
    auto ret = std::make_shared<smt::Model>(FN);

    for(auto&& kv : m.assignments()) {
        ret->getAssignments().emplace(convert(FN, kv.key()), convert(FN, kv.value()));
    }

    for(auto&& kv : m.memories()) {
        ret->getMemories().emplace(static_cast<size_t>(kv.key()), std::move(*convert(FN, kv.value())));
    }

    for(auto&& kv : m.bounds()) {
        ret->getBounds().emplace(static_cast<size_t>(kv.key()), std::move(*convert(FN, kv.value())));
    }

    for(auto&& kv : m.properties()) {
        ret->getProperties().emplace(kv.key(), std::move(*convert(FN, kv.value())));
    }

    return ret;
}

} /* namespace borealis */
