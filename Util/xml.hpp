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
    static XMLNodePtr toXml(XMLDocumentRef, XMLNodePtr node, const std::string& = "") {
        return node;
    }
};

template<>
struct xml_traits<const char*> {
    static XMLNodePtr toXml(XMLDocumentRef doc, const char* s, const std::string& name = "string") {
        auto* res = doc.NewElement(name.c_str());
        res->InsertFirstChild(
            doc.NewText(s)
        );
        return res;
    }
};

template<>
struct xml_traits<std::string> {
    static XMLNodePtr toXml(XMLDocumentRef doc, const std::string& s, const std::string& name = "string") {
        auto* res = doc.NewElement(name.c_str());
        res->InsertFirstChild(
            doc.NewText(s.c_str())
        );
        return res;
    }
};

template<class T>
struct xml_traits<T, GUARD(std::is_integral<T>::value)> {
    static XMLNodePtr toXml(XMLDocumentRef doc, T val, const std::string& name = "integer") {
        auto* res = doc.NewElement(name.c_str());
        res->InsertFirstChild(
            doc.NewText(util::toString(val).c_str())
        );
        return res;
    }
};

template<class T>
struct xml_traits<T, GUARD(std::is_floating_point<T>::value)> {
    static XMLNodePtr toXml(XMLDocumentRef doc, T val, const std::string& name = "float") {
        auto* res = doc.NewElement(name.c_str());
        res->InsertFirstChild(
            doc.NewText(util::toString(val).c_str())
        );
        return res;
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class T>
XMLNodePtr toXml(XMLDocumentRef doc, const T& val) {
    return xml_traits<T>::toXml(doc, val);
}

template<class T>
XMLNodePtr toXml(XMLDocumentRef doc, const T& val, const std::string& name) {
    return xml_traits<T>::toXml(doc, val, name);
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

    template<class T>
    struct Named {
        const std::string& name;
        const T& value;
    };

    template<class T>
    static Named<T> AsNamed(const std::string& name, const T& value) {
        return Named<T>{ name, value };
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
    Xml& operator<<(const Named<T>& e) {
        nodeStack.top()->InsertEndChild(xml_traits<T>::toXml(*doc, e.value, e.name));
        return *this;
    }

    template<class T>
    Xml& operator<<(const T& value) {
        nodeStack.top()->InsertEndChild(xml_traits<T>::toXml(*doc, value));
        return *this;
    }
};

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"

#endif /* XML_HPP_ */
