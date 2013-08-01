/*
 * test_xml.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: kopcap
 */

#include <gtest/gtest.h>

#include "Util/util.h"
#include "Util/xml_traits.hpp"

namespace borealis {
namespace util {

struct foo {
    int i;
    float f;
    std::string s;
};

template<>
struct xml_traits<foo> {
    static XMLNodePtr toXml(XMLNodePtr p, const foo& val) {
        auto* doc = p->GetDocument();
        auto* i = doc->NewElement("i");
        p->InsertEndChild(
            util::toXml(i, val.i)
        );
        auto* f = doc->NewElement("f");
        p->InsertEndChild(
            util::toXml(f, val.f)
        );
        auto* s = doc->NewElement("s");
        p->InsertEndChild(
            util::toXml(s, val.s)
        );
        return p;
    }
};

} // namespace util
} // namespace borealis

namespace {

using namespace borealis::util;

TEST(Xml, write) {

    {
        foo foo { 3, 14.15, "21" };

        auto xml = Xml("root")
                       >> "foo"
                           >> "bar"
                               << foo
                           << Xml::END
                       << Xml::END
                       >> "baz"
                           << foo;

        std::ostringstream str;
        str << xml;

        tinyxml2::XMLDocument xml2;
        xml2.Parse(
            "<root>"
                "<foo>"
                    "<bar>"
                        "<i>3</i>"
                        "<f>14.15</f>"
                        "<s>21</s>"
                    "</bar>"
                "</foo>"
                "<baz>"
                    "<i>3</i>"
                    "<f>14.15</f>"
                    "<s>21</s>"
                "</baz>"
            "</root>"
        );

        tinyxml2::XMLPrinter p;
        xml2.Print(&p);

        std::ostringstream str2;
        str2 << p.CStr();

        EXPECT_EQ(str2.str(), str.str());
    }

}

} // namespace
