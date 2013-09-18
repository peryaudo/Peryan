// but also it resolves the types of the symbols which are specified explicitly.
// Forward reference of type definitions is disallowed, so types will be resolved in this class.
// You can't resolve symbols inside data structure (i.e. class or namespace) without types,
// so SymbolResolver won't resolve such symbols. (right hand side of MemberExpr, StaticMemberExpr)

#include <iostream>

#include "SymbolTable.h"
#include "SymbolResolver.h"
#include "Options.h"
#include "WarningPrinter.h"

//#define DBG_PRINT(TYPE, FUNC_NAME) std::cout<<#TYPE<<#FUNC_NAME<<std::endl
#define DBG_PRINT(TYPE, FUNC_NAME)

namespace Peryan {

void SymbolResolver::visit(TransUnit *tu) {
	DBG_PRINT(+, TransUnit);
	assert(tu != NULL);

	Scope *scope = tu->scope;

	scopes.push(scope);

	for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
		(*it)->accept(this);
	}

	scopes.pop();

	assert(scopes.empty());

	DBG_PRINT(-, TransUnit);
	return;
}

void SymbolResolver::visit(FuncDefStmt *fds) {
	DBG_PRINT(+, FuncDefStmt);
	assert(fds != NULL);

	scopes.push(fds->symbol);

	if (fds->retTypeSpec != NULL) {
		fds->retTypeSpec->accept(this);
	}

	Type *curType = (fds->retTypeSpec != NULL ? fds->retTypeSpec->type : NULL);

	if (curType != NULL && curType->getTypeType() == Type::NAMESPACE_TYPE) {
		throw SemanticsError(fds->token.getPosition(),
			std::string("error : cannot use namespace type ")
			+ curType->getTypeName()
			+ " as a return value");
	}

	for (std::vector<Expr *>::iterator it = fds->defaults.begin(); it != fds->defaults.end(); ++it) {
		if (*it != NULL)
			(*it)->accept(this);
	}

	if (fds->params.size() > 0) {
		for (std::vector<Identifier *>::reverse_iterator it = fds->params.rbegin();
				it != fds->params.rend(); ++it) {
			(*it)->accept(this);

			if ((*it)->type != NULL && (*it)->type->getTypeType() == Type::NAMESPACE_TYPE) {
				throw SemanticsError(fds->token.getPosition(),
					std::string("error : cannot use namespace type ")
					+ (*it)->type->getTypeName()
					+ " as a parameter value");
			}

			curType = new FuncType((*it)->type, curType);
		}
	} else {
		curType = new FuncType(symbolTable_.Void_, curType);
	}

	fds->body->accept(this);

	fds->symbol->setType(curType);

	fds->symbol->defaults = &(fds->defaults);

	scopes.pop();

	DBG_PRINT(-, FuncDefStmt);
	return;
}

void SymbolResolver::visit(VarDefStmt *vds) {
	DBG_PRINT(+, VarDefStmt);
	assert(vds != NULL);

	vds->id->accept(this);

	if (vds->init != NULL)
		vds->init->accept(this);

	DBG_PRINT(-, VarDefStmt);
	return;
}

void SymbolResolver::visit(InstStmt *is) {
	DBG_PRINT(+, InstStmt);
	assert(is != NULL);

	is->inst->accept(this);

	for (std::vector<Expr *>::iterator it = is->params.begin();
			it != is->params.end(); ++it) {
		if (*it != NULL)
			(*it)->accept(this);
	}

	DBG_PRINT(-, InstStmt);
	return;
}

void SymbolResolver::visit(AssignStmt *as) {
	DBG_PRINT(+, AssignStmt);
	assert(as != NULL);

	as->lhs->accept(this);

	if (as->rhs != NULL)
		as->rhs->accept(this);

	DBG_PRINT(-, AssignStmt);
	return;
}

void SymbolResolver::visit(CompStmt *cs) {
	DBG_PRINT(+, CompStmt);
	assert(cs != NULL);

	scopes.push(cs->scope);

	for (std::vector<Stmt *>::iterator it = cs->stmts.begin();
			it != cs->stmts.end(); ++it) {
		(*it)->accept(this);
	}

	scopes.pop();

	DBG_PRINT(-, CompStmt);
	return;
}

