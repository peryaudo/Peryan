#ifndef PERYAN_SYMBOL_RESOLVER_H__
#define PERYAN_SYMBOL_RESOLVER_H__

#include <stack>

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

	std::stack<Scope *> scopes;
public:
	SymbolResolver(SymbolTable& symbolTable, Options& options, WarningPrinter& wp)
		: symbolTable_(symbolTable), options_(options), wp_(wp) {}

	void visit(TransUnit *tu);

	void visit(Stmt *stmt);
	void visit(FuncDefStmt *fds);
	void visit(VarDefStmt *vds);
	void visit(InstStmt *is);
	void visit(AssignStmt *as);
	void visit(CompStmt *cs);
	void visit(IfStmt *is);
	void visit(RepeatStmt *rs);
	void visit(GotoStmt *gs);
	void visit(GosubStmt *gs);
	void visit(ReturnStmt *rs);
	void visit(ExternStmt *es);
	void visit(NamespaceStmt *ns);
	void visit(Expr *expr);
	void visit(Identifier *id);
	void visit(Label *label);
	void visit(BinaryExpr *be);
	void visit(UnaryExpr *ue);
	void visit(IntLiteralExpr *lit);
	void visit(StrLiteralExpr *lit);
	void visit(CharLiteralExpr *lit);
	void visit(FloatLiteralExpr *lit);
	void visit(BoolLiteralExpr *lit);
	void visit(ArrayLiteralExpr *ale);
	void visit(FuncCallExpr *fce);
	void visit(ConstructorExpr *fce);
	void visit(SubscrExpr *se);

	// always disallow implicit variable declaration in these situation
	void visit(MemberExpr *me);
	void visit(StaticMemberExpr *sme);
	void visit(TypeSpec *ts);
	void visit(ArrayTypeSpec *ts);
	void visit(FuncTypeSpec *fs);
	void visit(MemberTypeSpec *mts);
};

};

#endif
