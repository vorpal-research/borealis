//
// Created by belyaev on 10/28/15.
//

#ifndef AURORA_SANDBOX_CTYPEINFER_H
#define AURORA_SANDBOX_CTYPEINFER_H

#include "Term/Term.def"
#include "Util/util.h"
#include "Codegen/CType/CTypeUtils.h"

#include "Util/macros.h"

namespace borealis {

// XXX: maybe better make it a transformer?
class CTypeInfer {
    CTypeFactory* CTF;
    std::unordered_map<Term::Ptr, CType::Ptr> gamma;

public:
    CTypeInfer(CTypeFactory& CTF) :
        CTF(&CTF) {}

    void hintType(Term::Ptr term, CType::Ptr type) {
        gamma[term] = type;
    }

    CType::Ptr getBoolType() {
        return CTF->getInteger("bor.bool", 1, llvm::Signedness::Unsigned);
    }

    CType::Ptr getPropertyType() {
        return CTF->getInteger("bor.property_t", TypeFactory::defaultTypeSize, llvm::Signedness::Unsigned);
    }

    CType::Ptr getSmallIntegerType() {
        return CTF->getInteger("bor.min_int_t", 1, llvm::Signedness::Unknown);
    }

    CType::Ptr inferType(Term::Ptr term) {
        CType::Ptr res;
        if(gamma.count(term)) return gamma[term];

        ON_SCOPE_EXIT(gamma[term] = res);

        if(auto&& axiom = dyn_cast<AxiomTerm>(term)) {
            return res = inferType(axiom->getLhv());
        } else if(auto&& binary = dyn_cast<BinaryTerm>(term)) {
            return res = CTypeUtils::commonType(*CTF, inferType(binary->getLhv()), inferType(binary->getRhv()));
        } else if(auto&& load = dyn_cast<LoadTerm>(term)) {
            return res = CTypeUtils::loadType(inferType(load->getRhv()));
        } else if(auto&& index = dyn_cast<OpaqueIndexingTerm>(term)) {
            return res = CTypeUtils::indexType(inferType(index->getLhv()));
        } else if(auto&& ma = dyn_cast<OpaqueMemberAccessTerm>(term)) {
            if(ma->isIndirect()) {
                return res = CTypeUtils::getField(CTypeUtils::loadType(inferType(ma->getLhv())), ma->getProperty())->getType();
            }
            return res = CTypeUtils::getField(inferType(ma->getLhv()), ma->getProperty())->getType();
        } else if(auto&& ta = dyn_cast<TernaryTerm>(term)) {
            return res = CTypeUtils::commonType(*CTF, inferType(ta->getTru()), inferType(ta->getFls()));
        } else if(auto&& unary = dyn_cast<UnaryTerm>(term)) {
            return res = inferType(unary->getRhv());
        }

        // unhandled here:
        // Argument; Bound; Cast; Cmp; Const; Gep; bool; builtin;
        // call; float; int; invalid; nullptr; string; undef; var; property; return; sign;
        // value

        if(is_one_of<ArgumentTerm, ConstTerm, OpaqueBuiltinTerm, OpaqueCallTerm,
                     OpaqueStringConstantTerm, OpaqueUndefTerm, OpaqueVarTerm, ReturnValueTerm, ValueTerm>(term)) {
            // wheew
            warns() << tfm::format("Cannot infer CType for term %s", term->getName()) << endl;
            return res = nullptr;
        }

        // unhandled here:
        // Bound; Cast; Cmp; Gep; bool; float; int; invalid; nullptr; property; sign;

        if(is_one_of<CmpTerm, SignTerm, OpaqueBoolConstantTerm>(term)) {
            return res = getBoolType();
        }

        // unhandled here:
        // Bound; Cast; Gep; float; int; invalid; nullptr; property;

        if(is_one_of<BoundTerm, ReadPropertyTerm>(term)) {
            return res = getPropertyType();
        }

        // unhandled here:
        // Cast; Gep; float; int; invalid; nullptr;

        if(is_one_of<OpaqueInvalidPtrTerm, OpaqueNullPtrTerm>(term))
            if(isa<type::UnknownType>(term->getType())){
                return res = CTF->getPointer(CTF->getVoid());
        }

        if(is_one_of<OpaqueIntConstantTerm, OpaqueBigIntConstantTerm, OpaqueNamedConstantTerm>(term))
            if(isa<type::UnknownType>(term->getType())){
                return res = getSmallIntegerType();
        }

        // unhandled here:
        // Cast; Gep; float; int; invalid; nullptr;
        return res = tryCastTypeToCType(term->getType());
    }