void SymbolResolver::visit(IfStmt *is) {
	DBG_PRINT(+, IfStmt);
	assert(is != NULL);

	is->ifCond->accept(this);
	is->ifThen->accept(this);

	for (int i = 0, len = is->elseIfCond.size(); i < len; ++i) {
		is->elseIfCond[i]->accept(this);
		is->elseIfThen[i]->accept(this);
	}

	if (is->elseThen != NULL)
		is->elseThen->accept(this);

	DBG_PRINT(-, IfStmt);
	return;
}

void SymbolResolver::visit(RepeatStmt *rs) {
	DBG_PRINT(+, RepeatStmt);
	assert(rs != NULL);

	if (rs->count != NULL) {
		rs->count->accept(this);
	}

	rs->scope->define(new VarSymbol("cnt", symbolTable_.Int_, rs->token.getPosition()));

	scopes.push(rs->scope);

	for (std::vector<Stmt *>::iterator it = rs->stmts.begin();
			it != rs->stmts.end(); ++it) {
		(*it)->accept(this);
	}

	scopes.pop();

	DBG_PRINT(-, RepeatStmt);
	return;
}

void SymbolResolver::visit(GotoStmt *gs) {
	DBG_PRINT(+, GotoStmt);
	assert(gs != NULL);

	gs->to->accept(this);

	DBG_PRINT(-, GotoStmt);
	return;
}

void SymbolResolver::visit(GosubStmt *gs) {
	DBG_PRINT(+, GosubStmt);
	assert(gs != NULL);

	gs->to->accept(this);

	DBG_PRINT(-, GosubStmt);
	return;
}

void SymbolResolver::visit(ReturnStmt *rs) {
	DBG_PRINT(+, ReturnStmt);
	assert(rs != NULL);

	if (rs->expr != NULL)
		rs->expr->accept(this);

	DBG_PRINT(-, ReturnStmt);
	return;
}

void SymbolResolver::visit(ExternStmt *es) {
	DBG_PRINT(+, ExternStmt);
	assert(es != NULL);
	assert(es->id != NULL);

	es->id->accept(this);

	if (es->id->type->getTypeType() == Type::MODIFIER_TYPE) {
		ModifierType *mt = static_cast<ModifierType *>(es->id->type);
		if (mt->isRef()) {
			throw SemanticsError(es->id->token.getPosition(), "error: cannot make extern function have reference type");
		}
		if (mt->isConst()) {
			throw SemanticsError(es->id->token.getPosition(), "error: cannot make extern function have const type");
		}
	}

	for (std::vector<Expr *>::iterator it = es->defaults.begin(); it != es->defaults.end(); ++it) {
		if (*it != NULL)
			(*it)->accept(this);
	}

	es->symbol->defaults = &(es->defaults);

	DBG_PRINT(+, ExternStmt);

	return;
}

void SymbolResolver::visit(NamespaceStmt *ns) {
	scopes.push(ns->symbol);

	for (std::vector<Stmt *>::iterator it = ns->stmts.begin(); it != ns->stmts.end(); ++it)
		(*it)->accept(this);

	scopes.pop();

	return;
}

void SymbolResolver::visit(Identifier *id) {
	DBG_PRINT(+, Identifier);
	assert(id != NULL);
	assert(id->type == NULL);

	Scope *scope = scopes.top();

	// There are four statuses of symbol and typeSpec (type is always NULL)
	// sym\ts o  x
	//  o     *  *
	//  x     -  * (- won't occur)

	// if typeSpec is null you should concentrate on symbol resolving

	if (id->symbol != NULL && id->typeSpec != NULL) {
		// on its definition
		id->typeSpec->accept(this);

		assert(id->typeSpec->type != NULL);

		id->type = id->typeSpec->type;
		id->symbol->setType(id->typeSpec->type);

	} else if (id->symbol != NULL && id->typeSpec == NULL) {
		// the symbol was resolved, and still no type information so that nothing to do.

	} else if (id->symbol == NULL && id->typeSpec != NULL) {
		// couldn't occur
		assert(false);

	} else if (id->symbol == NULL && id->typeSpec == NULL) {
		// usual identifier reference
		Symbol *symbol = scope->resolve(id->getString(), id->token.getPosition());
		if (symbol == NULL) {
			if (options_.hspCompat) {
				wp_.add(id->token.getPosition(),
					"warning: implicit global variable declaration is deprecated");
				VarSymbol *varSymbol = new VarSymbol(id->getString(), 0);
				varSymbol->isImplicit = true;
				bool res = symbolTable_.getGlobalScope()->define(varSymbol);
				assert(res == false);
				symbol = varSymbol;
			} else {
				throw SemanticsError(id->token.getPosition(),
					std::string("error : unknown identifier ") + id->getString());
			}
		}
		id->symbol = symbol;
	}

	DBG_PRINT(-, Identifier);
	return;
}

