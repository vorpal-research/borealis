/*
 * logstream.hpp
 *
 *  Created on: Oct 30, 2012
 *      Author: belyaev
 */

#ifndef LOGSTREAM_HPP_
#define LOGSTREAM_HPP_

#define LOG4CPP_FIX_ERROR_COLLISION 1
#include <log4cpp/CategoryStream.hh>

namespace borealis {
namespace logging {

namespace impl {
    typedef log4cpp::CategoryStream stream_t;
}


enum class PriorityLevel {
    EMERG  = 0,
    FATAL  = 0,
    ALERT  = 100,
    CRIT   = 200,
    ERROR  = 300,
    WARN   = 400,
    NOTICE = 500,
    INFO   = 600,
    DEBUG  = 700,
    NOTSET = 800
};

class logstream {
    impl::stream_t inner;

    logstream(impl::stream_t inner): inner(inner) {};
public:
    template<class T>
    logstream& operator<<(const T& val) {
        inner << val;
        return *this;
    }

    logstream& operator<<(logstream&(*mutator)(logstream&)) {
        return mutator(*this);
    }

    void flush() {
        inner.flush();
    }

    friend logstream dbgsFor(const std::string& category);
    friend logstream infosFor(const std::string& category);
    friend logstream warnsFor(const std::string& category);
    friend logstream errsFor(const std::string& category);
    friend logstream criticalsFor(const std::string& category);
    friend logstream logsFor(PriorityLevel lv, const std::string& category);
};

logstream dbgsFor(const std::string& category);
logstream infosFor(const std::string& category);
logstream warnsFor(const std::string& category);
logstream errsFor(const std::string& category);
logstream criticalsFor(const std::string& category);
logstream logsFor(PriorityLevel lv, const std::string& category);

logstream& endl(logstream&);
logstream& end(logstream&);

void configureLoggingFacility(const std::string filename);

}
}


#endif /* LOGSTREAM_HPP_ */
