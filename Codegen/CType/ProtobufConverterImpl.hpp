//
// Created by belyaev on 11/2/15.
//

#ifndef AURORA_SANDBOX_PROTOBUFCONVERTERIMPL_HPP
#define AURORA_SANDBOX_PROTOBUFCONVERTERIMPL_HPP

#include <Protobuf/Gen/Util/Signedness.pb.h>
#include <Protobuf/Gen/Codegen/CType/CQualifier.pb.h>
#include "Protobuf/ConverterUtil.h"

#include "Protobuf/Gen/Codegen/CType/CType.pb.h"
#include "Protobuf/Gen/Codegen/CType/CVoid.pb.h"
#include "Protobuf/Gen/Codegen/CType/CInteger.pb.h"
#include "Protobuf/Gen/Codegen/CType/CFloat.pb.h"
#include "Protobuf/Gen/Codegen/CType/CPointer.pb.h"
#include "Protobuf/Gen/Codegen/CType/CAlias.pb.h"
#include "Protobuf/Gen/Codegen/CType/CArray.pb.h"
#include "Protobuf/Gen/Codegen/CType/CStruct.pb.h"
#include "Protobuf/Gen/Codegen/CType/CFunction.pb.h"
#include "Protobuf/Gen/Codegen/CType/CTypeRef.pb.h"
#include "Protobuf/Gen/Codegen/CType/CStructMember.pb.h"
#include "Protobuf/Gen/Codegen/CType/CTypeContext.pb.h"

#include "Codegen/CType/CTypeFactory.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<>
struct protobuf_traits<CType> {
    typedef CType normal_t;
    typedef proto::CType proto_t;
    typedef borealis::CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& t);
    static CType::Ptr fromProtobuf(context_t fn, const proto_t& t);
};

template<>
struct protobuf_traits<CTypeRef> {
    typedef CTypeRef normal_t;
    typedef proto::CTypeRef proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        res->set_name(p.getName());
        return std::move(res);
    }

    static std::unique_ptr<CTypeRef> fromProtobuf(context_t f, const proto_t& p) {
        if(p.has_name()) {
            return util::make_unique<normal_t>(f->getRef(p.name()));
        } else return nullptr;
    };
};

template<>
struct protobuf_traits_impl<CStructMember> {
    typedef CStructMember normal_t;
    typedef proto::CStructMember proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        res->set_name(p.getName());
        res->set_offset(p.getOffset());
        res->set_allocated_type(
            protobuf_traits<CTypeRef>::toProtobuf(p.getType()).release()
        );
        return std::move(res);
    }

    static std::unique_ptr<CStructMember> fromProtobuf(context_t f, const proto_t& p) {
        if(p.has_name() && p.has_offset() && p.has_type()) {
            return util::make_unique<normal_t>(p.offset(), p.name(), *protobuf_traits<CTypeRef>::fromProtobuf(f, p.type()));
        } else return nullptr;
    };
};

template<>
struct protobuf_traits_impl<CVoid> {
    typedef CVoid normal_t;
    typedef proto::CVoid proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& /* p */) {
        auto res = util::uniq(new proto_t());
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t f, const std::string&, const proto_t& /* p */) {
        return f->getVoid();
    }
};

template<>
struct protobuf_traits_impl<CInteger> {
    typedef CInteger normal_t;
    typedef CType::Ptr base_t;
    typedef proto::CInteger proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        res->set_bitsize(p.getBitsize());
        res->set_signedness(static_cast<proto::Signedness>(p.getSignedness()));
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t fn, const std::string& name, const proto_t& p) {
        return fn->getInteger(name, p.bitsize(), static_cast<llvm::Signedness>(p.signedness()));
    }
};

template<>
struct protobuf_traits_impl<CFloat> {
    typedef CFloat normal_t;
    typedef proto::CFloat proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        res->set_bitsize(p.getBitsize());
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t fn, const std::string& name, const proto_t& p) {
        return fn->getFloat(name, p.bitsize());
    }
};

template<>
struct protobuf_traits_impl<CPointer> {
    typedef CPointer normal_t;
    typedef proto::CPointer proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        auto element_ = p.getElement();
        res->set_allocated_element( protobuf_traits<CTypeRef>::toProtobuf(element_).release() );
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t fn, const std::string&, const proto_t& p) {
        auto pointed = protobuf_traits<CTypeRef>::fromProtobuf(fn, p.element());
        if(pointed) return fn->getPointer(*pointed);
        else return nullptr;
    }
};

