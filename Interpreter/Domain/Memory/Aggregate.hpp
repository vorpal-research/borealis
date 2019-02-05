//
// Created by abdullin on 2/5/19.
//

#ifndef BOREALIS_AGGREGATE_HPP
#define BOREALIS_AGGREGATE_HPP

namespace borealis {
namespace absint {

template <typename MachineInt>
class Aggregate : public AbstractDomain {
public:
    virtual Ptr load(Ptr) const = 0;
    virtual void store(Ptr, Ptr) = 0;
};

} // namespace absint
} // namespace borealis

#endif //BOREALIS_AGGREGATE_HPP
