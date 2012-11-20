/*
 * MemoryContext.h
 *
 *  Created on: Nov 20, 2012
 *      Author: belyaev
 */

#ifndef MEMORYCONTEXT_H_
#define MEMORYCONTEXT_H_

#include <z3/z3++.h>

class MemoryContext {
    z3::context ctx;

public:
    MemoryContext();
    virtual ~MemoryContext();



};

#endif /* MEMORYCONTEXT_H_ */
