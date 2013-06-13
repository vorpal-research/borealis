/*
 * FuncInfo.h
 *
 *  Created on: Jan 14, 2013
 *      Author: belyaev
 */

#ifndef FUNCINFO_H_
#define FUNCINFO_H_

#include "Codegen/VarInfo.h"

#include "Util/macros.h"

namespace borealis {

struct FuncInfo {
    VarInfo func;
    std::vector<VarInfo> locals;

    auto begin() QUICK_RETURN(this->locals.begin())
    auto end() QUICK_RETURN(this->locals.end())

    auto begin() QUICK_CONST_RETURN(this->locals.begin())
    auto end() QUICK_CONST_RETURN(this->locals.end())
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* FUNCINFO_H_ */
