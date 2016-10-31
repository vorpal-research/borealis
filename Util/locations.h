/*
 * Locations.h
 *
 *  Created on: Oct 9, 2012
 *      Author: belyaev
 */

#ifndef LOCATIONS_H_
#define LOCATIONS_H_

#include <clang/Basic/SourceManager.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Instruction.h>

#include <iostream>
#include <utility>

#include "Protobuf/Gen/Util/locations.pb.h"

#include "Util/indexed_string.hpp"
#include "Util/hash.hpp"
#include "Util/json_traits.hpp"
#include "Util/xml_traits.hpp"

#include "Util/generate_macros.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

/** protobuf -> Util/locations.proto
package borealis.proto;

message LocalLocus {
    optional uint32 line = 1;
    optional uint32 col = 2;
}

message Locus {
    optional borealis.proto.LocalLocus loc = 1;
    optional string filename = 2;
}

message LocusRange {
    optional borealis.proto.Locus lhv = 1;
    optional borealis.proto.Locus rhv = 2;
}

**/
struct LocalLocus {
    enum class measure{ line, col };
    static constexpr unsigned UNKNOWN_LOC = ~0U;

    unsigned line;
    unsigned col;

    LocalLocus(): line(UNKNOWN_LOC), col(UNKNOWN_LOC) {};
    LocalLocus(const LocalLocus& that) = default;
    LocalLocus(unsigned line, unsigned col): line(line), col(col) {};
    LocalLocus(const llvm::DebugLoc& dbg): line(dbg.getLine()), col(dbg.getCol()) {};
    LocalLocus(const clang::SourceLocation& src, const clang::SourceManager& mng):
        line(mng.getExpansionLineNumber(src)), col(mng.getExpansionColumnNumber(src)) {};

    const LocalLocus& operator=(const LocalLocus& that) {
        this->line = that.line;
        this->col  = that.col;
        return *this;
    }

    GENERATE_EQ(LocalLocus, line, col)
    GENERATE_LESS(LocalLocus, line, col)
    GENERATE_AUX_COMPARISONS(LocalLocus)

    const LocalLocus& operator+=(const LocalLocus& that) {
        this->line += that.line;
        this->col  += that.col;
        return *this;
    }

    LocalLocus operator+(const LocalLocus& that) const {
        LocalLocus copy(*this);
        copy += that;
        return std::move(copy);
    }

    const LocalLocus& operator-=(const LocalLocus& that) {
        if(this->line > that.line) this->line -= that.line;
        else this->line = 1U;
        if(this->col > that.col) this->col -= that.col;
        else this->col = 1U;

        return *this;
    }

    LocalLocus operator-(const LocalLocus& that) const {
        LocalLocus copy(*this);
        copy -= that;
        return std::move(copy);
    }

    void operator()(int ch) {
        if(ch == '\n') {
            col = 1U;
            line++;
        } else col++;
    }

    void operator()(std::string str) {
        for (const auto& ch : str) (*this)(ch);
    }

    LocalLocus& operator++() {
        (*this)('\n');
        return *this;
    }

    LocalLocus operator++(int) {
        LocalLocus copy(*this);
        ++(*this);
        return std::move(copy);
    }

    bool isUnknown() const {
        return col == UNKNOWN_LOC || line == UNKNOWN_LOC;
    }

    LocalLocus advance(int howmuch, measure what) const {
        if(this->isUnknown()) return *this;

        LocalLocus that(*this);
        switch(what) {
            case measure::col: {
                long long newCol = that.col;
                newCol += howmuch;
                that.col = newCol < 1 ? 1U : static_cast<unsigned>(newCol);
                break;
            }
            case measure::line: {
                long long newLine = that.line;
                newLine += howmuch;
                that.line = newLine < 1 ? 1U : static_cast<unsigned>(newLine);
                that.col = 1U;
                break;
            }
        }
        return std::move(that);
    }

    // FIXME: some ambiguity shit going on here, fix
    friend std::ostream& operator<<(std::ostream& ost, const LocalLocus& ll) {
        if (ll.line == LocalLocus::UNKNOWN_LOC) ost << "<unknown-line>";
        else ost << ll.line;

        ost << ":";

        if (ll.col == LocalLocus::UNKNOWN_LOC) ost << "<unknown-col>";
        else ost << ll.col;

        // this is generally fucked up
        return ost;
    }
};



namespace util {
template<>
struct json_traits<LocalLocus> {
    typedef std::unique_ptr<LocalLocus> optional_ptr_t;

    static Json::Value toJson(const LocalLocus& val) {
        Json::Value dict;
        dict["line"] = util::toJson(val.line);
        dict["col"] = util::toJson(val.col);
        return dict;
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        using borealis::util::json_object_builder;

        json_object_builder<LocalLocus, unsigned, unsigned> builder {
            "line", "col"
        };
        return optional_ptr_t {
            builder.build(json)
        };
    }
};
} // namespace util

