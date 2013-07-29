/*
 * TermFactory.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include "Term/TermFactory.h"

namespace borealis {

TermFactory::TermFactory(SlotTracker* st, TypeFactory::Ptr TyF) :
    st(st), TyF(TyF) {}

} // namespace borealis
