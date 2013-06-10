/*
 * iterators.hpp
 *
 *  Created on: Feb 5, 2013
 *      Author: belyaev
 */

#ifndef ITERATORS_HPP_
#define ITERATORS_HPP_

#include <iterator>
#include <list>

#include "Util/meta.hpp"

namespace borealis {
namespace util {

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

////////////////////////////////////////////////////////////////////////////////
//
// Mapped iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class RootIt, class UnaryFunc>
class mapped_iterator {
  RootIt current;
  UnaryFunc Fn;
public:
  typedef typename std::iterator_traits<RootIt>::iterator_category
          iterator_category;
  typedef typename std::iterator_traits<RootIt>::difference_type
          difference_type;
  typedef decltype(Fn(*current)) value_type;

  typedef struct{} pointer;
  // typedef typename UnaryFunc::result_type* pointer;
  typedef value_type reference; // Can't modify value returned by Fn

  typedef RootIt iterator_type;
  typedef mapped_iterator<RootIt, UnaryFunc> _Self;

  inline const RootIt& getCurrent() const { return current; }
  inline const UnaryFunc& getFunc() const { return Fn; }

  inline mapped_iterator(const RootIt& I, UnaryFunc F) : current(I), Fn(F) {}
  inline mapped_iterator(const mapped_iterator& It) = default;

  inline value_type operator*() const {    // All this work to do this
    return Fn(*current);                   // little change
  }

  _Self& operator++() { ++current; return *this; }
  _Self& operator--() { --current; return *this; }
  _Self  operator++(int) { _Self __tmp = *this; ++current; return __tmp; }
  _Self  operator--(int) { _Self __tmp = *this; --current; return __tmp; }

  _Self  operator+ (difference_type n) const {
    return _Self(current + n, Fn);
  }
  _Self& operator+=(difference_type n) { current += n; return *this; }
  _Self  operator- (difference_type n) const {
    return _Self(current - n, Fn);
  }
  _Self& operator-=(difference_type n) { current -= n; return *this; }

  inline bool operator!=(const _Self& X) const { return !operator==(X); }
  inline bool operator==(const _Self& X) const { return current == X.current; }
  inline bool operator< (const _Self& X) const { return current <  X.current; }

  inline difference_type operator-(const _Self& X) const {
    return current - X.current;
  }
};

template<class _Iterator, class Func>
inline mapped_iterator<_Iterator, Func>
operator+(typename mapped_iterator<_Iterator, Func>::difference_type N,
          const mapped_iterator<_Iterator, Func>& X) {
  return mapped_iterator<_Iterator, Func>(X.getCurrent() - N, X.getFunc());
}

// map_iterator - provides a convenient way to create mapped_iterators,
// just like std::make_pair is used for creating pairs...
//
template<class ItTy, class FuncTy>
inline mapped_iterator<ItTy, FuncTy> map_iterator(const ItTy& I, FuncTy F) {
  return mapped_iterator<ItTy, FuncTy>(I, F);
}

////////////////////////////////////////////////////////////////////////////////
//
// Glued iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class RootIt>
class glued_iterator {
    typedef std::list<std::pair<RootIt, RootIt>> Ranges;
    Ranges rest;

    const RootIt& current() const {
        return rest.front().first;
    }

    RootIt& current() {
        return rest.front().first;
    }

    RootIt current_end() const {
        return rest.front().second;
    }

    void validate() {
        while (!rest.empty() && current() == current_end()) {
            rest.pop_front();
        }
    }

public:
    // only bidir iterators capable
    typedef std::forward_iterator_tag iterator_category;
    typedef typename std::iterator_traits<RootIt>::difference_type
            difference_type;
    typedef typename std::iterator_traits<RootIt>::value_type
            value_type;

    typedef typename std::iterator_traits<RootIt>::pointer
            pointer;
    typedef typename std::iterator_traits<RootIt>::reference
            reference;

    typedef RootIt iterator_type;
    typedef glued_iterator self;

    inline glued_iterator() {};
    inline explicit glued_iterator(const Ranges& ranges) : rest(ranges) {
        validate();
    }

    inline explicit glued_iterator(Ranges&& ranges) : rest(std::move(ranges)) {
        validate();
    }

    inline glued_iterator(const glued_iterator& It) = default;
    inline glued_iterator(glued_iterator&& It) = default;

