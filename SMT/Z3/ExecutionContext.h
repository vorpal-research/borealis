/*
 * ExecutionContext.h
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#ifndef Z3_EXECUTIONCONTEXT_H_
#define Z3_EXECUTIONCONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SMT/Z3/Z3Engine.h"

#include "SMT/Z3/ExprFactory.h"
#include "Util/split_join.hpp"

#define NAMESPACE z3_
#define BACKEND Z3

#include "SMT/ExecutionContext.stub.h"

#undef NAMESPACE
#undef BACKEND

#endif /* Z3_EXECUTIONCONTEXT_H_ */
