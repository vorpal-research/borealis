/*
 * PhiNodeInformation.cpp
 *
 *  Created on: 23 апр. 2015 г.
 *      Author: kivi
 */


#include "Passes/Misc/Decompiler/PhiNodeInformation.h"
#include <algorithm>

namespace borealis {
namespace decompiler{

void PhiNodeInformation::addEntry(llvm::BasicBlock* bb, PhiEntry phi) {
    if(std::find(phiInfo[bb].begin(), phiInfo[bb].end(), phi) == phiInfo[bb].end())
        phiInfo[bb].push_back(phi);
}

void PhiNodeInformation::addValue(llvm::Value* v) {
    typeStates[v] = false;
}

void PhiNodeInformation::disablePrintType(llvm::Value* v) {
    typeStates[v] = true;
}

std::vector<PhiEntry>& PhiNodeInformation::getBlockEntries(llvm::BasicBlock* bb) {
    return phiInfo[bb];
}

bool PhiNodeInformation::isPrintType(llvm::Value* v) {
    auto&& it = typeStates.find(v);
    if(it != typeStates.end()) return it->second;
    else return true;
}

void PhiNodeInformation::clear() {
    phiInfo.clear();
}

} /* namespace decompiler */
} /* namespace borealis */
