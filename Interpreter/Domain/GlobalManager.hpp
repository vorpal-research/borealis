//
// Created by abdullin on 2/11/19.
//

#ifndef BOREALIS_GLOBALMANAGER_HPP
#define BOREALIS_GLOBALMANAGER_HPP

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "Domain.h"


namespace borealis {
namespace absint {

namespace impl_ {

class GlobalVar : public std::enable_shared_from_this<GlobalVar> {
public:
    enum Color {
        WHITE, GREY, BLACK
    };

    using Ptr = std::shared_ptr<GlobalVar>;
    using Edges = std::unordered_set<const llvm::GlobalVariable*>;

    explicit GlobalVar(const llvm::GlobalVariable* value);

    void addEdge(const llvm::GlobalVariable* edge);

    const Edges& getEdges() const;

    const llvm::GlobalVariable* getValue();

    Color getColor() const;

    void setColor(Color color);

    static void getAllGlobals(std::unordered_set<const llvm::GlobalVariable*>& globals, const llvm::Value* value);

private:
    void initEdges();

    const llvm::GlobalVariable* value_;
    Edges edges_;
    Color color_;
};

} // namespace impl_

namespace ir {

class Module;
class Function;

} // namespace ir

class GlobalManager {
public:

    using DomainMap = std::unordered_map<const llvm::Value*, AbstractDomain::Ptr>;
    using StringMap = std::unordered_map<std::string, const llvm::Value*>;

    GlobalManager(ir::Module* module);

    void init(const std::vector<const llvm::GlobalVariable*>& globs);
    AbstractDomain::Ptr findGlobal(const llvm::Value* val) const;
    AbstractDomain::Ptr get(const std::string& name);
    std::shared_ptr<ir::Function> getFunctionByName(const std::string& name);
    std::shared_ptr<ir::Function> get(const llvm::Function* function);

    const DomainMap& getGlobals() const;

private:

    void topologicalSort(const llvm::GlobalVariable* current,
                         std::unordered_map<const llvm::GlobalVariable*, impl_::GlobalVar::Ptr>& globals,
                         std::vector<const llvm::GlobalVariable*>& ordered,
                         std::unordered_set<const llvm::GlobalVariable*>& cycled);

    AbstractDomain::Ptr allocate(const llvm::GlobalObject* object) const;

private:

    ir::Module* module_;
    DomainMap globals_;
    StringMap names_;

};

} // namespace absint
} // namespace borealis


#endif //BOREALIS_GLOBALMANAGER_HPP
