#ifndef _DATABASE_UTIL_H_
#define _DATABASE_UTIL_H_

#include <linux/limits.h>
#include <unistd.h>
#include <stdlib.h>

#include <DB.hpp>

#include "Config/config.h"
#include "Database/SerialTemplateSpec.hpp"

#include "Util/macros.h"

namespace borealis {

static config::StringConfigEntry db_name("database", "database");

std::string getexepath() {
    char result[PATH_MAX];
    readlink("/proc/self/exe", result, PATH_MAX);
    auto found = std::string(result).find_last_of("/");
    return (std::string(result).substr(0, found) + "/");
}

void startDatabaseDaemon() {
    auto& name = db_name.get();
    if (not name) return;

    if (not leveldb_mp::DB::isDaemonStarted(name.getUnsafe())) {
        std::string exePath = getexepath();
        auto pid = fork();
        if (pid == 0) {
            std::string runCmd = exePath + "leveldb_daemon";
            execl(runCmd.c_str(), "leveldb_daemon", name.getUnsafe().c_str());
        }
    }
    auto&& db = leveldb_mp::DB::getInstance();
    db->connect(name.getUnsafe());
}

}   /* namespace borealis */

#include "Util/unmacros.h"

#endif // _DATABASE_UTIL_H_
