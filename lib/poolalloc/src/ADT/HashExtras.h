/*
 * HashExtras.h
 *
 *  Created on: Aug 28, 2012
 *      Author: belyaev
 *      Based on ../ADT/HashExtras.h that has been removed for some reason
 */

#ifndef HASHEXTRAS_H_
#define HASHEXTRAS_H_

#include <unordered_map>
#include <unordered_set>

namespace std {


//// Provide a hash function for arbitrary pointers...
//template <class T> struct hash<T *> {
//  inline size_t operator()(const T *Val) const {
//    return reinterpret_cast<size_t>(Val);
//  }
//};
//
//template <> struct hash<std::string> {
//  size_t operator()(std::string const &str) const {
//    return hash<char const *>()(str.c_str());
//  }
//};

}





#endif /* HASHEXTRAS_H_ */
