//
// Created by ice-phoenix on 6/9/15.
//

#include <fstream>

#include "Config/config.h"
#include "SMT/Z3/Tactics.h"
#include "Util/functional.hpp"
#include "Util/util.hpp"

namespace borealis {
namespace z3_ {

Tactics Tactics::load() {
    auto&& res = Tactics{};

    static config::ConfigEntry <std::string> z3TacticsFile("z3", "tactics");
    auto&& z3TacticsFile_ = z3TacticsFile.get("");

    if (z3TacticsFile_ != "") {
        std::ifstream z3TacticsStream{z3TacticsFile_};

        z3TacticsStream >> util::jsonify(res.data);
    }

    return res;
}

z3::tactic Tactics::build(z3::context& ctx) const {
    return util::viewContainer(data)
            .map([&](auto&& d) {
                z3::params params{ctx};
                for (auto&& p : d.params) {
                    auto&& key = p.first;
                    auto&& value = p.second;

                    if (value.isBool()) {
                        params.set(key.c_str(), value.asBool());
                    } else if (value.isUnsigned()) {
                        params.set(key.c_str(), value.asUnsignedInt());
                    } else if (value.isFloating()) {
                        params.set(key.c_str(), value.asDouble());
                    } else {
                        params.set(key.c_str(), value.asCString());
                    }
                }
                return z3::with(
                    z3::tactic(ctx, d.type.c_str()),
                    params
                );
            })
            .reduce(z3::tactic(ctx, "simplify") & z3::tactic(ctx, "smt"), ops::bit_and);
}

} // namespace z3_
} // namespace borealis
