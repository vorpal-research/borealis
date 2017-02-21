//
// Created by abdullin on 2/8/17.
//

#ifndef BOREALIS_ENVIRONMENT_H
#define BOREALIS_ENVIRONMENT_H

#include <memory>

#include "Domain/DomainFactory.h"
#include "Util/slottracker.h"

namespace borealis {
namespace absint {

class Environment : public std::enable_shared_from_this<const Environment> {
public:
    using Ptr = std::shared_ptr<const Environment>;

    Environment() = default;

    const DomainFactory& getDomainFactory() const;

private:

    DomainFactory factory_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_ENVIRONMENT_H
