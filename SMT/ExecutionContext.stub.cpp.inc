#ifndef NAMESPACE
#define NAMESPACE stub_
#endif

#ifndef BACKEND
#define BACKEND NoSMT
#endif

namespace borealis {

static config::BoolConfigEntry CraigColtonMode("analysis", "craig-colton-bounds");

namespace NAMESPACE {

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

    initGepBounds(0);
};

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::MemArrayWithVersion ExecutionContext::memory(size_t memspace) const {
    auto id = tfm::format("%s%d", MEMORY_ID, memspace);
    return get(id);
}

void ExecutionContext::memory(size_t memspace, const MemArrayWithVersion& value) {
    auto id = tfm::format("%s%d", MEMORY_ID, memspace);
    set(id, value);
}

////////////////////////////////////////////////////////////////////////////////

void ExecutionContext::initGepBounds(size_t memspace) {
    auto id = tfm::format("%s%d", GEP_BOUNDS_ID, memspace);
    if (CraigColtonMode.get(false)) {
        memArrays.emplace(
            id,
            factory.getDefaultMemoryArray(id, 0)
        );
    } else {
        memArrays.emplace(
            id,
            factory.getEmptyMemoryArray(id)
        );
    }

    gepBounds(memspace, gepBounds(memspace).store(
        factory.getNullPtr(),
        factory.getIntConst(-1)
    ) );
}

ExecutionContext::MemArrayWithVersion ExecutionContext::gepBounds(size_t memspace) {
    auto id = tfm::format("%s%d", GEP_BOUNDS_ID, memspace);
    if(not util::containsKey(memArrays, id)) {
        initGepBounds(memspace);
    }
    return get(id);
}

void ExecutionContext::gepBounds(size_t memspace, const MemArrayWithVersion& value) {
    auto id = tfm::format("%s%d", GEP_BOUNDS_ID, memspace);
    if(not util::containsKey(memArrays, id)) {
        initGepBounds(memspace);
    }
    set(id, value);
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::MemArrayWithVersion ExecutionContext::get(const std::string& id) const {
    if (not util::containsKey(memArrays, id)) {
        MemArrayWithVersion mem = factory.getNoMemoryArray(id);
        memArrays.emplace(id, mem);
        initialMemArrays.emplace(id, mem);
    }
    return memArrays.at(id);
}

void ExecutionContext::set(const std::string& id, const MemArrayWithVersion& value) {
    if (util::containsKey(memArrays, id)) {
        memArrays.erase(id);
    }
    memArrays.emplace(id, value);
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::MemArrayIds ExecutionContext::getMemArrayIds() const {
    return util::viewContainerKeys(memArrays).toHashSet();
}

ExecutionContext::MemArray ExecutionContext::getCurrentMemoryContents(size_t memspace) {
    return memory(memspace).getMem();
}

ExecutionContext::MemArray ExecutionContext::getInitialMemoryContents(size_t memspace) {
    auto id = tfm::format("%s%d", MEMORY_ID, memspace);
    for(auto&& k : util::at(initialMemArrays, id)) {
        return k.getMem();
    }
    return get(id).getMem();
}

ExecutionContext::MemArray ExecutionContext::getCurrentGepBounds(size_t memspace) {
    return gepBounds(memspace).getMem();
}

ExecutionContext::MemArray ExecutionContext::getCurrentProperties(const std::string& pname) {
    return get(pname).getMem();
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::Pointer ExecutionContext::getGlobalPtr(size_t memspace, size_t offsetSize) {
    return getGlobalPtr(memspace, offsetSize, factory.getIntConst(offsetSize));
}

ExecutionContext::Pointer ExecutionContext::getGlobalPtr(size_t memspace, size_t offsetSize, Integer origSize) {
    auto&& ret = factory.getPtrConst(globalPtr);
    globalPtr += offsetSize;
    writeBound(ret, origSize, memspace);
    return ret;
}

ExecutionContext::Pointer ExecutionContext::getLocalPtr(size_t memspace,size_t offsetSize) {
    return getLocalPtr(memspace, offsetSize, factory.getIntConst(offsetSize));
}

ExecutionContext::Pointer ExecutionContext::getLocalPtr(size_t memspace,size_t offsetSize, Integer origSize) {
    auto&& ret = factory.getPtrConst(localPtr);
    localPtr += offsetSize;
    writeBound(ret, origSize, memspace);
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

        res.set(id, MemArrayWithVersion::merge(name, defaultContext.get(id), alternatives));
    }

    // Merge context axioms

    impl_::smtExprSet mergedAxioms;
    if(contexts.empty()) {
        res.contextAxioms = impl_::smtExprSet{};
    } else {
        auto h = util::head(contexts).second.contextAxioms;
        auto&& t = util::tail(contexts);
        for(auto&& ctx : t) {
            auto&& e = ctx.second.contextAxioms;
            auto&& newh = impl_::smtExprSet::join(std::move(h), e);
            h = std::move(newh);
        }
        res.contextAxioms = std::move(h);
        // res.contextAxioms.finalize();
    }

    return res;
};

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::Integer ExecutionContext::getBound(const Pointer& p, size_t bitSize, size_t memspace) {

    auto gepKey = tfm::format("%s%d", GEP_BOUNDS_ID, memspace);

    if (not CraigColtonMode.get(false)) return gepBounds(memspace).select(p, bitSize);

    auto&& zero = factory.getIntConst(0, bitSize);

    auto storedBound = [&](auto ptr) -> Integer { return this->readProperty(gepKey, ptr, bitSize); };

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

void ExecutionContext::writeBound(const Pointer& p, const Integer& bound, size_t memspace) {
    if (CraigColtonMode.get(false)) {
        gepBounds( memspace, gepBounds(memspace).store(p, Byte::forceCast(bound)) );
    } else {
        contextAxioms.insert((getBound(p, bound.getBitSize(), memspace) == bound).asAxiom());
    }
}

////////////////////////////////////////////////////////////////////////////////

ExecutionContext::Bool ExecutionContext::toSMT() const {
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
