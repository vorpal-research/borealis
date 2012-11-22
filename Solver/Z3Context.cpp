/*
 * Z3Context.cpp
 *
 *  Created on: Nov 22, 2012
 *      Author: belyaev
 */

#include "Z3Context.h"

namespace borealis {

void Z3Context::mutateMemory( const std::function<z3::expr(z3::expr)>& mutator ) {
    memory = mutator(memory);
}

} // namespace borealis
