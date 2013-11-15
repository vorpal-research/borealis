/*
 * RecordBody.h
 *
 *  Created on: Oct 17, 2013
 *      Author: belyaev
 */

#ifndef RECORDBODY_H_
#define RECORDBODY_H_

#include "Type/Type.h"
#include "Util/util.h"
#include "Util/option.hpp"

namespace borealis {
namespace type {

class RecordField {
    Type::Ptr type;
    std::vector<std::string> ids;
public:
#include "Util/macros.h"
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(RecordField);
#include "Util/unmacros.h"
    RecordField(Type::Ptr type) :
        type{type}, ids{} {};
    RecordField(Type::Ptr type, const std::vector<std::string>& ids) :
        type{type}, ids{ids} {};

    const Type::Ptr& getType() const noexcept { return type; }
    void setType(const Type::Ptr& type) noexcept { this->type = type; }

    const std::vector<std::string>& getIds() const noexcept { return ids; }
    void setIds(const std::vector<std::string>& ids) noexcept { this->ids = ids; }
    void pushId(const std::string& id) { ids.push_back(id); }
};

class RecordBody {
    std::vector<RecordField> fields;
    std::unordered_map<std::string, Type::Ptr> naming;
public:
    RecordBody(const std::vector<RecordField>& fields): fields(fields) {
        for(const auto& fld : fields) {
            for(const auto& name : fld.getIds()) {
                naming[name] = fld.getType();
            }
        }
    }

#include "Util/macros.h"
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(RecordBody);
    auto begin() QUICK_CONST_RETURN(fields.begin());
    auto end() QUICK_CONST_RETURN(fields.end());
    auto empty() QUICK_CONST_RETURN(fields.empty());
#include "Util/unmacros.h"

    void push_back(const RecordField& fld) {
        fields.push_back(fld);
        for(const auto& name : fld.getIds()) {
            naming[name] = fld.getType();
        }
    }

    util::option_ref<const Type::Ptr> getFieldByIdx(unsigned long long idx) const {
        return fields.size() > idx ? util::justRef(fields.at(idx).getType()) : util::nothing();
    }

    util::option_ref<const Type::Ptr> getFieldByName(const std::string& name) const {
        return util::at(naming, name);
    }

    ~RecordBody(){}
};

class RecordRegistry {
public:
    using self = RecordRegistry;
    using Ptr = std::weak_ptr<self>;
    using StrongPtr = std::shared_ptr<self>;

private:
    std::map<std::string, RecordBody> data;
public:
    util::option_ref<const RecordBody> operator[](const std::string& type) const {
        return util::at(data, type);
    }

    RecordBody& operator[](const std::string& type) {
        return data[type];
    }

    util::option_ref<const RecordBody> at(const std::string& type) const {
        return util::at(data, type);
    }

    RecordBody& at(const std::string& type) {
        return data[type];
    }
};

/** protobuf -> Type/RecordBodyRef.proto
import "Type/Type.proto";

package borealis.proto;

message RecordBodyRef {
    message RecordField {
        optional Type type = 1;
        repeated string ids = 2;
    };

    message RecordBody {
        required string id = 1;
        repeated RecordField fields = 2;
    };

    optional string id = 1;
    repeated RecordBody bodyTable = 2;
};

**/
#include "Util/macros.h"

class RecordBodyRef {
public:
    using self = RecordBodyRef;
    using Ptr = std::shared_ptr<self>;
private:
    RecordRegistry::Ptr registry;
    std::string name;
public:
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(RecordBodyRef);
    RecordBodyRef(RecordRegistry::Ptr registry, const std::string& name):
        registry(registry), name(name) {};

    const RecordBody& get() const {
        auto registry = this->registry.lock();
        return registry->at(name);
    }

    RecordRegistry::Ptr getRegistry() const { return registry; }
};

#include "Util/unmacros.h"

} /* namespace type */
} /* namespace borealis */
#endif /* RECORDBODY_H_ */
