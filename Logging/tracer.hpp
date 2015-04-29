/*
 * tracer.hpp
 *
 *  Created on: Nov 23, 2012
 *      Author: belyaev
 */

#ifndef TRACER_HPP_
#define TRACER_HPP_

#include <chrono>

#include <tinyformat/tinyformat.h>

#include "Logging/logstream.hpp"

namespace borealis {
namespace logging {

class func_tracer {
    const char* fname_;
    borealis::logging::logstream log;

public:

    static const std::string logDomain;

    func_tracer(const char* fname, borealis::logging::logstream log):
            fname_(fname), log(log) {
        printTime('>');
    }

    ~func_tracer() {
        printTime('<');
    }

private:
    void printTime(char eventType) {
        using namespace std::chrono;
        auto micros = duration_cast<microseconds>(system_clock::now().time_since_epoch());
        log << eventType
            << " "
            << fname_
            << " : "
            << micros.count()
            << " µs"
            << borealis::logging::endl
            << borealis::logging::end;
    }
};

static struct null_trace_stream_t {} null_trace_stream;

template<class Arg>
inline null_trace_stream_t operator<<(null_trace_stream_t st, Arg&&) { return st; }

} // namespace logging
} // namespace borealis

#ifndef NO_TRACING

#define TRACE_FUNC \
    borealis::logging::func_tracer ftracer( \
        __PRETTY_FUNCTION__, \
        borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain));

#define TRACE_PARAM(P) borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain) \
    << #P << " = " << P << borealis::logging::endl

#define TRACE_FMT(P...) borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain) \
    << tfm::format(P) << borealis::logging::endl

#define TRACES() borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain)

#define TRACE_BLOCK(ID) \
    borealis::logging::func_tracer ftracer( \
        ID, \
        borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain));

#define TRACE_MEASUREMENT(M...) \
    borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain) \
        << "= " << borealis::util::join(M) << borealis::logging::endl;

#define TRACE_UP(M...) \
    borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain) \
        << "> " << borealis::util::join(M) << borealis::logging::endl;

#define TRACE_DOWN(M...) \
    borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain) \
        << "< " << borealis::util::join(M) << borealis::logging::endl;

#else
#define TRACE_FUNC
#define TRACE_PARAM(...)
#define TRACE_FMT(...)
#define TRACES() borealis::logging::null_trace_stream
#define TRACE_BLOCK(ID)
#define TRACE_MEASUREMENT(M...)
#define TRACE_UP(M...)
#define TRACE_DOWN(M...)
#endif

#endif /* TRACER_HPP_ */
