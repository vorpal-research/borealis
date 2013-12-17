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

namespace impl_ {
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
    impl_::stream_t inner;
    unsigned char indent;
    bool shouldIndentNext;

    unsigned long long mode;

    logstream(impl_::stream_t inner) :
        inner(inner), indent(0U), shouldIndentNext(false), mode(0ULL) {};

    logstream& incIndent() {
        ++indent;
        return *this;
    }

    logstream& decIndent() {
        --indent;
        return *this;
    }

    logstream& putIndent() {
        if (shouldIndentNext) {
            shouldIndentNext = false;
            for (auto i = indent; i != 0; --i) {
                inner << "  ";
            }
        }
        return *this;
    }

    logstream& indentOn() {
        shouldIndentNext = true;
        return *this;
    }

    logstream& indentOff() {
        shouldIndentNext = false;
        return *this;
    }

public:

    struct mode_on { unsigned long long mode; };
    struct mode_off { unsigned long long mode; };

    template<class T>
    logstream& operator<<(const T& val) {
        putIndent();
        inner << val;
        return *this;
    }

    logstream& operator<<(logstream&(*mutator)(logstream&)) {
        putIndent();
        return mutator(*this);
    }

    logstream& operator<<(const mode_on& m) {
        mode |= m.mode;
        return *this;
    }

    logstream& operator<<(const mode_off& m) {
        mode &= ~m.mode;
        return *this;
    }

    bool hasMode(unsigned long long m) const {
        return mode & m;
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

    friend logstream& indent(logstream&);
    friend logstream& il(logstream&);
    friend logstream& ir(logstream&);

    friend logstream& endl(logstream&);
    friend logstream& end(logstream&);
};

logstream dbgsFor(const std::string& category);
logstream infosFor(const std::string& category);
logstream warnsFor(const std::string& category);
logstream errsFor(const std::string& category);
logstream criticalsFor(const std::string& category);
logstream logsFor(PriorityLevel lv, const std::string& category);

logstream& indent(logstream&);
logstream& il(logstream&);
logstream& ir(logstream&);

logstream& endl(logstream&);
logstream& end(logstream&);

logstream& print_predicate_locus_on(logstream&);
logstream& print_predicate_locus_off(logstream&);
bool with_predicate_locus(logstream&);

void configureLoggingFacility(const std::string& filename);
void configureZ3Log(const std::string& filename);

} // namespace logging
} // namespace borealis

#endif /* LOGSTREAM_HPP_ */
