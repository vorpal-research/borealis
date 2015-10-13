//
// Created by ice-phoenix on 9/9/15.
//

#ifndef SANDBOX_RETYPER_H
#define SANDBOX_RETYPER_H

#include "State/Transformer/CachingTransformer.hpp"

namespace borealis {

class Retyper : public borealis::CachingTransformer<Retyper> {

    using Base = CachingTransformer<Retyper>;

    std::stack<Type::Ptr> types;

public:

    Retyper(FactoryNest FN);

    using Base::transformBase;
    Term::Ptr transformBase(Term::Ptr t);

    Term::Ptr transformTerm(Term::Ptr t);

    Term::Ptr transformBinary(BinaryTermPtr t);
    Term::Ptr transformCmp(CmpTermPtr t);

    Predicate::Ptr transformSeqData(SeqDataPredicatePtr p);

};

} // namespace borealis

#endif //SANDBOX_RETYPER_H
