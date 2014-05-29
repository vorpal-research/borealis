/* 
 * Cfgparser - configuration reader C++ library
 * Copyright (c) 2008, Seznam.cz, a.s.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Seznam.cz, a.s.
 * Radlicka 2, Praha 5, 15000, Czech Republic
 * http://www.seznam.cz, mailto:cfgparser@firma.seznam.cz
 * 
 * $Id: $
 *
 * PROJECT
 * Configuration file parser
 *
 * DESCRIPTION
 * Small utility to get value from configuration file
 *
 * AUTHOR
 * Eduard Veleba <eduard.veleba@firma.seznam.cz>
 *
 */


#include <cfgparser.h>
#include <iostream>

/** @short Get value from configuration file.
  *
  * Usage: cfggetvalue CONFIG_FILE SECTION VARIABLE
  */

int main(int argc, char **argv) {

    if (argc != 4) {
        std::cerr << "Usage:" << std::endl
                  << argv[0] << " {CONFIG_FILE} {SECTION} {VARIABLE}"
                  << std::endl;
        return 1;
    }

    std::string filename(argv[1]);
    std::string section(argv[2]);
    std::string variable(argv[3]);

    ConfigParser_t cfg;
    if (cfg.readFile(filename)) {
        std::cerr << "Can't open configuration file '"
                  << filename << "'" << std::endl;
        return 2;
    }

    std::string value;
    if (!cfg.getValue(section, variable, &value)) {
        std::cerr << "Can't find variable '" << variable << "' "
                  << "in section '" << section << "'" << std::endl;
        return 3;
    }

    std::cout << value << std::endl;
    return 0;
}
