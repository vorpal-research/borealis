/*
 * GlobalVariableLifting.cpp
 *
 *  Created on: 23 марта 2015 г.
 *      Author: abdullin
 */
#include "GlobalVariableLifting.h"


#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>
#include <unordered_set>


namespace borealis{

bool GlobalVariableLifting::runOnFunction(llvm::Function& F){
    string_value_map globals;
    std::unordered_map<llvm::BasicBlock*, string_value_map> locals_begin, locals_end;
    std::unordered_set<llvm::Value*> delete_instructions;

    // find all global variables used by function
    for (auto&& i = inst_begin(F); i != inst_end(F); ++i) {
        for (auto&& op : i->operand_values()) {
            if (llvm::isa<llvm::GlobalVariable>(op)) {
            	if (!op->getType()->getPointerElementType()->isArrayTy())
                	globals[op->getName()] = op;
            }
        }
    }

    for (auto&& it = F.begin(); it != F.end(); ++it) {
        auto* bb = &*it;
        auto&& j = bb->begin();

        // insert loads at the start of current BB
        for (auto&& g : globals) {
            auto&& gName = g.first();
            auto&& gValue = g.second;
            auto* load = new llvm::LoadInst(gValue, gName + LOCAL_POSTFIX, &bb->front());
            locals_begin[bb][gName] = locals_end[bb][gName] = load;
        }

        //replase globals to locals in current BB
        for ( ; j != bb->end(); ++j) {
            for (auto&& op : j->operand_values()) {
                if (llvm::isa<llvm::GlobalVariable>(op)) {
                   if (auto* load = llvm::dyn_cast<llvm::LoadInst>(j)) {
                        j->replaceAllUsesWith(locals_end[bb][op->getName()]);
                        delete_instructions.insert(j);
                    } else if (auto* store = llvm::dyn_cast<llvm::StoreInst>(j)) {
                        locals_end[bb][op->getName()] = store->getValueOperand();
                        delete_instructions.insert(j);
                    }
                }
            }
            //if j is terminate instruction - store all
            if (auto* terminate = llvm::dyn_cast<llvm::TerminatorInst>(j)) {
            	if(j->getOpcode() != llvm::Instruction::Br){
					for (auto&& g : globals) {
						auto&& gName = g.first();
						new llvm::StoreInst(locals_end[bb][gName], globals[gName], terminate);
					}
               	}
            //if j is call instruction - store all before call and load all after call
            } else if (auto* call = llvm::dyn_cast<llvm::CallInst>(j)) {
             	for (auto&& g : globals) {
                    auto&& gName = g.first();
              		new llvm::StoreInst(locals_end[bb][gName], globals[gName], call);
               	}
               	++j;
               	for (auto&& g : globals) {
                    auto&& gName = g.first();
                    auto&& gValue = g.second;
                    auto* load = new llvm::LoadInst(gValue, gName + LOCAL_POSTFIX, j);
                    locals_end[bb][gName] = load;
              	}
               	--j;
            }
        }
    }

    for (auto&& it = ++F.begin(); it != F.end(); ++it) {
        auto* bb = &*it;

        // add phi-nodes in blocks with several predecessors
        if (bb->getUniquePredecessor() == nullptr) {
            auto&& pred_num = 0U;
            for (auto&& p = pred_begin(bb); p != pred_end(bb); ++p) ++pred_num;

            for (auto&& g : globals) {
                auto&& gName = g.first();
                auto* phi = llvm::PHINode::Create(
                    locals_begin[bb][gName]->getType(),
                    pred_num,
                    gName + LOCAL_POSTFIX,
                    &bb->front()
                );
                for (auto&& p = pred_begin(bb); p != pred_end(bb); ++p) {
                    auto* pred = *p;
                    phi->addIncoming(locals_end[pred][gName], pred);
                }
                if (locals_begin[bb][gName] == locals_end[bb][gName]) {
                    locals_end[bb][gName] = phi;
                }
                locals_begin[bb][gName]->replaceAllUsesWith(phi);
                delete_instructions.insert(locals_begin[bb][gName]);
                locals_begin[bb][gName] = phi;
            }
        // remove unused locals in basic blocks with one predecessor
        } else {
            auto* pred = bb->getUniquePredecessor();
            for (auto&& g : globals) {
                auto&& gName = g.first();
                if (locals_begin[bb][gName] == locals_end[bb][gName]) {
                    locals_end[bb][gName] = locals_end[pred][gName];
                }
                locals_begin[bb][gName]->replaceAllUsesWith(locals_end[pred][gName]);
                delete_instructions.insert(locals_begin[bb][gName]);
                locals_begin[bb][gName] = locals_end[pred][gName];
            }
        }
    }

    // deleting unused instructions
    for (auto&& i : delete_instructions) {
        if (auto* ii = llvm::dyn_cast<llvm::Instruction>(i)) {
            ii->eraseFromParent();
        }
    }
    return true;
}

char GlobalVariableLifting::ID=0;
static llvm::RegisterPass<GlobalVariableLifting> X("gvl", "global variable lifting pass", false, false);

}  /* namespace borealis */






