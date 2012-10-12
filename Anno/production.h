#ifndef PRODUCTION_H_
#define PRODUCTION_H_

#include <memory>
using std::shared_ptr;

#include <string>
using std::string;

#include <exception>

class productionVisitor;

class production {
protected:
	class notImplemented: public virtual std::exception {
	public:
		virtual const char* what() const noexcept {
			return "production: method not implemented";
		}
	};
	inline void unimplement() const { throw notImplemented(); }
public:
	virtual void accept(productionVisitor&) const;
	virtual ~production();

};

typedef shared_ptr<production> prod_t;

enum bin_opcode {
	OPCODE_PLUS,
	OPCODE_MINUS,
	OPCODE_MULT,
	OPCODE_DIV,
	OPCODE_MOD,
	OPCODE_EQ,
	OPCODE_NE,
	OPCODE_GT,
	OPCODE_LT,
	OPCODE_GE,
	OPCODE_LE,
	OPCODE_LAND,
	OPCODE_LOR,
	OPCODE_BAND,
	OPCODE_BOR,
	OPCODE_XOR,
	OPCODE_LSH,
	OPCODE_RSH
};

enum un_opcode {
	OPCODE_NEG,
	OPCODE_NOT,
	OPCODE_BNOT
};

enum type {
	INTEGER,
	FLOATING,
	BOOLEAN,
	INVALID
};

class productionFactory;

class productionVisitor {
protected:
	class notImplemented: public std::exception {
	public:
		virtual const char* what() const noexcept {
			return "productionVisitor: method not implemented";
		}
	};

	inline void unimplement() { throw notImplemented(); }

public:
	virtual ~productionVisitor();
	virtual void onDoubleConstant(double);
	virtual void onIntConstant(int);
	virtual void onBoolConstant(bool);

	virtual void onVariable(const std::string&);
	virtual void onBuiltin(const std::string&);
	virtual void onMask(const std::string&);

	virtual void onBinary(bin_opcode op, \
			const prod_t&, \
			const prod_t&);
	virtual void onUnary(un_opcode op, \
			const prod_t&);
};


class doubleConstant: public virtual production {
	long double value_;
public:
	doubleConstant(long double value);
	virtual void accept(productionVisitor& pv) const;
};
class intConstant: public virtual production {
	long long value_;
public:
	intConstant(long long value);
	virtual void accept(productionVisitor& pv) const;

};
class boolConstant: public virtual production {
	bool value_;
public:
	boolConstant(bool value);
	virtual void accept(productionVisitor& pv) const;

};

class mask: public virtual production {
    string mask_;
public:
    explicit mask(const std::string& mask);
    explicit mask(std::string&& mask);
    virtual void accept(productionVisitor& pv) const;
};

class builtin: public virtual production {
	string vname_;
public:
	explicit builtin(const std::string& vname);	;
	explicit builtin(std::string&& vname);
	virtual void accept(productionVisitor& pv) const;
};

class variable: public virtual production {
	string vname_;
public:
	explicit variable(const std::string& vname);
	explicit variable(std::string&& vname);
	virtual void accept(productionVisitor& pv) const;
};

class binary: public virtual production {
	bin_opcode code_;
	prod_t op0_;
	prod_t op1_;
public:
	binary(bin_opcode code, prod_t&& op0, prod_t&& op1);
	virtual void accept(productionVisitor& pv) const;
};

class unary: public virtual production {
	un_opcode code_;
	prod_t op_;
public:
	unary(un_opcode code, prod_t&& op);
	virtual void accept(productionVisitor& pv) const;
};

class productionFactory {
public:

	static prod_t bind(double v);
	static prod_t bind(long long v);
	static prod_t bind(bool v);
	// solving ambiguity that results into bind("") being bind(true) instead of bind(string(""))
	static prod_t bind(const char* v);
	static prod_t bind(string v);

	static prod_t createDouble(double v);
	static prod_t createInt(int v);
	static prod_t createBool(bool v);
	static prod_t createVar(string v);
	static prod_t createBuiltin(string v);
	static prod_t createMask(string v);
	static prod_t createBinary(bin_opcode code, prod_t&& op0, prod_t&& op1);
	static prod_t createUnary(un_opcode code, prod_t&& op);
};


class printingVisitor: public virtual productionVisitor {
	std::ostream& ost_;

	virtual void onDoubleConstant(double v);
	virtual void onIntConstant(int v);
	virtual void onBoolConstant(bool v);
	virtual void onVariable(const std::string& name);
	virtual void onBuiltin(const std::string& name);
	virtual void onMask(const std::string& mask);
	virtual void onBinary(bin_opcode opc, const prod_t& op0, const prod_t& op1);
	virtual void onUnary(un_opcode opc, const prod_t& op0);
public:
	printingVisitor(std::ostream& ost);
};

std::ostream& operator<<( std::ostream& ost, const production& prod);



prod_t operator+(prod_t&& op0, prod_t&& op1);
prod_t operator-(prod_t&& op0, prod_t&& op1);
prod_t operator*(prod_t&& op0, prod_t&& op1);
prod_t operator/(prod_t&& op0, prod_t&& op1);
prod_t operator%(prod_t&& op0, prod_t&& op1);
prod_t operator==(prod_t&& op0, prod_t&& op1) ;
prod_t operator!=(prod_t&& op0, prod_t&& op1) ;
prod_t operator>(prod_t&& op0, prod_t&& op1);
prod_t operator<(prod_t&& op0, prod_t&& op1);
prod_t operator>=(prod_t&& op0, prod_t&& op1);
prod_t operator<=(prod_t&& op0, prod_t&& op1);
prod_t operator&&(prod_t&& op0, prod_t&& op1);
prod_t operator||(prod_t&& op0, prod_t&& op1);
prod_t operator&(prod_t&& op0, prod_t&& op1);
prod_t operator|(prod_t&& op0, prod_t&& op1);
prod_t operator^(prod_t&& op0, prod_t&& op1);
prod_t operator<<(prod_t&& op0, prod_t&& op1);
prod_t operator>>(prod_t&& op0, prod_t&& op1);
prod_t operator!(prod_t&& op0);
prod_t operator-(prod_t&& op0);
prod_t operator~(prod_t&& op0);

#endif // PRODUCTION_H_

