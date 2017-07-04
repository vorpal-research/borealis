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

    static WideningInterface* getInstance() {
        static IntegerWidening* instance = new IntegerWidening();
        return instance;
    }

    virtual Integer::Ptr get_prev(const Integer::Ptr& value, DomainFactory* factory) const;
    virtual Integer::Ptr get_next(const Integer::Ptr& value, DomainFactory* factory) const;

};

class FloatWidening : public WideningInterface<llvm::APFloat> {
protected:

    FloatWidening() = default;
public:

    static WideningInterface* getInstance() {
        static FloatWidening* instance = new FloatWidening();
        return instance;
    }

    virtual llvm::APFloat get_prev(const llvm::APFloat& value, DomainFactory* factory) const;
    virtual llvm::APFloat get_next(const llvm::APFloat& value, DomainFactory* factory) const;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_INTERVALWIDENING_H
