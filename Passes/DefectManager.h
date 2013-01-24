/*
 * DefectManager.h
 *
 *  Created on: Jan 24, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFECTMANAGER_H_
#define DEFECTMANAGER_H_

#include <llvm/Instructions.h>
#include <llvm/Pass.h>

#include <set>
#include <tuple>

#include <Util/locations.h>

namespace borealis {

enum class DefectType {
    INI_03
};

class DefectManager: public llvm::ModulePass {

public:

    typedef std::set< std::pair<DefectType, const Locus> > DefectData;
    typedef DefectData::value_type DefectDataEntry;

    static char ID;

    DefectManager();
    virtual bool runOnModule(llvm::Module& /*M*/) { return false; }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual ~DefectManager() {};

    void addDefect(DefectType type, llvm::Instruction* where);

private:

    DefectData data;

};

} /* namespace borealis */

#endif /* DEFECTMANAGER_H_ */
