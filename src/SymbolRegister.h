#ifndef PERYAN_SYMBOL_REGISTER_H__
#define PERYAN_SYMBOL_REGISTER_H__

#include <stack>

#include "SymbolTable.h"
#include "AST.h"
#include "ASTVisitor.h"

namespace Peryan {

class Options;
class WarningPrinter;

class SymbolRegister : public ASTVisitor {
private:
	SymbolRegister(const SymbolRegister&);
	SymbolRegister& operator=(const SymbolRegister&);

	SymbolTable& symbolTable_;

	Options& options_;
	WarningPrinter& wp_;

	std::stack<Scope *> scopes;

public:
	SymbolRegister(SymbolTable& symbolTable, Options& options, WarningPrinter& wp)
		: symbolTable_(symbolTable), options_(options), wp_(wp) {}

	bool isDisallowedIdentifier(const std::string& name);

	virtual void visit(TransUnit *tu);
	virtual void visit(Label *label)		{ return; }

	virtual void visit(FuncDefStmt *fds);
	virtual void visit(VarDefStmt *vds);
	virtual void visit(CompStmt *cs);
	virtual void visit(IfStmt *is);
	virtual void visit(RepeatStmt *rs);
	virtual void visit(LabelStmt *ls);
	virtual void visit(ExternStmt *ls);
	virtual void visit(NamespaceStmt *ns);

	virtual void visit(AssignStmt *as)		{ return; }
	virtual void visit(GotoStmt *gs)		{ return; }
	virtual void visit(GosubStmt *gs)		{ return; }
	virtual void visit(ContinueStmt *cs)		{ return; }
	virtual void visit(BreakStmt *bs)		{ return; }
	virtual void visit(ReturnStmt *rs)		{ return; }

	virtual void visit(TypeSpec *ts)		{ return; }
	virtual void visit(ArrayTypeSpec *ats)		{ return; }
	virtual void visit(FuncTypeSpec *fts)		{ return; }
	virtual void visit(MemberTypeSpec *mts)		{ return; }

	virtual void visit(Identifier *id)		{ return; }
	virtual void visit(BinaryExpr *be)		{ return; }
	virtual void visit(UnaryExpr *ue)		{ return; }
	virtual void visit(StrLiteralExpr *sle)		{ return; }
	virtual void visit(IntLiteralExpr *ile)		{ return; }
	virtual void visit(FloatLiteralExpr *fle)	{ return; }
	virtual void visit(CharLiteralExpr *cle)	{ return; }
	virtual void visit(BoolLiteralExpr *ble)	{ return; }
	virtual void visit(ArrayLiteralExpr *ale)	{ return; }
	virtual void visit(FuncCallExpr *fce)		{ return; }
	virtual void visit(ConstructorExpr *ce)		{ return; }
	virtual void visit(SubscrExpr *se)		{ return; }
	virtual void visit(MemberExpr *me)		{ return; }
	virtual void visit(RefExpr *re)			{ return; }
	virtual void visit(DerefExpr *de)		{ return; }
	virtual void visit(FuncExpr *de)		{ return; }
	virtual void visit(StaticMemberExpr *sme)	{ return; }

};

}

#endif
