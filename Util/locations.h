/*
 * Locations.h
 *
 *  Created on: Oct 9, 2012
 *      Author: belyaev
 */

#ifndef LOCATIONS_H_
#define LOCATIONS_H_

#include <utility>

#include <llvm/Instruction.h>
#include <clang/Basic/SourceManager.h>

namespace borealis {

using std::move;

struct LocalLocus {
	static constexpr unsigned UNKNOWN_LOC = ~0U;

	unsigned line;
	unsigned col;

	LocalLocus(): line(UNKNOWN_LOC), col(UNKNOWN_LOC) {};
	LocalLocus(const LocalLocus& that): line(that.line), col(that.col) {};
	LocalLocus(unsigned line, unsigned col): line(line), col(col) {};
	LocalLocus(const llvm::DebugLoc& dbg): line(dbg.getLine()), col(dbg.getCol()) {};
	LocalLocus(const clang::SourceLocation& src, const clang::SourceManager& mng):
		line(mng.getExpansionLineNumber(src)), col(mng.getExpansionColumnNumber(src)) {}


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

	const LocalLocus& operator+=(const LocalLocus& that) {
		this->line += that.line;
		this->col  += that.col;
		return *this;
	}

	LocalLocus operator+(const LocalLocus& that) const {
		LocalLocus copy(*this);
		copy += that;
		return move(copy);
	}

	const LocalLocus& operator-=(const LocalLocus& that) {
		this->line -= that.line;
		this->col  -= that.col;
		return *this;
	}

	LocalLocus operator-(const LocalLocus& that) const {
		LocalLocus copy(*this);
		copy -= that;
		return move(copy);
	}

	void operator()(int ch) {
		if(ch == '\n') {
			col = 0;
			line++;
		} else col++;
	}

	void operator()(std::string str) {
		for(auto it = str.begin(); it != str.end(); ++it) (*this)(*it);
	}

	LocalLocus& operator++() {
		(*this)('\n');
		return *this;
	}

	LocalLocus operator++(int) {
		LocalLocus copy(*this);
		++(*this);
		return move(copy);
	}
};

template<class Streamer>
Streamer& operator<<(Streamer& ost, const LocalLocus& ll) {
	// 1:1
	return ost << ll.line << ":" << ll.col;
}

struct Locus {
	static constexpr auto UNKNOWN_NAME = "";

	std::string filename;
	LocalLocus loc;

	Locus(): filename(UNKNOWN_NAME), loc() {};
	Locus(const Locus& that): filename(that.filename), loc(that.loc) {};
	Locus(Locus&& that): filename(move(that.filename)), loc(move(that.loc)) {};
	explicit Locus(const LocalLocus& that): filename(UNKNOWN_NAME), loc(that) {};
	explicit Locus(LocalLocus&& that): filename(UNKNOWN_NAME), loc(move(that)) {};
	Locus(const clang::PresumedLoc& that): filename(that.getFilename()), loc(that.getLine(), that.getColumn()) {};
	Locus(const std::string& filename, const LocalLocus& loc): filename(filename), loc(loc) {};
	Locus(const std::string& filename, unsigned line, unsigned col): filename(filename), loc(line,col) {};
	Locus(std::string&& filename, LocalLocus&& loc): filename(move(filename)), loc(loc) {};
	Locus(std::string&& filename, unsigned line, unsigned col): filename(move(filename)), loc(line,col) {};

	const Locus& operator=(const Locus& that) {
		this->filename = that.filename;
		this->loc  = that.loc;
		return *this;
	}

	const Locus& operator=(Locus&& that) {
		this->filename = move(that.filename);
		this->loc  = move(that.loc);
		return *this;
	}

	bool operator==(const Locus& that) const {
		return (filename == that.filename) && (loc == that.loc);
	}

	bool operator<(const Locus& that) const {
		return (filename == that.filename) && (loc < that.loc);
	}

};

template<class Streamer>
Streamer& operator<<(Streamer& ost, const Locus& ll) {
	// file.txt:1:2
	return ost << ll.filename << ":" << ll.loc;
}

struct LocusRange {
    Locus lhv;
	Locus rhv;
};

} // namespace borealis

namespace std {
template<>
class hash<borealis::LocalLocus> {
public:
    size_t operator()(const borealis::LocalLocus &l) const
    {
        size_t h1 = std::hash<unsigned>()(l.line);
        size_t h2 = std::hash<unsigned>()(l.col);
        return h2 + (h1 << 16);
    }
};

template<>
class hash<borealis::Locus> {
public:
    size_t operator()(const borealis::Locus &l) const
    {
        size_t h1 = std::hash<borealis::LocalLocus>()(l.loc);
        size_t h2 = std::hash<std::string>()(l.filename);
        return h1 + (h2 << 16);
    }
};
}

#endif /* LOCATIONS_H_ */
