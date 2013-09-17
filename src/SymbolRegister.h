#ifndef PERYAN_SYMBOL_REGISTER_H__
#define PERYAN_SYMBOL_REGISTER_H__

#include <stack>

#include "SymbolTable.h"
#include "AST.h"

namespace Peryan {

class Options;
class WarningPrinter;

class SymbolRegister {
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
	void visit(TransUnit *tu);
	void visit(Stmt *stmt);
	void visit(FuncDefStmt *fds);
	void visit(VarDefStmt *vds);
	void visit(CompStmt *cs);
	void visit(IfStmt *is);
	void visit(RepeatStmt *rs);
	void visit(LabelStmt *ls);
	void visit(ExternStmt *ls);
	void visit(NamespaceStmt *ns);
};

}

#endif
