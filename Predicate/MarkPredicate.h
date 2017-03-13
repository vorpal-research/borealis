#ifndef MARK_PREDICATE_H
#define MARK_PREDICATE_H

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/MarkPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message MarkPredicate {
    extend borealis.proto.Predicate {
        optional MarkPredicate ext = $COUNTER_PRED;
    }

    optional Term id = 1;
}

**/

class MarkPredicate: public borealis::Predicate {

    MarkPredicate(
        Term::Ptr id,
        const Locus& loc,
        PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(MarkPredicate);

    Term::Ptr getId() const;

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>*) const {
        return this->shared_from_this();
    }
};

template<class Impl>
struct SMTImpl<Impl, MarkPredicate> {
    static Bool<Impl> doit(
        const MarkPredicate* p,
        ExprFactory<Impl>& ef,
        ExecutionContext<Impl>*) {

        warns() << "Mark predicate in formula: " << p->toString() << endl;

        return ef.getTrue();
    }
};

} /* namespace borealis */

#endif // MARK_PREDICATE_H
