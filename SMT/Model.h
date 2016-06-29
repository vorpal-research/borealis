//
// Created by belyaev on 6/28/16.
//

#ifndef SMT_MODEL_H
#define SMT_MODEL_H

#include "Term/Term.h"
#include "Factory/Nest.h"

namespace borealis {
namespace smt {

namespace proto{ class MemoryShape; }
/** protobuf -> SMT/MemoryShape.proto
import "Term/Term.proto";

package borealis.smt.proto;

message MemoryShape {
    message TermPair {
        optional borealis.proto.Term key = 1;
        optional borealis.proto.Term value = 2;
    }

    repeated TermPair initialShape = 1;
    repeated TermPair finalShape = 2;
}
**/
class MemoryShape {
    using memory_shape_t = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;
    memory_shape_t initialMemoryShape;
    memory_shape_t finalMemoryShape;

public:
    MemoryShape() = default;
    MemoryShape(const MemoryShape&) = default;
    MemoryShape(MemoryShape&&) = default;

    memory_shape_t& getInitialMemoryShape() { return initialMemoryShape; }
    memory_shape_t& getFinalMemoryShape() { return finalMemoryShape; }

    const memory_shape_t& getInitialMemoryShape() const { return initialMemoryShape; }
    const memory_shape_t& getFinalMemoryShape() const { return finalMemoryShape; }

    friend std::ostream& operator<<(std::ostream&, const MemoryShape& mem);
};

namespace proto{ class Model; }
/** protobuf -> SMT/Model.proto
import "Term/Term.proto";
import "SMT/MemoryShape.proto";

package borealis.smt.proto;

message Model {
    message TermPair {
        optional borealis.proto.Term key = 1;
        optional borealis.proto.Term value = 2;
    }

    message SizeShapePair {
        optional uint64 key = 1;
        optional MemoryShape value = 2;
    }

    message StringShapePair {
        optional string key = 1;
        optional MemoryShape value = 2;
    }

    repeated TermPair assignments = 1;
    repeated SizeShapePair memories = 2;
    repeated SizeShapePair bounds = 3;
    repeated StringShapePair properties = 4;
}
**/

class Model {

public:
    using assignments_t = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;
    using memory_spaces_t = std::unordered_map<size_t, MemoryShape>;
    using property_spaces_t = std::unordered_map<std::string, MemoryShape>;

private:
    FactoryNest FN;
    assignments_t assignments;
    memory_spaces_t memories;
    memory_spaces_t bounds;
    property_spaces_t properties;

    const assignments_t& getAssignments() const { return assignments; }
    const memory_spaces_t& getMemories() const { return memories; }
    const memory_spaces_t& getBounds() const { return bounds; }
    const property_spaces_t& getProperties() const { return properties; }

    friend struct protobuf_traits<Model>;

public:
    Model(const FactoryNest& FN): FN(FN) {}

    assignments_t& getAssignments() { return assignments; }
    memory_spaces_t& getMemories() { return memories; }
    memory_spaces_t& getBounds() { return bounds; }
    property_spaces_t& getProperties() { return properties; }

    const FactoryNest& getFactoryNest() const { return FN; }

    Term::Ptr query(Term::Ptr) const;

    friend std::ostream& operator<<(std::ostream& ost, const Model& model) {
        ost << "model: {";
        for(auto&& ass: model.assignments) {
            ost << tfm::format("\n    %s = %s", ass.first, ass.second);
        }

        for(auto&& mem: model.memories) {
            for(auto&& entry: mem.second.getFinalMemoryShape()){
                ost << tfm::format("\n    *(%s)<%d> = %s", entry.first, mem.first, entry.second);
            }
        }

        for(auto&& mem: model.bounds) {
            for(auto&& entry: mem.second.getFinalMemoryShape()){
                ost << tfm::format("\n    \\bound(%s)<%d> = %s", entry.first, mem.first, entry.second);
            }
        }

        for(auto&& mem: model.properties) {
            for(auto&& entry: mem.second.getFinalMemoryShape()){
                ost << tfm::format("\n    \\property(%s, %s) = %s", mem.first, entry.first, entry.second);
            }
        }
        ost << "\n}\n";
        return ost;
    }

    void dump() const { std::cerr << *this; }
};

} /* namespace smt */
} /* namespace borealis */

#endif //SMT_MODEL_H
