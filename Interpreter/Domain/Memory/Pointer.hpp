//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_POINTER_HPP
#define BOREALIS_POINTER_HPP

#include "Interpreter/Domain/Domain.h"
#include "MemoryLocation.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

//template <typename MachineInt>
//class Pointer : public AbstractDomain<Pointer<MachineInt>> {
//public:
//
//    using Ptr = std::shared_ptr<Pointer<MachineInt>>;
//    using ConstPtr = std::shared_ptr<const Pointer<MachineInt>>;
//
//    using IntervalT = Interval<MachineInt>;
//    using IntervalPtr = typename IntervalT::Ptr;
//    using Location = MemoryLocation<MachineInt>;
//    using PointsToSet = std::unordered_set<Location>;
//
//private:
//public:
//
//
//};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_POINTER_HPP
