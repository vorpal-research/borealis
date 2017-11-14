//
// Created by abdullin on 11/13/17.
//

#include "CallPredicate.h"

#include "Util/macros.h"

namespace borealis {

CallPredicate::CallPredicate(
        Term::Ptr lhv,
        Term::Ptr function,
        const std::vector<Term::Ptr>& args,
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {
    hasLhv_ = (lhv != nullptr);
    if (hasLhv_) ops.insert(ops.end(), lhv);
    ops.insert(ops.end(), function);
    ops.insert(ops.end(), args.begin(), args.end());

    asString = "";
    if(hasLhv())
        asString = getLhv()->getName() + "=";
    asString += getFunctionName()->getName() + "(" + getArgs()
                                                     .map(LAM(a, a->getName()))
                                                     .reduce("", LAM2(acc, e, acc + ", " + e)) + ")";
}

bool CallPredicate::hasLhv() const {
    return hasLhv_;
}

Term::Ptr CallPredicate::getFunctionName() const {
    return hasLhv() ? ops[1] : ops[0];
}

auto CallPredicate::getArgs() const -> decltype(util::viewContainer(ops)) {
    return util::viewContainer(ops).drop((hasLhv() ? 2 : 1));
}

Term::Ptr CallPredicate::getLhv() const {
    return (hasLhv()) ? ops[0] : nullptr;
}

} /* namespace borealis */

#include "Util/unmacros.h"