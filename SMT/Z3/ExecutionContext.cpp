/*
 * ExecutionContext.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#include "SMT/Z3/ExecutionContext.h"

#include <unordered_set>
#include "Util/union_set.hpp"

namespace borealis {

static config::BoolConfigEntry CraigColtonMode("analysis", "craig-colton-bounds");

namespace z3_ {

ExecutionContext::ExecutionContext(
    ExprFactory& factory,
    unsigned long long localMemoryStart,
    unsigned long long localMemoryEnd) :
    factory(factory),
    globalPtr(1ULL),
    localPtr(localMemoryStart),
    localMemoryStart(localMemoryStart),
    localMemoryEnd(localMemoryEnd) {

    initialMemArrays.emplace(MEMORY_ID, factory.getNoMemoryArray(MEMORY_ID));

    initGepBounds();
};

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::MemArray ExecutionContext::memory() const {
    return get(MEMORY_ID);
}

void ExecutionContext::memory(const MemArray& value) {
    set(MEMORY_ID, value);
}

////////////////////////////////////////////////////////////////////////////////

void ExecutionContext::initGepBounds() {
    if (CraigColtonMode.get(false)) {
        memArrays.emplace(
            GEP_BOUNDS_ID,
            factory.getDefaultMemoryArray(GEP_BOUNDS_ID, 0)
        );
    } else {
        memArrays.emplace(
            GEP_BOUNDS_ID,
            factory.getEmptyMemoryArray(GEP_BOUNDS_ID)
        );
    }

    gepBounds( gepBounds().store(
        factory.getNullPtr(),
        factory.getIntConst(-1)
    ) );
}

ExecutionContext::MemArray ExecutionContext::gepBounds() const {
    return get(GEP_BOUNDS_ID);
}

void ExecutionContext::gepBounds(const MemArray& value) {
    set(GEP_BOUNDS_ID, value);
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::MemArray ExecutionContext::get(const std::string& id) const {
    if (not util::containsKey(memArrays, id)) {
        auto mem = factory.getNoMemoryArray(id);
        memArrays.emplace(id, mem);
        initialMemArrays.emplace(id, mem);
    }
    return memArrays.at(id);
}

void ExecutionContext::set(const std::string& id, const MemArray& value) {
    if (util::containsKey(memArrays, id)) {
        memArrays.erase(id);
    }
    memArrays.emplace(id, value);
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::MemArrayIds ExecutionContext::getMemArrayIds() const {
    return util::viewContainerKeys(memArrays).toHashSet();
}

ExecutionContext::MemArray ExecutionContext::getCurrentMemoryContents() {
    return memory();
}

ExecutionContext::MemArray ExecutionContext::getInitialMemoryContents() {
    return initialMemArrays.at(MEMORY_ID);
}

ExecutionContext::MemArray ExecutionContext::getCurrentGepBounds() {
    return gepBounds();
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::Pointer ExecutionContext::getGlobalPtr(size_t offsetSize) {
    return getGlobalPtr(offsetSize, factory.getIntConst(offsetSize));
}

ExecutionContext::Pointer ExecutionContext::getGlobalPtr(size_t offsetSize, Integer origSize) {
    auto&& ret = factory.getPtrConst(globalPtr);
    globalPtr += offsetSize;
    writeBound(ret, origSize);
    return ret;
}

ExecutionContext::Pointer ExecutionContext::getLocalPtr(size_t offsetSize) {
    return getLocalPtr(offsetSize, factory.getIntConst(offsetSize));
}

ExecutionContext::Pointer ExecutionContext::getLocalPtr(size_t offsetSize, Integer origSize) {
    auto&& ret = factory.getPtrConst(localPtr);
    localPtr += offsetSize;
    writeBound(ret, origSize);
    return ret;
}

ExecutionContext::LocalMemoryBounds ExecutionContext::getLocalMemoryBounds() const {
    return { localMemoryStart, localMemoryEnd };
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext& ExecutionContext::switchOn(
    const std::string& name,
    const std::vector<Choice>& contexts) {
    auto&& merged = ExecutionContext::mergeMemory(name, *this, contexts);

    this->memArrays = std::move(merged.memArrays);
    this->globalPtr = merged.globalPtr;
    this->localPtr = merged.localPtr;

    this->contextAxioms = std::move(merged.contextAxioms);

    return *this;
}

ExecutionContext ExecutionContext::mergeMemory(
    const std::string& name,
    ExecutionContext defaultContext,
    const std::vector<Choice>& contexts) {
    ExecutionContext res{ defaultContext.factory, 0ULL, 0ULL };

    // Merge pointers
    for (auto&& e : contexts) {
        res.globalPtr = std::max(res.globalPtr, e.second.globalPtr);
        res.localPtr = std::max(res.localPtr, e.second.localPtr);
    }

    // Collect all active memory array ids
    auto&& memArrayIds = util::viewContainer(contexts)
        .fold(
            defaultContext.getMemArrayIds(),
            [](auto&& a, auto&& e) {
                auto ids = e.second.getMemArrayIds();
                a.insert(ids.begin(), ids.end());
                return a;
            }
        );

    // Merge memory arrays
    for (auto&& id : memArrayIds) {
        auto&& alternatives = util::viewContainer(contexts)
            .map([&](auto&& p) { return std::make_pair(p.first, p.second.get(id)); })
            .toVector();

        res.set(id, MemArray::merge(name, defaultContext.get(id), alternatives));
    }

    // Merge context axioms

    impl_::z3exprSet mergedAxioms;
    if(contexts.empty()) {
        res.contextAxioms = impl_::z3exprSet{};
    } else {
        auto h = util::head(contexts).second.contextAxioms;
        auto&& t = util::tail(contexts);
        for(auto&& ctx : t) {
            auto&& e = ctx.second.contextAxioms;
            auto&& newh = impl_::z3exprSet::join(std::move(h), e);
            h = std::move(newh);
        }
        res.contextAxioms = std::move(h);
        // res.contextAxioms.finalize();
    }

    return res;
};

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::Integer ExecutionContext::getBound(const Pointer& p, size_t bitSize) {

    if (not CraigColtonMode.get(false)) return gepBounds().select(p, bitSize);

    auto&& zero = factory.getIntConst(0, bitSize);

    auto storedBound = [&](auto ptr) -> Integer { return this->readProperty(GEP_BOUNDS_ID, ptr, bitSize); };

    auto&& pSize = storedBound(p);

    auto&& baseFree = factory.getPtrVar("$$__base_free__$$(" + p.getName() + ")");
    auto&& baseFreeSize = storedBound(baseFree);
    std::function<Bool(Pointer)> baseFreeAxiom =
        [=](Pointer any) -> Bool {
            return factory.implies(
                UComparable(any).ugt(baseFree) && UComparable(any).ule(p),
                storedBound(any) == zero
            );
        };
    auto&& bfa = UComparable(baseFree).ule(p)
                 && baseFreeSize != zero
                 && factory.forAll(baseFreeAxiom);
    baseFree = baseFree.withAxiom(bfa);

    auto&& base = factory.if_(pSize != zero)
                         .then_(p)
                         .else_(baseFree);
    auto&& baseSize = storedBound(base);

    return
        factory.if_(base == p)
               .then_(baseSize)
               .else_(
                    factory.if_(UComparable(baseSize).ugt(p - base))
                           .then_(baseSize - (p - base))
                           .else_(zero)
        );
}

void ExecutionContext::writeBound(const Pointer& p, const Integer& bound) {
    if (CraigColtonMode.get(false)) {
        gepBounds( gepBounds().store(p, Byte::forceCast(bound)) );
    } else {
        contextAxioms.insert((getBound(p, bound.getBitSize()) == bound).asAxiom());
    }
}

////////////////////////////////////////////////////////////////////////////////

Z3::Bool ExecutionContext::toSMT() const {
    return factory.getTrue();
}

std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx) {
    using std::endl;
    return s << "ctx state:" << endl
             << "< global offset = " << ctx.globalPtr << " >" << endl
             << "< local offset = " << ctx.localPtr << " >" << endl
             << ctx.memArrays;
}

const std::string ExecutionContext::MEMORY_ID = "$$__memory__$$";
const std::string ExecutionContext::GEP_BOUNDS_ID = "$$__gep_bound__$$";

} // namespace z3_
} // namespace borealis