    CType::Ptr tryCastLLVMTypeToCType(llvm::Type* tp) {
        if(tp == nullptr) return CTF->getVoid(); // void in llvm is indistinguishable from error

        if(auto&& rec = dyn_cast<llvm::StructType>(tp)) {
            UNREACHABLE(tfm::format("Stray record type detected: %s", util::toString(*rec)));
        }

        if(auto&& int_ = dyn_cast<llvm::IntegerType>(tp)) {
            auto name = tfm::format("bor.integer.%d", int_->getIntegerBitWidth());
            return CTF->getInteger(name, int_->getIntegerBitWidth(), llvm::Signedness::Unknown);
        }

        if(tp->isFloatingPointTy()) {
            return CTF->getFloat("bor.float", TypeFactory::defaultTypeSize);
        }

        if(auto&& ptr = dyn_cast<llvm::PointerType>(tp)) {
            return CTF->getPointer(tryCastLLVMTypeToCType(ptr->getPointerElementType()));
        }

        if(auto&& fun = dyn_cast<llvm::FunctionType>(tp)) {
            return CTF->getFunction(
                CTF->getRef(tryCastLLVMTypeToCType(fun->getReturnType())),
                util::view(fun->param_begin(), fun->param_end())
                     .map(APPLY(tryCastLLVMTypeToCType))
                     .map(APPLY(CTF->getRef))
                     .toVector()
            );
        }

        if(auto&& arr = dyn_cast<llvm::ArrayType>(tp)) {
            if(arr->getArrayNumElements()) {
                return CTF->getArray(CTF->getRef(tryCastLLVMTypeToCType(arr->getArrayElementType())), arr->getArrayNumElements());
            } else return CTF->getArray(CTF->getRef(tryCastLLVMTypeToCType(arr->getArrayElementType())));
        }

        UNREACHABLE("Unsupported type encountered");
    }

    CType::Ptr tryCastTypeToCType(Type::Ptr tp) {
        if(auto&& te = dyn_cast<type::TypeError>(tp)) {
            UNREACHABLE(tfm::format("Type error detected: %s", te->getMessage()));
        }

        if(isa<type::UnknownType>(tp)) {
            warns() << "tryCastTypeToCType: Unknown type detected" << endl;
            return nullptr;
        }

        if(auto&& rec = dyn_cast<type::Record>(tp)) {
            UNREACHABLE(tfm::format("Stray record type detected: %s", util::toString(*rec)));
        }

        if(auto&& int_ = dyn_cast<type::Integer>(tp)) {
            auto name = tfm::format("bor.integer.%d", int_->getBitsize());
            if(int_->getSignedness() == llvm::Signedness::Signed) {
                name = "signed " + name;
            } else if(int_->getSignedness() == llvm::Signedness::Unsigned) {
                name = "unsigned " + name;
            }
            return CTF->getInteger(name, int_->getBitsize(), int_->getSignedness());
        }

        if(isa<type::Float>(tp)) {
            return CTF->getFloat("bor.float", TypeFactory::defaultTypeSize);
        }

        if(isa<type::Bool>(tp)) {
            return CTF->getInteger("bor.bool", 1, llvm::Signedness::Unsigned);
        }

        if(auto&& ptr = dyn_cast<type::Pointer>(tp)) {
            return CTF->getPointer(tryCastTypeToCType(ptr->getPointed()));
        }

        if(auto&& fun = dyn_cast<type::Function>(tp)) {
            return CTF->getFunction(
                CTF->getRef(tryCastTypeToCType(fun->getRetty())),
                util::viewContainer(fun->getArgs())
                     .map(APPLY(tryCastTypeToCType))
                     .map(APPLY(CTF->getRef))
                     .toVector()
            );
        }

        if(auto&& arr = dyn_cast<type::Array>(tp)) {
            if(arr->getSize()) {
                return CTF->getArray(CTF->getRef(tryCastTypeToCType(arr->getElement())), arr->getSize().getUnsafe());
            } else return CTF->getArray(CTF->getRef(tryCastTypeToCType(arr->getElement())));
        }

        UNREACHABLE("Unsupported type encountered");
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif //AURORA_SANDBOX_CTYPEINFER_H
