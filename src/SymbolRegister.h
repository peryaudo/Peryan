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
	void visit(TransUnit *tu) throw (SemanticsError);
	void visit(Stmt *stmt, Scope *scope) throw (SemanticsError);
	void visit(FuncDefStmt *fds, Scope *scope) throw (SemanticsError);
	void visit(VarDefStmt *vds, Scope *scope) throw (SemanticsError);
	void visit(CompStmt *cs, Scope *scope) throw (SemanticsError);
	void visit(IfStmt *is, Scope *scope) throw (SemanticsError);
	void visit(RepeatStmt *rs, Scope *scope) throw (SemanticsError);
	void visit(LabelStmt *ls, Scope *scope) throw (SemanticsError);
	void visit(ExternStmt *ls, Scope *scope) throw (SemanticsError);
	void visit(NamespaceStmt *ns, Scope *scope) throw (SemanticsError);
};

}

#endif
