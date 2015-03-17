/*
 * NameContext.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef NAMECONTEXT_H_
#define NAMECONTEXT_H_

#include <llvm/IR/Function.h>

#include "Util/locations.h"

namespace borealis {

struct NameContext {
    enum class Placement { InnerScope, OuterScope, GlobalScope } placement;
    llvm::Function* func;
    Locus loc;
};

template<class Streamer>
Streamer& operator<<(Streamer& str, const NameContext& nc) {
    switch (nc.placement) {
    case NameContext::Placement::InnerScope:
        str << "in function " << nc.func->getName() << ": " << nc.loc;
        break;
    case NameContext::Placement::OuterScope:
        str << "on function " << nc.func->getName() << ": " << nc.loc;
        break;
    case NameContext::Placement::GlobalScope:
        str << "global scope: " << nc.loc;
        break;
    }
    return str;
}

} /* namespace borealis */

#endif /* NAMECONTEXT_H_ */
