//
// Created by belyaev on 4/6/15.
//

#ifndef RESULT_H
#define RESULT_H

#include <memory>
#include <string>
#include <unordered_map>

#include "Term/Term.h"
#include "Term/ProtobufConverterImpl.hpp"
#include "Factory/Nest.h"
#include "Protobuf/Gen/SMT/Result.pb.h"
#include "SMT/Model.h"

#include "Util/unions.hpp"

#include "Util/macros.h"

/** protobuf -> SMT/Result.proto

import "SMT/Model.proto";

package borealis.smt.proto;

message Result {
    enum ResType {
        SAT = 0;
        UNSAT = 1;
        UNKNOWN = 2;
    };

    optional ResType restype = 1;
    optional borealis.smt.proto.Model model = 2;
}

**/

namespace borealis {
namespace smt {

class UnsatResult {
    /* ... */
public:
    UnsatResult() = default;
    UnsatResult(const UnsatResult&) = default;
};

class UnknownResult {
    /* ... */
public:
    UnknownResult() = default;
    UnknownResult(const UnknownResult&) = default;
};


class SatResult {
public:
    std::shared_ptr<const Model> model;
private:
    friend struct borealis::util::json_traits<SatResult>;

public:
    SatResult();
    SatResult(const SatResult&) = default;

    SatResult(std::shared_ptr<const Model> model): model(model){}

    bool valid() const { return static_cast<bool>(model); }

    const Model& getModel() const { return *model; }

    friend std::ostream& operator<<(std::ostream& ost, const SatResult& res);
    //friend struct borealis::protobuf_traits<Result>;
};

// poor man's variant
class Result {
    union {
        SatResult sat;
        UnsatResult unsat;
        UnknownResult unknown;
    };
    enum {SAT, UNSAT, UNKNOWN} restype;
    std::string watermark;

public:
    Result(const SatResult& sat) : sat(sat), restype(SAT) {}
    Result(const UnsatResult& unsat) : unsat(unsat), restype(UNSAT) {}
    Result(const UnknownResult& unknown): unknown(unknown), restype(UNKNOWN) {}
    Result(const Result& that) : restype(that.restype) {
        if (restype == UNSAT) unions::construct(&unsat, that.unsat);
        else if (restype == UNKNOWN) unions::construct(&unknown, that.unknown);
        else unions::construct(&sat, that.sat);
    }
    ~Result() {
        if (restype == UNSAT) unions::destruct(&unsat);
        else if(restype == UNKNOWN) unions::destruct(&unknown);
        else unions::destruct(&sat);
    }

    const SatResult* getSatPtr() const {
        return (restype != SAT) ? nullptr : &sat;
    }

    bool isUnsat() const {
        return restype == UNSAT;
    }

    bool isUnknown() const {
        return restype == UNKNOWN;
    }

    bool isSat() const {
        return restype == SAT;
    }

    friend std::ostream& operator<<(std::ostream& ost, const Result& res) {
        if(res.isUnknown()) {
            ost << "unknown";
        }
        if(res.isUnsat()) {
            ost << "unsat";
        }
        if(res.isSat()) {
            ost << *res.getSatPtr();
        }
        return ost;
    }

    //friend struct borealis::protobuf_traits<Result>;
};

} /* namespace smt */

} /* namespace borealis */

#include "Util/unmacros.h"

#endif //RESULT_H
