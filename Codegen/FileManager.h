/*
 * FileManager.h
 *
 *  Created on: Sep 9, 2013
 *      Author: belyaev
 */

#ifndef FILEMANAGER_H_
#define FILEMANAGER_H_

#include <memory>
#include <string>

#include "Util/locations.h"

namespace borealis {

class FileManager {

    struct impl;
    std::unique_ptr<impl> pimpl;

public:
    FileManager();
    ~FileManager();

    std::string read(const LocusRange& where);
};

} /* namespace borealis */

#endif /* FILEMANAGER_H_ */
