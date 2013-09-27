/*
 * production.h
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef PRODUCTION_H_
#define PRODUCTION_H_

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "Util/util.h"
// FIXME: move to borealis::anno from default namespace?

class productionVisitor;

class production {
protected:
    class notImplemented: public virtual std::exception {
    public:
        virtual const char* what() const noexcept override {
            return "production: method not implemented";
        }
    };
    inline void unimplement() const { throw notImplemented(); }
public:
    virtual void accept(productionVisitor&) const;
    virtual ~production();
};

typedef std::shared_ptr<production> prod_t;

enum class bin_opcode {
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
    OPCODE_RSH,
    OPCODE_CALL, // operator()
    OPCODE_INDEX, // operator[]
    OPCODE_PROPERTY, // a.b
    OPCODE_INDIR_PROPERTY // a->b
};

enum class un_opcode {
    OPCODE_LOAD,
    OPCODE_NEG,
    OPCODE_NOT,
    OPCODE_BNOT
};

enum class type {
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
        virtual const char* what() const noexcept override {
            return "productionVisitor: method not implemented";
        }
    };

    inline void unimplement() { throw notImplemented(); }
public:

    virtual ~productionVisitor();
    virtual void defaultBehaviour();

    virtual void onDoubleConstant(double);
    virtual void onIntConstant(int);
    virtual void onBoolConstant(bool);

    virtual void onVariable(const std::string&);
    virtual void onBuiltin(const std::string&);
    virtual void onMask(const std::string&);

    virtual void onList(const std::list<prod_t>& data);

    virtual void onBinary(
            bin_opcode op,
            const prod_t&,
            const prod_t&);
    virtual void onUnary(
            un_opcode op,
            const prod_t&);
};

class doubleConstant: public virtual production {
    long double value_;
public:
    doubleConstant(long double value);
    virtual void accept(productionVisitor& pv) const override;
};
class intConstant: public virtual production {
    long long value_;
public:
    intConstant(long long value);
    virtual void accept(productionVisitor& pv) const override;
};
class boolConstant: public virtual production {
    bool value_;
public:
    boolConstant(bool value);
    virtual void accept(productionVisitor& pv) const override;
};

class mask: public virtual production {
    std::string mask_;
public:
    explicit mask(const std::string& mask);
    explicit mask(std::string&& mask);
    virtual void accept(productionVisitor& pv) const override;
};

class builtin: public virtual production {
    std::string vname_;
public:
    explicit builtin(const std::string& vname);
    explicit builtin(std::string&& vname);
    virtual void accept(productionVisitor& pv) const override;
};

class productionList: public virtual production {
    std::list<prod_t> data_;
public:
    explicit productionList(const std::list<prod_t>& data);
    explicit productionList(std::list<prod_t>&& data);
    virtual void accept(productionVisitor& pv) const override;
};

class variable: public virtual production {
    std::string vname_;
public:
    explicit variable(const std::string& vname);
    explicit variable(std::string&& vname);
    virtual void accept(productionVisitor& pv) const override;
};

class binary: public virtual production {
    bin_opcode code_;
    prod_t op0_;
    prod_t op1_;
public:
    binary(bin_opcode code, const prod_t& op0, const prod_t& op1);
    virtual void accept(productionVisitor& pv) const override;
};

class unary: public virtual production {
    un_opcode code_;
    prod_t op_;
public:
    unary(un_opcode code, const prod_t& op);
    virtual void accept(productionVisitor& pv) const override;
};

class productionFactory {
public:
    static prod_t bind(double v);
    static prod_t bind(long long v);
    static prod_t bind(bool v);
    // solving ambiguity that results into bind("") being bind(true) instead of bind(string(""))
    static prod_t bind(const char* v);
    static prod_t bind(const std::string& v);

    static prod_t createDouble(double v);
    static prod_t createInt(int v);
    static prod_t createBool(bool v);
    static prod_t createVar(const std::string& v);
    static prod_t createBuiltin(const std::string& v);
    static prod_t createMask(const std::string& v);
    static prod_t createList(const prod_t& op0, const prod_t& op1);
    static prod_t createBinary(bin_opcode code, const prod_t& op0, const prod_t& op1);
    static prod_t createUnary(un_opcode code, const prod_t& op);
};

class printingVisitor: public virtual productionVisitor {
    std::ostream& ost_;

    virtual void onDoubleConstant(double v) override;
    virtual void onIntConstant(int v) override;
    virtual void onBoolConstant(bool v) override;
    virtual void onVariable(const std::string& name) override;
    virtual void onBuiltin(const std::string& name) override;
    virtual void onList(const std::list<prod_t>& data) override;
    virtual void onMask(const std::string& mask) override;
    virtual void onBinary(bin_opcode opc, const prod_t& op0, const prod_t& op1) override;
    virtual void onUnary(un_opcode opc, const prod_t& op0) override;
public:
    printingVisitor(std::ostream& ost);
};

std::ostream& operator<<(std::ostream& ost, const production& prod);

prod_t index(const prod_t&, const prod_t&);
prod_t call(const prod_t&, const prod_t&);
prod_t property_access(const prod_t&, const prod_t&);
prod_t property_indirect_access(const prod_t&, const prod_t&);
prod_t operator+ (const prod_t&, const prod_t&);
prod_t operator- (const prod_t&, const prod_t&);
prod_t operator* (const prod_t&, const prod_t&);
prod_t operator/ (const prod_t&, const prod_t&);
prod_t operator% (const prod_t&, const prod_t&);
prod_t operator==(const prod_t&, const prod_t&);
prod_t operator!=(const prod_t&, const prod_t&);
prod_t operator> (const prod_t&, const prod_t&);
prod_t operator< (const prod_t&, const prod_t&);
prod_t operator>=(const prod_t&, const prod_t&);
prod_t operator<=(const prod_t&, const prod_t&);
prod_t operator&&(const prod_t&, const prod_t&);
prod_t operator||(const prod_t&, const prod_t&);
prod_t operator& (const prod_t&, const prod_t&);
prod_t operator| (const prod_t&, const prod_t&);
prod_t operator^ (const prod_t&, const prod_t&);
prod_t operator<<(const prod_t&, const prod_t&);
prod_t operator>>(const prod_t&, const prod_t&);
prod_t deref     (const prod_t&);
prod_t operator! (const prod_t&);
prod_t operator- (const prod_t&);
prod_t operator~ (const prod_t&);

#endif // PRODUCTION_H_
