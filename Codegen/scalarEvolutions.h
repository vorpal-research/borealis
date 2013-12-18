/*
 * scalarEvolutions.h
 *
 *  Created on: Dec 13, 2013
 *      Author: belyaev
 */

#ifndef SCALAREVOLUTIONS_H_
#define SCALAREVOLUTIONS_H_

#include <llvm/Analysis/ScalarEvolutionExpressions.h>

namespace llvm {

typedef DenseMap<const Value*, Value*> ValueToValueMap;

/// The SCEVParameterRewriter takes a scalar evolution expression and updates
/// the SCEVUnknown components following the Map (Value -> Value).
struct SCEVParameterRewriter : public SCEVVisitor<SCEVParameterRewriter, const SCEV*> {
public:
    static const SCEV *rewrite(const SCEV *Scev, ScalarEvolution &SE,
        ValueToValueMap &Map) {
        SCEVParameterRewriter Rewriter(SE, Map);
        return Rewriter.visit(Scev);
    }

    SCEVParameterRewriter(ScalarEvolution &S, ValueToValueMap &M)
    :
        SE(S), Map(M) {
    }

    const SCEV *visitConstant(const SCEVConstant *Constant) {
        return Constant;
    }

    const SCEV *visitTruncateExpr(const SCEVTruncateExpr *Expr) {
        const SCEV *Operand = visit(Expr->getOperand());
        return SE.getTruncateExpr(Operand, Expr->getType());
    }

    const SCEV *visitZeroExtendExpr(const SCEVZeroExtendExpr *Expr) {
        const SCEV *Operand = visit(Expr->getOperand());
        return SE.getZeroExtendExpr(Operand, Expr->getType());
    }

    const SCEV *visitSignExtendExpr(const SCEVSignExtendExpr *Expr) {
        const SCEV *Operand = visit(Expr->getOperand());
        return SE.getSignExtendExpr(Operand, Expr->getType());
    }

    const SCEV *visitAddExpr(const SCEVAddExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getAddExpr(Operands);
    }

    const SCEV *visitMulExpr(const SCEVMulExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getMulExpr(Operands);
    }

    const SCEV *visitUDivExpr(const SCEVUDivExpr *Expr) {
        return SE.getUDivExpr(visit(Expr->getLHS()), visit(Expr->getRHS()));
    }

    const SCEV *visitAddRecExpr(const SCEVAddRecExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getAddRecExpr(Operands, Expr->getLoop(),
            Expr->getNoWrapFlags());
    }

    const SCEV *visitSMaxExpr(const SCEVSMaxExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getSMaxExpr(Operands);
    }

    const SCEV *visitUMaxExpr(const SCEVUMaxExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getUMaxExpr(Operands);
    }

    const SCEV *visitUnknown(const SCEVUnknown *Expr) {
        Value *V = Expr->getValue();
        if (Map.count(V))
            return SE.getUnknown(Map[V]);
        return Expr;
    }

    const SCEV *visitCouldNotCompute(const SCEVCouldNotCompute *Expr) {
        return Expr;
    }

private:
    ScalarEvolution &SE;
    ValueToValueMap &Map;
};

typedef DenseMap<const Loop*, const SCEV*> LoopToScevMapT;

/// The SCEVApplyRewriter takes a scalar evolution expression and applies
/// the Map (Loop -> SCEV) to all AddRecExprs.
struct SCEVApplyRewriter
: public SCEVVisitor<SCEVApplyRewriter, const SCEV*> {
public:
    static const SCEV *rewrite(const SCEV *Scev, LoopToScevMapT &Map,
        ScalarEvolution &SE) {
        SCEVApplyRewriter Rewriter(SE, Map);
        return Rewriter.visit(Scev);
    }

    SCEVApplyRewriter(ScalarEvolution &S, LoopToScevMapT &M)
    :
        SE(S), Map(M) {
    }

    const SCEV *visitConstant(const SCEVConstant *Constant) {
        return Constant;
    }

    const SCEV *visitTruncateExpr(const SCEVTruncateExpr *Expr) {
        const SCEV *Operand = visit(Expr->getOperand());
        return SE.getTruncateExpr(Operand, Expr->getType());
    }

    const SCEV *visitZeroExtendExpr(const SCEVZeroExtendExpr *Expr) {
        const SCEV *Operand = visit(Expr->getOperand());
        return SE.getZeroExtendExpr(Operand, Expr->getType());
    }

    const SCEV *visitSignExtendExpr(const SCEVSignExtendExpr *Expr) {
        const SCEV *Operand = visit(Expr->getOperand());
        return SE.getSignExtendExpr(Operand, Expr->getType());
    }

    const SCEV *visitAddExpr(const SCEVAddExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getAddExpr(Operands);
    }

    const SCEV *visitMulExpr(const SCEVMulExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getMulExpr(Operands);
    }

    const SCEV *visitUDivExpr(const SCEVUDivExpr *Expr) {
        return SE.getUDivExpr(visit(Expr->getLHS()), visit(Expr->getRHS()));
    }

    const SCEV *visitAddRecExpr(const SCEVAddRecExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));

        const Loop *L = Expr->getLoop();
        const SCEV *Res = SE.getAddRecExpr(Operands, L, Expr->getNoWrapFlags());

        if (0 == Map.count(L))
            return Res;

        const SCEVAddRecExpr *Rec = (const SCEVAddRecExpr *) Res;
        return Rec->evaluateAtIteration(Map[L], SE);
    }

    const SCEV *visitSMaxExpr(const SCEVSMaxExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getSMaxExpr(Operands);
    }

    const SCEV *visitUMaxExpr(const SCEVUMaxExpr *Expr) {
        SmallVector<const SCEV *, 2> Operands;
        for (int i = 0, e = Expr->getNumOperands(); i < e; ++i)
            Operands.push_back(visit(Expr->getOperand(i)));
        return SE.getUMaxExpr(Operands);
    }

    const SCEV *visitUnknown(const SCEVUnknown *Expr) {
        return Expr;
    }

    const SCEV *visitCouldNotCompute(const SCEVCouldNotCompute *Expr) {
        return Expr;
    }

private:
    ScalarEvolution &SE;
    LoopToScevMapT &Map;
};

/// Applies the Map (Loop -> SCEV) to the given Scev.
static inline const SCEV *apply(const SCEV *Scev, LoopToScevMapT &Map,
    ScalarEvolution &SE) {
    return SCEVApplyRewriter::rewrite(Scev, Map, SE);
}

} // namespace llvm

#endif /* SCALAREVOLUTIONS_H_ */
