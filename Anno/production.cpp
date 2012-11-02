/*
 * production.cpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include "production.h"

#include <ostream>
using std::boolalpha;
using std::make_shared;
using std::ostream;

productionVisitor::~productionVisitor(){};
void productionVisitor::onDoubleConstant(double){ unimplement(); }
void productionVisitor::onIntConstant(int){ unimplement(); }
void productionVisitor::onBoolConstant(bool){ unimplement(); }

void productionVisitor::onVariable(const std::string&){ unimplement(); }
void productionVisitor::onBuiltin(const std::string&){ unimplement(); }
void productionVisitor::onMask(const std::string&){ unimplement(); }

void productionVisitor::onBinary(bin_opcode, const prod_t&, const prod_t&){ unimplement(); }
void productionVisitor::onUnary(un_opcode, const prod_t&){ unimplement(); }

void production::accept(productionVisitor&) const { unimplement(); }
production::~production() {};

doubleConstant::doubleConstant(long double value) : value_(value) {}
void doubleConstant::accept(productionVisitor& pv) const {
    pv.onDoubleConstant(value_);
}

intConstant::intConstant(long long value) : value_(value) {}
void intConstant::accept(productionVisitor& pv) const {
    pv.onIntConstant(value_);
}

boolConstant::boolConstant(bool value) : value_(value) {}
void boolConstant::accept(productionVisitor& pv) const {
    pv.onBoolConstant(value_);
}

builtin::builtin(const std::string& vname) : vname_(vname) {}
builtin::builtin(std::string&& vname): vname_(vname) {};
void builtin::accept(productionVisitor& pv) const {
    pv.onBuiltin(vname_);
};

variable::variable(const std::string& vname) : vname_(vname) {}
variable::variable(std::string&& vname): vname_(vname) {};
void variable::accept(productionVisitor& pv) const {
    pv.onVariable(vname_);
};

mask::mask(const std::string& mask) : mask_(mask) {}
mask::mask(std::string&& mask): mask_(mask) {};
void mask::accept(productionVisitor& pv) const {
    pv.onMask(mask_);
};

binary::binary(bin_opcode code, prod_t&& op0, prod_t&& op1): code_(code), op0_(std::move(op0)), op1_(std::move(op1)){};
void binary::accept(productionVisitor& pv) const {
    pv.onBinary(code_, op0_, op1_);
};

unary::unary(un_opcode code, prod_t&& op): code_(code), op_(std::move(op)){};
void unary::accept(productionVisitor& pv) const {
    pv.onUnary(code_, op_);
};



prod_t productionFactory::bind(double v) {
    return createDouble(v);
}
prod_t productionFactory::bind(long long v) {
    return createInt(v);
}
prod_t productionFactory::bind(bool v) {
    return createBool(v);
}
// solving ambiguity that results into bind("") being bind(true) instead of bind(string(""))
prod_t productionFactory::bind(const char* v) {
    return createVar(v);
}
prod_t productionFactory::bind(std::string v) {
    if(v.size() > 0 && v[0] == '\\') return createBuiltin(v.substr(1, std::string::npos));
    else return createVar(v);
}



prod_t productionFactory::createDouble(double v) {
    return make_shared<doubleConstant>(v);
}
prod_t productionFactory::createInt(int v) {
    return make_shared<intConstant>(v);
}
prod_t productionFactory::createBool(bool v) {
    return make_shared<boolConstant>(v);
}
prod_t productionFactory::createVar(std::string v) {
    return make_shared<variable>(v);
}
prod_t productionFactory::createMask(std::string v) {
    return make_shared<mask>(v);
}
prod_t productionFactory::createBuiltin(std::string v) {
    return make_shared<builtin>(v);
}
prod_t productionFactory::createBinary(bin_opcode code, prod_t&& op0, prod_t&& op1) {
    return make_shared<binary>(code, std::move(op0), std::move(op1));
}
prod_t productionFactory::createUnary(un_opcode code, prod_t&& op) {
    return make_shared<unary>(code, std::move(op));
}



void printingVisitor::onDoubleConstant(double v) {
    ost_ << v;
}
void printingVisitor::onIntConstant(int v) {
    ost_ << v;
}
void printingVisitor::onBoolConstant(bool v) {
    ost_ << '#' << boolalpha << v;
}
void printingVisitor::onVariable(const std::string& name) {
    ost_ << name;
}
void printingVisitor::onBuiltin(const std::string& name) {
    ost_ << '\\' << '(' << name << ')';
}
void printingVisitor::onMask(const std::string& mask) {
    ost_ << mask;
}

void printingVisitor::onBinary(bin_opcode opc, const prod_t& op0, const prod_t& op1) {
    std::string ops;
    typedef bin_opcode op;
    switch(opc) {
    case op::OPCODE_PLUS:
        ops = "+"; break;
    case op::OPCODE_MINUS:
        ops = "-"; break;
    case op::OPCODE_DIV:
        ops = "/"; break;
    case op::OPCODE_MULT:
        ops = "*"; break;
    case op::OPCODE_MOD:
        ops = "%"; break;
    case op::OPCODE_LAND:
        ops = "&&"; break;
    case op::OPCODE_LOR:
        ops = "||"; break;
    case op::OPCODE_EQ:
        ops = "=="; break;
    case op::OPCODE_NE:
        ops = "!="; break;
    case op::OPCODE_GT:
        ops = ">"; break;
    case op::OPCODE_LT:
        ops = "<"; break;
    case op::OPCODE_GE:
        ops = ">="; break;
    case op::OPCODE_LE:
        ops = "<="; break;
    case op::OPCODE_BAND:
        ops = "&"; break;
    case op::OPCODE_BOR:
        ops = "|"; break;
    case op::OPCODE_XOR:
        ops = "^"; break;
    case op::OPCODE_LSH:
        ops = "<<"; break;
    case op::OPCODE_RSH:
        ops = ">>"; break;
    default:
        ops = "???"; break;
    }

    ost_ << "(";
    (*op0).accept(*this);
    ost_ << " " << ops << " ";
    (*op1).accept(*this);
    ost_ << ")";
}

void printingVisitor::onUnary(un_opcode opc, const prod_t& op0) {
    typedef un_opcode op;
    std::string ops = (opc == op::OPCODE_NEG) ? "-" :
                      (opc == op::OPCODE_NOT) ? "!" :
                      (opc == op::OPCODE_BNOT)? "~" :
                      "???";
    ost_ << ops;
    (*op0).accept(*this);
}

printingVisitor::printingVisitor(std::ostream& ost) : ost_(ost) {}

std::ostream& operator<<( std::ostream& ost, const production& prod) {
    printingVisitor pf(ost);
    prod.accept(pf);
    return ost;
}

prod_t operator+(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_PLUS, std::move(op0), std::move(op1));
}

prod_t operator-(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_MINUS, std::move(op0), std::move(op1));
}

prod_t operator*(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_MULT, std::move(op0), std::move(op1));
}

prod_t operator/(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_DIV, std::move(op0), std::move(op1));
}

prod_t operator%(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_MOD, std::move(op0), std::move(op1));
}

prod_t operator==(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_EQ, std::move(op0), std::move(op1));
}

prod_t operator!=(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_NE, std::move(op0), std::move(op1));
}

prod_t operator>(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_GT, std::move(op0), std::move(op1));
}

prod_t operator<(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LT, std::move(op0), std::move(op1));
}

prod_t operator>=(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_GE, std::move(op0), std::move(op1));
}

prod_t operator<=(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LE, std::move(op0), std::move(op1));
}

prod_t operator&&(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LAND, std::move(op0), std::move(op1));
}

prod_t operator||(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LOR, std::move(op0), std::move(op1));
}

prod_t operator&(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_BAND, std::move(op0), std::move(op1));
}

prod_t operator|(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_BOR, std::move(op0), std::move(op1));
}

prod_t operator^(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_XOR, std::move(op0), std::move(op1));
}

prod_t operator<<(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LSH, std::move(op0), std::move(op1));
}

prod_t operator>>(prod_t&& op0, prod_t&& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_RSH, std::move(op0), std::move(op1));
}

prod_t operator!(prod_t&& op0) {
    return productionFactory::createUnary(un_opcode::OPCODE_NOT, std::move(op0));
}

prod_t operator-(prod_t&& op0) {
    return productionFactory::createUnary(un_opcode::OPCODE_NEG, std::move(op0));
}

prod_t operator~(prod_t&& op0) {
    return productionFactory::createUnary(un_opcode::OPCODE_BNOT, std::move(op0));
}
