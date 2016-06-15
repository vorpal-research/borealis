/*
 * ExecutionContext.h
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#ifndef MATHSAT_EXECUTIONCONTEXT_H_
#define MATHSAT_EXECUTIONCONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SMT/MathSAT/MathSATEngine.h"

#include "SMT/MathSAT/ExprFactory.h"
#include "Util/split_join.hpp"

#define NAMESPACE mathsat_
#define BACKEND MathSAT

#include "SMT/ExecutionContext.stub.h"

#undef NAMESPACE
#undef BACKEND

#endif /* MATHSAT_EXECUTIONCONTEXT_H_ */
