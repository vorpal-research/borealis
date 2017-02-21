//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_DOMAINFACTORY_H
#define BOREALIS_DOMAINFACTORY_H

#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Value.h>

#include "Domain.h"
#include "Pointer.h"

namespace borealis {
namespace absint {

class DomainFactory {
/// No data?
public:

    Domain::Ptr get(const llvm::Type& type, Domain::Value value = Domain::BOTTOM) const;
    Domain::Ptr get(const llvm::Value* val, Domain::Value value = Domain::BOTTOM) const;
    Domain::Ptr get(const llvm::Constant* constant) const;

    Domain::Ptr getInteger(unsigned width, bool isSigned = false) const;
    Domain::Ptr getInteger(Domain::Value value, unsigned width, bool isSigned = false) const;
    Domain::Ptr getInteger(const llvm::APSInt& val) const;
    Domain::Ptr getInteger(const llvm::APSInt& from, const llvm::APSInt& to) const;

    Domain::Ptr getFloat(const llvm::fltSemantics& semantics) const;
    Domain::Ptr getFloat(Domain::Value value, const llvm::fltSemantics& semantics) const;
    Domain::Ptr getFloat(const llvm::APFloat& val) const;
    Domain::Ptr getFloat(const llvm::APFloat& from, const llvm::APFloat& to) const;

    Domain::Ptr getPointer() const;
    Domain::Ptr getPointer(Domain::Value value) const;
    Domain::Ptr getPointer(bool isValid) const;
    Domain::Ptr getPointer(Pointer::Status status) const;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_DOMAINFACTORY_H
