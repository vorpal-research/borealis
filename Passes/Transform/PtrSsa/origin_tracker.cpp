/*
 * origin_tracker.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include "Passes/Transform/PtrSsa/origin_tracker.h"

using namespace borealis;

origin_tracker::value origin_tracker::getOrigin(origin_tracker::value other) const {
    return origins.count(other) ? origins.at(other) : nullptr;
}

void origin_tracker::setOrigin(origin_tracker::value k, origin_tracker::value v) {
    origins[k] = v;
}

void origin_tracker::mergeOriginInfoFrom(const origin_tracker& other) {
    const auto& that = other.getResolver();
    origins.insert(that.begin(), that.end());
}
