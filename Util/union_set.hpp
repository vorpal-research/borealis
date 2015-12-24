#ifndef UNION_SET_HPP
#define UNION_SET_HPP

#include <memory>

#include "Util/collections.hpp"

#include "Util/cow_set.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

template<class T, template<class> class Set>
class union_set {
    cow_set<std::shared_ptr<Set<T>>> lookBack;
    std::shared_ptr<Set<T>> current;

    Set<T>& getCurrent() {
        if(!current) current = std::make_shared<Set<T>>();
        return *current;
    }

public:
    union_set(const union_set& that): lookBack(that.lookBack), current() {
        if(that.current) lookBack.insert(that.current);
    }
    union_set(union_set&&) = default;
    union_set(): lookBack(), current() {}

    union_set& operator=(const union_set& that) {
        current.reset();
        lookBack = that.lookBack;
        if(that.current) lookBack.insert(that.current);
    }
    union_set& operator=(union_set&& that) = default;

    template<class Arg>
    void insert(Arg&& value) {
        getCurrent().insert(std::forward<Arg>(value));
    }

    static union_set unite( const union_set& lhv, const union_set& rhv) {
        union_set result = lhv;
        result.lookBack.insert(rhv.lookBack.begin(), rhv.lookBack.end());
        if(rhv.current) result.lookBack.insert(rhv.current);
        return std::move(result);
    }

    static union_set unite( union_set&& lhv, const union_set& rhv) {
        lhv.lookBack.insert(rhv.lookBack.begin(), rhv.lookBack.end());
        if(rhv.current) lhv.lookBack.insert(rhv.current);
        return std::move(lhv);
    }

    static union_set unite( const union_set& lhv, union_set&& rhv) {
        rhv.lookBack.insert(lhv.lookBack.begin(), lhv.lookBack.end());
        if(lhv.current) rhv.lookBack.insert(lhv.current);
        return std::move(rhv);
    }

    void finalize() {}

    template<class = void>
    auto toView() const {
        static Set<T> empty;
        auto&& currentView = current? util::viewContainer(*current) : util::viewContainer(empty);
        return util::viewContainer(lookBack).map(ops::dereference).flatten() >> currentView;
    }

    template<class = void>
    auto begin() const { return toView().begin(); }
    template<class = void>
    auto end() const { return toView().end(); }

};


} /* namespace util */
} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* UNION_SET_HPP */
