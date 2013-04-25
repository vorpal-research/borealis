/*
 * wrapper.h
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#ifndef WRAPPER_H_
#define WRAPPER_H_

namespace borealis {

enum {
    OK                           = 0x0000,
    E_ILLEGAL_COMPILER_OPTIONS   = 0x0001,
    E_GATHER_COMMENTS            = 0x0002,
    E_EMIT_LLVM                  = 0x0003
};

} // namespace borealis

#endif /* WRAPPER_H_ */
