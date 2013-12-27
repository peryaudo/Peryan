#ifndef PERYAN_AST_VISITOR_H__
#define PERYAN_AST_VISITOR_H__

namespace Peryan {

class TransUnit;
class TypeSpec;
class ArrayTypeSpec;
class FuncTypeSpec;
class MemberTypeSpec;
class Identifier;
class BinaryExpr;
class UnaryExpr;
class StrLiteralExpr;
class IntLiteralExpr;
class FloatLiteralExpr;
class CharLiteralExpr;
class BoolLiteralExpr;
class ArrayLiteralExpr;
class FuncCallExpr;
class ConstructorExpr;
class SubscrExpr;
class MemberExpr;
class RefExpr;
class DerefExpr;
class FuncExpr;
class StaticMemberExpr;
class CompStmt;
class FuncDefStmt;
class VarDefStmt;
class AssignStmt;
class IfStmt;
class RepeatStmt;
class Label;
class LabelStmt;
class GotoStmt;
class GosubStmt;
class ContinueStmt;
class BreakStmt;
class ReturnStmt;
class ExternStmt;
class NamespaceStmt;

class ASTVisitor {
public:
	// Neither statement nor expression
	virtual void visit(TransUnit *tu) = 0;
	virtual void visit(Label *label) = 0;

	// Statements which affect symbol table
	virtual void visit(FuncDefStmt *fds) = 0;
	virtual void visit(VarDefStmt *vds) = 0;
	virtual void visit(CompStmt *cs) = 0;
	virtual void visit(IfStmt *is) = 0;
	virtual void visit(RepeatStmt *rs) = 0;
	virtual void visit(LabelStmt *ls) = 0;
	virtual void visit(ExternStmt *es) = 0;
	virtual void visit(NamespaceStmt *es) = 0;

	// Statements which don't affect symbol table but require the table
	virtual void visit(AssignStmt *as) = 0;
	virtual void visit(GotoStmt *gs) = 0;
	virtual void visit(GosubStmt *gs) = 0;
	virtual void visit(ContinueStmt *cs) = 0;
	virtual void visit(BreakStmt *bs) = 0;
	virtual void visit(ReturnStmt *rs) = 0;

	// Type Specifiers
	virtual void visit(TypeSpec *ts) = 0;
	virtual void visit(ArrayTypeSpec *ats) = 0;
	virtual void visit(FuncTypeSpec *fts) = 0;
	virtual void visit(MemberTypeSpec *mts) = 0;

	// Expressions
	virtual void visit(Identifier *id) = 0;
	virtual void visit(BinaryExpr *be) = 0;
	virtual void visit(UnaryExpr *ue) = 0;
	virtual void visit(StrLiteralExpr *sle) = 0;
	virtual void visit(IntLiteralExpr *ile) = 0;
	virtual void visit(FloatLiteralExpr *fle) = 0;
	virtual void visit(CharLiteralExpr *cle) = 0;
	virtual void visit(BoolLiteralExpr *ble) = 0;
	virtual void visit(ArrayLiteralExpr *ale) = 0;
	virtual void visit(FuncCallExpr *fce) = 0;
	virtual void visit(ConstructorExpr *ce) = 0;
	virtual void visit(SubscrExpr *se) = 0;
	virtual void visit(MemberExpr *me) = 0;
	virtual void visit(RefExpr *re) = 0;
	virtual void visit(DerefExpr *de) = 0;
	virtual void visit(FuncExpr *de) = 0;
	virtual void visit(StaticMemberExpr *sme) = 0;

};

}

#endif
