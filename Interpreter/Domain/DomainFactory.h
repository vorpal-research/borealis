//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_DOMAINFACTORY_H
#define BOREALIS_DOMAINFACTORY_H

#include <unordered_map>

#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Value.h>
#include <andersen/include/Andersen.h>

#include "Domain.h"
#include "FloatInterval.h"
#include "IntegerInterval.h"
#include "Pointer.h"
#include "AggregateObject.h"

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

    using PointerCache = std::unordered_map<const llvm::Value*,
            Domain::Ptr>;

    DomainFactory(const Andersen* aa);
    ~DomainFactory();

    Domain::Ptr getTop(const llvm::Type& type);
    Domain::Ptr getBottom(const llvm::Type& type);

    Domain::Ptr get(const llvm::Value* val);
    Domain::Ptr get(const llvm::Constant* constant);
    Domain::Ptr getInMemory(const llvm::Type& type);

    Domain::Ptr getIndex(uint64_t indx);
    Domain::Ptr getInteger(unsigned width, bool isSigned = false);
    Domain::Ptr getInteger(Domain::Value value, unsigned width, bool isSigned = false);
    Domain::Ptr getInteger(const llvm::APInt& val, bool isSigned = false);
    Domain::Ptr getInteger(const llvm::APInt& from, const llvm::APInt& to, bool isSigned = false);

    Domain::Ptr getFloat(const llvm::fltSemantics& semantics);
    Domain::Ptr getFloat(Domain::Value value, const llvm::fltSemantics& semantics);
    Domain::Ptr getFloat(const llvm::APFloat& val);
    Domain::Ptr getFloat(const llvm::APFloat& from, const llvm::APFloat& to);

    Domain::Ptr getAggregateObject(Domain::Value value, const llvm::Type& type);
    Domain::Ptr getAggregateObject(const llvm::Type& type);
    Domain::Ptr getAggregateObject(const llvm::Type& type, std::vector<Domain::Ptr> elements);

    Domain::Ptr getPointer(Domain::Value value, const llvm::Type& elementType);
    Domain::Ptr getPointer(const llvm::Type& elementType);
    Domain::Ptr getPointer(const llvm::Type& elementType, const Pointer::Locations& locations);

    MemoryObject::Ptr getMemoryObject(const llvm::Type& type);
    MemoryObject::Ptr getMemoryObject(Domain::Ptr value);

private:

    Domain::Ptr cached(const IntegerInterval::ID& key);
    Domain::Ptr cached(const FloatInterval::ID& key);

    IntCache ints_;
    FloatCache floats_;
    PointerCache heap_;

    const Andersen* aa_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_DOMAINFACTORY_H
