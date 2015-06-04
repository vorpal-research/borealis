//
// Created by ice-phoenix on 6/3/15.
//

#ifndef SANDBOX_PARAMS_H
#define SANDBOX_PARAMS_H

#include <z3/z3++.h>

#include <tuple>

namespace borealis {
namespace z3_ {

enum class ParamType {
    BOOL,
    UINT,
    DOUBLE,
    UNKNOWN
};

class Params {

    using param_t = std::tuple<std::string, std::string, ParamType>;
    using storage_t = std::vector<param_t>;

public:
    static Params load();

    void apply() const;
    Params& add(const std::string& key, const std::string& value, ParamType type);

private:

    storage_t params;

};

} // namespace z3_
} // namespace borealis

#endif //SANDBOX_PARAMS_H
