/*
 * tracer.hpp
 *
 *  Created on: Nov 23, 2012
 *      Author: belyaev
 */

#ifndef TRACER_HPP_
#define TRACER_HPP_

#include <chrono>

#include "Logging/logstream.hpp"

namespace borealis {
namespace logging {

class func_tracer {

    const char* fname_;
    borealis::logging::logstream log;
    std::chrono::time_point<std::chrono::system_clock> start;

public:

    static const std::string logDomain;

    func_tracer(
        const char* fname,
        borealis::logging::logstream log):
            fname_(fname), log(log), start(std::chrono::system_clock::now()) {
        log << "> " << fname_ << borealis::logging::endl;
    }

    ~func_tracer() {
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        log << "< "
            << fname_
            << " : "
            << duration
            << " µs"
            << borealis::logging::endl;
    }
};

} // namespace logging
} // namespace borealis

#ifndef NO_TRACE_FUNC

#define TRACE_FUNC \
    borealis::logging::func_tracer ftracer( \
        __PRETTY_FUNCTION__, \
        borealis::logging::dbgsFor(borealis::logging::func_tracer::logDomain));

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
#define TRACE_BLOCK(ID)
#define TRACE_MEASURMENT(M...)
#define TRACE_UP(M...)
#define TRACE_DOWN(M...)
#endif

#endif /* TRACER_HPP_ */
