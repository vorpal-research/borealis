/*
 * PrinterPasses.cpp
 *
 *  Created on: Jan 24, 2013
 *      Author: belyaev
 */

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/CallGraphSCCPass.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "Logging/logger.hpp"
#include "Util/util.h"


// gracefully stolen from opt.cpp
namespace borealis {

using namespace llvm;

struct CallGraphSCCPassPrinter : public CallGraphSCCPass {
    static char ID;
    const PassInfo *PassToPrint;
    std::string PassName;

    CallGraphSCCPassPrinter(const PassInfo *PI)
    : CallGraphSCCPass(ID), PassToPrint(PI) {
        std::string PassToPrintName =  PassToPrint->getPassName();
        PassName = "CallGraphSCCPass Printer: " + PassToPrintName;
    }

    virtual bool runOnSCC(CallGraphSCC &SCC) {
        infos() << "Printing analysis '" << PassToPrint->getPassName() << "':\n";

        // Get and print pass...
        for (CallGraphSCC::iterator I = SCC.begin(), E = SCC.end(); I != E; ++I) {
            Function *F = (*I)->getFunction();

            std::string buf;
            raw_string_ostream Out(buf);
            if (F)
                getAnalysisID<Pass>(PassToPrint->getTypeInfo())
                .print(Out, F->getParent());
            infos() << Out.str();
        }
        return false;
    }

    virtual const char *getPassName() const { return PassName.c_str(); }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequiredID(PassToPrint->getTypeInfo());
        AU.setPreservesAll();
    }
};
char CallGraphSCCPassPrinter::ID = 0;

struct ModulePassPrinter : public ModulePass {
    static char ID;
    const PassInfo *PassToPrint;
    std::string PassName;

    ModulePassPrinter(const PassInfo *PI)
    : ModulePass(ID), PassToPrint(PI) {
        std::string PassToPrintName =  PassToPrint->getPassName();
        PassName = "ModulePass Printer: " + PassToPrintName;
    }

    virtual bool runOnModule(Module &M) {
        infos() << "Printing analysis '" << PassToPrint->getPassName() << "':\n";

        std::string buf;
        raw_string_ostream Out(buf);
        // Get and print pass...
        getAnalysisID<Pass>(PassToPrint->getTypeInfo())
        .print(Out, &M);
        infos() << Out.str();
        return false;
    }

    virtual const char *getPassName() const { return PassName.c_str(); }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequiredID(PassToPrint->getTypeInfo());
        AU.setPreservesAll();
    }
};
char ModulePassPrinter::ID = 0;

struct FunctionPassPrinter : public FunctionPass {
    const PassInfo *PassToPrint;
    static char ID;
    std::string PassName;

    FunctionPassPrinter(const PassInfo *PI)
    : FunctionPass(ID), PassToPrint(PI) {
        std::string PassToPrintName =  PassToPrint->getPassName();
        PassName = "FunctionPass Printer: " + PassToPrintName;
    }

    virtual bool runOnFunction(Function &F) {
        infos() << "Printing analysis '" << PassToPrint->getPassName()
                << "' for function '" << F.getName() << "':\n";

        std::string buf;
        raw_string_ostream Out(buf);
        // Get and print pass...
        getAnalysisID<Pass>(PassToPrint->getTypeInfo())
        .print(Out, F.getParent());
        infos() << Out.str();
        return false;
    }

    virtual const char *getPassName() const { return PassName.c_str(); }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequiredID(PassToPrint->getTypeInfo());
        AU.setPreservesAll();
    }
};
char FunctionPassPrinter::ID = 0;

struct LoopPassPrinter : public LoopPass {
    static char ID;
    const PassInfo *PassToPrint;
    std::string PassName;

    LoopPassPrinter(const PassInfo *PI)
    : LoopPass(ID), PassToPrint(PI) {
        std::string PassToPrintName =  PassToPrint->getPassName();
        PassName = "LoopPass Printer: " + PassToPrintName;
    }

    virtual bool runOnLoop(Loop *L, LPPassManager &) {
        infos() << "Printing analysis '" << PassToPrint->getPassName() << "':\n";

        std::string buf;
        raw_string_ostream Out(buf);
        // Get and print pass...
        getAnalysisID<Pass>(PassToPrint->getTypeInfo())
        .print(Out, L->getHeader()->getParent()->getParent());
        infos() << Out.str();
        return false;
    }

    virtual const char *getPassName() const { return PassName.c_str(); }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequiredID(PassToPrint->getTypeInfo());
        AU.setPreservesAll();
    }
};
char LoopPassPrinter::ID = 0;

struct RegionPassPrinter : public RegionPass {
    static char ID;
    const PassInfo *PassToPrint;
    std::string PassName;

    RegionPassPrinter(const PassInfo *PI)
    : RegionPass(ID), PassToPrint(PI) {
        std::string PassToPrintName =  PassToPrint->getPassName();
        PassName = "RegionPass Printer: " + PassToPrintName;
    }

    virtual bool runOnRegion(Region *R, RGPassManager &) {
        infos() << "Printing analysis '" << PassToPrint->getPassName() << "' for "
                << "region: '" << R->getNameStr() << "' in function '"
                << R->getEntry()->getParent()->getName() << "':\n";

        std::string buf;
        raw_string_ostream Out(buf);
        // Get and print pass...
        getAnalysisID<Pass>(PassToPrint->getTypeInfo())
        .print(Out, R->getEntry()->getParent()->getParent());
        infos() << Out.str();
        return false;
    }

    virtual const char *getPassName() const { return PassName.c_str(); }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequiredID(PassToPrint->getTypeInfo());
        AU.setPreservesAll();
    }
};
char RegionPassPrinter::ID = 0;

struct BasicBlockPassPrinter : public BasicBlockPass {
    const PassInfo *PassToPrint;
    static char ID;
    std::string PassName;

    BasicBlockPassPrinter(const PassInfo *PI)
    : BasicBlockPass(ID), PassToPrint(PI) {
        std::string PassToPrintName =  PassToPrint->getPassName();
        PassName = "BasicBlockPass Printer: " + PassToPrintName;
    }

    virtual bool runOnBasicBlock(BasicBlock &BB) {
        infos() << "Printing Analysis info for BasicBlock '" << BB.getName()
                << "': Pass " << PassToPrint->getPassName() << ":\n";

        std::string buf;
        raw_string_ostream Out(buf);
        // Get and print pass...
        getAnalysisID<Pass>(PassToPrint->getTypeInfo())
        .print(Out, BB.getParent()->getParent());
        infos() << Out.str();
        return false;
    }

    virtual const char *getPassName() const { return PassName.c_str(); }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequiredID(PassToPrint->getTypeInfo());
        AU.setPreservesAll();
    }
};
char BasicBlockPassPrinter::ID = 0;

Pass* createPrinterFor(const PassInfo* PassInf, Pass* pass) {
    switch(pass->getPassKind()) {
    case PT_BasicBlock:
      return new BasicBlockPassPrinter(PassInf);
      break;
    case PT_Region:
      return new RegionPassPrinter(PassInf);
      break;
    case PT_Loop:
      return new LoopPassPrinter(PassInf);
      break;
    case PT_Function:
      return new FunctionPassPrinter(PassInf);
      break;
    case PT_CallGraphSCC:
      return new CallGraphSCCPassPrinter(PassInf);
      break;
    default:
      return new ModulePassPrinter(PassInf);
      break;
    }
}

} /* namespace borealis */
