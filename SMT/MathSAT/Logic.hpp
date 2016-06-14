/*
 * Logic.hpp
 *
 *  Created on: Jul 31, 2013
 *      Author: Sam Kolton
 */

#ifndef MSAT_LOGIC_HPP_
#define MSAT_LOGIC_HPP_

#include <functional>
#include <vector>
#include <numeric>
#include <algorithm>

#include "Config/config.h"

#include "SMT/MathSAT/MathSAT.h"
#include "SMT/MathSAT/MathSATEngine.h"
#include "Util/util.h"

#define NAMESPACE mathsat_
#define ENGINE MathSATEngine

#include "SMT/Logic.stub.hpp"

#undef NAMESPACE
#undef ENGINE

#endif  //MSAT_LOGIC_HPP_
