/*
 * Logic.cpp
 *
 *  Created on: Jun 15, 2016
 *      Author: belyaev
 */

#ifndef CVC4_LOGIC_HPP
#define CVC4_LOGIC_HPP

#include <functional>
#include <numeric>
#include <algorithm>

#include "Util/util.h"
#include "Util/functional.hpp"

#include "Config/config.h"

#include "SMT/CVC4/CVC4Engine.h"

#define ENGINE CVC4Engine
#define NAMESPACE cvc4_

#include "SMT/Logic.stub.hpp"

#undef NAMESPACE
#undef ENGINE

#endif // Z3_LOGIC_HPP
