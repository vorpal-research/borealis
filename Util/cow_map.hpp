//
// Created by abdullin on 9/29/17.
//

#ifndef BOREALIS_COW_MAP_HPP
#define BOREALIS_COW_MAP_HPP

#include <unordered_map>

namespace borealis {
namespace util {

namespace cow_map_impl_ {

template <class Key, class Value>
using HashMapImpl = std::unordered_map<Key, Value>;

}   // namespace cow_map_impl_

template <class Key, class Value, template<class,class> class Map = cow_map_impl_::HashMapImpl>
class cow_map {
    template<class ...Args>
    static std::shared_ptr<Map<Key, Value>> mkMap(Args&&... args) {
        return std::make_shared<Map<Key, Value>>(std::forward<Args>(args)...);
    }

    std::shared_ptr<Map<Key, Value>> shared_ = mkMap();

    std::shared_ptr<Map<Key, Value>>& split() {
        if(!shared_.unique())
            shared_ = mkMap(*shared_);
        return shared_;
    }

public:
    using iterator = typename Map<Key, Value>::iterator;
    using value_type = std::pair<Key, Value>;

    cow_map() = default;
    cow_map(cow_map&&) = default;
    cow_map(const cow_map& that): shared_(that.shared_) {}
    cow_map& operator=(cow_map&&) = default;
    cow_map& operator=(const cow_map& that){
        shared_ = that.shared_;
        return *this;
    }

    cow_map(std::initializer_list<value_type> il): shared_(mkMap(il)) {}
    template <class InputIt>
    cow_map(InputIt begin, InputIt end): shared_(mkMap(begin, end)) {}

    size_t count( const Key& key ) const {
        return shared_->count(key);
    }

    size_t size() const {
        return shared_->size();
    }

    Value& at( const Key& key ) {
        return split()->at(key);
    }

    const Value& at( const Key& key ) const {
        return shared_->at(key);
    }

    iterator begin() const {
        return shared_->begin();
    }

    iterator end() const {
        return shared_->end();
    }

    iterator find( const Key& key ) const {
        return shared_->find(key);
    }

    bool sameAs( const cow_map& other ) const {
        return shared_ == other.shared_;
    }

    bool empty() const {
        return shared_->empty();
    }

    void clear() {
        // faster than split()->clear();
        shared_ = mkMap();
    }

    Value& operator[]( const Key& key ) {
        return split()->operator[](key);
    }

    Value& operator[]( Key&& key ) {
        return split()->operator[](std::move(key));
    }

    std::pair<iterator,bool> insert( const value_type& value ) {
        return split()->insert(value);
    }

    std::pair<iterator,bool> insert( value_type&& value ) {
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
    size_t erase( const Key& key ) {
        return split()->erase(key);
    }

    void swap(cow_map& that) {
        std::swap(shared_, that.shared_);
    }

};

}   // namespace util
}   // namespace borealis

#endif //BOREALIS_COW_MAP_HPP
