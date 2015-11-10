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

#include "Codegen/intrinsics_manager.h"
#include "State/Transformer/AnnotationSubstitutor.h"
#include "Util/streams.hpp"

using namespace llvm;
using namespace std;

namespace borealis {
namespace decompiler{

const std::map<llvm::Type::TypeID, std::string> types = {
        {llvm::Type::TypeID::IntegerTyID, "int"},
        {llvm::Type::TypeID::DoubleTyID, "double"},
        {llvm::Type::TypeID::FP128TyID, "long double"},
        {llvm::Type::TypeID::FloatTyID, "float"},
        {llvm::Type::TypeID::HalfTyID, "float16"},
        {llvm::Type::TypeID::PPC_FP128TyID, "long double"},
        {llvm::Type::TypeID::X86_FP80TyID, "float80"},
        {llvm::Type::TypeID::FunctionTyID, "function"},
        {llvm::Type::TypeID::LabelTyID, "label"},
        {llvm::Type::TypeID::MetadataTyID, "metadata"},
        {llvm::Type::TypeID::VoidTyID, "void"}
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
    for(int i = 0; i < num; ++i) infos_ << "    ";
}

void DecInstVisitor::printType(llvm::Type* t) {
    auto&& it = types.find(t->getTypeID());
    if(it != types.end()) {
        if(t->isIntegerTy()){
            if(intTypes.find(t->getIntegerBitWidth()) != intTypes.end()) infos_ << intTypes[t->getIntegerBitWidth()];
            else infos_ << "int"<<t->getIntegerBitWidth();
        }
        else infos_ << it->second;
        return;
    } else if(t->isPointerTy()) {
        printType(t->getPointerElementType());
        infos_ << "*";
        return;
    }
    else if(t->isArrayTy()) {
        printType(t->getArrayElementType());
        infos_ << "["<<t->getArrayNumElements()<<"]";
        return;
    } else if(t->isStructTy()) {
        auto&& st = cast<llvm::StructType>(t);
        if(!st->isLiteral()) {
            infos_ << st->getStructName();
        }
        return;
    } else if(t->isVectorTy()) {
        infos_ << "vector<";
        printType(t->getVectorElementType());
        infos_ << ">("<<t->getVectorNumElements()<<")";
        return;
    } else {
        std::string temp = "";
        llvm::raw_string_ostream rs(temp);
        t->print(rs);
        infos_ << rs.str();
        return;
    }
}

void DecInstVisitor::printCond(llvm::Instruction* ins){
    if(auto&& ici = dyn_cast<ICmpInst>(ins)) {
        writeValueToStream(ins->getOperand(0), true);
        infos_ << " "<<icmpOp.find(ici->getSignedPredicate())->second<<" ";
        writeValueToStream(ins->getOperand(1), true);
    } else if(isa<FCmpInst>(ins)) {
        writeValueToStream(ins->getOperand(0), true);
        infos_ << " "<<fcmpOp.find(ici->getPredicate())->second<<" ";
        writeValueToStream(ins->getOperand(1), true);
    } else if(auto&& bi = dyn_cast<BranchInst>(ins)) { // inst not found
        writeValueToStream(bi->getCondition(), true);
    } else writeValueToStream(ins, true);
}

void DecInstVisitor::writeValueToStream(llvm::Value* v, bool deleteType) {
    if(!deleteType) {
        printType(v->getType());
        infos_ << " ";
    }
    isStruct(v->getType()); // wtf???

    if(auto I = llvm::dyn_cast<llvm::Instruction>(v)) {
        auto ST = STP->getSlotTracker(I);
        infos_ << ST->getLocalName(I);
        return;
    }

    if(auto ci = llvm::dyn_cast<ConstantInt>(v)) {
        infos_ << ci->getValue().getLimitedValue();
        return;
    }

    if(llvm::isa<ConstantAggregateZero>(v)) {
        infos_ << "{0}";
        return;
    }

    if(llvm::isa<ConstantPointerNull>(v)) {
        infos_ << "nullptr";
        return;
    }


    if(auto cs = llvm::dyn_cast<ConstantDataSequential>(v)) {
        if(cs->isString() || cs->isCString()) {
            std::string res;
            llvm::raw_string_ostream rso(res);
            rso << "\"";
            rso.write_escaped(cs->getRawDataValues(), true);
            rso << "\"";
            infos_ << rso.str();
        }
        else {
            infos_ << "{";
            writeValueToStream(cs->getAggregateElement(0U));
            for(size_t i = 1; i < cs->getNumElements(); ++i) {
                infos_ << ",";
                writeValueToStream(cs->getAggregateElement(i), true);
            }
            infos_ << "}";
        }
        return;
    }

    if(auto cs = llvm::dyn_cast<GEPOperator>(v)) {
        visitGEPOperator(*cs);
        return;
    }

    if(auto cst = llvm::dyn_cast<PtrToIntOperator>(v)) {
        visitPtrToIntOperator(*cst);
        return;
    }

    //if value don't have name, it's very bad
    if(v->getName().empty()) {
        std::string temp = "";
        llvm::raw_string_ostream rs(temp);
        v->printAsOperand(rs);

        infos_ << temp;
    } else infos_ << v->getName();
}

void DecInstVisitor::printPhiInstructions(llvm::BasicBlock& bb) {
    auto&& phi = phiInfo.getBlockEntries(&bb);
    for(auto&& i : phi) {
        printTabulation(nesting);
        writeValueToStream(i.value);
        infos_ << " = ";
        writeValueToStream(i.initializer, true);
        infos_ << ";\n";
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
    infos_ << i.getOpcodeName()<<"\n";
    infos_ << i<<";\n";
}


void DecInstVisitor::visitStoreInst(llvm::StoreInst &i) {
    printTabulation(nesting);
    infos_ << "*";
    writeValueToStream(i.getPointerOperand(), true);
    infos_ << " = ";
    writeValueToStream(i.getValueOperand(), true);
    infos_ << ";\n";
}

void DecInstVisitor::visitAllocaInst(llvm::AllocaInst &i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = alloca(";
    printType(i.getAllocatedType());
    infos_ << ");\n";
}

void DecInstVisitor::visitLoadInst(LoadInst &i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = *";
    writeValueToStream(i.getPointerOperand(), true);
    infos_ << ";\n";
}


void DecInstVisitor::visitBinaryOperator(llvm::BinaryOperator &i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = ";
    writeValueToStream(i.getOperand(0), true);
    infos_ << " "<<binOp.find(i.getOpcode())->second<<" ";
    writeValueToStream(i.getOperand(1), true);
    infos_ << ";\n";
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
        infos_ << " = (";
        writeValueToStream(i.getOperand(0), true);
        infos_ << " "<<icmpOp.find(i.getSignedPredicate())->second<<" ";
        writeValueToStream(i.getOperand(1), true);
        infos_ << ");\n";
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
        infos_ << " = (";
        writeValueToStream(i.getOperand(0), true);
        infos_ << " "<<fcmpOp.find(i.getPredicate())->second<<" ";
        writeValueToStream(i.getOperand(1), true);
        infos_ << ");\n";
    }
}

void DecInstVisitor::visitCastInst(llvm::CastInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = cast<";
    printType(i.getDestTy());
    infos_ << ">(";
    writeValueToStream(i.getOperand(0), true);
    infos_ << ");\n";
}


void DecInstVisitor::visitPtrToIntOperator(llvm::PtrToIntOperator& i) {
    infos_ << "cast<";
    printType(i.getType());
    infos_ << ">(";
    writeValueToStream(i.getPointerOperand(), true);
    infos_ << ")";
}

void DecInstVisitor::visitCallInst(llvm::CallInst &i) {
    static auto&& im = IntrinsicsManager::getInstance();

    if(im.getIntrinsicType(i) != function_type::UNKNOWN) {
        auto type = im.getIntrinsicType(i);
        switch(type) {
        case function_type::INTRINSIC_NONDET: {
            printTabulation(nesting);
            writeValueToStream(&i);
            infos_ << " = (?);\n";
            return;
        }
        case function_type::INTRINSIC_ANNOTATION: {
            auto&& anno = substituteAnnotationCall(*FN, &i);
            printTabulation(nesting);
            infos_ << "@" << anno->getKeyword() << anno->argToString() << " // " << anno->getLocus() << "\n";
            return;
        }
        case function_type::INTRINSIC_CONSUME:
        case function_type::INTRINSIC_UNREACHABLE:
        case function_type::INTRINSIC_VALUE:
        case function_type::INTRINSIC_DECLARE:
        case function_type::INTRINSIC_GLOBAL:
            return;
        case function_type::ACTION_DEFECT: {
            printTabulation(nesting);
            infos_ << "!defect(";
            writeValueToStream(i.getArgOperand(0), true);
            infos_ << ");\n";
            return;
        }
        case function_type::BUILTIN_BOR_ASSERT: {
            infos_ << "@assert (";
            writeValueToStream(i.getArgOperand(0), true);
            infos_ << ");\n";
            return;
        }
        case function_type::BUILTIN_BOR_ASSUME: {
            infos_ << "@assume (";
            writeValueToStream(i.getArgOperand(0), true);
            infos_ << ");\n";
            return;
        }
        default:
            break;
        }
    }

    printTabulation(nesting);
    if(!i.getType()->isVoidTy()) {
        writeValueToStream(&i);
        infos_ << " = ";
    }

    if(i.getCalledFunction() != nullptr) {
        infos_ << i.getCalledFunction()->getName()<<"(";
    } else { // this is an indirect call
        infos_ << "(*";
        writeValueToStream(i.getCalledValue());
        infos_ << ")(";
    }

    int arg_num = std::distance(i.arg_operands().begin(), i.arg_operands().end());
    int num = 0;
    for(auto&& op : i.arg_operands()) {
        writeValueToStream(op, true);
        if(num < arg_num - 1) {
            infos_ << ", ";
            ++num;
        }
    }
    infos_ << ");\n";

}

void DecInstVisitor::visitInvokeInst(llvm::InvokeInst &i) {
    if(i.getCalledFunction() != nullptr) {
        printTabulation(nesting);
        if(!i.getType()->isVoidTy()) {
            writeValueToStream(&i);
            infos_ << " = ";
        }
        infos_ << "invoke ";
        infos_ << i.getCalledFunction()->getName()<<"(";
        for(auto j = 0U; j < i.getNumArgOperands(); ++j) {
            i.getArgOperand(j)->clearSubclassOptionalData();
            writeValueToStream(i.getArgOperand(j), true);
            if(j < i.getNumArgOperands() - 1) infos_ << ", ";
        }
        infos_ << ") to "<<i.getNormalDest()->getName()<<", unwind "<<i.getUnwindDest()->getName()<<";\n";
        bbInfo.setPrintName(*i.getNormalDest());
        bbInfo.setPrintName(*i.getUnwindDest());
    }
}

void DecInstVisitor::visitLandingPadInst(llvm::LandingPadInst &i) {
    printTabulation(nesting);
    if(!i.getType()->isVoidTy()) {
        writeValueToStream(&i, true);
        infos_ << " = ";
    }
    infos_ << "landingpad ";
    std::string temp = "";
    llvm::raw_string_ostream rs(temp);
    i.getPersonalityFn()->printAsOperand(rs);
    auto nameStart = rs.str().find('@');
    if(nameStart < temp.size()) {
        temp.erase(0,nameStart);
        for(nameStart = 0; nameStart < temp.size(); ++nameStart) {
            if(temp[nameStart] == ' ') {
                temp.erase(nameStart, temp.size() - nameStart);
                break;
            }
        }
    }
    infos_ << temp;
    infos_ << " {\n";
    if(i.isCleanup()) {
        printTabulation(nesting + 1);
        infos_ << "cleanup;\n";
    }
    else {
        for(auto j = 0U; j < i.getNumClauses(); ++j) {
            printTabulation(nesting + 1);
            i.getClause(j)->clearSubclassOptionalData();
            if(i.isCatch(j)) infos_ << "catch ";
            else infos_ << "filter ";
            writeValueToStream(i.getClause(j));
            infos_ << ";\n";
        }
    }
    printTabulation(nesting);
    infos_ << "}\n";
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
            infos_ << "while(";
            printCond(ins);
            infos_ << ") {\n";
            ++nesting;
        //process do-while_end block
        } else if(currPos == BBPosition::DO_WHILE_END) {
            if(nesting > 1) --nesting;
            printTabulation(nesting);
            infos_ << "} ";
            infos_ << "while(";
            printCond(ins);
            infos_ << ");\n";
        //process if-else condition
        } else {
            BasicBlock* first = i.getSuccessor(0);
            BasicBlock* second = i.getSuccessor(1);
            BasicBlock* pred = nullptr;
            printTabulation(nesting);
            infos_ << "if(";
            if(first->isUsedInBasicBlock(second)) {
                infos_ << "!(";
                std::swap(first,second);
                printCond(ins);
                infos_ << ")";
            }
            else printCond(ins);
            infos_ << ") {\n";
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
                            infos_ << "goto "<<te->getName()<<";\n";
                            break;
                        }
                    }
                }
            }
            isInIf = false;
            if(nesting > 1) --nesting;
            printTabulation(nesting);
            infos_ << "}\n";
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
                infos_ << "else {\n";
                ++nesting;
                for(auto&& b = second; b != pred; b = b->getNextNode()) {
                    displayBasicBlock(*b, bbInfo.getPosition(*b));
                    if(succ_begin(b) != succ_end(b)) {
                        for(auto te = succ_begin(b); te != succ_end(b); ++te) {
                            BasicBlock* temp = *te;
                            if(temp == bbInfo.getLoopHead(*b) && !bbInfo.isInGoto(*te.getSource())) {
                                bbInfo.setInGoto(*te.getSource());
                                printTabulation(nesting + 1);
                                infos_ << "goto "<<te->getName()<<";\n";
                                break;
                            }
                        }
                    }
                }
                if(nesting > 1) --nesting;
                printTabulation(nesting);
                isInIf = false;
                infos_ << "}\n";
            }
        }
    } else {
        auto&& bb = i.getSuccessor(0);
        if(isOutGoto || (bb != i.getParent()->getNextNode() && bb != bbInfo.getLoopHead(*i.getParent()))) {
            bbInfo.setPrintName(*bb);
            printTabulation(nesting);
            infos_ << "goto "<<bb->getName()<<";\n";
        }
    }
}

