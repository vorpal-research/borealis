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

#include "Logging/logger.hpp"

namespace borealis {
namespace driver {

class plugin_loader: public borealis::logging::DelegateLogging {
    struct impl;
    std::unique_ptr<impl> pimpl;

public:
    enum class status { SUCCESS, FAILURE };
	
    plugin_loader();
    ~plugin_loader();

    void add(const std::string& libname);
    void add(std::string&& libname);
    status run();
};

} // namespace driver
} // namespace borealis

#endif /* PLUGIN_LOADER_H */
