/*
 * iterators.hpp
 *
 *  Created on: Feb 5, 2013
 *      Author: belyaev
 */

#ifndef ITERATORS_HPP_
#define ITERATORS_HPP_

#include <iterator>
#include <cmath>
#include <list>
#include <memory>

#include "Util/meta.hpp"
#include "Util/option.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

namespace impl_ {

template<class It, class Dist>
void advanceWithLimit(It& it, Dist dist, const It& limit, std::forward_iterator_tag) {
    for(Dist i = 0; i < dist; ++i) {
        if(it == limit) break;
        ++it;
    }
}

template<class It, class Dist>
void advanceWithLimit(It& it, Dist dist, const It& limit, std::bidirectional_iterator_tag) {
    for(Dist i = 0; i < dist; ++i) {
        if(it == limit) break;
        ++it;
    }
}

template<class It, class Dist>
void advanceWithLimit(It& it, Dist dist, const It& limit, std::random_access_iterator_tag) {
    Dist realDist = std::distance(it, limit);
    it += std::min(realDist, dist);
}

} /* namespace impl_ */

template<class It, class Dist>
void advanceWithLimit(It& it, Dist dist, const It& limit) {
    impl_::advanceWithLimit(it, dist, limit, typename std::iterator_traits<It>::iterator_category{});
}


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

template<
    class BaseIt,
    class Derived,
    class Arrow      = BaseIt,
    class Reference  = typename std::iterator_traits<BaseIt>::reference,
    class Value      = typename std::iterator_traits<BaseIt>::value_type,
    class Difference = typename std::iterator_traits<BaseIt>::difference_type
>
struct iterator_adapter {
    // XXX: support other categories?
    using iterator_category = std::forward_iterator_tag;
    using value_type        = Value;
    using difference_type   = Difference;
    using reference         = Reference;
    using pointer           = Arrow;

private:
    Derived* derived() { return static_cast<Derived*>(this); }
    const Derived* derived() const { return static_cast<const Derived*>(this); }

public:
    Derived& operator++() {
        if(!derived()->hasNext()) return *derived();
        derived()->increment();
        derived()->validate();
        return *derived();
    }
    Derived operator++(int) {
        Derived copy = *derived();
        ++(*derived());
        return copy;
    }

    friend bool operator==(const Derived& lhv, const Derived& rhv) {
        return lhv.equals(&rhv);
    }
    friend bool operator!=(const Derived& lhv, const Derived& rhv) {
        return !lhv.equals(&rhv);
    }
    friend bool operator< (const Derived& lhv, const Derived& rhv) {
        return lhv.compare(&rhv) < 0U;
    }
    friend bool operator<= (const Derived& lhv, const Derived& rhv) {
        return lhv.compare(&rhv) <= 0U;
    }
    friend bool operator> (const Derived& lhv, const Derived& rhv) {
        return lhv.compare(&rhv) > 0U;
    }
    friend bool operator>= (const Derived& lhv, const Derived& rhv) {
        return lhv.compare(&rhv) >= 0U;
    }

    auto operator*() const -> Reference {
        return (derived()->dereference());
    }
    auto operator*() -> Reference {
        return (derived()->dereference());
    }

    auto operator->() const -> Arrow {
        return (derived()->current());
    }

    bool hasNext() const {
        return true;
    }

    void increment() {
        ++derived()->current();
    }

    void validate() {}

    bool equals(const Derived* that) const {
        return derived()->current() == that->current();
    }

    int compare(const Derived* that) const {
        return derived()->current() > that->current() ? 1:
               derived()->current() < that->current() ? -1:
               0;
    }

    BaseIt arrow() const {
        return derived()->current();
    }

    reference dereference() const {
        return (*derived()->current());
    }

};

template<class Elem>
struct storage_ppointer {
    Elem val;