void SymbolResolver::visit(Label *label) {
	DBG_PRINT(+, Label);
	assert(label != NULL);

	Scope *scope = scopes.top();

	if (label->symbol == NULL) {
		Symbol *symbol = scope->resolve(std::string("*") + label->token.getString(), label->token.getPosition());
		if (symbol == NULL) {
			throw SemanticsError(label->token.getPosition(),
					"error: unknown label " + label->token.getString());
		}

		label->symbol = static_cast<LabelSymbol *>(symbol);
	}

	DBG_PRINT(-, Label);
	return;
}

void SymbolResolver::visit(BinaryExpr *be) {
	DBG_PRINT(+, BinaryExpr);
	assert(be != NULL);

	be->lhs->accept(this);

	be->rhs->accept(this);

	DBG_PRINT(-, BinaryExpr);
	return;
}

void SymbolResolver::visit(UnaryExpr *ue) {
	DBG_PRINT(+, UnaryExpr);
	assert(ue != NULL);

	ue->rhs->accept(this);

	DBG_PRINT(-, UnaryExpr);
	return;
}

void SymbolResolver::visit(IntLiteralExpr *lit) {
	DBG_PRINT(+, IntLiteralExpr);
	assert(lit != NULL);
	DBG_PRINT(-, IntLiteralExpr);
	return;
}

