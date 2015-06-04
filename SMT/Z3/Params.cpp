//
// Created by ice-phoenix on 6/3/15.
//

#include "z3/z3++.h"

#include <fstream>
#include <sstream>

#include "Config/config.h"
#include "SMT/Z3/Params.h"

namespace borealis {
namespace z3_ {

ParamType string2ParamType(const std::string& s) {
    if ("(bool)" == s) return ParamType::BOOL;
    else if ("(unsigned int)" == s) return ParamType::UINT;
    else if ("(double)" == s) return ParamType::DOUBLE;
    else return ParamType::UNKNOWN;
}

std::istream& operator>>(std::istream& s, ParamType& pt) {
    std::string pts;
    s >> pts;
    pt = string2ParamType(pts);
    return s;
}

Params Params::load() {
    auto&& res = Params{};

    static config::ConfigEntry <std::string> z3ParamFile("z3", "params");
    auto&& z3ParamFile_ = z3ParamFile.get("");

    if (z3ParamFile_ != "") {
        std::ifstream z3ParamStream{z3ParamFile_};

        std::string line;
        while (std::getline(z3ParamStream, line)) {
            std::string key, sep, value;
            ParamType type;
            std::istringstream(line) >> key >> sep >> value >> type;
            if ("" != key && "=" == sep && "" != value && ParamType::UNKNOWN != type) {
                res.add(key, value, type);
            }
        }
    }

    return res;
}

void Params::apply() const {
    for (auto&& p : params) {
        z3::set_param(std::get<0>(p).c_str(), std::get<1>(p).c_str());
    }
}

Params& Params::add(const std::string& key, const std::string& value, ParamType type) {
    params.push_back(std::make_tuple(key, value, type));
    return *this;
}

} // namespace z3_
} // namespace borealis
