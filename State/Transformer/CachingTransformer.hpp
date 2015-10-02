//
// Created by ice-phoenix on 10/1/15.
//

#ifndef SANDBOX_CACHINGTRANSFORMER_H
#define SANDBOX_CACHINGTRANSFORMER_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

template<class T>
class CachingTransformer : public Transformer<T> {

    using Base = Transformer<T>;

    std::unordered_map<PredicateState::Ptr, PredicateState::Ptr> predicateStateCache;

public:

    CachingTransformer(FactoryNest FN) : Base(FN) {};

    using Base::transformBase;

    PredicateState::Ptr transformBase(PredicateState::Ptr ps) {
        if (auto cached = util::at(predicateStateCache, ps)) {
            return cached.getUnsafe();
        } else {
            return predicateStateCache[ps] = Base::transformBase(ps);
        }
    }

};

} // namespace borealis

#endif //SANDBOX_CACHINGTRANSFORMER_H
