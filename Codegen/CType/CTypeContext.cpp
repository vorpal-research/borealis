//
// Created by belyaev on 5/12/15.
//

#include "Codegen/CType/CTypeContext.h"

#include <sstream>
#include <tinyformat/tinyformat.h>

#include "Codegen/CType/CTypeUtils.h"
#include "Util/sayonara.hpp"
#include "Util/macros.h"


namespace borealis {

template<class Map>
static std::string ppAsJson(const Map& m) {
    std::ostringstream ost;
    ost << "{" << std::endl;
    for(auto&& kv: m) {
        ost << "    '" << kv.first << "':" << std::endl
            << "        '" << kv.second << "'," << std::endl;

    }
    ost << "}" << std::endl;
    return ost.str();
}

void CTypeContext::selfCheck() const {
    for(auto&& tp : types) {
        ASSERT(
            tp.first == tp.second->getName(),
            tfm::format("Inconsistent naming: '%s' != '%s'\nctx:\n%s", tp.first, tp.second->getName(), ppAsJson(types))
        );

        auto&& members = CTypeUtils::memberRefs(tp.second);

        for(auto&& member: members) {
            ASSERT(
                types.count(member.getName()),
                tfm::format("Type '%s' is referenced by '%s' but never defined in context\nctx:\n%s", member.getName(), tp.second->getName(), ppAsJson(types))
            )
        }
    }
}

} /* namespace borealis */

#include "Util/unmacros.h"
