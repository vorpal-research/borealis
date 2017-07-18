//
// Created by abdullin on 3/2/17.
//

#include "Interpreter/Util.hpp"
#include "Module.h"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

namespace {

std::set<const llvm::GlobalVariable*> cycled;

struct Global {
    using Edges = std::vector<const llvm::GlobalVariable*>;
    enum Color {
        WHITE, GREY, BLACK
    };
    Color color_;
    Edges edges_;
};

void getAllGlobals(const llvm::Value* value, Global::Edges& edges) {
    if (auto global = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
        edges.push_back(global);
    } else if (auto constantExpr = llvm::dyn_cast<llvm::ConstantExpr>(value)) {
        for (auto&& op : constantExpr->operands()) {
            getAllGlobals(op, edges);
        }
    } else if (auto constantArray = llvm::dyn_cast<llvm::ConstantArray>(value)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < constantArray->getNumOperands(); ++i) {
            getAllGlobals(constantArray->getOperand(i), edges);
        }
    } else if (auto&& constantStruct = llvm::dyn_cast<llvm::ConstantStruct>(value)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < constantStruct->getNumOperands(); ++i) {
            getAllGlobals(constantStruct->getOperand(i), edges);
        }
    }
}

Global::Edges getEdges(const llvm::GlobalVariable* g) {
    if (not g->hasInitializer()) return {};
    Global::Edges edges;
    for (auto&& op : g->getInitializer()->operands()) {
        getAllGlobals(op, edges);
    }
    return std::move(edges);
}

void topologicalSort(std::map<const llvm::GlobalVariable*, Global>& globals,
                     const llvm::GlobalVariable* current,
                     std::vector<const llvm::GlobalVariable*>& result) {
    if (not util::at(globals, current)) return;
    if (globals[current].color_ == Global::BLACK) return;
    if (globals[current].color_ == Global::GREY) {
        cycled.insert(current);
        return;
    }
    globals[current].color_ = Global::GREY;
    for (auto&& edge : globals[current].edges_) {
        topologicalSort(globals, edge, result);
    }
    globals[current].color_ = Global::BLACK;
    result.push_back(current);
}

} // namespace

Module::Module(const llvm::Module* module, SlotTrackerPass* st)
        : instance_(module),
          ST_(st),
          factory_(this) {
    // Initialize all global variables
    std::vector<const llvm::GlobalVariable*> order; // all global variables in order how they should be declared
    std::map<const llvm::GlobalVariable*, Global> globals; // graph of global variables
    // build graph
    for (auto&& it : instance_->globals()) {
        auto&& edges = getEdges(&it);
        if (edges.empty()) order.push_back(&it);
        else globals.insert({&it, {Global::WHITE, edges}});
    }
    // do topological sorting
    for (auto&& it : globals) {
        if (it.second.color_ == Global::WHITE) {
            topologicalSort(globals, it.first, order);
        }
    }
    // pre-initialize cycled global variables
    for (auto&& it : cycled) {
        globals_[it] = factory_.getBottom(*it->getType());
    }
    initGlobals(order);
}

void Module::initGlobals(std::vector<const llvm::GlobalVariable*>& globals) {
    for (auto&& it : globals) {
        Domain::Ptr globalDomain;
        if (it->hasInitializer()) {
            Domain::Ptr content;
            auto& elementType = *it->getType()->getPointerElementType();
            // If global is simple type, that we should wrap it like array of size 1
            if (elementType.isIntegerTy() || elementType.isFloatingPointTy()) {
                auto simpleConst = factory_.get(it->getInitializer());
                auto&& arrayType = llvm::ArrayType::get(&elementType, 1);
                content = factory_.getAggregateObject(*arrayType, {simpleConst});
            // else just create domain
            } else {
                content = factory_.get(it->getInitializer());
            }
            ASSERT(content, "Unsupported constant");

            PointerLocation loc = {factory_.getIndex(0), content};
            auto newDomain = factory_.getPointer(elementType, {loc});
            // we need this because GEPs for global structs and arrays contain one additional index at the start
            if (not (elementType.isIntegerTy() || elementType.isFloatingPointTy())) {
                auto newArray = llvm::ArrayType::get(it->getType(), 1);
                auto newLevel = factory_.getAggregateObject(*newArray, {newDomain});
                PointerLocation loc2 = {factory_.getIndex(0), newLevel};
                globalDomain = factory_.getPointer(*newArray, {loc2});
            } else {
                globalDomain = newDomain;
            }

        } else {
            globalDomain = factory_.getBottom(*it->getType());
        }

        ASSERT(globalDomain, "Could not create domain for: " + ST_->toString(it));
        globals_.insert( {it, globalDomain} );
    }
}

