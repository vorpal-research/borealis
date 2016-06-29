/*
 * ConverterImpl.hpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#ifndef SMT_PROTOBUF_CONVERTER_IMPL_HPP_
#define SMT_PROTOBUF_CONVERTER_IMPL_HPP_

#include "Protobuf/ConverterUtil.h"

#include "SMT/Model.h"

#include "Protobuf/Gen/SMT/Model.pb.h"
#include "Protobuf/Gen/SMT/MemoryShape.pb.h"

#include "Term/ProtobufConverterImpl.hpp"

#include "Factory/Nest.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<smt::MemoryShape> {
    typedef smt::MemoryShape normal_t;
    typedef smt::proto::MemoryShape proto_t;
    typedef borealis::FactoryNest context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& t);
    static std::unique_ptr<normal_t> fromProtobuf(const context_t& fn, const proto_t& t);
};

template<>
struct protobuf_traits<smt::Model> {
    typedef smt::Model normal_t;
    typedef smt::proto::Model proto_t;
    typedef borealis::FactoryNest context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& t);
    static std::shared_ptr<normal_t> fromProtobuf(const context_t& fn, const proto_t& t);
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* SMT_PROTOBUF_CONVERTER_IMPL_HPP_ */
