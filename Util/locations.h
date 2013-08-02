/*
 * Locations.h
 *
 *  Created on: Oct 9, 2012
 *      Author: belyaev
 */

#ifndef LOCATIONS_H_
#define LOCATIONS_H_

#include <clang/Basic/SourceManager.h>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Instruction.h>

#include <iostream>
#include <utility>

#include "Util/json_traits.hpp"
#include "Util/xml_traits.hpp"

namespace borealis {

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

    bool operator==(const LocalLocus& that) const {
        return (line == that.line) && (col == that.col);
    }

    bool operator<(const LocalLocus& that) const {
        return (line < that.line) || (line == that.line && col < that.col);
    }

    bool operator>(const LocalLocus& that) const {
        return !(*this == that || *this < that);
    }

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
                long long newCol = that.col + howmuch;
                that.col = newCol < 1 ? 1U : static_cast<unsigned>(newCol);
                break;
            }
            case measure::line: {
                long long newLine = that.line + howmuch;
                that.line = newLine < 1 ? 1U : static_cast<unsigned>(newLine);
                that.col = 1U;
                break;
            }
        }
        return std::move(that);
    }
};

template<class Streamer>
Streamer& operator<<(Streamer& ost, const LocalLocus& ll) {
    if (ll.line == LocalLocus::UNKNOWN_LOC) ost << "<unknown-line>";
    else ost << ll.line;

    ost << ":";

    if (ll.col == LocalLocus::UNKNOWN_LOC) ost << "<unknown-col>";
    else ost << ll.col;

    // this is generally fucked up
    return static_cast<Streamer&>(ost);
}

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
} //namespace util

struct Locus {
    static constexpr auto UNKNOWN_NAME = "";

    std::string filename;
    LocalLocus loc;

    Locus(): filename(UNKNOWN_NAME) {};
    Locus(const Locus& that) = default;
    Locus(Locus&& that) = default;
    explicit Locus(const LocalLocus& that): filename(UNKNOWN_NAME), loc(that) {};
    explicit Locus(LocalLocus&& that): filename(UNKNOWN_NAME), loc(std::move(that)) {};
    Locus(const clang::PresumedLoc& that): filename(that.getFilename()), loc(that.getLine(), that.getColumn()) {};
    Locus(const llvm::DILocation& that): filename(that.getFilename()), loc(that.getLineNumber(), that.getColumnNumber()) {};
    Locus(const std::string& filename, const LocalLocus& loc): filename(filename), loc(loc) {};
    Locus(const std::string& filename, unsigned line, unsigned col): filename(filename), loc(line, col) {};
    Locus(std::string&& filename, LocalLocus&& loc): filename(std::move(filename)), loc(std::move(loc)) {};
    Locus(std::string&& filename, unsigned line, unsigned col): filename(std::move(filename)), loc(line, col) {};

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

    bool operator==(const Locus& that) const {
        return (filename == that.filename) && (loc == that.loc);
    }

    bool operator<(const Locus& that) const {
        return (filename < that.filename) || ((filename == that.filename) && (loc < that.loc));
    }

    bool operator>(const Locus& that) const {
        return !(*this == that || *this < that);
    }

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

    operator void*() {
        if (isUnknown()) return nullptr;
        else return this;
    }

    void* operator!() {
        if (isUnknown()) return this;
        else return nullptr;
    }
};

template<class Streamer>
Streamer& operator<<(Streamer& ost, const Locus& ll) {
    // this is generally fucked up
    return static_cast<Streamer&>(ost << ll.filename << ":" << ll.loc);
}

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
} //namespace util

struct LocusRange {
    Locus lhv;
    Locus rhv;
};

} // namespace borealis

namespace std {
template<>
class hash<borealis::LocalLocus> {
public:
    size_t operator()(const borealis::LocalLocus &l) const {
        size_t h1 = std::hash<unsigned>()(l.line);
        size_t h2 = std::hash<unsigned>()(l.col);
        return h2 + (h1 << 16);
    }
};

template<>
class hash<borealis::Locus> {
public:
    size_t operator()(const borealis::Locus &l) const {
        size_t h1 = std::hash<std::string>()(l.filename);
        size_t h2 = std::hash<borealis::LocalLocus>()(l.loc);
        return h2 + (h1 << 16);
    }
};
}

#endif /* LOCATIONS_H_ */
