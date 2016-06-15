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

    ExprFactory& factory;
    mutable std::unordered_map<std::string, MemArray> memArrays;
    mutable std::unordered_map<std::string, MemArray> initialMemArrays;
    unsigned long long globalPtr;
    unsigned long long localPtr;

    unsigned long long localMemoryStart;
    unsigned long long localMemoryEnd;

    impl_::smtExprSet contextAxioms;

    static const std::string MEMORY_ID;
    MemArray memory() const;
    void memory(const MemArray& value);

    static const std::string GEP_BOUNDS_ID;
    void initGepBounds();
    MemArray gepBounds() const;
    void gepBounds(const MemArray& value);

    MemArray get(const std::string& id) const;
    void set(const std::string& id, const MemArray& value);

    using MemArrayIds = std::unordered_set<std::string>;
    MemArrayIds getMemArrayIds() const;

public:

    ExecutionContext(ExprFactory& factory, unsigned long long localMemoryStart, unsigned long long localMemoryEnd);
    ExecutionContext(const ExecutionContext&) = default;

    MemArray getCurrentMemoryContents();
    MemArray getCurrentGepBounds();

    MemArray getInitialMemoryContents();

    Pointer getGlobalPtr(size_t offsetSize = 1U);
    Pointer getGlobalPtr(size_t offsetSize, Integer origSize);

    Pointer getLocalPtr(size_t offsetSize = 1U);
    Pointer getLocalPtr(size_t offsetSize, Integer origSize);

    using LocalMemoryBounds = std::pair<unsigned long long, unsigned long long>;
    LocalMemoryBounds getLocalMemoryBounds() const;

    const impl_::smtExprSet& getAxioms() const { return contextAxioms; }

////////////////////////////////////////////////////////////////////////////////

    Dynamic readExprFromMemory(Pointer ix, size_t bitSize) {
        return memory().select(ix, bitSize);
    }
    template<class ExprClass>
    ExprClass readExprFromMemory(Pointer ix) {
        return memory().select<ExprClass>(ix);
    }
    void writeExprToMemory(Pointer ix, DynBV val) {
        writeExprToMemory(ix, Byte::forceCast(val));
    }
    template<class ExprClass>
    void writeExprToMemory(Pointer ix, ExprClass val) {
        memory( memory().store(ix, val) );
    }
    void writeExprRangeToMemory(Pointer from, size_t size, DynBV val) {
        writeExprRangeToMemory(from, size, Byte::forceCast(val));
    }
    template<class ExprClass>
    void writeExprRangeToMemory(Pointer from, size_t size, ExprClass val) {
        auto&& currentMemory = memory();

        auto&& newMem = factory.getEmptyMemoryArray(MEMORY_ID);

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

        return memory(newMem);
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

    static ExecutionContext mergeMemory(
        const std::string& name,
        ExecutionContext defaultContext,
        const std::vector<Choice>& contexts);

////////////////////////////////////////////////////////////////////////////////

    Integer getBound(const Pointer& p, size_t bitSize);
    void writeBound(const Pointer& p, const Integer& bound);

////////////////////////////////////////////////////////////////////////////////

    Bool toSMT() const;

    friend std::ostream& operator<<(std::ostream& s, const ExecutionContext& ctx);

};

} // namespace z3_
} // namespace borealis