struct Locus {
    static constexpr auto UNKNOWN_NAME = "";

    util::indexed_string filename;
    LocalLocus loc;

    Locus(): filename(UNKNOWN_NAME) {};
    Locus(const Locus& that) = default;
    Locus(Locus&& that) = default;
    explicit Locus(const LocalLocus& that): filename(UNKNOWN_NAME), loc(that) {};
    explicit Locus(LocalLocus&& that): filename(UNKNOWN_NAME), loc(std::move(that)) {};
    Locus(const clang::PresumedLoc& that):
        filename(that.isValid() ? that.getFilename() : UNKNOWN_NAME),
        loc(that.isValid() ? LocalLocus{ that.getLine(), that.getColumn() } : LocalLocus{}) {};
    Locus(const clang::SourceLocation& enc, const clang::SourceManager& mgr):
        Locus(mgr.getPresumedLoc(enc)) {};
    Locus(const llvm::DILocation& that): filename(that.getFilename()), loc(that.getLineNumber(), that.getColumnNumber()) {};
    Locus(const std::string& filename, const LocalLocus& loc): filename(filename), loc(loc) {};
    Locus(const std::string& filename, unsigned line, unsigned col): filename(filename), loc(line, col) {};
    Locus(std::string&& filename, LocalLocus&& loc): filename(std::move(filename)), loc(std::move(loc)) {};
    Locus(std::string&& filename, unsigned line, unsigned col): filename(std::move(filename)), loc(line, col) {};
    Locus(util::indexed_string filename, const LocalLocus& loc): filename(filename), loc(loc) {};
    Locus(util::indexed_string filename, unsigned line, unsigned col): filename(filename), loc(line, col) {};

    const Locus& operator=(const Locus& that) {
        this->filename = that.filename;
        this->loc  = that.loc;
        return *this;
    }

    const Locus& operator=(Locus&& that) {
        this->filename = std::move(that.filename);
        this->loc  = std::move(that.loc);
        return *this;
    }

    GENERATE_EQ(Locus, filename, loc)
    GENERATE_LESS(Locus, filename, loc)
    GENERATE_AUX_COMPARISONS(Locus)

    bool isUnknown() const {
        return filename.empty() || loc.isUnknown();
    }

    inline Locus advance(int howmuch, LocalLocus::measure what) const {
        return Locus{ this->filename, this->loc.advance(howmuch, what) };
    }

    inline Locus advanceLine(int howmuch) const {
        return advance(howmuch, LocalLocus::measure::line);
    }

    inline Locus advanceCol(int howmuch) const {
        return advance(howmuch, LocalLocus::measure::col);
    }

    explicit operator bool() {
        return not isUnknown();
    }

    bool operator!() {
        return isUnknown();
    }

    // FIXME: some ambiguity shit going on here, fix
    friend std::ostream& operator<<(std::ostream& ost, const Locus& ll) {
        // this is generally fucked up
        return (ost << ll.filename << ":" << ll.loc);
    }

};



namespace util {

template<>
struct json_traits<Locus> {
    typedef std::unique_ptr<Locus> optional_ptr_t;

    static Json::Value toJson(const Locus& val) {
        Json::Value dict;
        dict["filename"] = util::toJson(val.filename);
        dict["loc"] = util::toJson(val.loc);
        return dict;
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        using borealis::util::json_object_builder;

        json_object_builder<Locus, std::string, LocalLocus> builder {
            "filename", "loc"
        };
        return optional_ptr_t {
            builder.build(json)
        };
    }
};

template<>
struct xml_traits<Locus> {
    static XMLNodePtr toXml(XMLNodePtr p, const Locus& val) {
        return util::toXml(p, util::toString(val));
    }
};

} // namespace util

struct LocusRange {
    Locus lhv;
    Locus rhv;
};

} // namespace borealis

GENERATE_OUTLINE_HASH(borealis::LocusRange, lhv, rhv);
GENERATE_OUTLINE_JSON_TRAITS(borealis::LocusRange, lhv, rhv);

namespace std {
template<>
struct hash<borealis::LocalLocus> {
    size_t operator()(const borealis::LocalLocus &l) const {
        return borealis::util::hash::defaultHasher()(l.line, l.col);
    }
};

template<>
struct hash<borealis::Locus> {
    size_t operator()(const borealis::Locus &l) const {
        return borealis::util::hash::defaultHasher()(l.filename, l.loc);
    }
};
} // namespace std

#include "Util/generate_unmacros.h"

#endif /* LOCATIONS_H_ */
