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

void SymbolResolver::visit(TransUnit *tu) throw (SemanticsError) {
	DBG_PRINT(+, TransUnit);
	assert(tu != NULL);

	Scope *scope = tu->scope;

	for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
		visit(*it, scope);
	}

	DBG_PRINT(-, TransUnit);
	return;
}

void SymbolResolver::visit(Stmt *stmt, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, Stmt);
	assert(stmt != NULL);

	switch (stmt->getASTType()) {
	case AST::FUNC_DEF_STMT		: visit(static_cast<FuncDefStmt *>(stmt), scope); break;
	case AST::VAR_DEF_STMT		: visit(static_cast<VarDefStmt *>(stmt), scope); break;
	case AST::INST_STMT		: visit(static_cast<InstStmt *>(stmt), scope); break;
	case AST::ASSIGN_STMT		: visit(static_cast<AssignStmt *>(stmt), scope); break;
	case AST::COMP_STMT		: visit(static_cast<CompStmt *>(stmt), scope); break;
	case AST::IF_STMT		: visit(static_cast<IfStmt *>(stmt), scope); break;
	case AST::REPEAT_STMT		: visit(static_cast<RepeatStmt *>(stmt), scope); break;
	case AST::GOTO_STMT		: visit(static_cast<GotoStmt *>(stmt), scope); break;
	case AST::GOSUB_STMT		: visit(static_cast<GosubStmt*>(stmt), scope); break;
	case AST::RETURN_STMT		: visit(static_cast<ReturnStmt *>(stmt), scope); break;
	case AST::EXTERN_STMT		: visit(static_cast<ExternStmt *>(stmt), scope); break;
	case AST::NAMESPACE_STMT	: visit(static_cast<NamespaceStmt *>(stmt), scope); break;
	default				: ;
	}

	DBG_PRINT(-, Stmt);
	return;
}

void SymbolResolver::visit(FuncDefStmt *fds, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, FuncDefStmt);
	assert(fds != NULL);

	scope = fds->symbol;

	if (fds->retTypeSpec != NULL) {
		visit(fds->retTypeSpec, scope);
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
			visit(*it, scope);
	}

	if (fds->params.size() > 0) {
		for (std::vector<Identifier *>::reverse_iterator it = fds->params.rbegin();
				it != fds->params.rend(); ++it) {
			visit(*it, scope);

			if ((*it)->type != NULL && (*it)->type->getTypeType() == Type::NAMESPACE_TYPE) {
				throw SemanticsError(fds->token.getPosition(),
					std::string("error : cannot use namespace type ")
					+ (*it)->type->getTypeName()
					+ " as a parameter value");
			}

			curType = new FuncType((*it)->type, curType);
		}
	} else {
		Type *Void_ = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Void"));
		curType = new FuncType(Void_, curType);
	}

	visit(fds->body, scope);

	fds->symbol->setType(curType);

	fds->symbol->defaults = &(fds->defaults);

	DBG_PRINT(-, FuncDefStmt);
	return;
}

void SymbolResolver::visit(VarDefStmt *vds, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, VarDefStmt);
	assert(vds != NULL);

	visit(vds->id, scope);

	if (vds->init != NULL)
		visit(vds->init, scope);

	DBG_PRINT(-, VarDefStmt);
	return;
}

void SymbolResolver::visit(InstStmt *is, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, InstStmt);
	assert(is != NULL);

	visit(is->inst, scope);

	for (std::vector<Expr *>::iterator it = is->params.begin();
			it != is->params.end(); ++it) {
		if (*it != NULL)
			visit(*it, scope);
	}

	DBG_PRINT(-, InstStmt);
	return;
}

void SymbolResolver::visit(AssignStmt *as, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, AssignStmt);
	assert(as != NULL);

	visit(as->lhs, scope);

	if (as->rhs != NULL)
		visit(as->rhs, scope);

	DBG_PRINT(-, AssignStmt);
	return;
}

void SymbolResolver::visit(CompStmt *cs, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, CompStmt);
	assert(cs != NULL);

	scope = cs->scope;

	for (std::vector<Stmt *>::iterator it = cs->stmts.begin();
			it != cs->stmts.end(); ++it) {
		visit((*it), scope);
	}

	DBG_PRINT(-, CompStmt);
	return;
}

void SymbolResolver::visit(IfStmt *is, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, IfStmt);
	assert(is != NULL);

	visit(is->ifCond, scope);
	visit(is->ifThen, scope);

	for (int i = 0, len = is->elseIfCond.size(); i < len; ++i) {
		visit(is->elseIfCond[i], scope);
		visit(is->elseIfThen[i], scope);
	}

	if (is->elseThen != NULL)
		visit(is->elseThen, scope);

	DBG_PRINT(-, IfStmt);
	return;
}

