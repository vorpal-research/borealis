//
// Created by ice-phoenix on 10/1/15.
//

#ifndef SANDBOX_CACHINGTRANSFORMER_H
#define SANDBOX_CACHINGTRANSFORMER_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

#define CALL(CLASS, WHAT) \
    static_cast<T*>(this)->transform##CLASS(WHAT)

template<class T>
class CachingTransformer : public Transformer<T> {

    using Base = Transformer<T>;

    std::unordered_map<PredicateState::Ptr, PredicateState::Ptr> transformCache;

public:

    CachingTransformer(FactoryNest FN) : Base(FN) {};

    using Base::transformBase;

    PredicateState::Ptr transformBase(PredicateState::Ptr ps) {
        TRACE_FUNC;
        if (auto cached = util::at(transformCache, ps)) {
            return cached.getUnsafe();
        } else {
            return transformCache[ps] = Base::transformBase(ps);
        }
    }

};

} // namespace borealis

#endif //SANDBOX_CACHINGTRANSFORMER_H
