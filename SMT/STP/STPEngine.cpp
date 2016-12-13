#include "SMT/STP/STPEngine.h"

namespace borealis {

namespace stppp {

sort sort::mk_bool(){
    return { std::make_shared<bool_sort_impl>() };
}
sort sort::mk_bv(size_t bsize) {
    return { std::make_shared<bv_sort_impl>(bsize) };
}
sort sort::mk_array(sort ix, sort el) {
    return { std::make_shared<array_sort_impl>(ix, el) };
}
sort sort::mk_fun(sort res, const std::vector<sort>& args){
    return { std::make_shared<fun_sort_impl>(res, args) };
}

} /* namespace boolectorpp */

} /* namespace borealis */
