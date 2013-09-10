#ifndef PERYAN_SYMBOL_REGISTER_H__
#define PERYAN_SYMBOL_REGISTER_H__

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
public:
	SymbolRegister(SymbolTable& symbolTable, Options& options, WarningPrinter& wp)
		: symbolTable_(symbolTable), options_(options), wp_(wp) {}

	bool isDisallowedIdentifier(const std::string& name);
	void visit(TransUnit *tu);
	void visit(Stmt *stmt, Scope *scope);
	void visit(FuncDefStmt *fds, Scope *scope);
	void visit(VarDefStmt *vds, Scope *scope);
	void visit(CompStmt *cs, Scope *scope);
	void visit(IfStmt *is, Scope *scope);
	void visit(RepeatStmt *rs, Scope *scope);
	void visit(LabelStmt *ls, Scope *scope);
	void visit(ExternStmt *ls, Scope *scope);
	void visit(NamespaceStmt *ns, Scope *scope);
};

}

#endif
