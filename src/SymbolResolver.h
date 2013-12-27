#ifndef PERYAN_SYMBOL_RESOLVER_H__
#define PERYAN_SYMBOL_RESOLVER_H__

#include <stack>

#include "SymbolTable.h"
#include "AST.h"
#include "ASTVisitor.h"

namespace Peryan {

class Options;
class WarningPrinter;

class SymbolResolver : public ASTVisitor {
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

	virtual void visit(TransUnit *tu);

	virtual void visit(LabelStmt *ls)	{ return; }
	virtual void visit(ContinueStmt *cs)	{ return; }
	virtual void visit(BreakStmt *bs)	{ return; }

	virtual void visit(RefExpr *re)		{ return; }
	virtual void visit(DerefExpr *de)	{ return; }

	virtual void visit(FuncExpr *de)	{ return; }

	virtual void visit(FuncDefStmt *fds);
	virtual void visit(VarDefStmt *vds);
	virtual void visit(AssignStmt *as);
	virtual void visit(CompStmt *cs);
	virtual void visit(IfStmt *is);
	virtual void visit(RepeatStmt *rs);
	virtual void visit(GotoStmt *gs);
	virtual void visit(GosubStmt *gs);
	virtual void visit(ReturnStmt *rs);
	virtual void visit(ExternStmt *es);
	virtual void visit(NamespaceStmt *ns);
	virtual void visit(Identifier *id);
	virtual void visit(Label *label);
	virtual void visit(BinaryExpr *be);
	virtual void visit(UnaryExpr *ue);
	virtual void visit(IntLiteralExpr *lit);
	virtual void visit(StrLiteralExpr *lit);
	virtual void visit(CharLiteralExpr *lit);
	virtual void visit(FloatLiteralExpr *lit);
	virtual void visit(BoolLiteralExpr *lit);
	virtual void visit(ArrayLiteralExpr *ale);
	virtual void visit(FuncCallExpr *fce);
	virtual void visit(ConstructorExpr *fce);
	virtual void visit(SubscrExpr *se);

	// always disallow implicit variable declaration in these nodes
	virtual void visit(MemberExpr *me);
	virtual void visit(StaticMemberExpr *sme);
	virtual void visit(TypeSpec *ts);
	virtual void visit(ArrayTypeSpec *ts);
	virtual void visit(FuncTypeSpec *fs);
	virtual void visit(MemberTypeSpec *mts);
};

};

#endif
