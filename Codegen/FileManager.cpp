/*
 * FileManager.cpp
 *
 *  Created on: Sep 9, 2013
 *      Author: belyaev
 */

#include "Codegen/FileManager.h"

#include <sstream>
#include <fstream>
#include <vector>

#include "Util/macros.h"

namespace borealis {

struct FileManager::impl {
//    using line_t = size_t;
//    using locusMap = std::map<line_t, std::streamsize>;
//    using fileMap = std::unordered_map<std::string, locusMap>;
//
//    fileMap cascade;
};

FileManager::FileManager() {}
FileManager::~FileManager() {}

// TODO: implement this in a fancy way using seek(), tell() and caches
std::string FileManager::read(const LocusRange& where) {
    ASSERTC(where.lhv.filename == where.rhv.filename);

    std::ifstream file(where.lhv.filename);
    if(!file) return "";
    for(auto i = 0U; i < where.lhv.loc.line-1; ++i) {
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    file.ignore(where.lhv.loc.col-1);

    std::ostringstream out;
    for(auto i = where.lhv.loc.line; i < where.rhv.loc.line; ++i) {
        std::string line;
        std::getline(file, line);
        out << line << '\n';
    }

    {
        std::vector<char> data(where.rhv.loc.col+1);
        file.get(data.data(), where.rhv.loc.col);
        out << data.data(); // funny fact: std::vector<char> is guaranteed to be null-terminated
    }

    return out.str();
}

#include "Util/unmacros.h"

} /* namespace borealis */
