//
// Created by ice-phoenix on 4/27/15.
//

#include "SMT/Z3/Z3.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace z3 {

borealis::logging::logstream& operator<<(borealis::logging::logstream& os, const z3::func_interp& z3fi) {
    auto&& entries = borealis::util::range(0U, z3fi.num_entries()).map(APPLY(z3fi.entry));
    for (auto&& e : entries) {
        os << e << borealis::logging::endl;
    }
    return os << "else -> " << z3fi.else_value();
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& os, const z3::func_entry& z3fe) {
    auto&& args = borealis::util::range(0U, z3fe.num_args()).map(APPLY(z3fe.arg)).toVector();
    return os << args << " -> " << z3fe.value();
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& os, const z3::expr_vector& z3ev) {
    for (size_t i = 0U; i < z3ev.size(); ++i) os << z3ev[i] << borealis::logging::endl;
    return os;
}

} // namespace z3

#include "Util/unmacros.h"
