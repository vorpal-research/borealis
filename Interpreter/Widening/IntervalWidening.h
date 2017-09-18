//
// Created by abdullin on 6/26/17.
//

#ifndef BOREALIS_INTERVALWIDENING_H
#define BOREALIS_INTERVALWIDENING_H

#include "Interpreter/Domain/Integer/Integer.h"
#include "WideningInterface.hpp"

namespace borealis {
namespace absint {

class IntegerWidening : public WideningInterface<Integer::Ptr> {
protected:

    IntegerWidening() = default;
public:

    static IntegerWidening* getInstance() {
        static IntegerWidening* instance = new IntegerWidening();
        return instance;
    }

    Integer::Ptr get_prev(const Integer::Ptr& value, DomainFactory* factory) const override;
    Integer::Ptr get_next(const Integer::Ptr& value, DomainFactory* factory) const override;
    Integer::Ptr get_signed_prev(const Integer::Ptr& value, DomainFactory* factory) const;
    Integer::Ptr get_signed_next(const Integer::Ptr& value, DomainFactory* factory) const;

};

class FloatWidening : public WideningInterface<llvm::APFloat> {
protected:

    FloatWidening() = default;
public:

    static FloatWidening* getInstance() {
        static FloatWidening* instance = new FloatWidening();
        return instance;
    }

    llvm::APFloat::roundingMode getRoundingMode() const {
        return llvm::APFloat::rmNearestTiesToEven;
    }

    llvm::APFloat get_prev(const llvm::APFloat& value, DomainFactory* factory) const override;
    llvm::APFloat get_next(const llvm::APFloat& value, DomainFactory* factory) const override;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERVALWIDENING_H
