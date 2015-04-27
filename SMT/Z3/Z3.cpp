//
// Created by ice-phoenix on 4/27/15.
//

#include "SMT/Z3/Z3.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace z3_ {

std::ostream& operator<<(std::ostream& os, const z3::func_interp& z3fi) {
    auto&& entries = util::range(0U, z3fi.num_entries()).map(APPLY(z3fi.entry));
    for (auto&& e : entries) {
        os << "  " << e << std::endl;
    }
    return os << "  " << z3fi.else_value();
}

std::ostream& operator<<(std::ostream& os, const z3::func_entry& z3fe) {
    auto&& args = util::range(0U, z3fe.num_args()).map(APPLY(z3fe.arg)).toVector();
    return os << args << " -> " << z3fe.value();
}

} // namespace z3_
} // namespace borealis

#include "Util/unmacros.h"
