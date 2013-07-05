/*
 * logger.hpp
 *
 *  Created on: Oct 29, 2012
 *      Author: belyaev
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <type_traits>

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

protected:
    void assignLogger(const std::string& domain) {
        logger = domain;
    }

    ObjectLevelLogging(const std::string& domain) {
        assignLogger(domain);
    }

    stream_t dbgs() const { return dbgsFor(logger); }
    stream_t infos() const { return infosFor(logger); }
    stream_t warns() const { return warnsFor(logger); }
    stream_t errs() const { return errsFor(logger); }
    stream_t criticals() const { return criticalsFor(logger); }

    stream_t logs(priority_t ll = priority_t::DEBUG) const {
        return logsFor(ll, logger);
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
