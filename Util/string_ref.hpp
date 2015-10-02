/*
 * string_ref.hpp
 *
 *  Created on: Jan 22, 2014
 *      Author: belyaev
 */

#ifndef STRING_REF_HPP_
#define STRING_REF_HPP_

#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace util {

class string_ref {
public:
    typedef const char *iterator;
    static const size_t npos = ~size_t(0);

private:
    // The start of the string, in an external buffer.
    const char *Data;
    // The length of the string.
    size_t Length;

public:
    // Construct an empty string ref.
    /*implicit*/string_ref() : Data(0), Length(0) {}

    // Construct a string ref from a cstring.
    /*implicit*/string_ref(const char *Str)
    : Data(Str), Length(::strlen(Str)) {}

    // Construct a string ref from a pointer and length.
    /*implicit*/string_ref(const char *_Data, unsigned _Length)
    : Data(_Data), Length(_Length) {}

    // Construct a string ref from an std::string.
    /*implicit*/string_ref(const std::string &Str)
    : Data(Str.c_str()), Length(Str.length()) {}

    // name Iterators
    iterator begin() const {
        return Data;
    }

    iterator end() const {
        return Data + Length;
    }

    // String Operations

    // data - Get a pointer to the start of the string (which may not be null
    // terminated).
    const char *data() const {
        return Data;
    }

    /// empty - Check if the string is empty.
    bool empty() const {
        return Length == 0;
    }

    /// size - Get the string size.
    size_t size() const {
        return Length;
    }

    char back() const {
        ASSERTC(!empty());
        return Data[Length - 1];
    }

    // equals - Check for string equality, this is more efficient than
    // compare() in when the relative ordering of inequal strings isn't needed.
    bool equals(const string_ref &RHS) const {
        return (Length == RHS.Length &&
            memcmp(Data, RHS.Data, RHS.Length) == 0);
    }

    // compare - Compare two strings; the result is -1, 0, or 1 if this string
    // is lexicographically less than, equal to, or greater than the \arg RHS.
    int compare(const string_ref &RHS) const {
        // Check the prefix for a mismatch.
        if (int Res = memcmp(Data, RHS.Data, std::min(Length, RHS.Length)))
            return Res < 0 ? -1 : 1;

        // Otherwise the prefixes match, so we only need to check the lengths.
        if (Length == RHS.Length)
            return 0;
        return Length < RHS.Length ? -1 : 1;
    }

    // str - Get the contents as an std::string.
    std::string str() const {
        return std::string(Data, Length);
    }

    // @name Operator Overloads
    char operator[](size_t Index) const {
        ASSERT(Index < Length, "Invalid index!");
        return Data[Index];
    }

    // Type Conversions
    operator std::string() const {
        return str();
    }

    // String Predicates
    // startswith - Check if this string starts with the given Prefix.
    bool startswith(const string_ref &Prefix) const {
        return substr(0, Prefix.Length).equals(Prefix);
    }

    // endswith - Check if this string ends with the given Suffix.
    bool endswith(const string_ref &Suffix) const {
        return slice(size() - Suffix.Length, size()).equals(Suffix);
    }

    // Substring Operations
    // substr - Return a reference to the substring from [Start, Start + N).
    string_ref substr(size_t Start, size_t N = npos) const {
        Start = std::min(Start, Length);
        return string_ref(Data + Start, std::min(N, Length - Start));
    }

    // slice - Return a reference to the substring from [Start, End).
    string_ref slice(size_t Start, size_t End) const {
        Start = std::min(Start, Length);
        End = std::min(std::max(Start, End), Length);
        return string_ref(Data + Start, End - Start);
    }

    // split - Split into two substrings around the first occurence of a
    // separator character.
    std::pair<string_ref, string_ref> split(char Separator) const {
        auto it = std::find(begin(), end(), Separator);
        if (it == end())
            return std::make_pair(*this, string_ref());

        auto Idx = it - begin();
        return std::make_pair(slice(0, Idx), slice(Idx + 1, npos));
    }

    friend std::ostream& operator<<(std::ostream& ost, const string_ref& str) {
        if(ost.good()) {
            auto size = str.size();
            auto width = static_cast<decltype(size)>(ost.width());
            if(width <= size) {
                ost.write(str.Data, str.Length);
            } else {
                // FIXME: fix me
                BYE_BYE(std::ostream&, "Formatted output for string_refs not supported yet, sorry");
            }
        }

        return ost;
    }

};

// Comparison Operators
inline bool operator==(const string_ref &LHS, const string_ref &RHS) {
    return LHS.equals(RHS);
}

inline bool operator!=(const string_ref &LHS, const string_ref &RHS) {
    return !(LHS == RHS);
}

inline bool operator<(const string_ref &LHS, const string_ref &RHS) {
    return LHS.compare(RHS) == -1;
}

inline bool operator<=(const string_ref &LHS, const string_ref &RHS) {
    return LHS.compare(RHS) != 1;
}

inline bool operator>(const string_ref &LHS, const string_ref &RHS) {
    return LHS.compare(RHS) == 1;
}

inline bool operator>=(const string_ref &LHS, const string_ref &RHS) {
    return LHS.compare(RHS) != -1;
}

} /* namespace util */
} /* namespace borealis */

namespace std {

template<>
struct hash<borealis::util::string_ref> {
    size_t operator()(const borealis::util::string_ref& v) const noexcept {
        auto&& h = std::hash<char>{};

        if(v.empty()) return 0;

        size_t res = 3;
        for (auto&& c : v) {
            res = 17 * res + h(c);
        }
        return res;
    }
};

} /* namespace std */

#include "Util/unmacros.h"

#endif /* STRING_REF_HPP_ */
