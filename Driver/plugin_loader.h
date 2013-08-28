/*
 * plugin_loader.h
 *
 *  Created on: Aug 22, 2013
 *      Author: belyaev
 */

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <memory>
#include <string>

namespace borealis {
namespace driver {

class plugin_loader {
    struct impl;
    std::unique_ptr<impl> pimpl;
public:
    plugin_loader();
    ~plugin_loader();

    enum class status { SUCCESS, FAILURE };

    void add(const std::string& libname);
    void add(std::string&& libname);
    status run();
};

} // namespace driver
} // namespace borealis

#endif /* PLUGIN_LOADER_H */