Function::Ptr Module::get(const llvm::Function* function) {
    if (auto&& opt = util::at(functions_, function)) {
        return opt.getUnsafe();
    } else {
        auto result = Function::Ptr{ new Function(function, &factory_, ST_->getSlotTracker(function)) };
        functions_.insert( {function, result} );
        return result;
    }
}

Function::Ptr Module::get(const std::string& fname) {
    auto&& function = instance_->getFunction(fname);
    return (function) ? get(function) : nullptr;
}

const llvm::Module* Module::getInstance() const {
    return instance_;
}

const Module::GlobalsMap& Module::getGloabls() const {
    return globals_;
}

Domain::Ptr Module::findGlobal(const llvm::Value* val) const {
    auto&& it = globals_.find(val);
    return (it == globals_.end()) ? nullptr : it->second;
}

void Module::setGlobal(const llvm::Value* val, Domain::Ptr domain) {
    globals_[val] = domain;
}

std::string Module::toString() const {
    std::ostringstream ss;
    ss << *this;
    return ss.str();
}

DomainFactory* Module::getDomainFactory() {
    return &factory_;
}

SlotTrackerPass* Module::getSlotTracker() const {
    return ST_;
}

const Module::FunctionMap& Module::getFunctions() const {
    return functions_;
}

Domain::Ptr Module::getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location) {
    if (llvm::isa<llvm::GlobalVariable>(value)) {
        return findGlobal(value);
    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return factory_.get(constant);
    } else {
        return get(location->getParent())->getDomainFor(value, location);
    }
    UNREACHABLE("Unknown type of value: " + ST_->toString(value));
}

Module::GlobalsMap Module::getGlobalsFor(const Function::Ptr f) const {
    return util::viewContainer(f->getGlobals())
            .map([&](auto&& a) -> std::pair<const llvm::Value*, Domain::Ptr> { return {a, findGlobal(a)}; })
            .toMap();
}

Module::GlobalsMap Module::getGlobalsFor(const BasicBlock* bb) const {
    return util::viewContainer(bb->getGlobals())
            .map([&](auto&& a) -> std::pair<const llvm::Value*, Domain::Ptr> { return {a, findGlobal(a)}; })
            .toMap();
}

bool function_types_eq(const llvm::Type* lhv, const llvm::Type* rhv) {
    auto* lhvf = llvm::cast<llvm::FunctionType>(lhv);
    auto* rhvf = llvm::cast<llvm::FunctionType>(rhv);
    ASSERT(lhvf && rhvf, "Non-function types in comparing prototypes");
    if (not util::llvm_types_eq(lhvf->getReturnType(), rhvf->getReturnType())) return false;
    if (lhvf->isVarArg() && lhvf->getNumParams() > rhvf->getNumParams()) return false;
    if (not lhvf->isVarArg() && lhvf->getNumParams() != rhvf->getNumParams()) return false;
    for (auto i = 0U; i < lhvf->getNumParams(); ++i) {
        if (not util::llvm_types_eq(lhvf->getParamType(i), rhvf->getParamType(i))) return false;
    }
    return true;
}

std::vector<Function::Ptr> Module::findFunctionsByPrototype(const llvm::Type* prototype) const {
    std::vector<Function::Ptr> result;
    for (auto&& it : util::viewContainer(functions_)
            .filter([](auto&& a) -> bool {
                return a.first->hasAddressTaken();
            })) {
        if (function_types_eq(it.first->getType()->getPointerElementType(), prototype)) {
            result.push_back(it.second);
        }
    }
    return std::move(result);
}

std::ostream& operator<<(std::ostream& s, const Module& m) {
    if (not m.getGloabls().empty()) {
        s << "Global Variables: " << std::endl;
        for (auto&& global : m.getGloabls()) {
            s << "  " << global.first->getName().str() << " = " << global.second->toPrettyString("  ") << std::endl;
            s.flush();
        }
    }
    s << std::endl;
    for (auto&& it : m.getFunctions()) {
        s << std::endl << std::endl << it.second << std::endl << std::endl;
        s.flush();
    }
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m) {
    if (not m.getGloabls().empty()) {
        s << "Global Variables: " << endl;
        for (auto&& global : m.getGloabls()) {
            s << "  " << global.first->getName().str() << " = " << global.second->toPrettyString("  ") << endl;
            s.flush();
        }
    }
    s << endl;
    for (auto&& it : m.getFunctions()) {
        s << endl << endl << it.second << endl << endl;
        s.flush();
    }
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
