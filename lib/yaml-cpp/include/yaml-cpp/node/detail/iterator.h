#ifndef VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/dll.h"
#include "yaml-cpp/node/ptr.h"
#include "yaml-cpp/node/detail/node_iterator.h"
#include <type_traits>

namespace YAML {
namespace detail {

struct iterator_value;

template<typename V>
class iterator_base {

    typedef node_iterator RootIt;

    RootIt base;
    shared_memory_holder m_pMemory;

    template<typename> friend class iterator_base;

public:
    // only bidir iterators capable
    typedef std::forward_iterator_tag iterator_category;

    typedef size_t difference_type;
    typedef V value_type;
    typedef V& reference;

    typedef RootIt iterator_type;
    typedef iterator_base<V> self;

public:
    iterator_base() {}
    iterator_base(RootIt rhs, shared_memory_holder pMemory) :
        base(rhs), m_pMemory(pMemory) {}

    template<class W>
    iterator_base(
        const iterator_base<W>& rhs,
        typename std::enable_if<std::is_convertible<W*, V*>::value, void*>::type = nullptr
    ) : base(rhs.base), m_pMemory(rhs.m_pMemory) {}

    iterator_base(const iterator_base&) = default;

    inline value_type operator*() const {
        const auto& v = *base;
        if(v.pNode)
            return value_type(Node(*v, m_pMemory));
        if(v.first && v.second)
            return value_type(Node(*v.first, m_pMemory), Node(*v.second, m_pMemory));
        return value_type();
    }

    struct pointer {
        explicit pointer(const value_type& x) : m_ref(x) {}
        value_type* operator->() { return std::addressof(m_ref); }
        value_type m_ref;
    };

    inline pointer operator->() const {
        return pointer(operator*());
    }

    self& operator++() {
        base = std::next(base);
        return *this;
    }

    self operator++(int) { self tmp = *this; return self(++tmp); }

    inline bool operator!=(const self& X) const { return !operator==(X); }
    inline bool operator==(const self& X) const {
        return base == X.base;
    }
};

} // namespace detail
} // namespace YAML

#endif // VALUE_DETAIL_ITERATOR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
