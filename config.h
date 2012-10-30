/*
 * Config.h
 *
 *  Created on: Oct 25, 2012
 *      Author: belyaev
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>

#include <cfgparser.h>
#include <configwrapper.h>

#include "util.h"
#include "Util/option.hpp"

namespace borealis {
namespace config {

#define RESTRICT_PARAMETER_TO(PARAM, ...) class PARAM##Resolver = std::enable_if< \
    borealis::util::is_T_in<PARAM, __VA_ARGS__>::value >

class Config {

    ConfigParser_t parser;
    bool valid;

public:
    Config(const std::string& file): parser(), valid(parser.readFile(file) == 0) {}
    Config(const Config&) = default;
    Config(Config&&) = default;

    template<class T, RESTRICT_PARAMETER_TO(T, int, unsigned int, double, bool, std::string, std::vector<std::string>)>
    borealis::util::option<T> getValue(const std::string& section, const std::string& option) {
        using borealis::util::just;
        using borealis::util::nothing;
        using std::move;

        T ret;
        if(valid && parser.getValue(section, option, &ret)) {
            return just(move(ret));
        } else return nothing<T>();
    }

    std::multimap<std::string, std::string> optionsFor(const std::string& name) {
        return parser.getOptions(name);
    }



    inline bool ok() {
        return valid;
    }

    ~Config();

};

#undef RESTRICT_PARAMETER_TO

} // namespace config
} // namespace borealis

#endif /* CONFIG_H_ */