    inline reference operator*() const {
        return *current();
    }

    inline pointer operator->() const {
        return &*current();
    }

    self& operator++() {
        if (rest.empty()) return *this;

        ++current();
        validate();

        return *this;
    }

    self operator++(int) { self tmp = *this; return self(++tmp); }

    inline bool operator!=(const self& X) const { return !operator==(X); }
    inline bool operator==(const self& X) const {
        if (rest.empty()) return X.rest.empty();
        if (X.rest.empty()) return false;
        return current() == X.current();
    }
};

template<class ...It>
glued_iterator<util::head_of_row_q<It...>> glue_iterator(const std::pair<It, It>&... pairs) {
    typedef util::head_of_row_q<It...> RealIt;
    typedef std::pair<RealIt, RealIt> pair_t;
    return glued_iterator<RealIt>{std::list<pair_t>{pairs...}};
}

////////////////////////////////////////////////////////////////////////////////
//
// Flattened iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class RootIt>
class flattened_iterator {
    RootIt current;
    RootIt end;

    typedef decltype(std::begin(*current)) ChildIt;

    ChildIt child;

    void validate() {
        while (current != end && child == current->end()) {
            ++current;
            if (current != end) {
                child = current->begin();
            }
        }
    }

public:
    // only bidir iterators capable
    typedef std::forward_iterator_tag iterator_category;
    typedef typename std::iterator_traits<ChildIt>::difference_type
            difference_type;
    typedef typename std::iterator_traits<ChildIt>::value_type
            value_type;

    typedef typename std::iterator_traits<ChildIt>::pointer
            pointer;
    typedef typename std::iterator_traits<ChildIt>::reference
            reference;

    typedef RootIt iterator_type;
    typedef flattened_iterator<RootIt> self;

    inline const RootIt& getCurrent() const { return current; }
    inline ChildIt& getChild() { return child; }
    inline const ChildIt& getChild() const { return child; }

    inline flattened_iterator(const RootIt& I, const RootIt& E)
    : current(I), end(E), child(I != E ? I->begin() : ChildIt()) {
        validate();
    }

    inline explicit flattened_iterator(const std::pair<RootIt, RootIt>& P)
    : flattened_iterator(P.first, P.second) {}

    inline flattened_iterator(const flattened_iterator& It) = default;

    inline reference operator*() const {
        return *child;
    }

    inline pointer operator->() const {
        return &*child;
    }

    self& operator++() {
        if (current == end) return *this;

        ++getChild();
        validate();

        return *this;
    }

    self operator++(int) { self tmp = *this; return self(++tmp); }

    inline bool operator!=(const self& X) const { return !operator==(X); }
    inline bool operator==(const self& X) const {
        if (current != X.current) return false;
        return (current == end && X.current == end) ||
               (current == X.current && getChild() == X.getChild());
    }
};

template <class ItTy>
inline flattened_iterator<ItTy> flat_iterator(const ItTy& I, const ItTy& E) {
  return flattened_iterator<ItTy>(I, E);
}

template <class ItTy>
inline flattened_iterator<ItTy> flat_iterator(const ItTy& E) {
  return flat_iterator(E, E);
}

template <class ItTy>
inline flattened_iterator<flattened_iterator<ItTy>> flat2_iterator(const ItTy& I, const ItTy& E) {
  return flat_iterator(flat_iterator(I, E), flat_iterator(E));
}

template <class ItTy>
inline flattened_iterator<flattened_iterator<ItTy>> flat2_iterator(const ItTy& E) {
  return flat2_iterator(E, E);
}



template <class Con>
inline flattened_iterator<typename Con::iterator> flatBegin(Con& C) {
  return flat_iterator(C.begin(), C.end());
}

template <class Con>
inline flattened_iterator<typename Con::iterator> flatEnd(Con& C) {
  return flat_iterator(C.end());
}

template <class Con>
inline flattened_iterator<flattened_iterator<typename Con::iterator>> flat2Begin(Con& C) {
  return flat2_iterator(C.begin(), C.end());
}

template <class Con>
inline flattened_iterator<flattened_iterator<typename Con::iterator>> flat2End(Con& C) {
  return flat2_iterator(C.end());
}

template <class Con>
inline flattened_iterator<typename Con::const_iterator> flatBegin(const Con& C) {
  return flat_iterator(C.begin(), C.end());
}

template <class Con>
inline flattened_iterator<typename Con::const_iterator> flatEnd(const Con& C) {
  return flat_iterator(C.end());
}

template <class Con>
inline flattened_iterator<flattened_iterator<typename Con::const_iterator>> flat2Begin(const Con& C) {
  return flat2_iterator(C.begin(), C.end());
}

template <class Con>
inline flattened_iterator<flattened_iterator<typename Con::const_iterator>> flat2End(const Con& C) {
  return flat2_iterator(C.end());
}

////////////////////////////////////////////////////////////////////////////////
//
// Filtered iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class RootIt, class Pred>
class filtered_iterator {
    RootIt current;
    RootIt end;
    Pred pred;

