////
//// Created by abdullin on 4/7/17.
////
//
//#ifndef BOREALIS_POINTER_H
//#define BOREALIS_POINTER_H
//
//#include <unordered_set>
//
//#include "Interpreter/Domain/Domain.h"
//
//namespace borealis {
//namespace absint {
//
//struct PointerLocation {
//    mutable std::unordered_set<Domain::Ptr, DomainHash, DomainEquals> offsets_;
//    Domain::Ptr location_;
//};
//
//struct PtrLocationHash {
//    size_t operator() (const PointerLocation& loc) const noexcept {
//        return loc.location_->hashCode();
//    }
//};
//
//struct PtrLocationEquals {
//    bool operator() (const PointerLocation& lhv, const PointerLocation& rhv) const noexcept {
//        return lhv.location_->equals(rhv.location_.get());
////        return lhv.base_.get() == rhv.base_.get();
//    }
//};
//
//class NullptrDomain : public Domain {
//public:
//    explicit NullptrDomain(DomainFactory* factory);
//
//    void moveToTop() override {};
//    /// Poset
//    bool equals(const Domain* other) const override;
//    bool operator<(const Domain& other) const override;
//
//    /// Lattice
//    Domain::Ptr join(Domain::Ptr other) override;
//    Domain::Ptr meet(Domain::Ptr other) override;
//    Domain::Ptr widen(Domain::Ptr other) override;
//
//    /// Other
//    Domain::Ptr clone() const override;
//    std::size_t hashCode() const override;
//    std::string toPrettyString(const std::string& prefix) const override;
//
//    /// Memory
//    void store(Domain::Ptr value, Domain::Ptr offset) override;
//    Domain::Ptr load(Type::Ptr type, Domain::Ptr offset) override;
//    Domain::Ptr gep(Type::Ptr type, const std::vector<Domain::Ptr>& indices) override;
//
//    static bool classof(const Domain* other);
//};
//
///// Mutable
//class PointerDomain : public Domain {
//public:
//
//    using Locations = std::unordered_set<PointerLocation, PtrLocationHash, PtrLocationEquals>;
//
//    PointerDomain(Domain::Value value, DomainFactory* factory, Type::Ptr elementType, bool isGep = false);
//    PointerDomain(DomainFactory* factory, Type::Ptr elementType, const Locations& locations, bool isGep = false);
//    PointerDomain(const PointerDomain& other);
//
//    void moveToTop() override;
//    /// Poset
//    bool equals(const Domain* other) const override;
//    bool operator<(const Domain& other) const override;
//
//    /// Lattice
//    Domain::Ptr join(Domain::Ptr other) override;
//    Domain::Ptr meet(Domain::Ptr other) override;
//    Domain::Ptr widen(Domain::Ptr other) override;
//
//    /// Other
//    bool onlyNullptr() const;
//    bool isNullptr() const override;
//    Type::Ptr getElementType() const;
//    const Locations& getLocations() const;
//    Domain::Ptr clone() const override;
//    std::size_t hashCode() const override;
//    std::string toPrettyString(const std::string& prefix) const override;
//    Domain::Ptr getBound() const;
//
//    static bool classof(const Domain* other);
//
//    /// Semantics
//    Domain::Ptr sub(Domain::Ptr other) const override;
//    Domain::Ptr load(Type::Ptr type, Domain::Ptr offset) override;
//    void store(Domain::Ptr value, Domain::Ptr offset) override;
//    Domain::Ptr gep(Type::Ptr type, const std::vector<Domain::Ptr>& indices) override;
//    /// Cast
//    Domain::Ptr ptrtoint(Type::Ptr type) override;
//    Domain::Ptr bitcast(Type::Ptr type) override;
//    /// Cmp
//    Domain::Ptr icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const override;
//    /// Split
//    Split splitByEq(Domain::Ptr other) override;
//
//private:
//
//    Type::Ptr elementType_;
//    Locations locations_;
//    bool isGep_;
//};
//
//}   /* namespace absint */
//}   /* namespace borealis */
//
//#endif //BOREALIS_POINTER_H
