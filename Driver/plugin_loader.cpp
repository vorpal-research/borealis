/*
 * plugin_loader.cpp
 *
 *  Created on: Aug 22, 2013
 *      Author: belyaev
 */

#include "plugin_loader.h"

#include <llvm/Support/PluginLoader.h>

#include <vector>

namespace borealis {
namespace driver {

struct plugin_loader::impl {
    std::vector<std::string> libs;
};

plugin_loader::plugin_loader() : pimpl{ new impl{} } {}
plugin_loader::~plugin_loader() {}

void plugin_loader::add(const std::string& libname) {
    pimpl->libs.push_back(libname);
}
void plugin_loader::add(std::string&& libname) {
    pimpl->libs.push_back(libname);
}
plugin_loader::status plugin_loader::run() {
    llvm::PluginLoader pl;
    for(const auto& lib: pimpl->libs) {
        pl = lib;
    }
    return status::SUCCESS;
}

} // namespace driver
} // namespace borealis


