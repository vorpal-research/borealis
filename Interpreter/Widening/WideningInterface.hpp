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

    WideningInterface(DomainFactory* factory) : factory_(factory) {}
    virtual ~WideningInterface() = default;

    virtual T get_prev(const T& value) = 0;
    virtual T get_next(const T& value) = 0;

protected:

    DomainFactory* factory_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_WIDENINGINTERFACE_H
