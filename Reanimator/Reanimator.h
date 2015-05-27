//
// Created by ice-phoenix on 5/20/15.
//

#ifndef SANDBOX_REANIMATOR_H
#define SANDBOX_REANIMATOR_H

#include "SMT/Result.h"
#include "Term/Term.h"

namespace borealis {

class Reanimator {

    friend class View;
    friend class RaisingTypeVisitor;

public:

    class View {
    public:
        View(const Reanimator& r, Term::Ptr term);
        friend borealis::logging::logstream& operator<<(
            borealis::logging::logstream& s,
            const Reanimator::View& rv
        );
    private:
        const Reanimator& r;
        Term::Ptr term;
    };

    Reanimator(const smt::SatResult& result);

    const smt::SatResult& getResult() const;

    View raise(Term::Ptr value) const;

private:

    const smt::SatResult& result;

};

} // namespace borealis

#endif //SANDBOX_REANIMATOR_H
