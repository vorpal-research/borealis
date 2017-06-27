#ifndef TIME_HPP
#define TIME_HPP

#include <chrono>

namespace borealis {

using namespace std::literals::chrono_literals;

namespace util {

struct StopWatch {
    using clock_t = std::chrono::steady_clock;
    clock_t::time_point start = clock_t::now();

//    operator std::chrono::seconds() {
//        return { clock_t::now() - start };
//    }
//
//    operator std::chrono::milliseconds() {
//        return { clock_t::now() - start };
//    }
//
//    operator std::chrono::microseconds() {
//        return { clock_t::now() - start };
//    }

    std::chrono::nanoseconds duration() {
        return { clock_t::now() - start };
    }

    operator std::chrono::nanoseconds() {
        return duration();
    }



};

} /* namespace util */
} /* namespace borealis */

#endif // TIME_HPP
