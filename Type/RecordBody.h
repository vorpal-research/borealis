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
    size_t index = 0U;
public:
#include "Util/macros.h"
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(RecordField);
#include "Util/unmacros.h"
    RecordField(Type::Ptr type, size_t index) :
        type{type}, ids{}, index{index} {};
    RecordField(Type::Ptr type, const std::vector<std::string>& ids, size_t index) :
        type{type}, ids{ids}, index{index} {};

    const Type::Ptr& getType() const noexcept { return type; }
    void setType(const Type::Ptr& type) noexcept { this->type = type; }

    size_t getIndex() const noexcept { return index; };
    void setIndex(size_t ix) noexcept { index = ix; }

    const std::vector<std::string>& getIds() const noexcept { return ids; }
    void setIds(const std::vector<std::string>& ids) noexcept { this->ids = ids; }
    void pushId(const std::string& id) { ids.push_back(id); }
};

class RecordBody {
    std::vector<RecordField> fields;
    std::unordered_map<std::string, size_t> naming;
public:
    RecordBody(const std::vector<RecordField>& fields): fields(fields) {
        for(auto i = 0U; i < fields.size(); ++i) {
            this->fields.at(i).setIndex(i);
        }

        for(auto i = 0U; i < fields.size(); ++i) {
            for(const auto& name : fields.at(i).getIds()) {
                naming[name] = i;
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
        auto& inserted = fields.back();
        auto index = fields.size() - 1;
        inserted.setIndex(index);
        for(const auto& name : inserted.getIds()) {
            naming[name] = index;
        }
    }

    size_t getNumFields() const { return fields.size(); }

    util::option_ref<const RecordField> getFieldByName(const std::string& name) const {
        for(auto index: util::at(naming, name)) return util::justRef(fields.at(index));
        return util::nothing();
    }

    RecordField& at(size_t ix) {
        return fields.at(ix);
    }
    const RecordField& at(size_t ix) const {
        return fields.at(ix);
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
        return (*registry)[name];
    }

    RecordRegistry::Ptr getRegistry() const { return registry; }
};

#include "Util/unmacros.h"

} /* namespace type */
} /* namespace borealis */
#endif /* RECORDBODY_H_ */
