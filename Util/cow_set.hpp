#ifndef COW_SET_HPP
#define COW_SET_HPP

#include <unordered_set>
#include <memory>

#include "Util/generate_macros.h"

namespace borealis {
namespace util {

namespace cow_set_impl_ {

template <class T>
using HashSetImpl = std::unordered_set<T>;

} /* namespace cow_set_impl_ */

template<class T, template<class>class Set = cow_set_impl_::HashSetImpl>
class cow_set {
    template<class ...Args>
    static std::shared_ptr<Set<T>> mkSet(Args&&... args) {
        return std::make_shared<Set<T>>(std::forward<Args>(args)...);
    }

    std::shared_ptr<Set<T>> shared_ = mkSet();

    std::shared_ptr<Set<T>>& split() {
        if(!shared_.unique())
            shared_ = mkSet(*shared_);
        return shared_;
    }

public:
    using iterator = typename Set<T>::iterator;
    using value_type = T;

    cow_set() = default;
    cow_set(cow_set&&) = default;
    cow_set(const cow_set& that): shared_(that.shared_) {}
    cow_set& operator=(cow_set&&) = default;
    cow_set& operator=(const cow_set& that){
        shared_ = that.shared_;
        return *this;
    }

    cow_set(std::initializer_list<T> il): shared_(mkSet(il)) {}
    template<class InputIt>
    cow_set(InputIt begin, InputIt end): shared_(mkSet(begin, end)) {}

    bool empty() const {
        return shared_->empty();
    }

    size_t count(const T& element) const {
        return shared_->count(element);
    }

    size_t size() const {
        return shared_->size();
    }

    iterator find(const T& element) const {
        return shared_->find(element);
    }

    iterator begin() const {
        return shared_->begin();
    }

    iterator end() const {
        return shared_->end();
    }

    void clear() {
        // faster than split()->clear();
        shared_ = mkSet();
    }

    std::pair<iterator,bool> insert( const value_type& value ) {
        return split()->insert(value);
    }

    std::pair<iterator,bool> insert(value_type&& value ) {
        return split()->insert(std::move(value));
    }
    void insert( iterator first, iterator last ) {
        if(first == last) return;
        if(first == begin() && last == end()) return;
        split()->insert(first, last);
    }
    template< class InputIt >
    void insert( InputIt first, InputIt last ) {
        if(first == last) return;
        split()->insert(first, last);
    }
    void insert( std::initializer_list<value_type> ilist ) {
        if(ilist.size() == 0) return;
        split()->insert(ilist);
    }

    template<class ...Args>
    std::pair<iterator,bool> emplace( Args&&... args ) {
        return split()->emplace(std::forward<Args>(args)...);
    }

    iterator erase( iterator pos ) {
        return split()->erase(pos);
    }
    iterator erase( iterator first, iterator last ) {
        return split()->erase(first, last);
    }
    size_t erase( const T& key ) {
        return split()->erase(key);
    }
    void swap(cow_set& that) {
        std::swap(shared_, that.shared_);
    }

};

} /* namespace util */
} /* namespace borealis */

#include "Util/generate_unmacros.h"

#endif /* COW_SET_HPP */
