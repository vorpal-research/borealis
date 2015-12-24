/*
 * TypeUtils.h
 *
 *  Created on: Nov 25, 2013
 *      Author: ice-phoenix
 */

#ifndef TYPEUTILS_H_
#define TYPEUTILS_H_

#include "Type/Type.def"

namespace borealis {

struct TypeUtils {

    static bool isConcrete(Type::Ptr type);
    static bool isValid(Type::Ptr type);
    static bool isInvalid(Type::Ptr type);
    static bool isUnknown(Type::Ptr type);

    enum Verbosity{ Verbose, Short };
    static std::string toString(const Type& type, Verbosity verb = Verbose);

    static unsigned long long getTypeSizeInElems(Type::Ptr type);
    static unsigned long long getStructOffsetInElems(Type::Ptr type, unsigned idx);

    static Type::Ptr getPointerElementType(Type::Ptr type);

    static llvm::Type* tryCastBack(llvm::LLVMContext& C, Type::Ptr type);

};

} // namespace borealis

#endif /* TYPEUTILS_H_ */
