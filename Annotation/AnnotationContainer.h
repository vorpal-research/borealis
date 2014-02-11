/*
 * AnnotationContainer.h
 *
 *  Created on: Sep 3, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATIONCONTAINER_H_
#define ANNOTATIONCONTAINER_H_

#include <memory>
#include <vector>

#include "Actions/comments.h"
#include "Annotation/AnnotationCast.h"
#include "Protobuf/Gen/Annotation/AnnotationContainer.pb.h"

#include "Util/macros.h"

namespace borealis {

/** protobuf -> Annotation/AnnotationContainer.proto
import "Annotation/Annotation.proto";

package borealis.proto;

message AnnotationContainer {
    repeated borealis.proto.Annotation data = 1;
}

**/
class AnnotationContainer {
    typedef std::vector<Annotation::Ptr> data_t;
    data_t data_;
public:
    using Ptr = std::shared_ptr<AnnotationContainer>;
    using ProtoPtr = std::unique_ptr<proto::AnnotationContainer>;
    using reference = data_t::reference;
    using const_reference = data_t::const_reference;
    using pointer = data_t::pointer;
    using value_type = data_t::value_type;

    AnnotationContainer()                           = default;
    AnnotationContainer(const AnnotationContainer&) = default;
    AnnotationContainer(AnnotationContainer&&)      = default;
    AnnotationContainer(
            const borealis::comments::GatherCommentsAction& act,
            const borealis::TermFactory::Ptr& tf) {
        const auto& comments = act.getComments();
        data_.reserve(comments.size());
        for (const auto& comment: comments) {
            data_.push_back(fromParseResult(comment.first, comment.second, tf));
        }
    }

    auto begin() QUICK_RETURN(data_.begin());
    auto begin() QUICK_CONST_RETURN(data_.begin());
    auto end() QUICK_RETURN(data_.end());
    auto end() QUICK_CONST_RETURN(data_.begin());

    const Annotation& operator[](size_t ix)       { return *data_[ix]; }
    const Annotation& operator[](size_t ix) const { return *data_.at(ix); }

    void push_back(const Annotation::Ptr& ptr) {
        data_.push_back(ptr);
    }

    void push_back(Annotation::Ptr&& ptr) {
        data_.push_back(std::move(ptr));
    }

    void mergeIn(const AnnotationContainer::Ptr& other) {
        data_.insert(data_.begin(), other->data_.begin(), other->data_.end());
    }

    const data_t& data() const {
        return data_;
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* ANNOTATIONCONTAINER_H_ */
