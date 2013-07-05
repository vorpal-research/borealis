/*
 * logstream.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: belyaev
 */

#include <log4cpp/Category.hh>
#include <log4cpp/CategoryStream.hh>
#include <log4cpp/Priority.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <z3/z3++.h>

#include "Logging/logstream.hpp"

inline static log4cpp::Priority::PriorityLevel mapPriorities(borealis::logging::PriorityLevel pli) {
    typedef borealis::logging::PriorityLevel pl;
    typedef log4cpp::Priority::PriorityLevel rpl;

    switch(pli) {
    case pl::EMERG:     return rpl::EMERG;
    case pl::ALERT:     return rpl::ALERT;
    case pl::CRIT:      return rpl::CRIT;
    case pl::ERROR:     return rpl::ERROR;
    case pl::WARN:      return rpl::WARN;
    case pl::NOTICE:    return rpl::NOTICE;
    case pl::INFO:      return rpl::INFO;
    case pl::DEBUG:     return rpl::DEBUG;
    default :           return rpl::NOTSET;
    }
}

namespace borealis {
namespace logging {

using log4cpp::Category;
using log4cpp::CategoryStream;
using log4cpp::PropertyConfigurator;

typedef logstream stream_t;

inline Category& getCat(const std::string& cname) {
    if (cname.empty()) return Category::getRoot();
    else return Category::getInstance(cname);
}

stream_t dbgsFor(const std::string& category) {
    return stream_t(getCat(category).debugStream());
}
stream_t infosFor(const std::string& category) {
    return stream_t(getCat(category).infoStream());
}
stream_t warnsFor(const std::string& category) {
    return stream_t(getCat(category).warnStream());
}
stream_t errsFor(const std::string& category) {
    return stream_t(getCat(category).errorStream());
}
stream_t criticalsFor(const std::string& category) {
    return stream_t(getCat(category).critStream());
}

stream_t logsFor(PriorityLevel lvl, const std::string& category) {
    return stream_t(getCat(category) << mapPriorities(lvl));
}

void configureLoggingFacility(const std::string& filename) {
    PropertyConfigurator::configure(filename);
}

void configureZ3Log(const std::string& filename) {
    Z3_open_log(filename.c_str());
    atexit(Z3_close_log);
}

stream_t& indent(stream_t& st) {
    return st.indentOn();
}

stream_t& il(stream_t& st) {
    return st.incIndent();
}

stream_t& ir(stream_t& st) {
    return st.decIndent();
}

stream_t& endl(stream_t& st) {
    st << "\n";
    return st.indentOn();
}

stream_t& end(stream_t& st) {
    st << log4cpp::eol;
    return st.indentOn();
}

} // namespace logging
} // namespace borealis
