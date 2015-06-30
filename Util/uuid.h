//
// Created by ice-phoenix on 6/30/15.
//

#ifndef SANDBOX_UUID_H
#define SANDBOX_UUID_H

#include <memory>
#include <uuid/uuid.h>

namespace borealis {

class UUID {
public:

    using uuid_unparsed_t = std::unique_ptr<char[]>;

    const char* unparsed() {
        if (not isUnparsed()) {
            unparsed_ = uuid_unparsed_t{ new char[36 + 1] };
            uuid_unparse(storage_, unparsed_.get());
        }
        return unparsed_.get();
    }

    bool isUnparsed() const {
        return !!unparsed_;
    }

    static UUID generate() {
        UUID res;
        uuid_generate(res.storage_);
        return res;
    }

private:

    uuid_t storage_;
    uuid_unparsed_t unparsed_;

};

template<class Streamer>
Streamer& operator<<(Streamer& s, UUID& uuid) {
    return s << uuid.unparsed();
}

} // namespace borealis

#endif //SANDBOX_UUID_H
