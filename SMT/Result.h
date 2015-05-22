//
// Created by belyaev on 4/6/15.
//

#ifndef RESULT_H
#define RESULT_H

#include <memory>
#include <string>
#include <unordered_map>

#include "Term/OpaqueIntConstantTerm.h"
#include "Term/Term.h"

#include "Util/unions.hpp"

#include "Util/macros.h"

namespace borealis {
namespace smt {

class UnsatResult {
    /* ... */
public:
    UnsatResult() = default;
    UnsatResult(const UnsatResult&) = default;
};

class SatResult {
public:
    using model_t = std::unordered_map<std::string, Term::Ptr>;
    using memory_shape_t = std::unordered_map<uintptr_t, Term::Ptr>;

private:
    std::shared_ptr<model_t> modelPtr;
    std::shared_ptr<memory_shape_t> initialMemoryShapePtr;
    std::shared_ptr<memory_shape_t> finalMemoryShapePtr;

    friend struct borealis::util::json_traits<SatResult>;

    // this is here only for json traits
    const model_t& getModel() const { return *modelPtr; }
    const memory_shape_t& getInitialMemoryShape() const { return *initialMemoryShapePtr; }
    const memory_shape_t& getFinalMemoryShape() const { return *finalMemoryShapePtr; }

public:
    SatResult();
    SatResult(const SatResult&) = default;

    SatResult(
        std::shared_ptr<model_t> modelPtr,
        std::shared_ptr<memory_shape_t> initialShapePtr,
        std::shared_ptr<memory_shape_t> finalShapePtr
    );

    util::option_ref<Term::Ptr> at(const std::string& str) const;
    util::option_ref<Term::Ptr> deref(uintptr_t ptr) const;

    bool empty() const { return modelPtr->empty(); }
    bool valid() const { return !!modelPtr; }

    util::option<long long> valueOf(const std::string& str) const;
    util::option<long long> derefValueOf(uintptr_t ptr) const;

    friend std::ostream& operator<<(std::ostream& ost, const SatResult& res);
};

// poor man's variant
class Result {
    union {
        SatResult sat;
        UnsatResult unsat;
    };
    bool isUnsat_;

public:
    Result(const SatResult& sat) : sat(sat), isUnsat_(false) {}
    Result(const UnsatResult& unsat) : unsat(unsat), isUnsat_(true) {}
    Result(const Result& that) : isUnsat_(that.isUnsat_) {
        if (isUnsat_) unions::construct(&unsat, that.unsat);
        else unions::construct(&sat, that.sat);
    }
    ~Result() {
        if (isUnsat_) unions::destruct(&unsat);
        else unions::destruct(&sat);
    }

    const SatResult* getSatPtr() const {
        return isUnsat_ ? nullptr : &sat;
    }

    bool isUnsat() const {
        return isUnsat_;
    }

    bool isSat() const {
        return not isUnsat();
    }
};

} /* namespace smt */
} /* namespace borealis */

#include "Util/unmacros.h"

#endif //RESULT_H
