/*
 * ExecutionContext.h
 *
 *  Created on: Jun 15, 2016
 *      Author: belyaev
 */

#ifndef CVC4_EXECUTIONCONTEXT_H_
#define CVC4_EXECUTIONCONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SMT/CVC4/CVC4Engine.h"

#include "SMT/CVC4/ExprFactory.h"
#include "Util/split_join.hpp"

#define NAMESPACE cvc4_
#define BACKEND CVC4

#include "SMT/ExecutionContext.stub.h"

#undef NAMESPACE
#undef BACKEND

#endif /* CVC4_EXECUTIONCONTEXT_H_ */
