//
// Created by belyaev on 4/20/15.
//

#ifndef TERM_VALUE_MAPPER_H
#define TERM_VALUE_MAPPER_H

#include <tinyformat/tinyformat.h>

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class TermValueMapper: public Transformer<TermValueMapper> {

    std::map<Term::Ptr, const llvm::Value*, TermCompare> mapping; // the ordering matters!!!
    const llvm::Instruction* context;
public:
    TermValueMapper(const FactoryNest& FN, const llvm::Instruction* context) : Transformer(FN), context(context) { }

    Term::Ptr transformValueTerm(ValueTermPtr trm);

    Term::Ptr transformArgumentTerm(ArgumentTermPtr trm);

    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr trm);

    const std::map<Term::Ptr, const llvm::Value*, TermCompare>& getMapping() const;

};


} /* namespace borealis */



#endif /* TERM_VALUE_MAPPER_H */

