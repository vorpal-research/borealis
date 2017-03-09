//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_DOMAINFACTORY_H
#define BOREALIS_DOMAINFACTORY_H

#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Value.h>

#include "Domain.h"

namespace borealis {
namespace absint {

class DomainFactory {
/// No data?
public:

    Domain::Ptr get(const llvm::Type& type) const;
    Domain::Ptr get(const llvm::Value* val) const;
    Domain::Ptr get(const llvm::Constant* constant) const;
    Domain::Ptr getInteger(unsigned width, bool isSigned = false) const;
    Domain::Ptr getInteger(const llvm::APSInt& val) const;
    Domain::Ptr getInteger(const llvm::APSInt& from, const llvm::APSInt& to) const;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_DOMAINFACTORY_H