void SymbolResolver::visit(RepeatStmt *rs, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, RepeatStmt);
	assert(rs != NULL);

	if (rs->count != NULL) {
		visit(rs->count, scope);
	}

	Type *Int_ = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Int"));
	assert(Int_ != NULL);

	rs->scope->define(new VarSymbol("cnt", Int_, rs->token.getPosition()));

	for (std::vector<Stmt *>::iterator it = rs->stmts.begin();
			it != rs->stmts.end(); ++it) {
		visit(*it, rs->scope);
	}

	DBG_PRINT(-, RepeatStmt);
	return;
}

void SymbolResolver::visit(GotoStmt *gs, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, GotoStmt);
	assert(gs != NULL);

	visit(gs->to, scope);
	DBG_PRINT(-, GotoStmt);
	return;
}

void SymbolResolver::visit(GosubStmt *gs, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, GosubStmt);
	assert(gs != NULL);

	visit(gs->to, scope);
	DBG_PRINT(-, GosubStmt);
	return;
}

void SymbolResolver::visit(ReturnStmt *rs, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, ReturnStmt);
	assert(rs != NULL);

	if (rs->expr != NULL)
		visit(rs->expr, scope);

	DBG_PRINT(-, ReturnStmt);
	return;
}

void SymbolResolver::visit(ExternStmt *es, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, ExternStmt);
	assert(es != NULL);
	assert(es->id != NULL);

	visit(es->id, scope);

	for (std::vector<Expr *>::iterator it = es->defaults.begin(); it != es->defaults.end(); ++it) {
		if (*it != NULL)
			visit(*it, scope);
	}

	es->symbol->defaults = &(es->defaults);

	DBG_PRINT(+, ExternStmt);
}

void SymbolResolver::visit(NamespaceStmt *ns, Scope *Scope) throw (SemanticsError) {
	for (std::vector<Stmt *>::iterator it = ns->stmts.begin(); it != ns->stmts.end(); ++it)
		visit(*it, ns->symbol);
}


void SymbolResolver::visit(Expr *expr, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, Expr);
	assert(expr != NULL);

	switch (expr->getASTType()) {
	case AST::IDENTIFIER		: visit(static_cast<Identifier *>(expr), scope); break;
	case AST::LABEL			: visit(static_cast<Label *>(expr), scope); break;

	case AST::BINARY_EXPR		: visit(static_cast<BinaryExpr *>(expr), scope); break;
	case AST::UNARY_EXPR		: visit(static_cast<UnaryExpr *>(expr), scope); break;
	case AST::STR_LITERAL_EXPR	: visit(static_cast<StrLiteralExpr *>(expr), scope); break;
	case AST::INT_LITERAL_EXPR	: visit(static_cast<IntLiteralExpr *>(expr), scope); break;
	case AST::FLOAT_LITERAL_EXPR	: visit(static_cast<FloatLiteralExpr *>(expr), scope); break;
	case AST::CHAR_LITERAL_EXPR	: visit(static_cast<CharLiteralExpr *>(expr), scope); break;
	case AST::BOOL_LITERAL_EXPR	: visit(static_cast<BoolLiteralExpr *>(expr), scope); break;
	case AST::FUNC_CALL_EXPR	: visit(static_cast<FuncCallExpr *>(expr), scope); break;
	case AST::CONSTRUCTOR_EXPR	: visit(static_cast<ConstructorExpr *>(expr), scope); break;
	case AST::SUBSCR_EXPR		: visit(static_cast<SubscrExpr *>(expr), scope); break;
	case AST::MEMBER_EXPR		: visit(static_cast<MemberExpr *>(expr), scope); break;
	case AST::STATIC_MEMBER_EXPR	: visit(static_cast<StaticMemberExpr *>(expr), scope); break;
	default				: ;
	}

	DBG_PRINT(-, Expr);
	return;
}

void SymbolResolver::visit(Identifier *id, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, Identifier);
	assert(id != NULL);
	assert(id->type == NULL);

	// There are four statuses of symbol and typeSpec (type is always NULL)
	// sym\ts o  x
	//  o     *  *
	//  x     -  * (- won't occur)

	// if typeSpec is null you should concentrate on symbol resolving

	if (id->symbol != NULL && id->typeSpec != NULL) {
		// on its definition
		visit(id->typeSpec, scope);

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

void SymbolResolver::visit(Label *label, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, Label);
	assert(label != NULL);
	assert(scope != NULL);

	if (label->symbol == NULL) {
		Symbol *symbol = scope->resolve(std::string("*") + label->token.getString(), label->token.getPosition());
		if (symbol == NULL) {
			throw SemanticsError(label->token.getPosition(),
					"error: unknown label " + label->token.getString());
		}
	}

	DBG_PRINT(-, Label);
	return;
}

void SymbolResolver::visit(BinaryExpr *be, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, BinaryExpr);
	assert(be != NULL);

	visit(be->lhs, scope);

	if (be->rhs != NULL)
		visit(be->rhs, scope);

	DBG_PRINT(-, BinaryExpr);
	return;
}