void SymbolResolver::visit(StrLiteralExpr *lit) {
	DBG_PRINT(+-, StrLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(CharLiteralExpr *lit) {
	DBG_PRINT(+-, CharLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(FloatLiteralExpr *lit) {
	DBG_PRINT(+-, FloatLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(BoolLiteralExpr *lit) {
	DBG_PRINT(+-, BoolLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(ArrayLiteralExpr *ale) {
	DBG_PRINT(+, ArrayLiteralExpr);

	for (std::vector<Expr *>::iterator it = ale->elements.begin(); it != ale->elements.end(); ++it)
		(*it)->accept(this);

	DBG_PRINT(-, ArrayLiteralExpr);
	return;
}

void SymbolResolver::visit(FuncCallExpr *fce) {
	DBG_PRINT(+, FuncCallExpr);
	assert(fce != NULL);
	assert(fce->func != NULL);

	fce->func->accept(this);

	for (std::vector<Expr *>::iterator it = fce->params.begin();
			it != fce->params.end(); ++it) {
		(*it)->accept(this);
	}

	DBG_PRINT(-, FuncCallExpr);
	return;
}

void SymbolResolver::visit(ConstructorExpr *ce) {
	DBG_PRINT(+, ConstructorExpr);
	assert(ce != NULL);

	ce->constructor->accept(this);

	for (std::vector<Expr *>::iterator it = ce->params.begin();
			it != ce->params.end(); ++it) {
		(*it)->accept(this);
	}

	DBG_PRINT(-, ConstructorExpr);
	return;
}

void SymbolResolver::visit(SubscrExpr *se) {
	DBG_PRINT(+, SubscrExpr);
	assert(se != NULL);

	se->array->accept(this);
	se->subscript->accept(this);

	DBG_PRINT(-, SubscrExpr);
	return;
}

void SymbolResolver::visit(MemberExpr *me) {
	DBG_PRINT(+, MemberExpr);
	assert(me != NULL);

	me->receiver->accept(this);

	// won't visit identifier in usual case
	if (options_.hspCompat && me->member->getString() != "length") {
		me->member->accept(this);
	}

	DBG_PRINT(-, MemberExpr);
	return;
}

void SymbolResolver::visit(StaticMemberExpr *sme) {
	DBG_PRINT(+, StaticMemberExpr);
	assert(sme != NULL);

	sme->receiver->accept(this);

	DBG_PRINT(-, StaticMemberExpr);
	return;
}

void SymbolResolver::visit(TypeSpec *ts) {
	DBG_PRINT(+, TypeSpec);
	assert(ts != NULL);

	Scope *scope = scopes.top();

	Symbol *symbol = scope->resolve(ts->token.getString(), ts->token.getPosition());
	if (symbol == NULL) {
		throw SemanticsError(ts->token.getPosition(),
			std::string("error: unknown type \"") + ts->token.getString()
				+ std::string("\""));
	}

	assert((symbol->getSymbolType() == Symbol::BUILTIN_TYPE_SYMBOL
	    || symbol->getSymbolType() == Symbol::NAMESPACE_SYMBOL) && "class not supported yet");

	if (symbol->getSymbolType() == Symbol::BUILTIN_TYPE_SYMBOL) {
		BuiltInTypeSymbol *bts = static_cast<BuiltInTypeSymbol *>(symbol);

		if (ts->isConst || ts->isRef) {
			ts->type = new ModifierType(ts->isConst, ts->isRef, bts);
		} else {
			ts->type = bts;
		}
	} else if (symbol->getSymbolType() == Symbol::NAMESPACE_SYMBOL) {
		NamespaceSymbol *ns = static_cast<NamespaceSymbol *>(symbol);

		if (ts->isConst || ts->isRef) {
			ts->type = new ModifierType(ts->isConst, ts->isRef, ns);
		} else {
			ts->type = ns;
		}

	}

	DBG_PRINT(-, TypeSpec);
	return;
}

void SymbolResolver::visit(ArrayTypeSpec *ats) {
	DBG_PRINT(+, ArrayTypeSpec);
	assert(ats != NULL);

	ats->typeSpec->accept(this);
	ArrayType *at = new ArrayType(ats->typeSpec->type);

	if (ats->isConst || ats->isRef) {
		ats->type = new ModifierType(ats->isConst, ats->isRef, at);
	} else {
		ats->type = at;
	}

	DBG_PRINT(-, ArrayTypeSpec);
	return;
}

void SymbolResolver::visit(FuncTypeSpec *fts) {
	DBG_PRINT(+, FuncTypeSpec);
	assert(fts != NULL);

	fts->lhs->accept(this);
	fts->rhs->accept(this);
	FuncType *ft = new FuncType(fts->lhs->type, fts->rhs->type);

	if (fts->isConst || fts->isRef) {
		fts->type = new ModifierType(fts->isConst, fts->isRef, ft);
	} else {
		fts->type = ft;
	}

	DBG_PRINT(-, FuncTypeSpec);
	return;
}

void SymbolResolver::visit(MemberTypeSpec *mts) {
	DBG_PRINT(+, MemberTypeSpec);
	assert(mts != NULL);

	mts->lhs->accept(this);

	assert(mts->lhs->type != NULL);

	if (mts->lhs->type->getTypeType() != Type::CLASS_TYPE
		&& mts->lhs->type->getTypeType() != Type::NAMESPACE_TYPE) {
		throw SemanticsError(mts->lhs->token.getPosition(),
			std::string("error: type \"") + mts->lhs->type->getTypeName()
				+ "\" is neither class nor namespace");
	}

	assert(mts->lhs->type->getTypeType() != Type::CLASS_TYPE);
	if (mts->lhs->type->getTypeType() == Type::NAMESPACE_TYPE) {
		NamespaceSymbol *ns = static_cast<NamespaceSymbol *>(mts->lhs->type);
		Symbol *resolved = ns->resolveMember(mts->rhs.getString(), mts->rhs.getPosition());
		if (resolved == NULL) {
			throw SemanticsError(mts->rhs.getPosition(),
				std::string("error: unknown member ") + mts->rhs.getString());
		}
		if (resolved->getSymbolType() == Symbol::NAMESPACE_SYMBOL) {
			mts->type = static_cast<Type *>(static_cast<NamespaceSymbol *>(resolved));
		} else {
			assert(false && "unknown type");
		}
	}
	return;
}

}

