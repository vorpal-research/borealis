/*
 * Logic.cpp
 *
 *  Created on: Jun 15, 2016
 *      Author: belyaev
 */

#ifndef BOOLECTOR_LOGIC_HPP
#define BOOLECTOR_LOGIC_HPP

#include <functional>
#include <numeric>
#include <algorithm>

#include "Util/util.h"
#include "Util/functional.hpp"

#include "Config/config.h"

#include "SMT/Boolector/BoolectorEngine.h"

#define ENGINE BoolectorEngine
#define NAMESPACE boolector_

#include "SMT/Logic.stub.hpp"

#undef NAMESPACE
#undef ENGINE

#endif // BOOLECTOR_LOGIC_HPP
