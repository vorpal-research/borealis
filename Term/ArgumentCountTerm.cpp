/*
 * ArgumentCountTerm.cpp
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#include "Term/ArgumentCountTerm.h"

namespace borealis {

ArgumentCountTerm::ArgumentCountTerm(Type::Ptr type):
    Term(
        class_tag(*this),
        type,
        "<arg_count>"
    ) {};

} // namespace borealis
