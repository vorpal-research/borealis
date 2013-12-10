/*
 * production.cpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include <ostream>

#include "Anno/production.h"

namespace borealis {
namespace anno {

using std::boolalpha;
using std::make_shared;
using std::ostream;

productionVisitor::~productionVisitor() {};
void productionVisitor::defaultBehaviour() { unimplement(); };
void productionVisitor::onDoubleConstant(double) { defaultBehaviour(); }
void productionVisitor::onIntConstant(int) { defaultBehaviour(); }
void productionVisitor::onBoolConstant(bool) { defaultBehaviour(); }

void productionVisitor::onVariable(const std::string&) { defaultBehaviour(); }
void productionVisitor::onBuiltin(const std::string&) { defaultBehaviour(); }
void productionVisitor::onMask(const std::string&) { defaultBehaviour(); }
void productionVisitor::onList(const std::list<prod_t>&) { defaultBehaviour(); }

void productionVisitor::onBinary(bin_opcode, const prod_t&, const prod_t&) { defaultBehaviour(); }
void productionVisitor::onUnary(un_opcode, const prod_t&) { defaultBehaviour(); }

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
builtin::builtin(std::string&& vname) : vname_(std::move(vname)) {};
void builtin::accept(productionVisitor& pv) const {
    pv.onBuiltin(vname_);
};

productionList::productionList(const std::list<prod_t>& data): data_(data) {}
productionList::productionList(std::list<prod_t>&& data): data_(std::move(data)) {}
void productionList::accept(productionVisitor& pv) const {
    pv.onList(data_);
}

variable::variable(const std::string& vname) : vname_(vname) {}
variable::variable(std::string&& vname) : vname_(std::move(vname)) {};
void variable::accept(productionVisitor& pv) const {
    pv.onVariable(vname_);
};

mask::mask(const std::string& mask) : mask_(mask) {}
mask::mask(std::string&& mask) : mask_(std::move(mask)) {};
void mask::accept(productionVisitor& pv) const {
    pv.onMask(mask_);
};

binary::binary(bin_opcode code, const prod_t& op0, const prod_t& op1) :
        code_(code), op0_(op0), op1_(op1) {};
void binary::accept(productionVisitor& pv) const {
    pv.onBinary(code_, op0_, op1_);
};

unary::unary(un_opcode code, const prod_t& op) : code_(code), op_(op) {};
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
prod_t productionFactory::bind(const std::string& v) {
    if (v.size() > 0 && v[0] == '\\') return createBuiltin(v.substr(1, std::string::npos));
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
prod_t productionFactory::createVar(const std::string& v) {
    return make_shared<variable>(v);
}
prod_t productionFactory::createMask(const std::string& v) {
    return make_shared<mask>(v);
}
prod_t productionFactory::createBuiltin(const std::string& v) {
    return make_shared<builtin>(v);
}

namespace {
struct get_list_visitor: public virtual productionVisitor {
    prod_t arg;
    std::list<prod_t> res;

    void visit(const prod_t& arg) {
        this->arg = arg;
        this->arg->accept(*this);
    }

    void onList(const std::list<prod_t>& data) override {
        res = data;
    }
    void defaultBehaviour() override {
        res = std::list<prod_t>{arg};
    }
};
}

prod_t productionFactory::createList(const prod_t& op0, const prod_t& op1) {
    get_list_visitor asker{};
    asker.visit(op0);
    auto op0List = std::move(asker.res);
    asker.visit(op1);
    auto op1List = std::move(asker.res);

    op0List.insert(op0List.end(), op1List.begin(), op1List.end());
    return make_shared<productionList>(std::move(op0List));
}
prod_t productionFactory::createBinary(bin_opcode code, const prod_t& op0, const prod_t& op1) {
    return make_shared<binary>(code, std::move(op0), std::move(op1));
}
prod_t productionFactory::createUnary(un_opcode code, const prod_t& op) {
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
    ost_ << '\\' << name;
}
void printingVisitor::onList(const std::list<prod_t>& data) {
    ost_ << *borealis::util::head(data);
    for(const auto& prod: borealis::util::tail(data)) {
        ost_ << ", " << *prod;
    }
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

    // special handling:
    case op::OPCODE_CALL:
        op0->accept(*this);
        ost_ << "(";
        op1->accept(*this);
        ost_ << ")";
        return;
    case op::OPCODE_INDEX:
        op0->accept(*this);
        ost_ << "[";
        op1->accept(*this);
        ost_ << "]";
        return;

    // property access are special only to provide more sugar
    case op::OPCODE_PROPERTY:
        (*op0).accept(*this);
        ost_ << ".";
        (*op1).accept(*this);
        return;
    case op::OPCODE_INDIR_PROPERTY:
        (*op0).accept(*this);
        ost_ << "->";
        (*op1).accept(*this);
        return;
   // end of special handling


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

prod_t operator+(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_PLUS, op0, op1);
}

prod_t operator-(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_MINUS, op0, op1);
}

prod_t operator*(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_MULT, op0, op1);
}

prod_t operator/(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_DIV, op0, op1);
}

prod_t operator%(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_MOD, op0, op1);
}

prod_t operator==(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_EQ, op0, op1);
}

prod_t operator!=(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_NE, op0, op1);
}

prod_t operator>(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_GT, op0, op1);
}

prod_t operator<(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LT, op0, op1);
}

prod_t operator>=(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_GE, op0, op1);
}

prod_t operator<=(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LE, op0, op1);
}

prod_t operator&&(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LAND, op0, op1);
}

prod_t operator||(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LOR, op0, op1);
}

prod_t operator&(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_BAND, op0, op1);
}

prod_t operator|(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_BOR, op0, op1);
}

prod_t operator^(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_XOR, op0, op1);
}

prod_t operator<<(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_LSH, op0, op1);
}

prod_t operator>>(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_RSH, op0, op1);
}

prod_t call(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_CALL, op0, op1);
}

prod_t index(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_INDEX, op0, op1);
}

prod_t property_access(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_PROPERTY, op0, op1);
}

prod_t property_indirect_access(const prod_t& op0, const prod_t& op1) {
    return productionFactory::createBinary(bin_opcode::OPCODE_INDIR_PROPERTY, op0, op1);
}

prod_t deref(const prod_t& op0) {
    return productionFactory::createUnary(un_opcode::OPCODE_LOAD, op0);
}

prod_t operator!(const prod_t& op0) {
    return productionFactory::createUnary(un_opcode::OPCODE_NOT, op0);
}

prod_t operator-(const prod_t& op0) {
    return productionFactory::createUnary(un_opcode::OPCODE_NEG, op0);
}

prod_t operator~(const prod_t& op0) {
    return productionFactory::createUnary(un_opcode::OPCODE_BNOT, op0);
}

} /* namespace anno */
} /* namespace borealis */
