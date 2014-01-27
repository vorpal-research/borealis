/*
 * ForceInline.h
 *
 *  Created on: Jan 27, 2014
 *      Author: belyaev
 */

#ifndef FORCEINLINE_H_
#define FORCEINLINE_H_

#include <llvm/Transforms/IPO/InlinerPass.h>

namespace borealis {

class ForceInline: public virtual llvm::Inliner {
public:
    static char ID;

    ForceInline(): llvm::Inliner(ID) {}
    virtual ~ForceInline() {}

    virtual llvm::InlineCost getInlineCost(llvm::CallSite CS);
};

} /* namespace borealis */

#endif /* FORCEINLINE_H_ */
