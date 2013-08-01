/*
 * xml_traits.hpp
 *
 *  Created on: Aug 1, 2013
 *      Author: ice-phoenix
 */

#ifndef XML_TRAITS_HPP_
#define XML_TRAITS_HPP_

#include "Util/xml.hpp"

namespace borealis {
namespace util {

template<class T>
struct xml_traits<std::vector<T>> {
    static XMLNodePtr toXml(XMLNodePtr p, const std::vector<T>& vec, const std::string& elemName) {
        auto* doc = p->GetDocument();
        for (const auto& e : vec) {
            auto* node = doc->NewElement(elemName.c_str());
            p->InsertEndChild(
                util::toXml(node, e)
            );
        }
        return p;
    }
};

template<class T>
struct xml_traits<std::set<T>> {
    static XMLNodePtr toXml(XMLNodePtr p, const std::set<T>& set, const std::string& elemName) {
        auto* doc = p->GetDocument();
        for (const auto& e : set) {
            auto* node = doc->NewElement(elemName.c_str());
            p->InsertEndChild(
                util::toXml(node, e)
            );
        }
        return p;
    }
};

} // namespace util
} // namespace borealis

#endif /* XML_TRAITS_HPP_ */
