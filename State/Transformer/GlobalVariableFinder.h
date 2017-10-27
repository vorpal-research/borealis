//
// Created by abdullin on 10/26/17.
//

#ifndef BOREALIS_GLOBALVARIABLEFINDER_H
#define BOREALIS_GLOBALVARIABLEFINDER_H

#include "Transformer.hpp"

namespace borealis {

class GlobalVariableFinder : public Transformer<GlobalVariableFinder> {
public:
    using Base = Transformer<GlobalVariableFinder>;

    GlobalVariableFinder(FactoryNest FN);

    Term::Ptr transformValueTerm(ValueTermPtr term);
    const std::unordered_set<const llvm::Value*> getGlobals() const;

private:

    std::unordered_set<const llvm::Value*> globals;
};

}   // namespace borealis


#endif //BOREALIS_GLOBALVARIABLEFINDER_H
