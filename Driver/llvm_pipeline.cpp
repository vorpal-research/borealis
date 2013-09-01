/*
 * llvm_pipeline.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#include <llvm/InitializePasses.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetData.h>

#include "Driver/llvm_pipeline.h"
#include "Passes/Misc/PrinterPasses.h"

namespace borealis {
namespace driver {

namespace {

llvm::PassRegistry& initPassRegistry() {
    using namespace llvm;
    static bool didInit = false;
    static auto& reg = *PassRegistry::getPassRegistry();
    if (!didInit) {
        didInit = true;
        // initialize passes
        initializeCore(reg);
        initializeScalarOpts(reg);
        initializeIPO(reg);
        initializeAnalysis(reg);
        initializeIPA(reg);
        initializeTransformUtils(reg);
        initializeInstCombine(reg);
        initializeInstrumentation(reg);
        initializeTarget(reg);
    }
    return reg;
}

} // namespace

struct llvm_pipeline::impl {
    llvm::PassManager pm;
    std::shared_ptr<llvm::Module> module;

    impl(const std::shared_ptr<llvm::Module>& m): pm{}, module{m} {};
};

llvm_pipeline::~llvm_pipeline() {};

llvm_pipeline::llvm_pipeline(const std::shared_ptr<llvm::Module>& m)
    : pimpl{ new impl{m} } {
    pimpl->pm.add(new llvm::TargetData(m.get()));
};

void llvm_pipeline::addPass(llvm::Pass* pass) {
    pimpl->pm.add(pass);
}

void llvm_pipeline::add(const std::string& pname) {
    auto& reg = initPassRegistry();
    llvm::StringRef pass = pname;

    bool isPrinterPass = pass.endswith("-printer");
    llvm::StringRef passName = isPrinterPass ? pass.drop_back(8) : pass;

    auto* passInfo = reg.getPassInfo(passName);
    if (passInfo == nullptr) {
        errs() << "Pass " << passName << " cannot be found" << endl;
        return;
    }

    infos() << pass << ": " << passInfo->getPassName() << endl;

    if (auto* ctor = passInfo->getNormalCtor()) {
        auto thePass = ctor();
        pimpl->pm.add(thePass);
        if (isPrinterPass) {
            pimpl->pm.add(createPrinterFor(passInfo, thePass));
        }
    } else {
        errs() << "Could not create pass " << passInfo->getPassName() << endl;
    }
}

llvm_pipeline::status llvm_pipeline::run() {
    if (pimpl->pm.run(*pimpl->module)) return status::SUCCESS;
    else return status::FAILURE;
}

} // namespace driver
} // namespace borealis
