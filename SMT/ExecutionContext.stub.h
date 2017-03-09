#ifndef NAMESPACE
#define NAMESPACE stub_
#endif

#ifndef BACKEND
#define BACKEND NoSMT
#endif

namespace borealis {
namespace NAMESPACE {

using smt = typename BACKEND::Engine;

namespace impl_ {

using smtExprSet = util::split_join<const smt::expr_t>;

} /* namespace impl_ */

class ExecutionContext {

    USING_SMT_LOGIC(BACKEND);
    using ExprFactory = BACKEND::ExprFactory;

    class MemArrayWithVersion {
        MemArray mem;
        size_t version = 0;

    public:
        MemArrayWithVersion(MemArray mem, size_t version): mem(mem), version(version) {}
        MemArrayWithVersion(MemArray mem): mem(mem) {}

        const MemArray& getMem() const { return mem; }
        size_t getVersion() const { return version; }

        void incVersion() { ++version; }

        template<class Elem, class T>
        auto select(const T& key) const {
            return mem.select<Elem>(key);
        }

        template<class T>
        auto select(const T& key, size_t bs) const {
            return mem.select(key, bs);
        }

        template<class ...T>
        auto store(const T&... wha) {
            auto&& nmem = mem.store(wha...);
            return MemArrayWithVersion{ nmem, version + 1};
        }

        static auto merge(const std::string& name,
                          MemArrayWithVersion deflt,
                          const std::vector<std::pair<Bool, MemArrayWithVersion>>& alts) {
            auto ver = deflt.getVersion();
            auto maxVersion = ver;
            for(auto&& alt: alts) {
                maxVersion = std::max(maxVersion, alt.second.getVersion());
            }
            if(ver == maxVersion) return deflt;
            else{
                auto reMem = util::viewContainer(alts)
                            .map([&](auto&& alt){ return std::make_pair(alt.first, alt.second.getMem()); })
                            .toVector();
                return MemArrayWithVersion{
                    MemArray::merge(name, deflt.getMem(), reMem),
                    maxVersion + 1
                };
            }
        }

        Bool operator==(MemArrayWithVersion that) const {
            return mem == that.mem;
        }

        friend std::ostream& operator<<(std::ostream& ost, const MemArrayWithVersion& m) {
            return ost << m.getMem() << "{version:" << m.getVersion() << "}";
        }
    };

    ExprFactory& factory;
    mutable std::unordered_map<std::string, MemArrayWithVersion> memArrays;
    mutable std::unordered_map<std::string, MemArrayWithVersion> initialMemArrays;
    unsigned long long globalPtr;
    unsigned long long localPtr;

    unsigned long long localMemoryStart;
    unsigned long long localMemoryEnd;

    impl_::smtExprSet contextAxioms;

    static const std::string MEMORY_ID;
    MemArrayWithVersion memory(size_t memspace) const;
    void memory(size_t memspace, const MemArrayWithVersion& value);

    static const std::string GEP_BOUNDS_ID;
    void initGepBounds(size_t memspace);
    MemArrayWithVersion gepBounds(size_t memspace);
    void gepBounds(size_t memspace, const MemArrayWithVersion& value);

    MemArrayWithVersion get(const std::string& id) const;
    void set(const std::string& id, const MemArrayWithVersion& value);

    using MemArrayIds = std::unordered_set<std::string>;
    MemArrayIds getMemArrayIds() const;

public:

    ExecutionContext(ExprFactory& factory, unsigned long long localMemoryStart, unsigned long long localMemoryEnd);
    ExecutionContext(const ExecutionContext&) = default;

    MemArray getCurrentMemoryContents(size_t memspace);
    MemArray getCurrentGepBounds(size_t memspace);
    MemArray getCurrentProperties(const std::string& pname);

    Bool externalizeEverythingMutable(const std::string& postfix) {
        Bool ret = factory.getTrue();
        for(auto&& kv: memArrays) {
            auto fresh = MemArrayWithVersion(MemArray::mkVar(factory.unwrap(), tfm::format("%s%s", kv.first, postfix)), 0);
            ret = ret && (fresh == kv.second);
            kv.second = fresh;
        }
        return ret;
    };