    // restore the invariant ( pred(*current) == true || current == end )
    void validate() {
        while ((current != end) && !(pred(*current))) ++current;
    }

public:
    // only bidir iterators capable
    typedef std::forward_iterator_tag iterator_category;
    typedef typename std::iterator_traits<RootIt>::difference_type
            difference_type;
    typedef typename std::iterator_traits<RootIt>::value_type
            value_type;

    typedef typename std::iterator_traits<RootIt>::pointer
            pointer;
    typedef typename std::iterator_traits<RootIt>::reference
            reference;

    typedef RootIt iterator_type;
    typedef filtered_iterator<RootIt, Pred> self;

    inline const RootIt& getCurrent() const { return current; }

    inline filtered_iterator(const RootIt& I, const RootIt& E, Pred pred)
    : current(I), end(E), pred(pred) {
        validate();
    }

    inline filtered_iterator(const std::pair<RootIt, RootIt>& P, Pred pred)
    : filtered_iterator(P.first, P.second, pred) {}

    inline filtered_iterator(const filtered_iterator& It) = default;

    inline reference operator*() const {
        return *current;
    }

    inline pointer operator->() const {
        return &*current;
    }

    self& operator++() {
        if(current == end) return *this;

        ++current;
        validate();

        return *this;
    }

    self operator++(int) { self tmp = *this; return self(++tmp); }

    inline bool operator!=(const self& X) const { return !operator==(X); }
    inline bool operator==(const self& X) const {
        return current == X.current;
    }
};

template<class It, class Pred>
filtered_iterator<It, Pred> filter_iterator(It beg, It end, Pred pred) {
    return filtered_iterator<It, Pred>(beg, end, pred);
}

template<class It, class Pred>
filtered_iterator<It, Pred> filter_iterator(It end, Pred pred) {
    return filtered_iterator<It, Pred>(end, end, pred);
}

////////////////////////////////////////////////////////////////////////////////
//
// Key iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class Iterator>
class key_iterator {
    Iterator base;
public:
    typedef typename Iterator::iterator_category
            iterator_category;
    typedef typename Iterator::value_type::first_type
            value_type;
    typedef typename Iterator::difference_type
            difference_type;

    typedef value_type* pointer;
    typedef value_type& reference;

    key_iterator(const key_iterator&) = default;
    key_iterator(key_iterator&&) = default;
    key_iterator(const Iterator& base) : base(base) {};

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

////////////////////////////////////////////////////////////////////////////////
//
// Value iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class Iterator>
class value_iterator {
    Iterator base;
public:
    typedef typename Iterator::iterator_category
            iterator_category;
    typedef typename Iterator::value_type::second_type
            value_type;
    typedef typename Iterator::difference_type
            difference_type;

    typedef value_type* pointer;
    typedef value_type& reference;

    value_iterator(const value_iterator&) = default;
    value_iterator(value_iterator&&) = default;
    value_iterator(const Iterator& base) : base(base) {};

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

////////////////////////////////////////////////////////////////////////////////
//
// Value const iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class Iterator>
class value_citerator {
    Iterator base;
public:
    typedef typename Iterator::iterator_category
            iterator_category;
    typedef typename Iterator::value_type::second_type
            value_type;
    typedef typename Iterator::difference_type
            difference_type;

    typedef const value_type* pointer;
    typedef const value_type& reference;

    value_citerator(const value_citerator&) = default;
    value_citerator(value_citerator&&) = default;
    value_citerator(const Iterator& base) : base(base) {};

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

} // namespace util
} // namespace borealis

#endif /* ITERATORS_HPP_ */
