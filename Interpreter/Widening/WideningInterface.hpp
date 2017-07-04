//
// Created by abdullin on 6/26/17.
//

#ifndef BOREALIS_WIDENINGINTERFACE_H
#define BOREALIS_WIDENINGINTERFACE_H

namespace borealis {
namespace absint {

class DomainFactory;

template <class T>
class WideningInterface {
public:

    virtual ~WideningInterface() = default;

    virtual T get_prev(const T& value, DomainFactory* factory) const = 0;
    virtual T get_next(const T& value, DomainFactory* factory) const = 0;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_WIDENINGINTERFACE_H
