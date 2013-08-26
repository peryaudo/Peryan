#ifndef PERYAN_SYMBOL_RESOLVER_H__
#define PERYAN_SYMBOL_RESOLVER_H__

#include "SymbolTable.h"
#include "AST.h"

namespace Peryan {

class SymbolResolver {
private:
	SymbolResolver(const SymbolResolver&);
	SymbolResolver& operator=(const SymbolResolver&);

	SymbolTable& symbolTable_;
public:
	SymbolResolver(SymbolTable& symbolTable) : symbolTable_(symbolTable) {}

	void visit(TransUnit *tu) throw (SemanticsError);

	void visit(Stmt *stmt, Scope *scope) throw (SemanticsError);
	void visit(FuncDefStmt *fds, Scope *scope) throw (SemanticsError);
	void visit(VarDefStmt *vds, Scope *scope) throw (SemanticsError);
	void visit(InstStmt *is, Scope *scope) throw (SemanticsError);
	void visit(AssignStmt *as, Scope *scope) throw (SemanticsError);
	void visit(CompStmt *cs, Scope *scope) throw (SemanticsError);
	void visit(IfStmt *is, Scope *scope) throw (SemanticsError);
	void visit(RepeatStmt *rs, Scope *scope) throw (SemanticsError);
	void visit(GotoStmt *gs, Scope *scope) throw (SemanticsError);
	void visit(GosubStmt *gs, Scope *scope) throw (SemanticsError);
	void visit(ReturnStmt *rs, Scope *scope) throw (SemanticsError);
	void visit(ExternStmt *es, Scope *scope) throw (SemanticsError);
	void visit(Expr *expr, Scope *scope) throw (SemanticsError);
	void visit(Identifier *id, Scope *scope) throw (SemanticsError);
	void visit(Label *label, Scope *scope) throw (SemanticsError);
	void visit(BinaryExpr *be, Scope *scope) throw (SemanticsError);
	void visit(UnaryExpr *ue, Scope *scope) throw (SemanticsError);
	void visit(IntLiteralExpr *lit, Scope *scope) throw (SemanticsError);
	void visit(StrLiteralExpr *lit, Scope *scope) throw (SemanticsError);
	void visit(CharLiteralExpr *lit, Scope *scope) throw (SemanticsError);
	void visit(FloatLiteralExpr *lit, Scope *scope) throw (SemanticsError);
	void visit(BoolLiteralExpr *lit, Scope *scope) throw (SemanticsError);
	void visit(FuncCallExpr *fce, Scope *scope) throw (SemanticsError);
	void visit(ConstructorExpr *fce, Scope *scope) throw (SemanticsError);
	void visit(SubscrExpr *se, Scope *scope) throw (SemanticsError);
	void visit(MemberExpr *me, Scope *scope) throw (SemanticsError);
	Type *visit(TypeSpec *ts, Scope *scope) throw (SemanticsError);
	ArrayType *visit(ArrayTypeSpec *ts, Scope *scope) throw (SemanticsError);
	FuncType *visit(FuncTypeSpec *fs, Scope *scope) throw (SemanticsError);
};

};

#endif
