/*
 * DecInstVisitor.cpp
 *
 *  Created on: 27 марта 2015 г.
 *      Author: kivi
 */

#include "Passes/Misc/Decompiler/DecInstVisitor.h"

#include <llvm/IR/Instruction.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/GVMaterializer.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <string>
#include <stack>
#include <iterator>

#include "Util/streams.hpp"

using namespace llvm;
using namespace std;

namespace borealis {
namespace decompiler{

const std::map<Type::TypeID, std::string> types = {
        {Type::TypeID::IntegerTyID, "int"},
        {Type::TypeID::DoubleTyID, "double"},
        {Type::TypeID::FP128TyID, "long double"},
        {Type::TypeID::FloatTyID, "float"},
        {Type::TypeID::HalfTyID, "float16"},
        {Type::TypeID::PPC_FP128TyID, "long double"},
        {Type::TypeID::X86_FP80TyID, "float80"},
        {Type::TypeID::FunctionTyID, "function"},
        {Type::TypeID::LabelTyID, "label"},
        {Type::TypeID::MetadataTyID, "metadata"},
        {Type::TypeID::VoidTyID, "void"}
};

std::map<int, std::string> intTypes = {
        {1, "bool"},
        {8, "char"},
        {16, "short"},
        {32, "int"},
        {64, "long int"}
};

const std::map<int,std::string> binOp = {
        {8,"+"},
        {9,"+"},
        {10,"-"},
        {11,"-"},
        {12,"*"},
        {13,"*"},
        {14,"/"},
        {15,"/"},
        {16,"/"},
        {17,"%"},
        {18,"%"},
        {19,"%"},
        {20,"<<"},
        {21,">>"},
        {22,"a>>"},
        {23,"&&"},
        {24,"||"},
        {25,"^"}
};

const std::map<int,std::string> icmpOp = {
        {40,"<"},
        {32,"=="},
        {33,"!="},
        {34,"+>"},
        {35,"+>="},
        {36,"+<"},
        {37,"+<="},
        {38,">"},
        {39,">="},
        {41,"<="}
};

const std::map<int,std::string> fcmpOp = {
        {0,"false"},
        {1,".=="},
        {2,".>"},
        {3,".>="},
        {4,".<"},
        {5,".<="},
        {6,".!="},
        {7,"ordered"},
        {8,".=="},
        {9,".>"},
        {10,".>="},
        {11,".<"},
        {12,".<="},
        {13,".!="},
        {14,"unordered"},
        {15,"true"}
};

void DecInstVisitor::printTabulation(int num) {
    for(int i = 0; i < num; ++i) infos()<<"\t";
}

void DecInstVisitor::printType(llvm::Type* t) {
    auto&& it = types.find(t->getTypeID());
    if(it != types.end()) {
        if(t->isIntegerTy()){
            if(intTypes.find(t->getIntegerBitWidth()) != intTypes.end()) infos()<<intTypes[t->getIntegerBitWidth()];
            else infos()<<"int"<<t->getIntegerBitWidth();
        }
        else infos()<<it->second;
        return;
    } else if(t->isPointerTy()) {
        printType(t->getPointerElementType());
        infos()<<"*";
        return;
    }
    else if(t->isArrayTy()) {
        printType(t->getArrayElementType());
        infos()<<"["<<t->getArrayNumElements()<<"]";
        return;
    } else if(t->isStructTy()) {
        auto&& st = cast<llvm::StructType>(t);
        if(!st->isLiteral()) {
            infos()<<st->getStructName();
        }
        return;
    } else if(t->isVectorTy()) {
        infos()<<"vector<";
        printType(t->getVectorElementType());
        infos()<<">("<<t->getVectorNumElements()<<")";
        return;
    } else {
        std::string temp = "";
        llvm::raw_string_ostream rs(temp);
        t->print(rs);
        infos()<<rs.str();
        return;
    }
}

void DecInstVisitor::printCond(llvm::Instruction* ins){
    if(auto&& ici = dyn_cast<ICmpInst>(ins)) {
        writeValueToStream(ins->getOperand(0), true);
        infos()<<" "<<icmpOp.find(ici->getSignedPredicate())->second<<" ";
        writeValueToStream(ins->getOperand(1), true);
    } else if(auto&& fci = dyn_cast<FCmpInst>(ins)) {
        writeValueToStream(ins->getOperand(0), true);
        infos()<<" "<<fcmpOp.find(ici->getPredicate())->second<<" ";
        writeValueToStream(ins->getOperand(1), true);
    } else if(auto&& ci = dyn_cast<CastInst>(ins)) {
        writeValueToStream(ci, true);
    } else if(auto&& ci = dyn_cast<CallInst>(ins)) {
        writeValueToStream(ci, true);
    } else if(auto&& ii = dyn_cast<InvokeInst>(ins)) {
        writeValueToStream(ii, true);
    }
}

void DecInstVisitor::writeValueToStream(llvm::Value* v, bool deleteType) {
    if(!deleteType) {
        printType(v->getType());
        infos()<<" ";
    }
    isStruct(v->getType());
    //if value don't have name, it's very bad
    if(v->getName().empty()) {
        std::string temp = "";
        llvm::raw_string_ostream rs(temp);
        v->printAsOperand(rs);
        int nameStart = rs.str().find("\"");
        if(nameStart <= temp.size() && isString(v->getType())) {
            temp.erase(0,nameStart);
            infos()<<temp;
            return;
        }
        nameStart = temp.find('%');
        if(nameStart < temp.size() - 1 && temp[nameStart + 1] == '\"') {
            nameStart = temp.find("%", nameStart + 1);
        }
        if(nameStart > temp.size()) {
            if((nameStart = temp.find('@')) <= temp.size()) ++nameStart;
        }
        if( nameStart > temp.size()) {
            for(nameStart = temp.size() - 1; nameStart >= 0; --nameStart){
                if(temp[nameStart] == ' ') {
                    ++nameStart;
                    break;
                }
            }
        }
        temp.erase(0,nameStart);
        int first = 0; int second = -1;
        for( ; first < temp.size(); ++first) {
            if(temp[first] == ','){
                if(second >= 0) break;
                else second = first;
            }
        }
        if(second >= 0) {
            temp.erase(second, first - second);
            temp[second] = '[';
            for(first = second; first < temp.size(); ++first) {
                if(temp[first] == ','){
                    temp[first] = ']';
                    temp.insert(first,"]");
                }
                else if(temp[first] == ')') temp[first] = ']';
            }
        }
        while((nameStart = temp.find("[ ")) <= temp.size()) {
            int j = nameStart;
            for(int st = j + 1; st < temp.size(); ++st) {
                if(temp[st] == ' '){
                    temp.erase(j + 1,st - j);
                }
            }
        }
        infos()<<temp;
    } else infos()<<v->getName();
}

void DecInstVisitor::printPhiInstructions(llvm::BasicBlock& bb) {
    auto&& phi = phiInfo.getBlockEntries(&bb);
    for(auto&& i : phi) {
        printTabulation(nesting);
        writeValueToStream(i.value);
        infos()<<" = ";
        writeValueToStream(i.initializer, true);
        infos()<<";\n";
    }
}

bool DecInstVisitor::isString(llvm::Type* t) {
    bool isStr = false;
    if(t->isIntegerTy()){
        if(t->getIntegerBitWidth() == 8) isStr = true;
    } else if(t->isPointerTy()) {
        isStr = isString(t->getPointerElementType());
    }
    else if(t->isArrayTy()) {
        isStr = isString(t->getArrayElementType());
    }
    return isStr;
}

bool DecInstVisitor::isStruct(llvm::Type* t) {
    bool isStr = false;
    if(t->isStructTy()) {
        auto&& st = cast<llvm::StructType>(t);
        if(!st->isLiteral()) {
            usedStructs.insert(cast<StructType>(t));
            isStr = true;
        }
    } else if(t->isPointerTy()) {
        isStr = isStruct(t->getPointerElementType());
    }
    else if(t->isArrayTy()) {
        isStr = isStruct(t->getArrayElementType());
    }
    return isStr;
}

void DecInstVisitor::visitInstruction(llvm::Instruction &i) {
    printTabulation(nesting);
    infos()<<i.getOpcodeName()<<"\n";
    infos()<<i<<";\n";
}


void DecInstVisitor::visitStoreInst(llvm::StoreInst &i) {
    printTabulation(nesting);
    infos()<<"*";
    writeValueToStream(i.getPointerOperand(), true);
    infos()<<" = ";
    writeValueToStream(i.getValueOperand(), true);
    infos()<<";\n";
}

void DecInstVisitor::visitAllocaInst(llvm::AllocaInst &i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = alloca(";
    printType(i.getAllocatedType());
    infos()<<");\n";
}

void DecInstVisitor::visitLoadInst(LoadInst &i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = *";
    writeValueToStream(i.getPointerOperand(), true);
    infos()<<";\n";
}


void DecInstVisitor::visitBinaryOperator(llvm::BinaryOperator &i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = ";
    writeValueToStream(i.getOperand(0), true);
    infos()<<" "<<binOp.find(i.getOpcode())->second<<" ";
    writeValueToStream(i.getOperand(1), true);
    infos()<<";\n";
}

void DecInstVisitor::visitICmpInst(llvm::ICmpInst &i) {
    int c=0;
    bool isBr = false;
    for(auto&& it = i.user_begin(), ite = i.user_end(); it != ite; ++it) {
         if(isa<BranchInst>(*it)) {
             ++c;
             isBr = true;
         }
    }
    if(!(c == 1 && isBr))    {
        printTabulation(nesting);
        writeValueToStream(&i);
        infos()<<" = (";
        writeValueToStream(i.getOperand(0), true);
        infos()<<" "<<icmpOp.find(i.getSignedPredicate())->second<<" ";
        writeValueToStream(i.getOperand(1), true);
        infos()<<");\n";
    }
}

void DecInstVisitor::visitFCmpInst(llvm::FCmpInst &i) {
    int c = 0;
    bool isBr = false;
    for(auto&& it = i.user_begin(), ite = i.user_end(); it != ite; ++it) {
         if(isa<BranchInst>(*it)) {
             ++c;
             isBr = true;
         }
    }
    if(!(c == 1 && isBr))    {
        printTabulation(nesting);
        writeValueToStream(&i);
        infos()<<" = (";
        writeValueToStream(i.getOperand(0), true);
        infos()<<" "<<fcmpOp.find(i.getPredicate())->second<<" ";
        writeValueToStream(i.getOperand(1), true);
        infos()<<");\n";
    }
}

void DecInstVisitor::visitCastInst(llvm::CastInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = cast<";
    printType(i.getDestTy());
    infos()<<">(";
    writeValueToStream(i.getOperand(0), true);
    infos()<<");\n";
}


void DecInstVisitor::visitCallInst(llvm::CallInst &i) {
    if(i.getCalledFunction() != nullptr) {
        printTabulation(nesting);
        if(!i.getType()->isVoidTy()) {
            writeValueToStream(&i);
            infos()<<" = ";
        }
        infos()<<i.getCalledFunction()->getName()<<"(";
        int arg_num = 0;
        for(auto&& op : i.arg_operands()) ++arg_num;
        int num = 0;
        for(auto&& op : i.arg_operands()) {
            op.get()->clearSubclassOptionalData();
            writeValueToStream(op, true);
            if(num < arg_num - 1) {
                infos()<<", ";
                ++num;
            }
        }
        infos()<<");\n";
    }
}

void DecInstVisitor::visitInvokeInst(llvm::InvokeInst &i) {
    if(i.getCalledFunction() != nullptr) {
        printTabulation(nesting);
        if(!i.getType()->isVoidTy()) {
            writeValueToStream(&i);
            infos()<<" = ";
        }
        infos()<<"invoke ";
        infos()<<i.getCalledFunction()->getName()<<"(";
        for(auto j = 0U; j < i.getNumArgOperands(); ++j) {
            i.getArgOperand(j)->clearSubclassOptionalData();
            writeValueToStream(i.getArgOperand(j), true);
            if(j < i.getNumArgOperands() - 1) infos()<<", ";
        }
        infos()<<") to "<<i.getNormalDest()->getName()<<", unwind "<<i.getUnwindDest()->getName()<<";\n";
        bbInfo.setPrintName(*i.getNormalDest());
        bbInfo.setPrintName(*i.getUnwindDest());
    }
}

void DecInstVisitor::visitLandingPadInst(llvm::LandingPadInst &i) {
    printTabulation(nesting);
    if(!i.getType()->isVoidTy()) {
        writeValueToStream(&i, true);
        infos()<<" = ";
    }
    infos()<<"landingpad ";
    std::string temp = "";
    llvm::raw_string_ostream rs(temp);
    i.getPersonalityFn()->printAsOperand(rs);
    int nameStart = rs.str().find('@');
    if(nameStart < temp.size()) {
        temp.erase(0,nameStart);
        for(nameStart = 0; nameStart < temp.size(); ++nameStart) {
            if(temp[nameStart] == ' ') {
                temp.erase(nameStart, temp.size() - nameStart);
                break;
            }
        }
    }
    infos()<<temp;
    infos()<<" {\n";
    if(i.isCleanup()) {
        printTabulation(nesting + 1);
        infos()<<"cleanup;\n";
    }
    else {
        for(auto j = 0U; j < i.getNumClauses(); ++j) {
            printTabulation(nesting + 1);
            i.getClause(j)->clearSubclassOptionalData();
            if(i.isCatch(j)) infos()<<"catch ";
            else infos()<<"filter ";
            writeValueToStream(i.getClause(j));
            infos()<<";\n";
        }
    }
    printTabulation(nesting);
    infos()<<"}\n";
}

void DecInstVisitor::visitBranchInst(BranchInst &i) {
    printPhiInstructions(*i.getParent());
    if(i.getNumSuccessors() > 1) {
        Instruction* ins = &i;
        auto&& pre = i.getParent();
        while(ins != i.getCondition()) {
            if(ins == pre->getInstList().begin()) {
                ins = &i;
                break;
            }
            ins = ins->getPrevNode();
        }
        //process while_begin block
        if(currPos == BBPosition::WHILE_BEGIN) {
            printTabulation(nesting);
            infos()<<"while(";
            printCond(ins);
            infos()<<") {\n";
            ++nesting;
        //process do-while_end block
        } else if(currPos == BBPosition::DO_WHILE_END) {
            if(nesting > 1) --nesting;
            printTabulation(nesting);
            infos()<<"} ";
            infos()<<"while(";
            printCond(ins);
            infos()<<");\n";
        //process if-else condition
        } else {
            BasicBlock* first = i.getSuccessor(0);
            BasicBlock* second = i.getSuccessor(1);
            BasicBlock* pred = nullptr;
            printTabulation(nesting);
            infos()<<"if(";
            if(first->isUsedInBasicBlock(second)) {
                infos()<<"!(";
                std::swap(first,second);
                printCond(ins);
                infos()<<")";
            }
            else printCond(ins);
            infos()<<") {\n";
            ++nesting;
            bool isElse = false;
            isInIf = true;
            for(auto b = first; b != second; b= b->getNextNode()) {
                displayBasicBlock(*b, bbInfo.getPosition(*b));
                if(second->getPrevNode()) {
                    if(b == second->getPrevNode())
                        pred = b;
                }
                if(b == &b->getParent()->back()) break;

                if(succ_begin(b) != succ_end(b)) {
                    for(auto te = succ_begin(b);te != succ_end(b);++te) {
                        BasicBlock* temp = *te;
                        if(temp == bbInfo.getLoopHead(*b) && !bbInfo.isInGoto(*te.getSource()))
                        {
                            bbInfo.setInGoto(*te.getSource());
                            printTabulation(nesting);
                            infos()<<"goto "<<te->getName()<<";\n";
                            break;
                        }
                    }
                }
            }
            isInIf = false;
            if(nesting > 1) --nesting;
            printTabulation(nesting);
            infos()<<"}\n";
            if(pred){
                if(succ_begin(pred) != succ_end(pred)) {
                    pred = *succ_begin(pred);
                    if(second != pred){
                        for(auto b = second; ; b = b->getNextNode()) {
                            if( b == pred){
                                isElse = true;
                                break;
                            }
                            if(b == &b->getParent()->back()) break;
                        }
                    }
                }
            }
            if(isElse) {
                isInIf = true;
                printTabulation(nesting);
                infos()<<"else {\n";
                ++nesting;
                for(auto&& b = second; b != pred; b = b->getNextNode()) {
                    displayBasicBlock(*b, bbInfo.getPosition(*b));
                    if(succ_begin(b) != succ_end(b)) {
                        for(auto te = succ_begin(b); te != succ_end(b); ++te) {
                            BasicBlock* temp = *te;
                            if(temp == bbInfo.getLoopHead(*b) && !bbInfo.isInGoto(*te.getSource())) {
                                bbInfo.setInGoto(*te.getSource());
                                printTabulation(nesting + 1);
                                infos()<<"goto "<<te->getName()<<";\n";
                                break;
                            }
                        }
                    }
                }
                if(nesting > 1) --nesting;
                printTabulation(nesting);
                isInIf = false;
                infos()<<"}\n";
            }
        }
    } else {
        auto&& bb = i.getSuccessor(0);
        if(isOutGoto || (bb != i.getParent()->getNextNode() && bb != bbInfo.getLoopHead(*i.getParent()))) {
            bbInfo.setPrintName(*bb);
            printTabulation(nesting);
            infos()<<"goto "<<bb->getName()<<";\n";
        }
    }
}

void DecInstVisitor::visitSwitchInst(llvm::SwitchInst& i) {
    printPhiInstructions(*i.getParent());
    printTabulation(nesting);
    infos()<<"switch(";
    writeValueToStream(i.getCondition(), true);
    infos()<<") {\n";
    for(auto&& cs : i.cases()) {
        printTabulation(nesting);
        infos()<<"case ";
        writeValueToStream(cs.getCaseValue(), true);
        infos()<< ":\n";
        auto&& bb = cs.getCaseSuccessor();
        bool isOut = isOutGoto;
        isOutGoto = true;
        ++nesting;
        displayBasicBlock(*bb, bbInfo.getPosition(*bb));
        isOutGoto = isOut;
        for(auto&& sc = succ_begin(bb), sce = succ_end(bb); sc != sce; ++sc)
                bbInfo.setPrintName(**sc);
        if(nesting > 1) --nesting;
    }
    printTabulation(nesting);
    infos()<<"default:\n";
    bool isOut = isOutGoto;
    isOutGoto = true;
    ++nesting;
    displayBasicBlock(*i.getDefaultDest(), bbInfo.getPosition(*i.getDefaultDest()));
    isOutGoto = isOut;
    for(auto&& sc = succ_begin(i.getDefaultDest()), sce = succ_end(i.getDefaultDest()); sc != sce; ++sc)
            bbInfo.setPrintName(**sc);
    bbInfo.setPrintName(*i.getDefaultDest());
    if(nesting > 1) --nesting;
    printTabulation(nesting);
    infos()<<"}\n";
}

void DecInstVisitor::visitResumeInst(llvm::ResumeInst& i) {
    printTabulation(nesting);
    infos()<<"resume ";
    writeValueToStream(i.getValue(), true);
    infos()<<";\n";
}

void DecInstVisitor::visitUnreachableInst(llvm::UnreachableInst& i) {
    printTabulation(nesting);
    infos()<<"unreachable;\n";
}

void DecInstVisitor::visitPHINode(llvm::PHINode& phi) {/* Do nothing */}

void DecInstVisitor::visitSelectInst(llvm::SelectInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = (";
    writeValueToStream(i.getCondition(), true);
    infos()<<") ? ";
    writeValueToStream(i.getTrueValue(), true);
    infos()<<" : ";
    writeValueToStream(i.getFalseValue(), true);
    infos()<<";\n";
}


void DecInstVisitor::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = ";
    writeValueToStream(i.getPointerOperand(), true);
    for(auto&& it = i.idx_begin(); it != i.idx_end(); ++it) {
        if(it == i.idx_begin()) continue;
        infos()<<"[";
        writeValueToStream(it->get(), true);
        infos()<<"]";
    }
    infos()<<";\n";
}


void DecInstVisitor::visitExtractValueInst(llvm::ExtractValueInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = ";
    writeValueToStream(i.getAggregateOperand(), true);
    for (auto&& it : i.getIndices().vec()) {
        infos()<<"["<<it<<"]";
    }
    infos()<<";\n";
}

void DecInstVisitor::visitInsertValueInst(llvm::InsertValueInst& i) {
    printTabulation(nesting);
    if(!i.getType()->isVoidTy()) {
        writeValueToStream(&i, true);
        infos()<<" = ";
    }
    writeValueToStream(i.getAggregateOperand(), true);
    for(auto&& it : i.getIndices().vec()) {
        infos()<<"["<<it<<"]";
    }
    infos()<<" = ";
    writeValueToStream(i.getInsertedValueOperand(), true);
    infos()<<";\n";
}

void DecInstVisitor::visitExtractElementInst(llvm::ExtractElementInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos()<<" = ";
    writeValueToStream(i.getVectorOperand(), true);
    infos()<<"[";
    writeValueToStream(i.getIndexOperand(), true);
    infos()<<"];\n";
}

void DecInstVisitor::visitInsertElementInst(llvm::InsertElementInst& i){
    printTabulation(nesting);
    if(!i.getType()->isVoidTy()) {
        writeValueToStream(&i);
        infos()<<" = ";
    }
    writeValueToStream(i.getOperand(0), true);
    infos()<<"[";
    writeValueToStream(i.getOperand(2), true);
    infos()<<"] = ";
    writeValueToStream(i.getOperand(1), true);
    infos()<<";\n";
}

void DecInstVisitor::visitReturnInst(llvm::ReturnInst &i) {
    printTabulation(nesting);
    infos()<<"return";
    for(auto&& op : i.operand_values()){
        infos()<<" ";
        writeValueToStream(op, true);
    }
    infos()<<";\n";
}



void DecInstVisitor::displayGlobals(llvm::Module &M) {
    for(auto&& it : M.globals()) {
        if(it.isConstant()) infos()<<"const ";
        infos()<<"global ";
        writeValueToStream(&it);
        if(it.hasInitializer()) {
            infos()<<" = ";
            writeValueToStream(it.getInitializer(), true);
        }
        infos()<<";\n";
    }
    infos()<<"\n\n";
}

void DecInstVisitor::displayFunction(llvm::Function &F) {
    printType(F.getReturnType());
    infos()<<" "<<F.getName().str()+"(";
    int argNum = 0;
    for(auto&& arg : F.args()) ++argNum;
    int num = 0;
    auto&& ll=F.arg_begin();
    auto&& lz=F.arg_end();
    while(ll != lz) {
        writeValueToStream(ll);
        if(num < argNum - 1) infos()<<", ";
        ++ll;
        ++num;
    }
    infos()<<")";
    nesting = 1;
    isInIf = false;
    isOutGoto = false;
    while(!conditions.empty()) conditions.pop();
    currPos = BBPosition::NONE;
}

void DecInstVisitor::displayBasicBlock(llvm::BasicBlock& B, BBPosition position) {
    if(!bbInfo.isVisited(B)){
        if(bbInfo.isPrintName(B)) infos()<<B.getName()<<":\n";
        bbInfo.setVisited(B);
        switch(position) {
            case BBPosition::WHILE_BEGIN:
                for(auto&& i = B.begin(), e = B.end(); i != e; ++i) {
                    if(i->getOpcode() == Instruction::Load) conditions.push(i);
                }
                break;
            case BBPosition::DO_WHILE_BEGIN:
                printTabulation(nesting);
                infos()<<"do {\n";
                ++nesting;
                break;
            default: break;
        }
        for(auto&& BI = B.begin(), BE = B.end(); BI != BE; ++BI) {
            string s = BI->getOpcodeName();
            //nesting = depth;
            currPos = position;
            visit(BI);
        }
        if(position == BBPosition::WHILE_END) {
            if(conditions.size() != 0) {
                printTabulation(nesting);
                writeValueToStream(conditions.top(), true);
                infos()<<" = *";
                writeValueToStream(conditions.top()->getOperand(0), true);
                infos()<<";\n";
                conditions.pop();
            }
            if(nesting > 1) --nesting;
            printTabulation(nesting);
            infos()<<"}\n";
        }
    }
}

void DecInstVisitor::displayStructs() {
    for(auto&& it : usedStructs) {
        infos()<<it->getName()<<" {\n";
        for(auto i = 0U; i < it->getStructNumElements(); ++i) {
            infos()<<"\t"<<i<<" - ";
            printType(it->getStructElementType(i));
            infos()<<";\n";
        }
        infos()<<"};\n\n";
    }
}

void DecInstVisitor::setBasicBlockInfo(BasicBlockInformation& inf) {
    bbInfo = inf;
}

void DecInstVisitor::setPhiInfo(PhiNodeInformation& phi) {
    phiInfo = phi;
}

} /* namespace decompiler */
} /* namespace borealis */
