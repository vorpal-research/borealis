/*
 * log_entry.hpp
 *
 *  Created on: Oct 30, 2012
 *      Author: belyaev
 */

#ifndef LOG_ENTRY_HPP_
#define LOG_ENTRY_HPP_

#include <memory>
#include <sstream>

#include "Logging/logger.hpp"
#include "Logging/logstream.hpp"

namespace borealis {
namespace logging {

class log_entry {
    std::unique_ptr<std::ostringstream> buf;
    logstream log;

public:
    log_entry(const log_entry&) = default;
    log_entry(log_entry&&) = default;
    log_entry(logstream log) : buf(new std::ostringstream()), log(log) {}

    template<class T>
    log_entry& operator<<(const T& val) {
        *buf << val;
        return *this;
    }

    log_entry& operator<<(log_entry&(*val)(log_entry&)) {
        return val(*this);
    }

    log_entry& operator<<(logstream&(*val)(logstream&)) {
        if (val == &endl) {
            *buf << "\n";
        } else if (val == &end) {
            *buf << "\n";
            flush();
        }
        return *this;
    }

    void flush() {
        if (buf) {
            log << buf->str();
            log.flush();

            auto empty = std::unique_ptr<std::ostringstream>();
            buf.swap(empty);
        }
    }

    ~log_entry() { flush(); }

};

} // namespace logging
} // namespace borealis

#endif /* LOG_ENTRY_HPP_ */