    MemArray getInitialMemoryContents(size_t memspace);

    Pointer getGlobalPtr(size_t memspace, size_t offsetSize = 1U);
    Pointer getGlobalPtr(size_t memspace, size_t offsetSize, Integer origSize);

    Pointer getLocalPtr(size_t memspace, size_t offsetSize = 1U);
    Pointer getLocalPtr(size_t memspace, size_t offsetSize, Integer origSize);

    using LocalMemoryBounds = std::pair<unsigned long long, unsigned long long>;
    LocalMemoryBounds getLocalMemoryBounds() const;

    const impl_::smtExprSet& getAxioms() const { return contextAxioms; }

////////////////////////////////////////////////////////////////////////////////

    Dynamic readExprFromMemory(Pointer ix, size_t bitSize, size_t memspace) {
        return memory(memspace).select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readExprFromMemory(Pointer ix, size_t memspace) {
        return memory(memspace).select<ExprClass>(ix);
    }
    void writeExprToMemory(Pointer ix, DynBV val, size_t memspace) {
        writeExprToMemory(ix, Byte::forceCast(val), memspace);
    }
    template<class ExprClass>
    void writeExprToMemory(Pointer ix, ExprClass val, size_t memspace) {
        memory( memspace, memory(memspace).store(ix, val) );
    }
    void writeExprRangeToMemory(Pointer from, size_t size, DynBV val, size_t memspace) {
        writeExprRangeToMemory(from, size, Byte::forceCast(val), memspace);
    }
    template<class ExprClass>
    void writeExprRangeToMemory(Pointer from, size_t size, ExprClass val, size_t memspace) {
        auto&& currentMemory = memory(memspace);

        auto&& newMem = MemArrayWithVersion{
            factory.getEmptyMemoryArray(tfm::format("%s%d", MEMORY_ID, memspace)),
            currentMemory.getVersion() + 1
        };

        std::function<Bool(Pointer)> fun = [=](Pointer inner) {
            return
                factory.if_(inner >= from && inner < from + size)
                    .then_(newMem.select<ExprClass>(inner) == val)
                    .else_(newMem.select<ExprClass>(inner) == currentMemory.select<ExprClass>(inner));
        };
        std::function<std::vector<Dynamic>(Pointer)> patterns = [=](Pointer inner) {
            return util::make_vector(Dynamic(newMem.select<ExprClass>(inner)));
        };

        auto&& axiom = factory.forAll(fun, patterns);

        contextAxioms.insert(axiom.asAxiom());

        return memory(memspace, newMem);
    }

    Dynamic readProperty(const std::string& id, Pointer ix, size_t bitSize) {
        return get(id).select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readProperty(const std::string& id, Pointer ix) {
        return get(id).select<ExprClass>(ix);
    }
    void writeProperty(const std::string& id, Pointer ix, DynBV val) {
        writeProperty(id, ix, Byte::forceCast(val));
    }
    template<class ExprClass>
    void writeProperty(const std::string& id, Pointer ix, ExprClass val) {
        set( id, get(id).store(ix, val) );
    }

////////////////////////////////////////////////////////////////////////////////

    using Choice = std::pair<Bool, ExecutionContext>;

    ExecutionContext& switchOn(const std::string& name, const std::vector<Choice>& contexts);
    ExecutionContext& externalSwitchOn(const std::vector<Choice>& contexts, const std::string& prefix);

    static ExecutionContext mergeMemory(
        const std::string& name,
        ExecutionContext defaultContext,
        const std::vector<Choice>& contexts);
    static ExecutionContext externalMergeMemory(
        ExecutionContext defaultContext,
        const std::vector<Choice>& contexts,
        const std::string& prefix);

////////////////////////////////////////////////////////////////////////////////

    Integer getBound(const Pointer& p, size_t bitSize, size_t memspace);
    void writeBound(const Pointer& p, const Integer& bound, size_t memspace);

////////////////////////////////////////////////////////////////////////////////

    Bool toSMT() const;

    friend std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);

};

} // namespace z3_
} // namespace borealis
