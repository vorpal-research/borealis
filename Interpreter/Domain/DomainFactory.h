//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_DOMAINFACTORY_H
#define BOREALIS_DOMAINFACTORY_H

#include <unordered_map>

#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Value.h>

#include "Domain.h"
#include "FloatInterval.h"
#include "IntegerInterval.h"
#include "Pointer.h"

namespace borealis {
namespace absint {

class DomainFactory: public logging::ObjectLevelLogging<DomainFactory> {
public:

    using IntCache = std::unordered_map<IntegerInterval::ID,
            Domain::Ptr,
            IntegerInterval::IDHash,
            IntegerInterval::IDEquals>;

    using FloatCache = std::unordered_map<FloatInterval::ID,
            Domain::Ptr,
            FloatInterval::IDHash,
            FloatInterval::IDEquals>;

    using PtrCache = std::unordered_map<Pointer::ID,
            Domain::Ptr,
            Pointer::IDHash,
            Pointer::IDEquals>;

    DomainFactory();
    ~DomainFactory();

    Domain::Ptr get(const llvm::Type& type, Domain::Value value = Domain::BOTTOM);
    Domain::Ptr get(const llvm::Value* val, Domain::Value value = Domain::BOTTOM);
    Domain::Ptr get(const llvm::Constant* constant);

    Domain::Ptr getInteger(unsigned width, bool isSigned = false);
    Domain::Ptr getInteger(Domain::Value value, unsigned width, bool isSigned = false);
    Domain::Ptr getInteger(const llvm::APInt& val, bool isSigned = false);
    Domain::Ptr getInteger(const llvm::APInt& from, const llvm::APInt& to, bool isSigned = false);

    Domain::Ptr getFloat(const llvm::fltSemantics& semantics);
    Domain::Ptr getFloat(Domain::Value value, const llvm::fltSemantics& semantics);
    Domain::Ptr getFloat(const llvm::APFloat& val);
    Domain::Ptr getFloat(const llvm::APFloat& from, const llvm::APFloat& to);

    Domain::Ptr getPointer();
    Domain::Ptr getPointer(Domain::Value value);
    Domain::Ptr getPointer(bool isValid);
    Domain::Ptr getPointer(Pointer::Status status);

private:

    Domain::Ptr cached(const IntegerInterval::ID& key);
    Domain::Ptr cached(const FloatInterval::ID& key);
    Domain::Ptr cached(const Pointer::ID& key);

    IntCache ints_;
    FloatCache floats_;
    PtrCache ptrs_;

};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_DOMAINFACTORY_H
