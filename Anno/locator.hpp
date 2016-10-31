/*
 * locator.hpp
 *
 *  Created on: Apr 3, 2012
 *      Author: belyaev
 */

#ifndef LOCATOR_HPP_
#define LOCATOR_HPP_

#include <pegtl.hh>

#include <list>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <utility>

namespace borealis {
namespace anno {

template<class T> T some();

template<class Callable>
std::string streamify(Callable fun){
    std::ostringstream oss;
    fun(oss);
    return oss.str();
}

template< typename Location = pegtl::ascii_location >
class calc_exception : public std::exception {
     typedef std::list<std::pair<Location, std::string>> rule_stack;
     Location loc_;
     // using shared ptrs for satisfying
     // all the conditions of base, ostringstream and list
     std::shared_ptr<std::ostringstream> str_rep;
     std::shared_ptr<rule_stack> rule_stack_trace;
public:
     // this is (logically) a move constructor, but who cares
     calc_exception(const calc_exception&) = default;

     explicit calc_exception(const Location& loc) : loc_(loc) {
         str_rep = std::make_shared<std::ostringstream>();
         rule_stack_trace = std::make_shared<rule_stack>();
         *str_rep << loc << std::endl;
     }

     virtual const char* what() const throw() {
         return str_rep->str().c_str();
     }

     const Location& get_location() const {
         return loc_;
     }

     calc_exception& push_rule(const Location& loc, const std::string& rule_rep) {
         rule_stack_trace->push_back({loc,rule_rep});
         *str_rep << loc << ":" << rule_rep << std::endl;
         return *this;
     }

     inline const rule_stack& get_rules() const {
         return *rule_stack_trace;
     }
};

template< typename Location >
inline void except(const Location& loc) {
    throw calc_exception<Location>(loc);
}

template< typename Rule, typename Input, typename Debug >
struct anno_guard : private pegtl::nocopy< anno_guard< Rule, Input, Debug > >
{
    typedef typename Input::location_type Location;

    anno_guard( Location&& w, pegtl::counter& t )
    : m_location( std::move(w) ),
      m_counter( t ) {
        m_counter.enter();
    }

    ~anno_guard() {
        m_counter.leave();
    }

    bool operator()(bool result, bool must) const {
        if(!result && must) return false;
        return result;
    }

    const Location& location() const {
        return m_location;
    }

protected:
    const Location m_location;
    pegtl::counter& m_counter;
};

struct anno_debug : public pegtl::debug_base {
    template<typename TopRule>
    explicit anno_debug(const pegtl::tag<TopRule>& help) :
        unrecover(false), m_printer(help) {}

    std::vector<std::string> trace;
    bool unrecover;

    template<bool Must, typename Rule, typename Input, typename ...States>
    bool match(Input& in, States&&... st) {
        const anno_guard< Rule, Input, anno_debug > d( in.location(), m_counter );

        bool res = d( Rule::template match< Must >( in, *this, std::forward< States >( st )... ), Must );



        if (!res && (Must || unrecover)) {
            unrecover = true;
            trace.push_back(streamify([&](std::ostream& oss) {
                oss << d.location() << ":"
                    << m_counter.nest() << ": expected "
                    << m_printer.template rule< Rule >();
            }));
        }
        return res;
    }

protected:
    pegtl::counter m_counter;
    pegtl::printer m_printer;
};

template< typename TopRule, typename Input, typename ...States >
struct calc_parser {
    bool success;
    std::vector<std::string> str;

    calc_parser( Input& in, States&&... st ) {
        anno_debug de( pegtl::tag< TopRule >( 0 ) );

        success = de.template match< true, TopRule >( in, std::forward< States >( st )... );

        if (!success) {
            str = std::move(de.trace);
        }
    }
};

template< typename TopRule, typename Input, typename ...States >
calc_parser<TopRule, Input, States...> make_calc_parser( Input& in, States&&... st ) {
    return calc_parser<TopRule, Input, States...>(in, std::forward<States>(st)...);
}

template< typename TopRule, typename Location = pegtl::ascii_location, typename ...States >
calc_parser< TopRule, pegtl::string_input< Location >, States... >
calc_parse_string( const std::string& string, States&&... st ) {
   pegtl::string_input< Location > in( string );
   calc_parser< TopRule, decltype(in), States... > cp(in, std::forward<States>(st)...);
   return cp;
}

template< typename TopRule, typename Location, typename Iter, typename ...States >
calc_parser< TopRule, pegtl::forward_input<Iter, Location>, States... >
calc_parse_string( const std::pair<Iter, Iter>& string, States&&... st ) {
    pegtl::forward_input<Iter, Location> in{ string.first, string.second };
    calc_parser< TopRule, decltype(in), States... > cp{ in, std::forward<States>(st)... };
    return cp;
}

} // namespace anno
} // namespace borealis

#endif /* LOCATOR_HPP_ */
