// SymbolRegister registers symbols but doesn't resolve their types.
// It also doesn't resolve symbols because some symbols (function definition and variable definition in class definition)
// allows forward reference.
// It is done by SymbolResolver.

#include <cassert>

#include "AST.h"
#include "Lexer.h"
#include "SymbolTable.h"
#include "SymbolRegister.h"
#include "Options.h"
#include "WarningPrinter.h"

namespace Peryan {

std::string SemanticsError::toString(const Lexer& lexer) {
	return lexer.getPrettyPrint(position_, message_);
}

bool SymbolRegister::isDisallowedIdentifier(const std::string& name) {
	const std::string names[] = {
		"Int",
		"String",
		"Char",
		"Float",
		"Double",
		"Bool",
		"Void",
		"Label",
		"cnt"
	};
	for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
		if (names[i] == name)
			return true;
	}
	return false;
}

void SymbolRegister::visit(TransUnit *tu) {
	assert(tu != NULL);

	GlobalScope *scope = symbolTable_.getGlobalScope();

	tu->scope = scope;

	scopes.push(scope);
	for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
		(*it)->accept(this);
	}
	scopes.pop();

	assert(scopes.empty());

	return;
}

void SymbolRegister::visit(FuncDefStmt *fds) {
	assert(fds != NULL);

	Scope *scope = scopes.top();

	const std::string name = fds->name->getString();

	if (isDisallowedIdentifier(name)) {
		throw SemanticsError(fds->name->token.getPosition(),
				std::string("error: you can't use ") + name + " as an identifier");
	}

	FuncSymbol *funcSymbol =
		new FuncSymbol(name, scope, fds->name->token.getPosition());

	if (scope->define(funcSymbol)) {
		throw SemanticsError(fds->name->token.getPosition(),
				std::string("error: ") + name + " defined twice");
	}

	fds->symbol = funcSymbol;
	fds->name->symbol = funcSymbol;

	for (std::vector<Identifier *>::iterator it = fds->params.begin();
			it != fds->params.end(); ++it) {

		if (isDisallowedIdentifier((*it)->getString())) {
			throw SemanticsError((*it)->token.getPosition(),
					std::string("error: you can't use ")
					+ (*it)->getString() + " as an identifier");
		}

		VarSymbol *varSymbol = new VarSymbol((*it)->getString(), (*it)->token.getPosition());
		if (funcSymbol->define(varSymbol)) {
			throw SemanticsError((*it)->token.getPosition(),
				std::string("error: ") + (*it)->getString() + " defined twice");
		}
		(*it)->symbol = varSymbol;
	}


	scopes.push(funcSymbol);
	fds->body->accept(this);
	scopes.pop();

	return;
}

void SymbolRegister::visit(NamespaceStmt *ns) {
	Scope *scope = scopes.top();

	const std::string name = ns->name->token.getString();
	if (isDisallowedIdentifier(name)) {
		throw SemanticsError(ns->name->token.getPosition(),
				std::string("error: you can't use ") + name + "as an identifier");
	}

	NamespaceSymbol *namespaceSymbol =
		new NamespaceSymbol(name, scope, ns->name->token.getPosition());

	if (scope->define(namespaceSymbol)) {
		throw SemanticsError(ns->name->token.getPosition(),
				std::string("error: ") + name + " defined twice");
	}

	ns->symbol = namespaceSymbol;

	scopes.push(namespaceSymbol);
	for (std::vector<Stmt *>::iterator it = ns->stmts.begin(); it != ns->stmts.end(); ++it)
		(*it)->accept(this);
	scopes.pop();

	return;
}

void SymbolRegister::visit(VarDefStmt *vds) {
	assert(vds != NULL);

	Scope *scope = scopes.top();

	if (isDisallowedIdentifier(vds->id->getString())) {
		throw SemanticsError(vds->id->token.getPosition(),
				std::string("error: you can't use ") + vds->id->getString() + " as an identifier");
	}

	VarSymbol *varSymbol = new VarSymbol(vds->id->getString(), vds->id->token.getPosition());
	if (scope->define(varSymbol)) {
		throw SemanticsError(vds->id->token.getPosition(),
				std::string("error: ") + vds->id->getString() + " defined twice");
	}

	vds->symbol = varSymbol;
	vds->id->symbol = varSymbol;

	// TODO: if closure is supported, there should be visit of expr

	return;
}

void SymbolRegister::visit(ExternStmt *es) {
	assert(es != NULL);

	Scope *scope = scopes.top();

	if (isDisallowedIdentifier(es->id->getString())) {
		throw SemanticsError(es->id->token.getPosition(),
				std::string("error: you can't use ")
				+ es->id->getString() + " as an identifier");
	}

	ExternSymbol *externSymbol = new ExternSymbol(es->id->getString(), es->id->token.getPosition());
	if (scope->define(externSymbol)) {
		throw SemanticsError(es->id->token.getPosition(),
				std::string("error: ") + es->id->getString() + " defined twice");
	}

	es->symbol = externSymbol;
	es->id->symbol = externSymbol;
}

void SymbolRegister::visit(CompStmt *cs) {
	assert(cs != NULL);
	assert(cs->token.getType() != Token::UNKNOWN);

	Scope *scope = scopes.top();

	LocalScope *localScope = new LocalScope(scope, cs->token.getPosition());
	cs->scope = localScope;

	scopes.push(localScope);
	for (std::vector<Stmt *>::iterator it = cs->stmts.begin(); it != cs->stmts.end(); ++it)
		(*it)->accept(this);
	scopes.pop();

	return;
}

void SymbolRegister::visit(LabelStmt *ls) {
	assert(ls != NULL);

	Scope *scope = scopes.top();

	if (options_.hspCompat) {
		wp_.add(ls->label->token.getPosition(), "warning: use of label is deprecated");
	} else {
		throw SemanticsError(ls->label->token.getPosition(),
				"error: use of labels is deprecated. if you want to use them, try --hsp-compatible .");
	}

	if (isDisallowedIdentifier(ls->label->token.getString())) {
		throw SemanticsError(ls->label->token.getPosition(),
				std::string("error: you can't use ")
				+ ls->label->token.getString() + " as an identifier");
	}

	LabelSymbol *labelSymbol = new LabelSymbol(std::string("*") + ls->label->token.getString(),
								ls->label->token.getPosition());

	labelSymbol->setType(symbolTable_.Label_);
	ls->label->type = symbolTable_.Label_;

	if (scope->define(labelSymbol)) {
		throw SemanticsError(ls->token.getPosition(),
				std::string("error: ") + ls->label->token.getString() +
					" defined twice");
	}

	ls->label->symbol = labelSymbol;

	return;
}

void SymbolRegister::visit(IfStmt *is) {
	assert(is != NULL);

	for (std::vector<CompStmt *>::iterator it = is->ifThen.begin(); it != is->ifThen.end(); ++it) {
		(*it)->accept(this);
	}

	if (is->elseThen != NULL)
		is->elseThen->accept(this);

	return;
}

void SymbolRegister::visit(RepeatStmt *rs) {
	assert(rs != NULL);
	assert(rs->token.getType() != Token::UNKNOWN);

	Scope *scope = scopes.top();

	LocalScope *localScope = new LocalScope(scope, rs->token.getPosition());
	rs->scope = localScope;

	scopes.push(localScope);
	for (std::vector<Stmt *>::iterator it = rs->stmts.begin(); it != rs->stmts.end(); ++it)
		(*it)->accept(this);
	scopes.pop();

	return;
}

};
