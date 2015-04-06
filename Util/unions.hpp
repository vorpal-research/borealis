//
// Created by belyaev on 4/6/15.
//

#ifndef UNIONS_HPP
#define UNIONS_HPP

#include <utility>

namespace borealis {
namespace unions { // this is not intended for widespread use

template <class T, class ...Args>
void construct(T* member, Args&&... args) {
    new (member) T (std::forward<Args>(args)...);
}

template <class T>
void destruct(T* member) {
    member->~T();
}

} /* namespace unions */
} /* namespace borealis */



#endif //UNIONS_HPP
