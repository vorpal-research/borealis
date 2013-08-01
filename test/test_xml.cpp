/*
 * test_xml.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: kopcap
 */

#include <gtest/gtest.h>

#include "Util/util.h"
#include "Util/xml.hpp"
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
    static XMLNodePtr toXml(XMLDocumentRef doc, const foo& val, const std::string& name = "foo") {
        auto res = doc.NewElement(name.c_str());
        res->InsertEndChild(
            util::toXml(doc, val.i, "i")
        );
        res->InsertEndChild(
            util::toXml(doc, val.f, "f")
        );
        res->InsertEndChild(
            util::toXml(doc, val.s, "s")
        );
        return res;
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
                           << Xml::AsNamed("bar", foo)
                       << Xml::END
                       >> "baz"
                           << foo;

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
                    "<foo>"
                        "<i>3</i>"
                        "<f>14.15</f>"
                        "<s>21</s>"
                    "</foo>"
                "</baz>"
            "</root>"
        );

        EXPECT_TRUE(xml2.RootElement()->ShallowEqual(xml.RootElement()));
    }

}

} // namespace
