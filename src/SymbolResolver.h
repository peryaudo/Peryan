#ifndef PERYAN_SYMBOL_RESOLVER_H__
#define PERYAN_SYMBOL_RESOLVER_H__

#include "SymbolTable.h"
#include "AST.h"

namespace Peryan {

class Options;
class WarningPrinter;

class SymbolResolver {
private:
	SymbolResolver(const SymbolResolver&);
	SymbolResolver& operator=(const SymbolResolver&);

	SymbolTable& symbolTable_;
	Options& options_;
	WarningPrinter& wp_;
public:
	SymbolResolver(SymbolTable& symbolTable, Options& options, WarningPrinter& wp)
		: symbolTable_(symbolTable), options_(options), wp_(wp) {}

	void visit(TransUnit *tu);

	void visit(Stmt *stmt, Scope *scope);
	void visit(FuncDefStmt *fds, Scope *scope);
	void visit(VarDefStmt *vds, Scope *scope);
	void visit(InstStmt *is, Scope *scope);
	void visit(AssignStmt *as, Scope *scope);
	void visit(CompStmt *cs, Scope *scope);
	void visit(IfStmt *is, Scope *scope);
	void visit(RepeatStmt *rs, Scope *scope);
	void visit(GotoStmt *gs, Scope *scope);
	void visit(GosubStmt *gs, Scope *scope);
	void visit(ReturnStmt *rs, Scope *scope);
	void visit(ExternStmt *es, Scope *scope);
	void visit(NamespaceStmt *ns, Scope *Scope);
	void visit(Expr *expr, Scope *scope);
	void visit(Identifier *id, Scope *scope);
	void visit(Label *label, Scope *scope);
	void visit(BinaryExpr *be, Scope *scope);
	void visit(UnaryExpr *ue, Scope *scope);
	void visit(IntLiteralExpr *lit, Scope *scope);
	void visit(StrLiteralExpr *lit, Scope *scope);
	void visit(CharLiteralExpr *lit, Scope *scope);
	void visit(FloatLiteralExpr *lit, Scope *scope);
	void visit(BoolLiteralExpr *lit, Scope *scope);
	void visit(ArrayLiteralExpr *ale, Scope *scope);
	void visit(FuncCallExpr *fce, Scope *scope);
	void visit(ConstructorExpr *fce, Scope *scope);
	void visit(SubscrExpr *se, Scope *scope);

	// always disallow implicit variable declaration in these situation
	void visit(MemberExpr *me, Scope *scope);
	void visit(StaticMemberExpr *sme, Scope *scope);
	Type *visit(TypeSpec *ts, Scope *scope);
	ArrayType *visit(ArrayTypeSpec *ts, Scope *scope);
	FuncType *visit(FuncTypeSpec *fs, Scope *scope);
	Type *visit(MemberTypeSpec *mts, Scope *scope);
};

};

#endif
