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

#include "Util/util.h"
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

        T ret;
        if(valid && parser.getValue(section, option, &ret)) {
            return just(std::move(ret));
        } else return nothing();
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


#include "Util/macros.h"

class AppConfiguration {
    std::unique_ptr<Config> globalConfig;

    AppConfiguration(): globalConfig(nullptr) {};
    ~AppConfiguration(){};

public:
    static AppConfiguration& instance() {
        static AppConfiguration ac;
        return ac;
    }

    static void initialize(const std::string& filename) {
        instance().globalConfig.reset(new Config(filename));
    }

    template<class T>
    friend class ConfigEntry;
};

template<class T = std::string>
class ConfigEntry {
    std::string section;
    std::string key;
    mutable borealis::util::option<T> cached;

    static borealis::util::option<T> nothing;

public:
    ConfigEntry(const std::string& section, const std::string& key):
        section(section), key(key), cached() {};

    const borealis::util::option<T>& get() const {
        auto& conf = AppConfiguration::instance().globalConfig;
        if(!conf) return nothing;

        if(cached.empty()){
            cached = conf->getValue<T>(section, key);
        }
        return cached;
    }

    operator const borealis::util::option<T>& () const{
        return get();
    }

    auto begin() QUICK_CONST_RETURN(get().begin())
    auto end() QUICK_CONST_RETURN(get().end())

};

template<class T> borealis::util::option<T> ConfigEntry<T>::nothing;

template<>
class ConfigEntry<std::vector<std::string>> {
    std::string section;
    std::string key;
    mutable std::vector<std::string> cached;
public:
    ConfigEntry(const std::string& section, const std::string& key):
        section(section), key(key), cached() {};

    const std::vector<std::string>& get() const {
        auto& conf = AppConfiguration::instance().globalConfig;
        if(!conf) return cached;

        if(cached.empty()){
            cached =
                    AppConfiguration::instance().
                        globalConfig->
                            getValue<std::vector<std::string>>(section, key).
                                getOrElse(std::vector<std::string>());
        }
        return cached;
    }

    operator const std::vector<std::string>& () const{
        return get();
    }

    auto begin() QUICK_CONST_RETURN(get().begin())
    auto end() QUICK_CONST_RETURN(get().end())

    auto size() QUICK_CONST_RETURN(get().size())
    auto operator[](size_t index) QUICK_CONST_RETURN(get()[index])
};

typedef ConfigEntry<> StringConfigEntry;
typedef ConfigEntry<std::vector<std::string>> MultiConfigEntry;

#include "Util/unmacros.h"

} // namespace config
} // namespace borealis

#endif /* CONFIG_H_ */
