/*
 * Config.h
 *
 *  Created on: Oct 25, 2012
 *      Author: belyaev
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <cfgparser.h>
#include <configwrapper.h>

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Util/string_ref.hpp"
#include "Util/util.h"

namespace borealis {
namespace config {

#define RESTRICT_PARAMETER_TO(PARAM, ...) class PARAM##Resolver = \
    std::enable_if< borealis::util::is_T_in<PARAM, __VA_ARGS__>::value >

class ConfigSource {
public:
    template<class T>
    using option = borealis::util::option<T>;

    ConfigSource() {};
    virtual ~ConfigSource() {};
    virtual option<int>
    getIntValue(const std::string& section, const std::string& option) const = 0;
    virtual option<bool>
    getBoolValue(const std::string& section, const std::string& option) const = 0;
    virtual option<std::string>
    getStringValue(const std::string& section, const std::string& option) const = 0;
    virtual option<double>
    getDoubleValue(const std::string& section, const std::string& option) const = 0;
    virtual option<std::vector<std::string>>
    getVectorValue(const std::string& section, const std::string& option) const = 0;
};

class FileConfigSource: public ConfigSource {
    ConfigParser_t parser;
    bool valid;

public:
    FileConfigSource(const std::string& file) :
        parser{},
        valid{ parser.readFile(file) == 0 } {}
    FileConfigSource(const char* file) :
        parser{},
        valid{ parser.readFile(file) == 0 } {}

    template<class T, RESTRICT_PARAMETER_TO(T, int, unsigned int, double, bool, std::string, std::vector<std::string>)>
    borealis::util::option<T> getValue(const std::string& section, const std::string& option) const {
        using borealis::util::just;
        using borealis::util::nothing;

        T ret;

        if (valid && parser.getValue(section, option, &ret))
            return just(std::move(ret));

        return nothing();
    }

    virtual option<int> getIntValue(const std::string& section, const std::string& option) const override {
        return getValue<int>(section, option);
    }
    virtual option<bool> getBoolValue(const std::string& section, const std::string& option) const override {
        return getValue<bool>(section, option);
    }
    virtual option<std::string> getStringValue(const std::string& section, const std::string& option) const override {
        return getValue<std::string>(section, option);
    }
    virtual option<double> getDoubleValue(const std::string& section, const std::string& option) const override {
        return getValue<double>(section, option);
    }
    virtual option<std::vector<std::string>> getVectorValue(const std::string& section, const std::string& option) const override {
        return getValue<std::vector<std::string>>(section, option);
    }
};

namespace impl_ {

template<class T, class SFINAE>
struct valueAcquire;

template<class T, RESTRICT_PARAMETER_TO(T, int, unsigned int, double, std::string)>
struct valueAcquire {
    static borealis::util::option<T> doit(const std::vector<std::string>& opts) {
        using namespace borealis::util;
        if (opts.empty()) return nothing();
        else return fromString<T>(opts.back());
    }
};

template<>
struct valueAcquire<std::vector<std::string>> {
    static borealis::util::option<std::vector<std::string>> doit(const std::vector<std::string>& opts) {
        using namespace borealis::util;
        if (opts.empty()) return nothing();
        else return just(opts);
    }
};

template<>
struct valueAcquire<bool> {
    static borealis::util::option<bool> doit(const std::vector<std::string>& opts) {
        using namespace borealis::util;
        if (opts.empty()) return nothing();

        const auto& v = opts.back();
        if (v == "1" || v == "true" || v == "on" || v == "yes") {
            return just(true);
        }
        if (v == "0" || v == "false" || v == "off" || v == "no") {
            return just(false);
        }
        return nothing();
    }
};

} // namespace impl_

class StructuredConfigSource: public ConfigSource {
public:
    typedef std::unordered_map<
        std::string,
        std::unordered_map<
            std::string,
            std::vector<std::string>
        >
    > Overrides;

private:
    Overrides overrides;

public:
    StructuredConfigSource(const Overrides& overrides): overrides(overrides) {};
    StructuredConfigSource(Overrides&& overrides): overrides(std::move(overrides)) {};
    virtual ~StructuredConfigSource() {};

    template<class T, RESTRICT_PARAMETER_TO(T, int, unsigned int, double, std::string, bool, std::vector<std::string>)>
    borealis::util::option<T> getValue(const std::string& section, const std::string& option) const {
        using namespace borealis::util;
        for (const auto& sec : at(overrides, section)) {
            for (const auto& opt : at(sec, option)) {
                return impl_::valueAcquire<T>::doit(opt);
            }
        }
        return nothing();
    }

    virtual option<int> getIntValue(const std::string& section, const std::string& option) const override {
        return getValue<int>(section, option);
    }
    virtual option<bool> getBoolValue(const std::string& section, const std::string& option) const override {
        return getValue<bool>(section, option);
    }
    virtual option<std::string> getStringValue(const std::string& section, const std::string& option) const override {
        return getValue<std::string>(section, option);
    }
    virtual option<double> getDoubleValue(const std::string& section, const std::string& option) const override {
        return getValue<double>(section, option);
    }
    virtual option<std::vector<std::string>> getVectorValue(const std::string& section, const std::string& option) const override {
        return getValue<std::vector<std::string>>(section, option);
    }
};

class CommandLineConfigSource: public StructuredConfigSource {
public:
    CommandLineConfigSource(const std::vector<std::string>& opts):
        StructuredConfigSource{ [&opts]() -> StructuredConfigSource::Overrides {
            StructuredConfigSource::Overrides ret;
            for (util::string_ref opt : opts) {
                util::string_ref fst, snd, thrd;
                std::tie(fst, snd) = opt.split(':');
                std::tie(snd, thrd) = snd.split(':');

                if (snd.empty() || thrd.empty()) continue;

                ret[fst][snd].push_back(thrd);
            }
            return ret;
        }() } {}
};

namespace impl_ {
    template<class T>
    struct accessor;

    template<>
    struct accessor<int> {
        borealis::util::option<int> operator()(
                const ConfigSource& src,
                const std::string& section,
                const std::string& option
        ) const {
            return src.getIntValue(section, option);
        }
    };

    template<>
    struct accessor<bool> {
        borealis::util::option<bool> operator()(
                const ConfigSource& src,
                const std::string& section,
                const std::string& option
        ) const {
            return src.getBoolValue(section, option);
        }
    };

    template<>
    struct accessor<double> {
        borealis::util::option<double> operator()(
                const ConfigSource& src,
                const std::string& section,
                const std::string& option
        ) const {
            return src.getDoubleValue(section, option);
        }
    };

    template<>
    struct accessor<std::string> {
        borealis::util::option<std::string> operator()(
                const ConfigSource& src,
                const std::string& section,
                const std::string& option
        ) const {
            return src.getStringValue(section, option);
        }
    };

    template<>
    struct accessor<std::vector<std::string>> {
        borealis::util::option<std::vector<std::string>> operator()(
                const ConfigSource& src,
                const std::string& section,
                const std::string& option
        ) const {
            return src.getVectorValue(section, option);
        }
    };

    template<class T>
    struct combiner {
        borealis::util::option<T> operator() (
                const std::vector<std::unique_ptr<ConfigSource>>& sources,
                const std::string& section,
                const std::string& option
        ) {
            borealis::util::option<T> ret{};
            for (const auto& source : sources) {
                ret = accessor<T>()(*source, section, option);
                if (ret) break;
            }
            return std::move(ret);
        }
    };

    template<>
    struct combiner<std::vector<std::string>> {
        borealis::util::option<std::vector<std::string>> operator() (
                const std::vector<std::unique_ptr<ConfigSource>>& sources,
                const std::string& section,
                const std::string& option
        ) {
            std::vector<std::string> ret{};
            for (const auto& source : sources) {
                for (const auto& vec : accessor<std::vector<std::string>>()(*source, section, option)) {
                    ret.reserve(ret.size() + vec.size());
                    ret.insert(std::end(ret), std::begin(vec), std::end(vec));
                }
            }
            return util::just(std::move(ret));
        }
    };
}

class Config {
    typedef std::unique_ptr<ConfigSource> uniq;
    std::vector<uniq> sources;

public:
    Config() = default;
    Config(const Config&) = default;
    Config(Config&&) = default;

    template<class ...Ptrs>
    Config(Ptrs... sources) : sources{} {
        std::vector<ConfigSource*> tmp{ sources... };
        for (auto* ptr : tmp) this->sources.emplace_back(ptr);
    };
    ~Config() {};

    template<class T>
    borealis::util::option<T> getValue(
            const std::string& section,
            const std::string& option) const {
        return impl_::combiner<T>()(sources, section, option);
    }

};

#undef RESTRICT_PARAMETER_TO

#include "Util/macros.h"

class AppConfiguration {
    typedef Config Cfg;

    std::unique_ptr<Cfg> globalConfig;

    AppConfiguration(): globalConfig(nullptr) {};
    ~AppConfiguration() {};

public:
    static AppConfiguration& instance() {
        static AppConfiguration ac;
        return ac;
    }

    template<class ...Sources>
    static void initialize(const Sources& ...sources) {
        instance().globalConfig.reset(new Cfg({ sources... }));
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
        section(section), key(key) {};

    const borealis::util::option<T>& get() const {
        auto& conf = AppConfiguration::instance().globalConfig;
        if (!conf) return nothing;

        if (cached.empty()) {
            cached = conf->getValue<T>(section, key);
        }
        return cached;
    }

    template<class U>
    borealis::util::option<U> get() const {
        for(const auto& op : this->get()) {
            return borealis::util::just(U(op));
        }
        return borealis::util::nothing();
    }

    template<class U>
    auto get(U&& def) const -> decltype(cached.getOrElse(std::forward<U>(def))) {
        auto& conf = AppConfiguration::instance().globalConfig;
        if (!conf) return def;

        if (cached.empty()) {
            cached = conf->getValue<T>(section, key);
        }
        return cached.getOrElse(std::forward<U>(def));
    }

    operator const borealis::util::option<T>&() const { return get(); }

    auto begin() QUICK_CONST_RETURN(this->get().begin())
    auto end() QUICK_CONST_RETURN(this->get().end())
};

template<class T>
borealis::util::option<T> ConfigEntry<T>::nothing;

template<>
class ConfigEntry<std::vector<std::string>> {
    std::string section;
    std::string key;
    mutable std::vector<std::string> cached;

public:
    ConfigEntry(const std::string& section, const std::string& key):
        section(section), key(key) {};

    const std::vector<std::string>& get() const {
        auto& conf = AppConfiguration::instance().globalConfig;
        if (!conf) return cached;

        if (cached.empty()) {
            cached = AppConfiguration::instance()
                     .globalConfig
                     ->getValue<std::vector<std::string>>(section, key)
                     .getOrElse(std::vector<std::string>());
        }
        return cached;
    }

    operator const std::vector<std::string>&() const { return get(); }

    auto begin() QUICK_CONST_RETURN(get().begin())
    auto end() QUICK_CONST_RETURN(get().end())
    auto size() QUICK_CONST_RETURN(get().size())

    auto operator[](size_t index) QUICK_CONST_RETURN(get()[index])
};

typedef ConfigEntry<bool>                     BoolConfigEntry;
typedef ConfigEntry<std::string>              StringConfigEntry;
typedef ConfigEntry<std::vector<std::string>> MultiConfigEntry;

} // namespace config
} // namespace borealis

#include "Util/unmacros.h"

#endif /* CONFIG_H_ */