    Elem* operator->() {
        return &val;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// Mapped iterator
//
////////////////////////////////////////////////////////////////////////////////

namespace impl_ {

template<class RootIt>
using iter_ref = typename std::iterator_traits<RootIt>::reference;

template<class RootIt, class UnaryFunc>
using iter_mapped_ref = decltype(std::declval<UnaryFunc>()(std::declval<iter_ref<RootIt>>()));

} // namespace impl_

template<class RootIt, class UnaryFunc>
class mapped_iterator:
    public iterator_adapter<
        RootIt,
        mapped_iterator<RootIt, UnaryFunc>,
        storage_ppointer<impl_::iter_mapped_ref<RootIt, UnaryFunc>>,
        impl_::iter_mapped_ref<RootIt, UnaryFunc>,
        impl_::iter_mapped_ref<RootIt, UnaryFunc>
    > {
    RootIt root;
    std::shared_ptr<UnaryFunc> Fn;

    const UnaryFunc& fn() const { 
        if (!Fn) throw 0;
        return *Fn;
    }

public:
    typedef RootIt iterator_type;
    typedef mapped_iterator<RootIt, UnaryFunc> self;

    RootIt& current() { return root; }
    const RootIt& current() const { return root; }
    const UnaryFunc& getFunc() const { return fn(); }

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(mapped_iterator);

    mapped_iterator(const RootIt& I, UnaryFunc F) :
        root(I), Fn(std::make_shared<UnaryFunc>(F)) {}

    typename mapped_iterator::reference dereference() const {
        return fn()(*root);
    }

    typename mapped_iterator::pointer arrow() const {
        return typename mapped_iterator::pointer{ **this };
    }
};

// map_iterator - provides a convenient way to create mapped_iterators,
// just like std::make_pair is used for creating pairs...
//
template<class ItTy, class FuncTy>
inline mapped_iterator<ItTy, FuncTy> map_iterator(const ItTy& I, FuncTy F) {
    return mapped_iterator<ItTy, FuncTy>{ I, F };
}

////////////////////////////////////////////////////////////////////////////////
//
// Glued iterator
//
////////////////////////////////////////////////////////////////////////////////

template<class RootIt>
class glued_iterator:
    public iterator_adapter<RootIt,glued_iterator<RootIt>> {

    typedef std::list<std::pair<RootIt, RootIt>> Ranges;
    Ranges rest;

public:
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

    typedef RootIt iterator_type;
    typedef glued_iterator self;

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(glued_iterator);

    inline explicit glued_iterator(const Ranges& ranges) : rest(ranges) {
        validate();
    }
    inline explicit glued_iterator(Ranges&& ranges) : rest(std::move(ranges)) {
        validate();
    }

    bool hasNext() const {
        return !rest.empty();
    }

    bool equals(const self* X) const {
        if (rest.empty()) return X->rest.empty();
        if (X->rest.empty()) return false;
        return current() == X->current();
    }
};

template<class ...It>
glued_iterator<util::head_of_row_q<It...>> glue_iterator(const std::pair<It, It>&... pairs) {
    typedef util::head_of_row_q<It...> RealIt;
    typedef std::pair<RealIt, RealIt> pair_t;
    return glued_iterator<RealIt>{ std::list<pair_t>{pairs...} };
}

////////////////////////////////////////////////////////////////////////////////
//
// Flattened iterator
//
////////////////////////////////////////////////////////////////////////////////

namespace impl_ {
    template<class Iter>
    using flatten_one_level = decltype(std::begin(*std::declval<Iter>()));
} // namespace impl_

template<class RootIt>
class flattened_iterator:
    public iterator_adapter<
        impl_::flatten_one_level<RootIt>,
        flattened_iterator<RootIt>
    >{
    RootIt root;
    RootIt end;

    typedef impl_::flatten_one_level<RootIt> ChildIt;

    option<ChildIt> child;

public:
    typedef RootIt iterator_type;
    typedef flattened_iterator<RootIt> self;

    const RootIt& currentRoot() const { return root; }
    RootIt& currentRoot() { return root; }
    ChildIt& current() { if (!child) throw 0; return child.getUnsafe(); }
    const ChildIt& current() const { if (!child) throw 0; return child.getUnsafe(); }

    void validate() {
        while (root != end && child == std::end(*root)) {
            ++root;
            if (root != end) {
                child = just(std::begin(*root));
            }
        }
    }

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(flattened_iterator);

    flattened_iterator(const RootIt& I, const RootIt& E)
        : root(I), end(E), child(I != E ? just(std::begin(*I)) : nothing()) {
        validate();
    }
    explicit flattened_iterator(const std::pair<RootIt, RootIt>& P)
        : flattened_iterator(P.first, P.second) {}

    bool hasNext() const {
        return root != end;
    }

    inline bool equals(const self* that) const {
        if (this->root != that->root) return false;
        return (this->root == this->end && that->root == that->end) ||
               (this->root == that->root && this->current() == that->current());
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
class filtered_iterator:
    public iterator_adapter<
        RootIt,
        filtered_iterator<RootIt, Pred>
    > {
    RootIt root;
    RootIt end;
    std::shared_ptr<Pred> pred;

    const Pred& predf() const {
        if (!pred) throw 0;
        return *pred;
    }

public:
    typedef RootIt iterator_type;
    typedef filtered_iterator<RootIt, Pred> self;

    const RootIt& current() const { return root; }
    RootIt& current() { return root; }

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(filtered_iterator);

    filtered_iterator(const RootIt& I, const RootIt& E, Pred pred)
        : root(I), end(E), pred(std::make_shared<Pred>(pred)) {
        validate();
    }
    filtered_iterator(const std::pair<RootIt, RootIt>& P, Pred pred)
        : filtered_iterator(P.first, P.second, pred) {}

    // restore the invariant ( pred(*current) == true || current == end )
    void validate() {
        while ((root != end) && !(predf()(*root))) ++root;
    }

    bool hasNext() const {
        return root != end;
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
// Zipping iterator
// NB: Does not control buffer overflows!!!
//
////////////////////////////////////////////////////////////////////////////////

template<typename Iter1, typename Iter2>
class zipping_iterator {
    Iter1 iter1;
    Iter2 iter2;

public:
    typedef std::forward_iterator_tag iterator_category;
    typedef std::pair<typename std::iterator_traits<Iter1>::reference,
                      typename std::iterator_traits<Iter2>::reference> value_type;
    typedef std::ptrdiff_t difference_type;

    typedef value_type reference;

    struct pointer {
        value_type val;

        value_type* operator->() {
            return &val;
        }
    };


    DEFAULT_CONSTRUCTOR_AND_ASSIGN(zipping_iterator);

    zipping_iterator(const Iter1& i1, const Iter2& i2) : iter1(i1), iter2(i2) {};

    bool operator!=(const zipping_iterator& that) const { return !operator==(that); };
    bool operator==(const zipping_iterator& that) const {
        if (iter1 != that.iter1) return false;
        ASSERT(iter2 == that.iter2, "Malformed zip iterator!")
        return true;
    };

    reference operator*() const { return reference{ *iter1, *iter2 }; }
    pointer operator->() const { return pointer{ *this }; }

    zipping_iterator& operator++() { ++iter1; ++iter2; return *this; }
};

template<class Iter1, class Iter2>
zipping_iterator<Iter1, Iter2> zip(Iter1 i1, Iter2 i2) {
    return zipping_iterator<Iter1, Iter2>{ i1, i2 };
}

////////////////////////////////////////////////////////////////////////////////
//
// Counting iterator
//
////////////////////////////////////////////////////////////////////////////////

template<typename Elem>
class counting_iterator: public iterator_adapter<
    Elem,
    counting_iterator<Elem>,
    const Elem*,
    Elem, Elem, Elem // :-)
    > {
    Elem e;
public:
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(counting_iterator);

    counting_iterator(const Elem& e) : e(e) {};
    counting_iterator(Elem&& e) : e(std::move(e)) {};

    Elem dereference() const {
        return e;
    }

    const Elem* arrow() const {
        return &e;
    }

    bool equals(const counting_iterator* that) const {
        return this->e == that->e;
    }

    Elem compare(const counting_iterator* that) const {
        return this->e - that->e;
    }

    void increment() { ++e; }
};

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

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(key_iterator);

    key_iterator(const Iterator& base) : base(base) {};

    bool operator==(const key_iterator& that) const { return base == that.base; };
    bool operator!=(const key_iterator& that) const { return base != that.base; };

    reference operator*() const { return base->first; }
    pointer operator->() const { return &base->first; }

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

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(value_iterator);

    value_iterator(const Iterator& base) : base(base) {};

    bool operator==(const value_iterator& that) const { return base == that.base; };
    bool operator!=(const value_iterator& that) const { return base != that.base; };

    reference operator*() { return base->second; }
    pointer operator->() { return &base->second; }

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

    DEFAULT_CONSTRUCTOR_AND_ASSIGN(value_citerator);

    value_citerator(const Iterator& base) : base(base) {};

    bool operator==(const value_citerator& that) const { return base == that.base; };
    bool operator!=(const value_citerator& that) const { return base != that.base; };

    reference operator*() const { return base->second; }
    pointer operator->() const { return &base->second; }

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

#include "Util/unmacros.h"

#endif /* ITERATORS_HPP_ */
