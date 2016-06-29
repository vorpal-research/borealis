/*
 * ExecutionContext.h
 *
 *  Created on: Jun 15, 2016
 *      Author: belyaev
 */

#ifndef BOOLECTOR_EXECUTIONCONTEXT_H_
#define BOOLECTOR_EXECUTIONCONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SMT/Boolector/BoolectorEngine.h"

#include "SMT/Boolector/ExprFactory.h"
#include "Util/split_join.hpp"

#define NAMESPACE boolector_
#define BACKEND Boolector

#include "SMT/ExecutionContext.stub.h"

#undef NAMESPACE
#undef BACKEND

#endif /* BOOLECTOR_EXECUTIONCONTEXT_H_ */
