/*
 * logger.hpp
 *
 *  Created on: Oct 29, 2012
 *      Author: belyaev
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <type_traits>
#include <tinyformat/tinyformat.h>

#include "Logging/log_entry.hpp"
#include "Logging/logstream.hpp"

#include "Util/macros.h"

namespace borealis {
namespace logging {

typedef logstream stream_t;
typedef PriorityLevel priority_t;

template<class T>
class ClassLevelLogging {

private:
    static std::string logger;

public:
    const std::string& getLoggerDomain() const {
        return logger;
    }

protected:
    static stream_t dbgs() {
        return dbgsFor(logger);
    }
    static stream_t infos() {
        return infosFor(logger);
    }
    static stream_t warns() {
        return warnsFor(logger);
    }
    static stream_t errs() {
        return errsFor(logger);
    }
    static stream_t criticals() {
        return criticalsFor(logger);
    }

    static stream_t logs(priority_t ll = priority_t::DEBUG) {
        return logsFor(ll, logger);
    }
};

template<class ClassLevelLoggingParam>
std::string logDomainFor( GUARDED(void*, bool(ClassLevelLoggingParam::loggerDomain()[0])) ) {
    return ClassLevelLoggingParam::loggerDomain();
}

template<class ClassLevelLoggingParam>
std::string logDomainFor(...) {
    return "";
}

template<class T>
std::string ClassLevelLogging<T>::logger = logDomainFor<T>(nullptr);


template<class T>
class ObjectLevelLogging {

private:
    mutable std::string logger;

public:
    const std::string& getLoggerDomain() const {
        return logger;
    }

protected:
    void assignLogger(const std::string& domain) {
        logger = domain;
    }

    ObjectLevelLogging(const std::string& domain): logger(domain) {}

    stream_t dbgs() const { return dbgsFor(logger); }
    stream_t infos() const { return infosFor(logger); }
    stream_t warns() const { return warnsFor(logger); }
    stream_t errs() const { return errsFor(logger); }
    stream_t criticals() const { return criticalsFor(logger); }

    stream_t logs(priority_t ll = priority_t::DEBUG) const {
        return logsFor(ll, logger);
    }
};


class DelegateLogging {
    std::function<const std::string&()> keeper;

    static const std::string& def() {
        static std::string empty = "";
        return empty;
    }

public:

    template<class T>
    void assignLogger(const T& domain) {
        const T* ptr = &domain;
        keeper = [ptr] () -> const std::string& { return ptr->getLoggerDomain(); };
    }

    const std::string& getLoggerDomain() const {
        if(!keeper) throw std::logic_error(tfm::format("%s %s %s", __PRETTY_FUNCTION__, __FILE__, __LINE__));
        return keeper();
    }

protected:

    template<class T>
    DelegateLogging(const T& domain): keeper() {
        const T* ptr = &domain;
        keeper = [ptr]() -> const std::string& { return ptr->getLoggerDomain(); };
    }
    DelegateLogging(const std::string& domain): keeper([domain]() -> const std::string& { return domain; }) {}
    DelegateLogging(): keeper(def) {}

    // this copy-string business looks like a bug in gcc
    // gcc version crashes without it though it does not do anything at all =(
    stream_t dbgs() const { return dbgsFor(std::string(getLoggerDomain())); }
    stream_t infos() const { return infosFor(std::string(getLoggerDomain())); }
    stream_t warns() const { return warnsFor(std::string(getLoggerDomain())); }
    stream_t errs() const { return errsFor(std::string(getLoggerDomain())); }
    stream_t criticals() const { return criticalsFor(std::string(getLoggerDomain())); }

    stream_t logs(priority_t ll = priority_t::DEBUG) const {
        return logsFor(ll, std::string(getLoggerDomain()));
    }
};


inline stream_t dbgs() { return dbgsFor(""); }
inline stream_t infos() { return infosFor(""); }
inline stream_t warns() { return warnsFor(""); }
inline stream_t errs() { return errsFor(""); }
inline stream_t criticals() { return criticalsFor(""); }

inline stream_t wtf() { return errsFor("wtf"); }

inline stream_t logs(priority_t ll = priority_t::DEBUG) {
    return logsFor(ll, "");
}

} // namespace logging

using logging::dbgs;
using logging::infos;
using logging::warns;
using logging::errs;
using logging::criticals;
using logging::logs;
using logging::endl;
using logging::end;

} // namespace borealis

#include "Util/unmacros.h"

#endif /* LOGGER_HPP_ */
