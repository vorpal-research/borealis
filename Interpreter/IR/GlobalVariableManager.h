//
// Created by abdullin on 10/26/17.
//

#ifndef BOREALIS_GLOBALVARIABLEMANAGER_H
#define BOREALIS_GLOBALVARIABLEMANAGER_H

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <llvm/IR/GlobalVariable.h>

#include "Interpreter/Domain/DomainFactory.h"

namespace borealis {
namespace absint {
namespace ir {

//class GlobalVar: public std::enable_shared_from_this<GlobalVar> {
//public:
//    enum Color {
//        WHITE, GREY, BLACK
//    };
//
//    using Ptr = std::shared_ptr<GlobalVar>;
//    using Edges = std::unordered_set<const llvm::GlobalVariable*>;
//
//    explicit GlobalVar(const llvm::GlobalVariable* value);
//
//    void addEdge(const llvm::GlobalVariable* edge);
//    const Edges& getEdges() const;
//    const llvm::GlobalVariable* getValue();
//
//    Color getColor() const;
//    void setColor(Color color);
//
//    static void getAllGlobals(std::unordered_set<const llvm::GlobalVariable*>& globals, const llvm::Value* value);
//
//private:
//    void initEdges();
//
//    const llvm::GlobalVariable* value_;
//    Edges edges_;
//    Color color_;
//};
//
//class Module;
//
//class GlobalVariableManager {
//public:
//    using DomainMap = std::unordered_map<const llvm::Value*, Domain::Ptr>;
//    using StringMap = std::unordered_map<std::string, const llvm::Value*>;
//
//    GlobalVariableManager(Module* module);
//
//    void init(const std::vector<const llvm::GlobalVariable*>& globs);
//    Domain::Ptr findGlobal(const llvm::Value* val) const;
//    Domain::Ptr get(const std::string& name);
//    Function::Ptr getFunctionByName(const std::string& name);
//    Function::Ptr get(const llvm::Function* function);
//
//    const DomainMap& getGlobals() const;
//
//private:
//
//    void topologicalSort(const llvm::GlobalVariable* current,
//                         std::unordered_map<const llvm::GlobalVariable*, GlobalVar::Ptr>& globals,
//                         std::vector<const llvm::GlobalVariable*>& ordered,
//                         std::unordered_set<const llvm::GlobalVariable*>& cycled);
//
//    Module* module_;
//    DomainFactory* DF_;
//    DomainMap globals_;
//    StringMap names_;
//
//};

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */


#endif //BOREALIS_GLOBALVARIABLEMANAGER_H
