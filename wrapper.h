/*
 * wrapper.h
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#ifndef WRAPPER_H_
#define WRAPPER_H_

namespace borealis {

enum class Result {
    E_GATHER_COMMENTS = 0x1001,
    E_EMIT_LLVM       = 0x1002,
    OK                = 0x0000
};

} // namespace borealis

#endif /* WRAPPER_H_ */
