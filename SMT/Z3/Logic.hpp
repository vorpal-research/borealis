/*
 * Logic.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: belyaev
 */

#ifndef Z3_LOGIC_HPP
#define Z3_LOGIC_HPP

#include <z3/z3++.h>

#include <functional>
#include <numeric>
#include <algorithm>

#include "Util/util.h"
#include "Util/functional.hpp"

#include "Config/config.h"

#include "SMT/Z3/Z3Engine.h"

#define ENGINE Z3Engine
#define NAMESPACE z3_

#include "SMT/Logic.stub.hpp"

#undef NAMESPACE
#undef ENGINE

#endif // Z3_LOGIC_HPP
