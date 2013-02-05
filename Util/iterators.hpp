/*
 * iterators.hpp
 *
 *  Created on: Feb 5, 2013
 *      Author: belyaev
 */

#ifndef ITERATORS_HPP_
#define ITERATORS_HPP_

#include <iterator>

template<class Container>
std::pair<typename Container::iterator, typename Container::iterator>
begin_end_pair(Container& c) {
    return std::make_pair(c.begin(), c.end());
}

template<class Container>
std::pair<typename Container::const_iterator, typename Container::const_iterator>
begin_end_pair(const Container& c) {
    return std::make_pair(c.begin(), c.end());
}

template<class Iterator>
class key_iterator {
    Iterator base;
public:
    /// One of the @link iterator_tags tag types@endlink.
    typedef typename Iterator::iterator_category  iterator_category;
    /// The type "pointed to" by the iterator.
    typedef typename Iterator::value_type::first_type value_type;
    /// Distance between iterators is represented as this type.
    typedef typename Iterator::difference_type  difference_type;
    /// This type represents a pointer-to-value_type.
    typedef value_type*   pointer;
    /// This type represents a reference-to-value_type.
    typedef value_type& reference;

    key_iterator(const key_iterator&) = default;
    key_iterator(key_iterator&&) = default;
    key_iterator(const Iterator& base): base(base){};

    key_iterator& operator=(const key_iterator&) = default;
    key_iterator& operator=(key_iterator&&) = default;

    bool operator==(const key_iterator& that) { return base == that.base; };
    bool operator!=(const key_iterator& that) { return base != that.base; };

    reference operator*() const { return base->first; }
    reference operator->() const { return &base->first; }

    key_iterator& operator++() { ++base; return *this; }
    key_iterator& operator--() { --base; return *this; }
};

template<class Iterator>
key_iterator<Iterator> iterate_keys(const Iterator& it) {
    return key_iterator<Iterator>{ it };
}

template<class Iterator>
std::pair<key_iterator<Iterator>, key_iterator<Iterator>> iterate_keys(const std::pair<Iterator, Iterator>& it) {
    return std::pair<key_iterator<Iterator>, key_iterator<Iterator>>{
        iterate_keys(it.first),
        iterate_keys(it.second)
    };
}

template<class Iterator>
class value_iterator {
    Iterator base;
public:
    /// One of the @link iterator_tags tag types@endlink.
    typedef typename Iterator::iterator_category  iterator_category;
    /// The type "pointed to" by the iterator.
    typedef typename Iterator::value_type::second_type value_type;
    /// Distance between iterators is represented as this type.
    typedef typename Iterator::difference_type  difference_type;
    /// This type represents a pointer-to-value_type.
    typedef value_type*   pointer;
    /// This type represents a reference-to-value_type.
    typedef value_type& reference;

    value_iterator(const value_iterator&) = default;
    value_iterator(value_iterator&&) = default;
    value_iterator(const Iterator& base): base(base){};

    value_iterator& operator=(const value_iterator&) = default;
    value_iterator& operator=(value_iterator&&) = default;

    bool operator==(const value_iterator& that) { return base == that.base; };
    bool operator!=(const value_iterator& that) { return base != that.base; };

    reference operator*() { return base->second; }
    reference operator->() { return &base->second; }

    value_iterator& operator++() { ++base; return *this; }
    value_iterator& operator--() { --base; return *this; }
};

template<class Iterator>
value_iterator<Iterator> iterate_values(const Iterator& it) {
    return value_iterator<Iterator>{ it };
}

template<class Iterator>
std::pair<value_iterator<Iterator>, value_iterator<Iterator>> iterate_values(const std::pair<Iterator, Iterator>& it) {
    return std::pair<value_iterator<Iterator>, value_iterator<Iterator>>{
        iterate_values(it.first),
        iterate_values(it.second)
    };
}

template<class Iterator>
class value_citerator {
    Iterator base;
public:
    /// One of the @link iterator_tags tag types@endlink.
    typedef typename Iterator::iterator_category  iterator_category;
    /// The type "pointed to" by the iterator.
    typedef typename Iterator::value_type::second_type value_type;
    /// Distance between iterators is represented as this type.
    typedef typename Iterator::difference_type  difference_type;
    /// This type represents a pointer-to-value_type.
    typedef const value_type*   pointer;
    /// This type represents a reference-to-value_type.
    typedef const value_type& reference;

    value_citerator(const value_citerator&) = default;
    value_citerator(value_citerator&&) = default;
    value_citerator(const Iterator& base): base(base){};

    value_citerator& operator=(const value_citerator&) = default;
    value_citerator& operator=(value_citerator&&) = default;

    bool operator==(const value_citerator& that) { return base == that.base; };
    bool operator!=(const value_citerator& that) { return base != that.base; };

    reference operator*() const { return base->second; }
    reference operator->() const { return &base->second; }

    value_citerator& operator++() { ++base; return *this; }
    value_citerator& operator--() { --base; return *this; }
};

template<class Iterator>
value_citerator<Iterator> citerate_values(const Iterator& it) {
    return value_citerator<Iterator>{ it };
}

template<class Iterator>
std::pair<value_citerator<Iterator>, value_citerator<Iterator>> citerate_values(const std::pair<Iterator, Iterator>& it) {
    return std::pair<value_citerator<Iterator>, value_citerator<Iterator>>{
        citerate_values(it.first),
        citerate_values(it.second)
    };
}

#endif /* ITERATORS_HPP_ */
