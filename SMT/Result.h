//
// Created by belyaev on 4/6/15.
//

#ifndef RESULT_H
#define RESULT_H

#include <string>
#include <memory>
#include <unordered_map>
#include <Term/OpaqueIntConstantTerm.h>

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

private:
    std::shared_ptr<model_t> modelPtr;

    friend struct borealis::util::json_traits<SatResult>;

    const model_t& getModel() const { return *modelPtr; } // this is here only for json traits

public:

    SatResult() = default;
    SatResult(const SatResult&) = default;
    SatResult(const model_t& model):
        modelPtr(std::make_shared<model_t>(model)) { }

    Term::Ptr at(const std::string& str) const {
        return modelPtr->at(str);
    }

    long long valueAt(const std::string& str) const {
        if (auto ii = llvm::dyn_cast<borealis::OpaqueIntConstantTerm>(at(str))) {
            return ii->getValue();
        } else UNREACHABLE("Non-integer value in model");
    }
};

// poor man's variant
class Result {
    union {
        SatResult sat;
        UnsatResult unsat;
    };
    bool isUnsat_;
public:
    Result(const SatResult& sat): sat(sat), isUnsat_(false) {}
    Result(const UnsatResult& unsat): unsat(unsat), isUnsat_(true) {}
    Result(const Result& that): isUnsat_(that.isUnsat_) {
        using namespace unions;
        if(isUnsat_) construct(&unsat, that.unsat);
        else construct(&sat, that.sat);
    }
    ~Result() {
        if(isUnsat_) unions::destruct(&unsat);
        else unions::destruct(&sat);
    }

    const SatResult* getSatPtr() const {
        return isUnsat_? nullptr : &sat;
    }

    bool isUnsat() const {
        return isUnsat_;
    }

    bool isSat() const {
        return !isUnsat_;
    }
};

} /* namespace smt */
} /* namespace borealis */

#include "Util/unmacros.h"


#endif //RESULT_H
