//
// Created by ice-phoenix on 5/20/15.
//

#include "Reanimator/Reanimator.h"
#include "Type/TypeVisitor.hpp"

namespace borealis {

class RaisingTypeVisitor : public TypeVisitor<RaisingTypeVisitor> {

    using RetTy = void;

public:

    RaisingTypeVisitor(
        const Reanimator& r,
        const util::option<long long>& currentValue,
        logging::logstream& log
    ) : r(r), log(log) {
        values.push(currentValue);
    }

    RetTy visitBool(const type::Bool&) {
        if (values.top()) {
            auto&& v = values.top().getUnsafe();
            log << (v ? "true" : "false");
        } else {
            log << "unknown";
        }
        return;
    }

    RetTy visitInteger(const type::Integer& i) {
        if (values.top()) {
            auto&& v = values.top().getUnsafe();
            log << (llvm::Signedness::Unsigned == i.getSignedness() ? (unsigned long long) v : v);
        } else {
            log << "unknown";
        }
        return;
    }

    RetTy visitFloat(const type::Float&) {
        if (values.top()) {
            auto&& v = values.top().getUnsafe();
            log << (double) v;
        } else {
            log << "unknown";
        }
        return;
    }

    RetTy visitPointer(const type::Pointer& p) {
        log << "*";
        if (values.top()) {
            values.push(r.result.derefValueOf(values.top().getUnsafe()));
            visit(p.getPointed());
            values.pop();
        } else {
            values.push(util::nothing());
            visit(p.getPointed());
            values.pop();
        }
        return;
    }

    RetTy visitRecord(const type::Record& rec) {
        values.pop();

        log << "{" << logging::il;
        if (values.top()) {
            auto&& v = values.top().getUnsafe();
            for (auto&& f : rec.getBody()->get()) {
                log << endl << f.getIndex() << " : ";
                values.push(r.result.derefValueOf(v + f.getIndex()));
                visit(f.getType());
                values.pop();
            }
        } else {
            log << endl << "unknown";
        }
        log << logging::ir << endl << "}";

        values.push(util::nothing());
        return;
    }

    RetTy visitArray(const type::Array& arr) {
        values.pop();

        log << "[" << logging::il;
        if (values.top()) {
            auto&& v = values.top().getUnsafe();
            for (auto&& size : arr.getSize()) {
                for (auto&& i = 0U; i < size; ++i) {
                    log << endl;
                    values.push(r.result.derefValueOf(v + i));
                    visit(arr.getElement());
                    values.pop();
                }
            }
        } else {
            log << endl << "unknown";
        }
        log << logging::ir << endl << "]";

        values.push(util::nothing());
        return;
    }

private:
    const Reanimator& r;
    logging::logstream& log;

    std::stack<util::option<long long>> values;

};

Reanimator::View::View(const Reanimator& r, Term::Ptr term) :
    r(r), term(term) {}

borealis::logging::logstream& operator<<(
    borealis::logging::logstream& s,
    const Reanimator::View& rv
) {
    if (auto&& v = rv.r.getResult().valueOf(rv.term->getName())) {
        s << "Raising " << rv.term << " from the dead..." << endl;
        RaisingTypeVisitor(rv.r, v, s).visit(rv.term->getType());
        s << endl;
    } else {
        s << "Cannot raise " << rv.term << " from the dead!" << endl;
    }
    return s;
}

Reanimator::Reanimator(const smt::SatResult& result) : result(result) {}

const smt::SatResult& Reanimator::getResult() const {
    return result;
}

Reanimator::View Reanimator::raise(Term::Ptr value) const {
    return View(*this, value);
}

} // namespace borealis
