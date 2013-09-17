/*
 * intrinsics_manager.h
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#ifndef INTRINSICS_MANAGER_H_
#define INTRINSICS_MANAGER_H_

#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>

#include <functional>
#include <list>
#include <unordered_map>

#include "Codegen/intrinsics.h"
#include "Factory/Nest.h"
#include "Util/util.h"

namespace borealis {

class IntrinsicsManager {

public:

    typedef std::function<PredicateState::Ptr(llvm::Function*, FactoryNest)> state_generator;
    typedef std::function<function_type(const IntrinsicsManager&, const llvm::CallInst&)> type_resolver;

    struct IntrinsicInfo {
        function_type type;
        std::string name;
        state_generator generator;
    };

    struct RegisterIntrinsic : public IntrinsicInfo {
        RegisterIntrinsic(
                function_type type,
                const std::string& name,
                state_generator generator = DefaultGenerator,
                type_resolver type_resolver = nullptr)
        : IntrinsicInfo{ type, name, generator } {
            IntrinsicsManager::getInstance().registerIntrinsic(*this);
            if (type_resolver) {
                IntrinsicsManager::getInstance().registerResolver(type_resolver);
            }
        }

        static PredicateState::Ptr DefaultGenerator(
            llvm::Function*, FactoryNest FN
        ) {
            return FN.State->Basic();
        }
    };

    llvm::Function* createIntrinsic(
            function_type ft,
            const std::string& ext,
            llvm::FunctionType* funtype,
            llvm::Module* module);

    function_type getIntrinsicType(const llvm::CallInst& CI) const;

    void registerIntrinsic(const IntrinsicInfo& info) {
        info_cache[info.type] = info;
        name_cache[info.name] = info.type;
    }

    void registerResolver(type_resolver& resolver) {
        resolvers.push_back(resolver);
    }

    PredicateState::Ptr getPredicateState(
        function_type, llvm::Function*, FactoryNest) const;

    static IntrinsicsManager& getInstance() {
        static IntrinsicsManager instance;
        return instance;
    }

    const std::string getFuncName(function_type ft, const std::string& ext) const;

    void updateForModule(llvm::Module& M);

private:

    typedef std::unordered_map< std::tuple<function_type, llvm::FunctionType*, llvm::Module*>, llvm::Function*> typed_intrinsics_cache;
    typedef std::unordered_map< std::string, function_type > function_name_cache;
    typedef std::unordered_map<llvm::Function*, function_type> function_type_cache;
    typedef std::unordered_map<function_type, IntrinsicInfo> intrinsic_info_cache;
    typedef std::list<type_resolver> type_resolvers;

    typed_intrinsics_cache intrinsics_cache;
    function_type_cache type_cache;
    intrinsic_info_cache info_cache;
    function_name_cache name_cache;

    static function_type default_resolver(const IntrinsicsManager& m, const llvm::CallInst& CI) {
        return m.getIntrinsicType(CI.getCalledFunction());
    };

    type_resolvers resolvers {
        default_resolver
    };

    function_type getIntrinsicType(llvm::Function* F) const;
    IntrinsicInfo getIntrinsicInfo(function_type ft) const;
};

} // namespace borealis

#endif /* INTRINSICS_MANAGER_H_ */
