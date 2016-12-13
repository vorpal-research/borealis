/*
 * Logic.cpp
 *
 *  Created on: Jun 15, 2016
 *      Author: belyaev
 */

#ifndef STP_LOGIC_HPP
#define STP_LOGIC_HPP

#include <functional>
#include <numeric>
#include <algorithm>

#include "Util/util.h"
#include "Util/functional.hpp"

#include "Config/config.h"

#include "SMT/STP/STPEngine.h"

#define ENGINE STPEngine
#define NAMESPACE stp_

#include "SMT/Logic.stub.hpp"

#undef NAMESPACE
#undef ENGINE

#endif // STP_LOGIC_HPP