template<>
struct protobuf_traits_impl<CAlias> {
    typedef CAlias normal_t;
    typedef proto::CAlias proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        auto original_ = p.getOriginal();
        res->set_allocated_original( protobuf_traits<CTypeRef>::toProtobuf(original_).release() );
        res->set_qualifier( static_cast<proto::CQualifier>(p.getQualifier()) );
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t fn, const std::string& name, const proto_t& p) {
        auto pointed = protobuf_traits<CTypeRef>::fromProtobuf(fn, p.original());

        if(pointed) return fn->getAlias(name, static_cast<CQualifier>(p.qualifier()), *pointed);
        else return nullptr;
    }
};

template<>
struct protobuf_traits_impl<CArray> {
    typedef CArray normal_t;
    typedef proto::CArray proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        auto element_ = p.getElement();
        res->set_allocated_element( protobuf_traits<CTypeRef>::toProtobuf(element_).release() );

        if(auto&& size_ = p.getSize()) {
            res->set_size(size_.getUnsafe());
        }
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t fn, const std::string&, const proto_t& p) {
        auto element = protobuf_traits<CTypeRef>::fromProtobuf(fn, p.element());
        if(element) {
            if(p.has_size()) {
                return fn->getArray(*element, p.size());
            } else {
                return fn->getArray(*element);
            }
        } else return nullptr;
    }
};
template<>
struct protobuf_traits_impl<CStruct> {
    typedef CStruct normal_t;
    typedef proto::CStruct proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        for(auto&& element : p.getElements()) {
            res->mutable_elements()->AddAllocated(
                protobuf_traits_impl<CStructMember>::toProtobuf(element).release()
            );
        }

        res->set_opaque(p.getOpaque());
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t fn, const std::string& name, const proto_t& p) {
        std::vector<CStructMember> elems =
                    util::viewContainer(p.elements())
                    .map([&](auto&& csm) -> CStructMember {
                        return *protobuf_traits_impl<CStructMember>::fromProtobuf(fn, csm);
                    })
                    .toVector();
        return fn->getStruct(name, elems);
    }
};
template<>
struct protobuf_traits_impl<CFunction> {
    typedef CFunction normal_t;
    typedef proto::CFunction proto_t;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto_t> toProtobuf(const normal_t& p) {
        auto res = util::uniq(new proto_t());
        res->set_allocated_resulttype(protobuf_traits<CTypeRef>::toProtobuf(p.getResultType()).release());
        for(auto&& element : p.getArgumentTypes()) {
            res->mutable_argumenttypes()->AddAllocated(
                protobuf_traits<CTypeRef>::toProtobuf(element).release()
            );
        }
        return std::move(res);
    }

    static CType::Ptr fromProtobuf(context_t fn, const std::string&, const proto_t& p) {
        auto mapper = [&](auto&& ctr){ return *protobuf_traits<CTypeRef>::fromProtobuf(fn, ctr); };
        auto elems = util::viewContainer(p.argumenttypes())
                    .map(mapper)
                    .toVector();
        auto res = mapper(p.resulttype());
        return fn->getFunction(res, elems);
    }
};

template<>
struct protobuf_traits<CTypeContext> {
    using normal_t = CTypeContext;
    using proto_t = proto::CTypeContext;
    typedef CTypeFactory* context_t;

    static std::unique_ptr<proto::CTypeContext> toProtobuf(const CTypeContext& ctc) {
        auto res = util::uniq(new proto::CTypeContext);
        for(auto&& pp : ctc) {
            res->mutable_types()->AddAllocated(
                protobuf_traits<CType>::toProtobuf(*pp.second).release()
            );
        }
        return std::move(res);
    }

    static CTypeContext::Ptr fromProtobuf(CTypeFactory* f, const proto::CTypeContext& ctc) {
        auto res = f->getCtx(); // XXX: this is not really a good thing to do, but whatever
        for(auto&& pp : ctc.types()) {
            auto tp = protobuf_traits<CType>::fromProtobuf(f, pp);
            res->put(tp);
        }
        return std::move(res);
    }
};

} // namespace borealis

#include "Util/unmacros.h"


#endif //AURORA_SANDBOX_PROTOBUFCONVERTERIMPL_HPP
