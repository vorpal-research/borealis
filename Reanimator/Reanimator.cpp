//
// Created by ice-phoenix on 5/20/15.
//

#include "Reanimator/Reanimator.h"
#include "Type/TypeVisitor.hpp"

namespace borealis {

class RaisingTypeVisitor : public TypeVisitor<RaisingTypeVisitor> {

    using RetTy = void;
    using Base = TypeVisitor<RaisingTypeVisitor>;

public:

    RaisingTypeVisitor(
        const Reanimator& r,
        const util::option<long long>& currentValue,
        logging::logstream& log
    ) : r(r), log(log) {
        in(currentValue);
    }

    RetTy visit(Type::Ptr type) {
        return Base::visit(type);
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

        auto&& pType = p.getPointed();
        auto&& pSize = TypeUtils::getTypeSizeInElems(pType);

        if (values.top()) {
            auto&& base = values.top().getUnsafe();
            for (auto&& shift : r.getArrayBounds(base)) {
                long long addr = base + shift * pSize;
                printAddr(util::just(addr));

                auto&& v = r.getResult().derefValueOf(addr);
                in(v);
                visit(pType);
                out();
            }
        } else {
            in(util::nothing());
            visit(pType);
            out();
        }
        return;
    }

    RetTy visitRecord(const type::Record& rec) {
        out();

        log << "{" << logging::il;
        if (values.top()) {
            auto&& base = values.top().getUnsafe();
            for (auto&& f : rec.getBody()->get()) {
                log << endl << f.getOffset() << " : ";
                in(r.getResult().derefValueOf(base + f.getOffset()));
                visit(f.getType());
                out();
            }
        } else {
            log << endl << "unknown";
        }
        log << logging::ir << endl << "}";

        in(util::nothing());
        return;
    }

    RetTy visitArray(const type::Array& arr) {
        out();

        auto&& elemType = arr.getElement();
        auto&& elemSize = TypeUtils::getTypeSizeInElems(elemType);

        log << "[" << logging::il;
        if (values.top()) {
            auto&& base = values.top().getUnsafe();
            for (auto&& size : arr.getSize()) {
                for (auto&& i = 0U; i < size; ++i) {
                    log << endl;
                    long long addr = base + i * elemSize;
                    printAddr(util::just(addr));

                    auto&& v = r.getResult().derefValueOf(addr);
                    in(v);
                    visit(arr.getElement());
                    out();
                }
            }
        } else {
            log << endl << "unknown";
        }
        log << logging::ir << endl << "]";

        in(util::nothing());
        return;
    }

    RetTy visitUnknownType(const type::UnknownType&) {
        log << "???";
        return;
    }

    RetTy visitTypeError(const type::TypeError& err) {
        log << "|" << err.getMessage() << "|";
        return;
    }

private:
    Reanimator r;
    logging::logstream& log;

    std::stack<util::option<long long>> values;
    std::unordered_set<util::option<long long>> visited;

    void printAddr(util::option<long long> addr) {
        if (addr) {
            log << " [" << addr.getUnsafe() << "] ";
        } else {
            log << " [unknown] ";
        }
    }

    void in(util::option<long long> addr) {
        values.push(addr);
        visited.insert(values.top());
    }

    void out() {
        visited.erase(values.top());
        values.pop();
    }

};

ReanimatorView::ReanimatorView(const Reanimator& r, Term::Ptr term) :
    r(r), term(term) {}

borealis::logging::logstream& operator<<(
    borealis::logging::logstream& s,
    const ReanimatorView& rv
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

Reanimator::Reanimator(const smt::SatResult& result, const ArrayBoundsCollector::ArrayBounds& arrayBounds_) :
    result(result) {
    processArrayBounds(arrayBounds_);
}

const smt::SatResult& Reanimator::getResult() const {
    return result;
}

const Reanimator::ArrayBoundsMap& Reanimator::getArrayBoundsMap() const {
    return arrayBoundsMap;
}

const Reanimator::ArrayBounds& Reanimator::getArrayBounds(unsigned long long base) const {
    static ArrayBounds empty{ 0 };
    if (util::contains(arrayBoundsMap, base)) {
        return arrayBoundsMap.at(base);
    } else {
        return empty;
    }
}

void Reanimator::processArrayBounds(const ArrayBoundsCollector::ArrayBounds& arrayBounds_) {
    for (auto&& e : arrayBounds_) {
        auto&& pType = llvm::dyn_cast<type::Pointer>(e.first->getType());
        for (auto&& base : result.valueOf(e.first->getName())) {
            for (auto&& bound : e.second) {
                for (auto&& bound_ : result.valueOf(bound->getName())) {
                    arrayBoundsMap[base].insert(
                        (bound_ - base) / TypeUtils::getTypeSizeInElems(pType->getPointed())
                    );
                }
            }
        }
    }
}

ReanimatorView raise(const Reanimator& r, Term::Ptr value) {
    return ReanimatorView(r, value);
}

} // namespace borealis
