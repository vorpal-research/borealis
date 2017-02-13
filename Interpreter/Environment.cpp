//
// Created by abdullin on 2/8/17.
//

#include "Environment.h"

namespace borealis {
namespace absint {

Environment::Environment(const llvm::Module* module) : tracker_(module) {}

SlotTracker& Environment::getSlotTracker() const {
    return tracker_;
}

const DomainFactory& Environment::getDomainFactory() const {
    return factory_;
}

}   /* namespace absint */
}   /* namespace borealis */

