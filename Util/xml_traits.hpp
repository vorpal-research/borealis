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
    static XMLNodePtr toXml(XMLDocumentRef doc, const std::vector<T>& vec, const std::string& name = "vector") {
        auto* res = doc.NewElement(name.c_str());
        for (const auto& e : vec) {
            res->InsertEndChild(
                util::toXml(doc, e)
            );
        }
        return res;
    }
};

template<class T>
struct xml_traits<std::set<T>> {
    static XMLNodePtr toXml(XMLDocumentRef doc, const std::set<T>& set, const std::string& name = "set") {
        auto* res = doc.NewElement(name.c_str());
        for (const auto& e : set) {
            res->InsertEndChild(
                util::toXml(doc, e)
            );
        }
        return res;
    }
};

} // namespace util
} // namespace borealis

#endif /* XML_TRAITS_HPP_ */
