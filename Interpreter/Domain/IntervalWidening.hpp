////
//// Created by abdullin on 6/26/17.
////
//
//#ifndef BOREALIS_INTERVALWIDENING_H
//#define BOREALIS_INTERVALWIDENING_H
//
//#include "Interpreter/Domain/Integer/Integer.h"
//
//namespace borealis {
//namespace absint {
//
//class DomainFactory;
//
//template <class T>
//class WideningInterface {
//public:
//
//    virtual ~WideningInterface() = default;
//
//    virtual T get_prev(const T& value, DomainFactory* factory) const = 0;
//    virtual T get_next(const T& value, DomainFactory* factory) const = 0;
//};
//
//class IntegerWidening : public WideningInterface<Integer::Ptr> {
//protected:
//
//    IntegerWidening() = default;
//public:
//
//    static IntegerWidening* getInstance() {
//        static auto instance = new IntegerWidening();
//        return instance;
//    }
//
//    Integer::Ptr get_prev(const Integer::Ptr& value, DomainFactory* factory) const override;
//    Integer::Ptr get_next(const Integer::Ptr& value, DomainFactory* factory) const override;
//    Integer::Ptr get_signed_prev(const Integer::Ptr& value, DomainFactory* factory) const;
//    Integer::Ptr get_signed_next(const Integer::Ptr& value, DomainFactory* factory) const;
//
//};
//
//class FloatWidening : public WideningInterface<llvm::APFloat> {
//protected:
//
//    FloatWidening() = default;
//public:
//
//    static FloatWidening* getInstance() {
//        static auto instance = new FloatWidening();
//        return instance;
//    }
//
//    llvm::APFloat::roundingMode getRoundingMode() const {
//        return llvm::APFloat::rmNearestTiesToEven;
//    }
//
//    llvm::APFloat get_prev(const llvm::APFloat& value, DomainFactory* factory) const override;
//    llvm::APFloat get_next(const llvm::APFloat& value, DomainFactory* factory) const override;
//
//};
//
//}   /* namespace absint */
//}   /* namespace borealis */
//
//#endif //BOREALIS_INTERVALWIDENING_H
