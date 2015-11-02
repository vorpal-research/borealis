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

#include "Util/macros.h"
#include "Util/generate_macros.h"

namespace borealis {
namespace type {

class RecordField {
    Type::Ptr type;
    size_t offset = 0U;
public:
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(RecordField);

    RecordField(Type::Ptr type, size_t offset) :
        type{type}, offset{offset} {};

    const Type::Ptr& getType() const noexcept { return type; }
    size_t getOffset() const noexcept { return offset; };
};

class RecordBody {
    std::vector<RecordField> fields;
public:
    RecordBody(const std::vector<RecordField>& fields) : fields{fields} {}

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(RecordBody);

    auto begin() QUICK_CONST_RETURN(fields.begin());
    auto end() QUICK_CONST_RETURN(fields.end());
    auto empty() QUICK_CONST_RETURN(fields.empty());

    size_t getNumFields() const noexcept { return fields.size(); }

    size_t offsetToIndex(size_t offset) const {
        for(size_t i = 0; i < fields.size(); ++i) if(fields[i].getOffset() == offset) return i;

        return ~size_t(0);
    }

    util::option_ref<const RecordField> getFieldByOffset(size_t offset) const {
        for(auto&& field: fields) if(field.getOffset() == offset) {
            return util::justRef(field);
        }

        return util::nothing();
    }

    RecordField& at(size_t ix) {
        return fields.at(ix);
    }
    const RecordField& at(size_t ix) const {
        return fields.at(ix);
    }

    friend std::ostream& operator<<(std::ostream& ost, const RecordBody& rb) {
        ost << "{";
        for(auto&& field : rb.fields) {
            ost << field.getOffset() << ": " << field.getType().get() << "; ";
        }
        ost << "}";
        return ost;
    }

};

class RecordRegistry {
public:
    using self = RecordRegistry;
    using Ptr = std::weak_ptr<self>;
    using StrongPtr = std::shared_ptr<self>;
private:
    std::map<std::string, RecordBody> data;
public:

    RecordBody& operator[](const std::string& type) {
        return data[type];
    }

    util::option_ref<const RecordBody> at(const std::string& type) const {
        return util::at(data, type);
    }

    void clear() {
        data.clear();
    }

    bool empty() const{
        return data.empty();
    }

    size_t count(const std::string& key) const {
        return data.count(key);
    }

    auto begin() const -> decltype(data.begin()) { return data.begin(); }
    auto end() const -> decltype(data.end()) { return data.end(); }
};

/** protobuf -> Type/RecordBodyRef.proto
import "Type/Type.proto";

package borealis.proto;

message RecordBodyRef {
    message RecordField {
        optional Type type = 1;
        optional uint64 offset = 2;
    };

    message RecordBody {
        optional string id = 1;
        repeated RecordField fields = 2;
    };

    optional string id = 1;
    repeated RecordBody bodyTable = 2;
};

**/
class RecordBodyRef {
public:
    using self = RecordBodyRef;
    using Ptr = std::shared_ptr<self>;
private:
    RecordRegistry::Ptr registry;
    std::string name;
public:
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(RecordBodyRef);

    RecordBodyRef(RecordRegistry::Ptr registry, const std::string& name) :
        registry(registry), name(name) {};

    const RecordBody& get() const {
        auto registry = this->registry.lock();
        auto res = registry->at(name);
        ASSERTC(!res.empty());
        return res.getUnsafe();
    }

    RecordRegistry::Ptr getRegistry() const noexcept { return registry; }
};

} /* namespace type */
} /* namespace borealis */

#include "Util/unmacros.h"
#include "Util/generate_unmacros.h"

#endif /* RECORDBODY_H_ */
