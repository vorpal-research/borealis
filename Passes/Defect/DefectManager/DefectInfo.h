/*
 * DefectInfo.h
 *
 *  Created on: Apr 15, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFECTINFO_H_
#define DEFECTINFO_H_

#include "Util/json_traits.hpp"
#include "Util/util.h"
#include "Util/xml_traits.hpp"

#include "SMT/Result.h"

namespace borealis {

enum class DefectType {
#define HANDLE_DEFECT(NAME, STR, DESC) NAME,
#include "Passes/Defect/DefectManager/Defects.def"
};

struct DefectSummary {
    std::string type;
    std::string description;
};

template<class Streamer>
Streamer& operator<<(Streamer& str, const DefectSummary& ds) {
    // this is generally fucked up
    return static_cast<Streamer&>(str << "\"" << ds.type << "\": " << ds.description);
}

const std::map<DefectType, const DefectSummary> DefectTypes = {
#define HANDLE_DEFECT(NAME, STR, DESC) { DefectType::NAME, { STR, DESC } },
#include "Passes/Defect/DefectManager/Defects.def"
};

const std::unordered_map<std::string, DefectType> DefectTypesByName = {
#define HANDLE_DEFECT(NAME, STR, DESC) { STR, DefectType::NAME },
#include "Passes/Defect/DefectManager/Defects.def"
};

namespace util {

////////////////////////////////////////////////////////////////////////////////
// Json
////////////////////////////////////////////////////////////////////////////////
template<>
struct json_traits<DefectType> {
    typedef std::unique_ptr<DefectType> optional_ptr_t;

    static json::Value toJson(const DefectType& val) {
        return util::toJson(DefectTypes.at(val).type);
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        if (auto v = util::fromJson<std::string>(json)) {
            return optional_ptr_t{ new DefectType{ DefectTypesByName.at(*v) } };
        } else return nullptr;
    }
};

} /* namespace util */

#include "Util/generate_macros.h"

struct DefectInfo {
    std::string type;
    Locus location;

    GENERATE_EQ(DefectInfo, type, location);
    GENERATE_LESS(DefectInfo, type, location);
    GENERATE_PRINT_CUSTOM("", "", ":", DefectInfo, type, location);
};

} /* namespace borealis */

// FIXME: revise this. Do we need the model in json?
GENERATE_OUTLINE_JSON_TRAITS(borealis::DefectInfo, type, location);
GENERATE_OUTLINE_HASH(borealis::DefectInfo, type, location);

#include "Util/generate_unmacros.h"

namespace borealis {
namespace util {

////////////////////////////////////////////////////////////////////////////////
// XML
////////////////////////////////////////////////////////////////////////////////
template<>
struct xml_traits<DefectType> {
    static XMLNodePtr toXml(XMLNodePtr p, const DefectType& val) {
        return util::toXml(p, util::nochar(DefectTypes.at(val).type, '-'));
    }
};

template<>
struct xml_traits<DefectInfo> {
    static XMLNodePtr toXml(XMLNodePtr p, const DefectInfo& val) {
        auto* doc = p->GetDocument();
        auto* parentDefect = doc->NewElement("parentDefect");
        auto* type = doc->NewElement("type");
        parentDefect->InsertEndChild(
            util::toXml(type, val.type)
        );
        auto* loc = doc->NewElement("location");
        parentDefect->InsertEndChild(
            util::toXml(loc, val.location)
        );
        p->InsertEndChild(parentDefect);
        return p;
    }
};

} // namespace util
} // namespace borealis

#endif /* DEFECTINFO_H_ */
