/*
 * ExecutionContext.h
 *
 *  Created on: Jun 15, 2016
 *      Author: belyaev
 */

#ifndef STP_EXECUTIONCONTEXT_H_
#define STP_EXECUTIONCONTEXT_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SMT/STP/STPEngine.h"

#include "SMT/STP/ExprFactory.h"
#include "Util/split_join.hpp"

#define NAMESPACE stp_
#define BACKEND STP

#include "SMT/ExecutionContext.stub.h"

#undef NAMESPACE
#undef BACKEND

#endif /* STP_EXECUTIONCONTEXT_H_ */