void SymbolResolver::visit(UnaryExpr *ue, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, UnaryExpr);
	assert(ue != NULL);

	visit(ue->rhs, scope);

	DBG_PRINT(-, UnaryExpr);
	return;
}

void SymbolResolver::visit(IntLiteralExpr *lit, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, IntLiteralExpr);
	assert(lit != NULL);
	DBG_PRINT(-, IntLiteralExpr);
	return;
}

void SymbolResolver::visit(StrLiteralExpr *lit, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+-, StrLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(CharLiteralExpr *lit, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+-, CharLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(FloatLiteralExpr *lit, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+-, FloatLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(BoolLiteralExpr *lit, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+-, BoolLiteralExpr);
	assert(lit != NULL);
	return;
}

void SymbolResolver::visit(FuncCallExpr *fce, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, FuncCallExpr);
	assert(fce != NULL);
	assert(fce->func != NULL);

	visit(fce->func, scope);

	for (std::vector<Expr *>::iterator it = fce->params.begin();
			it != fce->params.end(); ++it) {
		visit((*it), scope);
	}

	DBG_PRINT(-, FuncCallExpr);
	return;
}

void SymbolResolver::visit(ConstructorExpr *ce, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, ConstructorExpr);
	assert(ce != NULL);

	visit(ce->constructor, scope);

	for (std::vector<Expr *>::iterator it = ce->params.begin();
			it != ce->params.end(); ++it) {
		visit((*it), scope);
	}

	DBG_PRINT(-, ConstructorExpr);
	return;
}

void SymbolResolver::visit(SubscrExpr *se, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, SubscrExpr);
	assert(se != NULL);

	visit(se->array, scope);
	visit(se->subscript, scope);

	DBG_PRINT(-, SubscrExpr);
	return;
}

void SymbolResolver::visit(MemberExpr *me, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, MemberExpr);
	assert(me != NULL);

	visit(me->receiver, scope);

	// won't visit identifier in usual case
	if (options_.hspCompat && me->member->getString() != "length") {
		visit(me->member, scope);
	}

	DBG_PRINT(-, MemberExpr);
	return;
}

void SymbolResolver::visit(StaticMemberExpr *sme, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, StaticMemberExpr);
	assert(sme != NULL);

	visit(sme->receiver, scope);

	DBG_PRINT(-, StaticMemberExpr);
	return;
}

Type *SymbolResolver::visit(TypeSpec *ts, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, TypeSpec);
	assert(ts != NULL);

	switch (ts->getASTType()) {
	case AST::TYPE_SPEC		: break;
	case AST::ARRAY_TYPE_SPEC	: return visit(static_cast<ArrayTypeSpec *>(ts), scope);
	case AST::FUNC_TYPE_SPEC	: return visit(static_cast<FuncTypeSpec *>(ts), scope);
	case AST::MEMBER_TYPE_SPEC	: return visit(static_cast<MemberTypeSpec *>(ts), scope);
	default				: assert(false && "unknown type specifier");
	}

	Symbol *symbol = scope->resolve(ts->token.getString(), ts->token.getPosition());
	if (symbol == NULL) {
		throw SemanticsError(ts->token.getPosition(),
			std::string("error: unknown type \"") + ts->token.getString()
				+ std::string("\""));
	}

	assert(symbol->getSymbolType() == Symbol::BUILTIN_TYPE_SYMBOL
	    || symbol->getSymbolType() == Symbol::NAMESPACE_SYMBOL && "class not supported yet");

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
	return ts->type;
}

ArrayType *SymbolResolver::visit(ArrayTypeSpec *ats, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, ArrayTypeSpec);
	assert(ats != NULL);

	ArrayType *at = new ArrayType(visit(ats->typeSpec, scope));

	if (ats->isConst || ats->isRef) {
		ats->type = new ModifierType(ats->isConst, ats->isRef, at);
	} else {
		ats->type = at;
	}

	DBG_PRINT(-, ArrayTypeSpec);
	return at;
}

FuncType *SymbolResolver::visit(FuncTypeSpec *fts, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, FuncTypeSpec);
	assert(fts != NULL);

	FuncType *ft = new FuncType(visit(fts->lhs, scope), visit(fts->rhs, scope));

	if (fts->isConst || fts->isRef) {
		fts->type = new ModifierType(fts->isConst, fts->isRef, ft);
	} else {
		fts->type = ft;
	}

	DBG_PRINT(-, FuncTypeSpec);
	return ft;
}

Type *SymbolResolver::visit(MemberTypeSpec *mts, Scope *scope) throw (SemanticsError) {
	DBG_PRINT(+, MemberTypeSpec);
	assert(mts != NULL);

	visit(mts->lhs, scope);

	assert (mts->lhs->type != NULL);

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
	return mts->type;
}

};

