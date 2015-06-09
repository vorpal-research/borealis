//
// Created by ice-phoenix on 6/9/15.
//

#ifndef SANDBOX_TACTICS_H
#define SANDBOX_TACTICS_H

#include <z3/z3++.h>

#include <unordered_map>

#include "Util/json.hpp"
#include "Util/json_traits.hpp"

#include "Util/generate_macros.h"

namespace borealis {
namespace z3_ {

struct TacticData {
    std::string name;
    std::string type;
    std::unordered_map<std::string, Json::Value> params;
};

class Tactics {

public:
    static Tactics load();
    z3::tactic build(z3::context& ctx) const;

private:
    std::vector<TacticData> data;

};

} // namespace z3_
} // namespace borealis

GENERATE_OUTLINE_JSON_TRAITS(borealis::z3_::TacticData, name, type, params)

#include "Util/generate_unmacros.h"

#endif //SANDBOX_TACTICS_H
