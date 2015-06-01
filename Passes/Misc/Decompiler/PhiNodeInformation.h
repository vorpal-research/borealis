/*
 * PhiNodeInformation.h
 *
 *  Created on: 23 апр. 2015 г.
 *      Author: kivi
 */

#ifndef PHINODEINFORMATION_H_
#define PHINODEINFORMATION_H_

#include <unordered_map>
#include <llvm/IR/BasicBlock.h>

namespace borealis {
namespace decompiler {

struct PhiEntry {
    llvm::Value* value;
    llvm::Value* initializer;
    bool operator==(const PhiEntry& right) {
        return ((this->value == right.value) && (this->initializer == right.initializer));
    }
};

class PhiNodeInformation {
private:
    std::unordered_map<llvm::BasicBlock*, std::vector<PhiEntry>> phiInfo;
    std::unordered_map<llvm::Value*, bool> typeStates;
public:
    PhiNodeInformation() : phiInfo() {}

    void addEntry(llvm::BasicBlock* bb, PhiEntry phi);
    void addValue(llvm::Value* v);
    void disablePrintType(llvm::Value* v);

    std::vector<PhiEntry>& getBlockEntries(llvm::BasicBlock* bb);
    bool isPrintType(llvm::Value* v);

    void clear();
};

} /* namespace decompiler */
} /* namespace borealis */

#endif /* PHINODEINFORMATION_H_ */
