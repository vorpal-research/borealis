/*
 * xml.hpp
 *
 *  Created on: Aug 1, 2013
 *      Author: ice-phoenix
 */

#ifndef XML_HPP_
#define XML_HPP_

#include <tinyxml2.h>

#include <memory>
#include <stack>

#include "Util/meta.hpp"
#include "Util/util.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

typedef tinyxml2::XMLDocument& XMLDocumentRef;
typedef tinyxml2::XMLNode* XMLNodePtr;

////////////////////////////////////////////////////////////////////////////////

template<class T, typename SFINAE = void>
struct xml_traits;

////////////////////////////////////////////////////////////////////////////////

template<>
struct xml_traits<XMLNodePtr> {
    static XMLNodePtr toXml(XMLNodePtr, XMLNodePtr node) {
        return node;
    }
};

template<>
struct xml_traits<const char*> {
    static XMLNodePtr toXml(XMLNodePtr p, const char* s) {
        auto* doc = p->GetDocument();
        p->InsertEndChild(
            doc->NewText(s)
        );
        return p;
    }
};

template<>
struct xml_traits<std::string> {
    static XMLNodePtr toXml(XMLNodePtr p, const std::string& s) {
        auto* doc = p->GetDocument();
        p->InsertFirstChild(
            doc->NewText(s.c_str())
        );
        return p;
    }
};

template<class T>
struct xml_traits<T, GUARD(std::is_arithmetic<T>::value)> {
    static XMLNodePtr toXml(XMLNodePtr p, T val) {
        auto* doc = p->GetDocument();
        p->InsertFirstChild(
            doc->NewText(util::toString(val).c_str())
        );
        return p;
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class T>
XMLNodePtr toXml(XMLNodePtr p, const T& val) {
    return xml_traits<T>::toXml(p, val);
}

////////////////////////////////////////////////////////////////////////////////

class Xml {

    std::shared_ptr<tinyxml2::XMLDocument> doc;
    std::stack<tinyxml2::XMLNode*> nodeStack;

public:

    Xml(const std::string& root) : doc(new tinyxml2::XMLDocument()) {
        auto* rootNode = doc->NewElement(root.c_str());
        doc->InsertFirstChild(rootNode);
        nodeStack.push(rootNode);
    }

    tinyxml2::XMLDocument* ToDocument() {
        return doc.get();
    }

    tinyxml2::XMLElement* RootElement() {
        return doc->RootElement();
    }

    friend std::ostream& operator<<(std::ostream& ost, Xml xml) {
        tinyxml2::XMLPrinter p;
        xml.doc->Print(&p);
        return ost << p.CStr();
    }

    Xml& operator>>(const std::string& tag) {
        auto* node = doc->NewElement(tag.c_str());
        nodeStack.top()->InsertEndChild(node);
        nodeStack.push(node);
        return *this;
    }

    enum class EndMarker {};
    static EndMarker END;
    Xml& operator<<(EndMarker) {
        if (nodeStack.size() > 1) nodeStack.pop();
        return *this;
    }

    template<class T>
    struct ListDesc {
        const std::string& elemName;
        const T& list;
    };

    template<class T>
    static ListDesc<T> ListOf(const std::string& elemName, const T& list) {
        return ListDesc<T>{ elemName, list };
    }

    template<class T>
    Xml& operator<<(const ListDesc<T>& desc) {
        auto* top = nodeStack.top();
        xml_traits<T>::toXml(top, desc.list, desc.elemName);
        return *this;
    }

    template<class T>
    Xml& operator<<(const T& value) {
        auto* top = nodeStack.top();
        xml_traits<T>::toXml(top, value);
        return *this;
    }
};

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"

#endif /* XML_HPP_ */
