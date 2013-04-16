/*
 * DefectInfo.h
 *
 *  Created on: Apr 15, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFECTINFO_H_
#define DEFECTINFO_H_

#include "Util/json_traits.hpp"
#include "Util/locations.h"

namespace borealis {

enum class DefectType {
    INI_03,
    REQ_01,
    ENS_01,
    ASR_01
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

const std::map<DefectType, const DefectSummary> DefectTypeNames = {
    { DefectType::INI_03, { "INI-03", "Dereferencing a nullptr" } },
    { DefectType::REQ_01, { "REQ-01", "Requires contract check failed" } },
    { DefectType::ENS_01, { "ENS-01", "Ensures contract check failed" } },
    { DefectType::ASR_01, { "ASR-01", "Assert check failed" } }
};

const std::map<std::string, DefectType> DefectTypesByName = {
    { "INI-03", DefectType::INI_03 },
    { "REQ-01", DefectType::REQ_01 },
    { "ENS-01", DefectType::ENS_01 },
    { "ASR-01", DefectType::ASR_01 }
};

struct DefectInfo {
    DefectType type;
    Locus location;
};

namespace util {

template<>
struct json_traits<DefectType> {
    typedef std::unique_ptr<DefectType> optional_ptr_t;

    static Json::Value toJson(const DefectType& val) {
        return util::toJson(DefectTypeNames.at(val).type);
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        if (auto v = util::fromJson<std::string>(json)) {
            return optional_ptr_t { new DefectType(DefectTypesByName.at(*v)) };
        } else return nullptr;
    }
};

template<>
struct json_traits<DefectInfo> {
    typedef std::unique_ptr<DefectInfo> optional_ptr_t;

    static Json::Value toJson(const DefectInfo& val) {
        Json::Value dict;
        dict["type"] = util::toJson(val.type);
        dict["location"] = util::toJson(val.location);
        return dict;
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        using borealis::util::json_object_builder;

        json_object_builder<DefectInfo, DefectType, Locus> builder {
            "type", "location"
        };
        return optional_ptr_t {
            builder.build(json)
        };
    }
};

} // namespace util

bool operator==(const DefectInfo& a, const DefectInfo& b);
bool operator<(const DefectInfo& a, const DefectInfo& b);
std::ostream& operator<<(std::ostream& s, const DefectInfo& di);

} // namespace borealis

#endif /* DEFECTINFO_H_ */