void DecInstVisitor::visitSwitchInst(llvm::SwitchInst& i) {
    printPhiInstructions(*i.getParent());
    printTabulation(nesting);
    infos_ << "switch(";
    writeValueToStream(i.getCondition(), true);
    infos_ << ") {\n";
    for(auto&& cs : i.cases()) {
        printTabulation(nesting);
        infos_ << "case ";
        writeValueToStream(cs.getCaseValue(), true);
        infos_ <<  ":\n";
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
    infos_ << "default:\n";
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
    infos_ << "}\n";
}

void DecInstVisitor::visitResumeInst(llvm::ResumeInst& i) {
    printTabulation(nesting);
    infos_ << "resume ";
    writeValueToStream(i.getValue(), true);
    infos_ << ";\n";
}

void DecInstVisitor::visitUnreachableInst(llvm::UnreachableInst&) {
    printTabulation(nesting);
    infos_ << "unreachable;\n";
}

void DecInstVisitor::visitPHINode(llvm::PHINode&) {/* Do nothing */}

void DecInstVisitor::visitSelectInst(llvm::SelectInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = (";
    writeValueToStream(i.getCondition(), true);
    infos_ << ") ? ";
    writeValueToStream(i.getTrueValue(), true);
    infos_ << " : ";
    writeValueToStream(i.getFalseValue(), true);
    infos_ << ";\n";
}

void DecInstVisitor::visitGEPOperator(llvm::GEPOperator& i) {
    infos_ << "&";
    writeValueToStream(i.getPointerOperand(), true);
    for(auto&& it = i.idx_begin(); it != i.idx_end(); ++it) {
        infos_ << "[";
        writeValueToStream(it->get(), true);
        infos_ << "]";
    }
}

void DecInstVisitor::visitGetElementPtrInst(llvm::GetElementPtrInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = ";
    visitGEPOperator(llvm::cast<GEPOperator>(i));
    infos_ << ";\n";
}


void DecInstVisitor::visitExtractValueInst(llvm::ExtractValueInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = ";
    writeValueToStream(i.getAggregateOperand(), true);
    for (auto&& it : i.getIndices().vec()) {
        infos_ << "["<<it<<"]";
    }
    infos_ << ";\n";
}

void DecInstVisitor::visitInsertValueInst(llvm::InsertValueInst& i) {
    printTabulation(nesting);
    if(!i.getType()->isVoidTy()) {
        writeValueToStream(&i, true);
        infos_ << " = ";
    }
    writeValueToStream(i.getAggregateOperand(), true);
    for(auto&& it : i.getIndices().vec()) {
        infos_ << "["<<it<<"]";
    }
    infos_ << " = ";
    writeValueToStream(i.getInsertedValueOperand(), true);
    infos_ << ";\n";
}

void DecInstVisitor::visitExtractElementInst(llvm::ExtractElementInst& i) {
    printTabulation(nesting);
    writeValueToStream(&i);
    infos_ << " = ";
    writeValueToStream(i.getVectorOperand(), true);
    infos_ << "[";
    writeValueToStream(i.getIndexOperand(), true);
    infos_ << "];\n";
}

void DecInstVisitor::visitInsertElementInst(llvm::InsertElementInst& i){
    printTabulation(nesting);
    if(!i.getType()->isVoidTy()) {
        writeValueToStream(&i);
        infos_ << " = ";
    }
    writeValueToStream(i.getOperand(0), true);
    infos_ << "[";
    writeValueToStream(i.getOperand(2), true);
    infos_ << "] = ";
    writeValueToStream(i.getOperand(1), true);
    infos_ << ";\n";
}

void DecInstVisitor::visitReturnInst(llvm::ReturnInst &i) {
    printTabulation(nesting);
    infos_ << "return";
    for(auto&& op : i.operand_values()){
        infos_ << " ";
        writeValueToStream(op, true);
    }
    infos_ << ";\n";
}



void DecInstVisitor::displayGlobals(llvm::Module &M) {
    for(auto&& it : M.globals()) {
        if(it.isConstant()) infos_ << "const ";
        infos_ << "global ";
        writeValueToStream(&it);
        if(it.hasInitializer()) {
            infos_ << " = ";
            writeValueToStream(it.getInitializer(), true);
        }
        infos_ << ";\n";
    }
    infos_ << "\n\n";
}

bool DecInstVisitor::displayFunction(llvm::Function &F) {
    static auto&& im = IntrinsicsManager::getInstance();
    if(im.getIntrinsicType(&F) != function_type::UNKNOWN) return false;
    if(F.isIntrinsic()) return false;

    printType(F.getReturnType());
    infos_ << " "<<F.getName().str()+"(";
    size_t argNum = F.arg_size();
    size_t num = 0;
    auto&& ll=F.arg_begin();
    auto&& lz=F.arg_end();
    while(ll != lz) {
        writeValueToStream(ll);
        if(num < argNum - 1) infos_ << ", ";
        ++ll;
        ++num;
    }
    infos_ << ")";
    nesting = 1;
    isInIf = false;
    isOutGoto = false;
    while(!conditions.empty()) conditions.pop();
    currPos = BBPosition::NONE;
    FN = util::make_unique<FactoryNest>(F.getDataLayout(), STP->getSlotTracker(F));
    return true;
}

void DecInstVisitor::displayBasicBlock(llvm::BasicBlock& B, BBPosition position) {
    if(!bbInfo.isVisited(B)){
        if(bbInfo.isPrintName(B)) infos_ << B.getName()<<":\n";
        bbInfo.setVisited(B);
        switch(position) {
            case BBPosition::WHILE_BEGIN:
                for(auto&& i = B.begin(), e = B.end(); i != e; ++i) {
                    if(i->getOpcode() == Instruction::Load) conditions.push(i);
                }
                break;
            case BBPosition::DO_WHILE_BEGIN:
                printTabulation(nesting);
                infos_ << "do {\n";
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
                infos_ << " = *";
                writeValueToStream(conditions.top()->getOperand(0), true);
                infos_ << ";\n";
                conditions.pop();
            }
            if(nesting > 1) --nesting;
            printTabulation(nesting);
            infos_ << "}\n";
        }
    }
}

void DecInstVisitor::displayStructs() {
    for(auto&& it : usedStructs) {
        infos_ << it->getName()<<" {\n";
        for(auto i = 0U; i < it->getStructNumElements(); ++i) {
            infos_ << "\t"<<i<<" - ";
            printType(it->getStructElementType(i));
            infos_ << ";\n";
        }
        infos_ << "};\n\n";
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
