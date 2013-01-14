/*
 * FuncInfo.h
 *
 *  Created on: Jan 14, 2013
 *      Author: belyaev
 */

#ifndef FUNCINFO_H_
#define FUNCINFO_H_

#include "VarInfo.h"

namespace borealis {

struct FuncInfo {
    VarInfo func;
    std::vector<VarInfo> locals;

    typedef std::vector<VarInfo>::iterator iterator;
    typedef std::vector<VarInfo>::const_iterator const_iterator;

    iterator begin() { return locals.begin(); }
    iterator end()   { return locals.end();   }

    const_iterator begin() const { return locals.begin(); }
    const_iterator end()   const { return locals.end();   }
};

}// namespace borealis


#endif /* FUNCINFO_H_ */
