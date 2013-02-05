/*
 * anno.h
 *
 *  Created on: Apr 12, 2012
 *      Author: belyaev
 */

#ifndef ANNO_H_
#define ANNO_H_

#include <iostream>

#include "command.hpp"

namespace borealis{
namespace anno{

struct expanded_location {
	const char* file;
	unsigned line;
	unsigned column;
};

class location{
	std::unique_ptr<expanded_location> inner_;

	void swap(location& that) {
		inner_.swap(that.inner_);
	}

public:

	location(const expanded_location& loc): inner_(new expanded_location(loc)){}
	location(const location& loc): inner_(new expanded_location(*loc.inner_)){}
	location(): inner_(new expanded_location()){}

	const location& operator=(const location& that) {
		location temp(that);
		swap(temp);
		return *this;
	}

	const expanded_location& inner() const{
		return *inner_;
	}

	void operator()(const int v) {
		if(v == '\r') return;

		if(v == '\n') {
			inner_->line++;
			inner_->column = 0;
		} else {
			inner_->column++;
		}
	}

	template<class Char>
	friend std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& ost, const location& loc);
};

template<class Char>
std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& ost, const location& loc){
	auto file = loc.inner_->file ? loc.inner_->file : "unknown";
	ost << "file: \"" << file <<
			"\", line: " << loc.inner_->line <<
			", column: " << loc.inner_->column;
	return ost;
}

class empty_visitor: public productionVisitor {
	virtual void onDoubleConstant(double){}
	virtual void onIntConstant(int){}
	virtual void onBoolConstant(bool){}

	virtual void onVariable(const std::string&){}
	virtual void onBuiltin(const std::string&){}
	virtual void onMask(const std::string&){}

	virtual void onBinary(bin_opcode, const std::shared_ptr<production>& op0, const std::shared_ptr<production>& op1){
		(*op0).accept(*this);
		(*op1).accept(*this);
	}
	virtual void onUnary(un_opcode, const std::shared_ptr<production>& op0){
		(*op0).accept(*this);
	}

public:
	virtual ~empty_visitor(){};
};

class collector{
public:
	static std::list<std::string> collect_vars(const command& com) {
		class var_visitor: public empty_visitor {
			std::list<std::string> vars_;
			virtual void onVariable(const std::string& name){
				vars_.push_back(name);
			}
		public:
			const std::list<std::string>& vars() { return vars_; }
		};

		var_visitor v;
		for (const prod_t& pr : com.args_) {
			pr->accept(v);
		}

		return v.vars();
	}

	static std::list<std::string> collect_builtins(const command& com) {
		class builtin_visitor: public empty_visitor {
			std::list<std::string> bs_;
			virtual void onBuiltin(const std::string& name){
				bs_.push_back(name);
			}
		public:
			const std::list<std::string>& builtins() { return bs_; }
		};

		builtin_visitor v;
        for (const prod_t& pr : com.args_) {
            pr->accept(v);
        }

		return v.builtins();
	}
};

std::vector<command> parse(const std::string& v);

} // namespace anno
} // namespace borealis

#endif /* ANNO_H_ */
