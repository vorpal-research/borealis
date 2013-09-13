/*
 * DataProvider.hpp
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#ifndef DATAPROVIDER_HPP_
#define DATAPROVIDER_HPP_

#include <llvm/Pass.h>

#include "Util/typeindex.hpp"
#include "Util/util.h"

namespace borealis {

template<class T>
class DataProvider;

template<class T>
DataProvider<T>* provideAsPass(const T* value) {
    return new DataProvider<T>(value);
}

template<class T>
class DataProvider : public llvm::ImmutablePass {

    struct static_ {
        static_() {
            static std::string tname = borealis::util::toString(borealis::type_id<T>());
            static std::string passname = std::string("data-provider-") + tname;
            static std::string passdesc = std::string("Provider of ") + tname;

            static llvm::RegisterPass< DataProvider<T> >
            X(passname.c_str(), passdesc.c_str(), false, false);
        }
    };

    static static_ st;

    virtual void enforce_static(static_&) {}

    const T* value;

public:

    static char ID;

    DataProvider() : llvm::ImmutablePass(ID) { enforce_static(st); }
    DataProvider(const T* v) : llvm::ImmutablePass(ID), value(v) { enforce_static(st); }

    virtual void initializePass() {}

    const T& provide() {
        return *value;
    }

    friend DataProvider<T>* provideAsPass(const T* value);
};

template<class T>
char DataProvider<T>::ID;

template<class T>
typename DataProvider<T>::static_ DataProvider<T>::st;

} // namespace borealis

#endif /* DATAPROVIDER_HPP_ */
