/*
 * log_entry.hpp
 *
 *  Created on: Oct 30, 2012
 *      Author: belyaev
 */

#ifndef LOG_ENTRY_HPP_
#define LOG_ENTRY_HPP_

#include "logger.hpp"
#include <sstream>

namespace borealis {
namespace logging {

class log_entry {
    std::unique_ptr<std::ostringstream> buf;
    logstream log;

public:
    log_entry(const log_entry&) = default;
    log_entry(log_entry&&) = default;
    log_entry(logstream log): buf(new std::ostringstream), log(log) {}

    template<class T>
    log_entry& operator << (const T& val) {
        *buf << val;
        return *this;
    }

    log_entry& operator << (log_entry&(*val)(log_entry&)) {
        return val(*this);
    }

    log_entry& operator << (logstream&(*val)(logstream&)) {
        if(val == &endl || val == &end) {
            *buf << "\n";
        }
        return *this;
    }

    ~log_entry() {
        log << buf->str();
    }

};

}
}


#endif /* LOG_ENTRY_HPP_ */
