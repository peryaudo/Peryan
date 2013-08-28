// For detail, see the top of the SymbolResolver

#include <iostream>

#include "SymbolTable.h"
#include "TypeResolver.h"


//#define DBG_PRINT(TYPE, FUNC_NAME) std::cout<<#TYPE<<#FUNC_NAME<<std::endl
#define DBG_PRINT(TYPE, FUNC_NAME)

namespace Peryan {

TypeResolver::TypeResolver(SymbolTable& symbolTable) : symbolTable_(symbolTable) {
	Int_	= static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Int"));
	String_ = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("String"));
	Char_	= static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Char"));
	Float_	= static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Float"));
	Double_ = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Double"));
	Bool_	= static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Bool"));
	Label_	= static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Label"));
	Void_	= static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve("Void"));

	assert(Int_ != NULL);
	//initPromotionTable();
	initBinaryPromotionTable();
}

// actually it's too verbose (because it only has a rule that float promotes to double!)
#if 0
 void TypeResolver::initPromotionTable() {
	const char *typeNames[7] = {"Int", "String", "Char", "Float", "Double", "Bool", "Label"};

	bool tbl[7][7] = {
	/* from \ to	Int	String	Char	Float	Double	Bool	Label */
	/* Int    */  { true,	false,	false,	false,	false,	false,	false	},
	/* String */  { false,	true,	false,	false,	false,	false,	false	},
	/* Char   */  { false,	false,	true,	false,	false,	false,	false	},
	/* Float  */  { false,	false,	false,	true,	true,	false,	false	},
	/* Double */  { false,	false,	false,	false,	true,	false,	false	},
	/* Bool   */  { false,	false,	false,	false,	false,	true,	false	},
	/* Label  */  { false,	false,	false,	false,	false,	false,	true	}};

	std::vector<Type *> types(sizeof(typeNames) / sizeof(typeNames[0]));
	for (int i = 0; i < types.size(); ++i) {
		Symbol *cur = symbolTable_.getGlobalScope()->resolve(typeNames[i]);
		assert(cur != NULL);
		assert(cur->getSymbolType() == Symbol::BUILTIN_TYPE_SYMBOL);

		types[i] = static_cast<BuiltInTypeSymbol *>(cur);
	}

	for (int i = 0, len = types.size(); i < len; ++i)
	for (int j = 0; j < len; ++j)
		if (tbl[i][j]) promotionTable.insert(PromotionKey(types[i], types[j]));

	return;
}
#endif

bool TypeResolver::canPromote(Type *from, Type *to, bool isFuncParam) {
	assert(from != NULL && to != NULL);

	// from != to
	if (!(from->unmodify()->is(to->unmodify()))) {
		// !(from == Float && to == Double)
		if (!(from->unmodify()->is(Float_) && to->unmodify()->is(Double_))) {
			return false;
		}
	}

	// From can promote to To, so we'll check modifiers
	// generally, "const to ref" and "const ref to ref" are prohibited, others are allowed
	// 0 to {const} ref is also prohibited when it is not function parameter

	// prohibit 0 to {const} ref (if it is not function parameter)
	if (!isFuncParam  && to->getTypeType() != Type::MODIFIER_TYPE
			&& to->getTypeType() == Type::MODIFIER_TYPE) {
		ModifierType *mtTo = static_cast<ModifierType *>(to);
		if (mtTo->isRef()) {
			return false;
		}
	}

	// allow 0 to 0
	if (from->getTypeType() != Type::MODIFIER_TYPE
		|| to->getTypeType() != Type::MODIFIER_TYPE) {
		return true;
	}

	ModifierType *mtFrom = static_cast<ModifierType *>(from);
	ModifierType *mtTo = static_cast<ModifierType *>(to);

	// prohibit const { ref } to ref
	if ((mtFrom->isConst() == true) && (mtTo->isConst() == false && mtTo->isRef() == true)) {
		return false;
	}

	return true;
}

Expr *TypeResolver::insertPromoter(Expr *from, Type *toType) {
	assert(from != NULL);
	assert(from->type != NULL);
	assert(toType != NULL);
	assert(canPromote(from->type, toType, true));

	if (from->type->is(toType))
		return from;

	if (!(from->type->unmodify()->is(toType->unmodify()))) {
		TypeSpec *ts = new TypeSpec(Token(Token::ID, toType->getTypeName(), from->token.getPosition()));
		ConstructorExpr *ce = new ConstructorExpr(from->token, ts);
		ce->type = toType->unmodify();
		ce->params.push_back(from);

		from = ce;
	}

	Type *fromType = from->type;
	assert(canPromote(fromType, toType, true));

	bool fromRef = false;
	if (fromType->getTypeType() == Type::MODIFIER_TYPE)
		fromRef = static_cast<ModifierType *>(fromType)->isRef();

	bool toRef = false;
	if (toType->getTypeType() == Type::MODIFIER_TYPE)
		fromRef = static_cast<ModifierType *>(toType)->isRef();

	if ((fromRef && toRef) || (!fromRef && !toRef)) {
		from->type = toType;
		return from;
	} else if (!fromRef && toRef) {
		Expr *res = new RefExpr(from);
		res->type = toType;
		return res;
	} else if (fromRef && !toRef) {
		Expr *res = new DerefExpr(from);
		res->type = toType;
		return res;
	} else {
		assert(false);
		return NULL;
	}
	// TODO: copy ArrayType?
}

void TypeResolver::initBinaryPromotionTable() {
	// TODO: rewrite this with the style of initPoromotionTable()

	// ^ | &
	binaryPromotionTable[BinaryPromotionKey(Bool_, Token::CARET, Bool_)] = Bool_;
	binaryPromotionTable[BinaryPromotionKey(Bool_, Token::PIPE, Bool_)] = Bool_;
	binaryPromotionTable[BinaryPromotionKey(Bool_, Token::AMP, Bool_)] = Bool_;

	binaryPromotionTable[BinaryPromotionKey(Int_, Token::CARET, Int_)] = Int_;
	binaryPromotionTable[BinaryPromotionKey(Int_, Token::PIPE, Int_)] = Int_;
	binaryPromotionTable[BinaryPromotionKey(Int_, Token::AMP, Int_)] = Int_;

	// = == != !
	// they only allows comparison with same type
	{
		const char *typeNames[7] = {"Int", "String", "Char", "Float", "Double", "Bool", "Label"};
		for (int i = 0; i < sizeof(typeNames) / sizeof(typeNames[0]); ++i) {
			Type *type = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve(typeNames[i]));
			binaryPromotionTable[BinaryPromotionKey(type, Token::EQL, type)] = Bool_;
			binaryPromotionTable[BinaryPromotionKey(type, Token::EQEQ, type)] = Bool_;
			binaryPromotionTable[BinaryPromotionKey(type, Token::EXCLEQ, type)] = Bool_;
			binaryPromotionTable[BinaryPromotionKey(type, Token::EXCL, type)] = Bool_;
		}
	}
	// < <= > >=
	{
		const char *typeNames[2] = {"Int", "Char"};
		for (int i = 0; i < sizeof(typeNames) / sizeof(typeNames[0]); ++i) {
			Type *type = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve(typeNames[i]));
			binaryPromotionTable[BinaryPromotionKey(type, Token::LT, type)] = Bool_;
			binaryPromotionTable[BinaryPromotionKey(type, Token::LTEQ, type)] = Bool_;
			binaryPromotionTable[BinaryPromotionKey(type, Token::GT, type)] = Bool_;
			binaryPromotionTable[BinaryPromotionKey(type, Token::GTEQ, type)] = Bool_;
		}
	}
	{
		const char *typeNames[2] = {"Float", "Double"};
		for (int i = 0; i < sizeof(typeNames) / sizeof(typeNames[0]); ++i) {
			for (int j = 0; j < sizeof(typeNames) / sizeof(typeNames[0]); ++j) {
				Type *lhs = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve(typeNames[i]));
				Type *rhs = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve(typeNames[j]));
				binaryPromotionTable[BinaryPromotionKey(lhs, Token::LT, rhs)] = Bool_;
				binaryPromotionTable[BinaryPromotionKey(lhs, Token::LTEQ, rhs)] = Bool_;
				binaryPromotionTable[BinaryPromotionKey(lhs, Token::GT, rhs)] = Bool_;
				binaryPromotionTable[BinaryPromotionKey(lhs, Token::GTEQ, rhs)] = Bool_;
			}
		}
	}

	// << >>
	binaryPromotionTable[BinaryPromotionKey(Int_, Token::LTLT, Int_)] = Int_;
	binaryPromotionTable[BinaryPromotionKey(Int_, Token::GTGT, Int_)] = Int_;

	// + - * / 
	{
		const char *typeNames[4] = {"Int", "Char", "Float", "Double"};
		for (int i = 0; i < sizeof(typeNames) / sizeof(typeNames[0]); ++i) {
			Type *type = static_cast<BuiltInTypeSymbol *>(symbolTable_.getGlobalScope()->resolve(typeNames[i]));
			binaryPromotionTable[BinaryPromotionKey(type, Token::PLUS, type)] = type;
			binaryPromotionTable[BinaryPromotionKey(type, Token::MINUS, type)] = type;
			binaryPromotionTable[BinaryPromotionKey(type, Token::STAR, type)] = type;
			binaryPromotionTable[BinaryPromotionKey(type, Token::SLASH, type)] = type;
		}
	}

	// %
	binaryPromotionTable[BinaryPromotionKey(Int_, Token::PERC, Int_)] = Int_;

	// + for String
	binaryPromotionTable[BinaryPromotionKey(String_, Token::PLUS, String_)] = String_;
	return;
}

void TypeResolver::visit(TransUnit *tu) throw (SemanticsError) {
	DBG_PRINT(+, TransUnit);
	assert(tu != NULL);

	for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
		visit(*it);
	}

	DBG_PRINT(-, TransUnit);
	return;
}

void TypeResolver::visit(Stmt *stmt) throw (SemanticsError) {
	DBG_PRINT(+, Stmt);
	assert(stmt != NULL);

	switch (stmt->getASTType()) {
	case AST::FUNC_DEF_STMT		: visit(static_cast<FuncDefStmt *>(stmt)); break;
	case AST::VAR_DEF_STMT		: visit(static_cast<VarDefStmt *>(stmt)); break;
	case AST::INST_STMT		: visit(static_cast<InstStmt *>(stmt)); break;
	case AST::ASSIGN_STMT		: visit(static_cast<AssignStmt *>(stmt)); break;
	case AST::COMP_STMT		: visit(static_cast<CompStmt *>(stmt)); break;
	case AST::IF_STMT		: visit(static_cast<IfStmt *>(stmt)); break;
	case AST::REPEAT_STMT		: visit(static_cast<RepeatStmt *>(stmt)); break;
	case AST::GOTO_STMT		: visit(static_cast<GotoStmt *>(stmt)); break;
	case AST::GOSUB_STMT		: visit(static_cast<GosubStmt*>(stmt)); break;
	case AST::RETURN_STMT		: visit(static_cast<ReturnStmt *>(stmt)); break;
	case AST::NAMESPACE_STMT	: visit(static_cast<NamespaceStmt *>(stmt)); break;
	default				: ;
	}

	DBG_PRINT(-, Stmt);
	return;
}

void TypeResolver::visit(FuncDefStmt *fds) throw (SemanticsError) {
	DBG_PRINT(+, FuncDefStmt);
	assert(fds != NULL);
	assert(fds->symbol != NULL);
	assert(fds->symbol->getType() != NULL);
	assert(fds->symbol->getType()->getTypeType() == Type::FUNC_TYPE);

	//[closure] TODO: make it capable of nesting
	curFuncRetType_ = static_cast<FuncType *>(fds->symbol->getType())->getReturnType();

	visit(fds->body);

	curFuncRetType_ = NULL;

	DBG_PRINT(-, FuncDefStmt);
	return;
}

void TypeResolver::visit(VarDefStmt *vds) throw (SemanticsError) {
	DBG_PRINT(+, VarDefStmt);
	assert(vds != NULL);
	assert(vds->id != NULL);

	Type *initType = NULL;
	if (vds->init != NULL) {
		initType = visit(vds->init);
		if (initType == NULL) {
			throw SemanticsError(vds->token.getPosition(),
					"error: cannot detect the type of initializer");
		}
	}

	// ex. : var foo :: String = "hi"
	if (vds->id->type != NULL && vds->init != NULL) {
		if (!canPromote(initType, vds->id->type, false)) {
			throw SemanticsError(vds->token.getPosition(),
					std::string("error: type of variable (")
					+ vds->id->type->getTypeName()
					+ std::string(") and initializer (")
					+ initType->getTypeName()
					+ std::string(") are incompatible"));
		}
		vds->init = insertPromoter(vds->init, vds->id->type);
	// ex. : var foo :: String
	} else if (vds->id->type != NULL && vds->init == NULL) {
		// nothing to do
		if (vds->id->type->getTypeType() == Type::MODIFIER_TYPE
			&& static_cast<ModifierType *>(vds->id->type)->isRef()) {
			throw SemanticsError(vds->token.getPosition(),
					"error: reference variable should be initialized");
		}
	// ex. : var foo = "hi"
	} else if (vds->id->type == NULL && vds->init != NULL) {
		// you can infer the type!

		// for convenience, deprive const and reference if not specified
		vds->id->type = initType;
		if (vds->id->type->getTypeType() == Type::MODIFIER_TYPE) {
			ModifierType *mt = static_cast<ModifierType *>(vds->id->type);
			vds->id->type = mt->getElemType();
			// TODO: delete
		}
		vds->init = insertPromoter(vds->init, vds->id->type);
		vds->id->symbol->setType(vds->id->type);
	} else if (vds->id->type == NULL && vds->init == NULL) {
			throw SemanticsError(vds->token.getPosition(),
					"error: (STUB) cannot infer the type of the variable");
	}

	DBG_PRINT(-, VarDefStmt);
	return;
}

void TypeResolver::visit(InstStmt *is) throw (SemanticsError) {
	DBG_PRINT(+, InstStmt);
	assert(is != NULL);
	assert(is->inst != NULL);

	FuncType *curFuncType = NULL;
	{
		Type *tmp = visit(is->inst);
		assert(tmp != NULL);
		tmp = tmp->unmodify();
		if (tmp->getTypeType() != Type::FUNC_TYPE) {
			throw SemanticsError(is->inst->token.getPosition(), "error: it is not function ("
				+ tmp->getTypeName() + std::string(")"));
		}
		curFuncType = static_cast<FuncType *>(tmp);
	}
	assert(curFuncType != NULL);

	for (std::vector<Expr *>::iterator it = is->params.begin();
			it != is->params.end(); ++it) {
		assert(curFuncType != NULL);
		assert(curFuncType->getCar() != NULL);
		assert(curFuncType->getCdr() != NULL);
		assert((*it) != NULL);
		Type *actual = visit(*it);
		assert(actual != NULL);
		if (!canPromote(actual, curFuncType->getCar(), true)) {
			throw SemanticsError((*it)->token.getPosition(),
					std::string("error: argument type (")
					+ actual->getTypeName()
					+ std::string(") doesn't match to the function's one (")
					+ curFuncType->getCar()->getTypeName()
					+ std::string(")"));
		}

		*it = insertPromoter(*it, curFuncType->getCar());

		if ((it + 1) != is->params.end()) {
			if (curFuncType->getCdr()->getTypeType() != Type::FUNC_TYPE) {
				throw SemanticsError((*it)->token.getPosition(),
						"error: too many arguments in the function call");
			}

			curFuncType = static_cast<FuncType *>(curFuncType->getCdr());
		}
	}

	assert(curFuncType->getCdr() != NULL);

	if (curFuncType->getCdr()->getTypeType() == Type::FUNC_TYPE) {
		throw SemanticsError(is->token.getPosition(),
				"error: fewer arguments in the function call");
	}

	DBG_PRINT(-, InstStmt);
	return;
}

void TypeResolver::visit(AssignStmt *as) throw (SemanticsError) {
	DBG_PRINT(+, AssignStmt);
	assert(as != NULL);

	Type *lhsType = visit(as->lhs);

	Type *rhsType = NULL;

	if (as->rhs != NULL)
		rhsType = visit(as->rhs);

	if (!(lhsType->unmodify()->is(Int_)) && !(lhsType->unmodify()->is(Char_))
			&& !(lhsType->unmodify()->is(Float_)) && !(lhsType->unmodify()->is(Double_))
			&& as->token.getType() != Token::EQL
			&&! (as->token.getType() == Token::PLUSEQ && lhsType->unmodify()->is(String_))) {
		throw SemanticsError(as->token.getPosition(),
			std::string("error: left hand side")
			+ lhsType->getTypeName()
			+ std::string(" should be numeric or String type"));
	}

	if (lhsType->getTypeType() != Type::MODIFIER_TYPE
		|| !(static_cast<ModifierType *>(lhsType)->isRef())) {
		throw SemanticsError(as->token.getPosition(),
			std::string("error: left hand side")
			+ lhsType->getTypeName()
			+ std::string(" should be reference to a variable"));
	}

	// AssignmentOperator : "=" | "++" | "--" | "+=" | "-=" | "*=" | "/=" ;
	switch (as->token.getType()) {
	case Token::PLUSPLUS:
	case Token::MINUSMINUS:
		if (!(lhsType->unmodify()->is(Int_)) && !(lhsType->unmodify()->is(Char_)))
			throw SemanticsError(as->token.getPosition(),
				std::string("error: left hand side (")
				+ lhsType->getTypeName()
				+ std::string(") should be integer type"));
		break;
	case Token::EQL:
	case Token::PLUSEQ:
	case Token::MINUSEQ:
	case Token::STAREQ:
	case Token::SLASHEQ:
		{
			if (!canPromote(rhsType, lhsType->unmodify(), false)) {
				throw SemanticsError(as->token.getPosition(),
					std::string("error: left hand side (")
					+ lhsType->getTypeName()
					+ std::string(") and right hand side (")
					+ rhsType->getTypeName()
					+ std::string(") should be same type"));
			}
			as->rhs = insertPromoter(as->rhs, lhsType->unmodify());
			break;
		}
	default: assert(false);
	}

	DBG_PRINT(-, AssignStmt);
	return;
}

void TypeResolver::visit(CompStmt *cs) throw (SemanticsError) {
	DBG_PRINT(+, CompStmt);
	assert(cs != NULL);

	for (std::vector<Stmt *>::iterator it = cs->stmts.begin();
			it != cs->stmts.end(); ++it) {
		visit(*it);
	}

	DBG_PRINT(-, CompStmt);
	return;
}

void TypeResolver::visit(NamespaceStmt *ns) throw (SemanticsError) {
	DBG_PRINT(+, NamespaceStmt);
	assert(ns != NULL);

	for (std::vector<Stmt *>::iterator it = ns->stmts.begin();
			it != ns->stmts.end(); ++it) {
		visit(*it);
	}

	DBG_PRINT(-, NamespaceStmt);

}

void TypeResolver::visit(IfStmt *is) throw (SemanticsError) {
	DBG_PRINT(+, IfStmt);
	assert(is != NULL);

	if (!canPromote(visit(is->ifCond), Bool_, false))
		throw SemanticsError(is->token.getPosition(), "error: condition should be Bool");
	is->ifCond = insertPromoter(is->ifCond, Bool_);


	visit(is->ifThen);

	for (int i = 0, len = is->elseIfCond.size(); i < len; ++i) {
		if (!canPromote(visit(is->elseIfCond[i]), Bool_, false))
			throw SemanticsError(is->token.getPosition(), "error: condition should be Bool");
		is->ifCond = insertPromoter(is->ifCond, Bool_);

		visit(is->elseIfThen[i]);
	}

	if (is->elseThen != NULL)
		visit(is->elseThen);

	DBG_PRINT(-, IfStmt);
	return;
}

void TypeResolver::visit(RepeatStmt *rs) throw (SemanticsError) {
	DBG_PRINT(+, RepeatStmt);
	assert(rs != NULL);

	if (rs->count != NULL) {
		if (!canPromote(visit(rs->count), Int_, false))
			throw SemanticsError(rs->token.getPosition(), "error: repeat counter should be Int");
		rs->count = insertPromoter(rs->count, Int_);
	}

	for (std::vector<Stmt *>::iterator it = rs->stmts.begin();
			it != rs->stmts.end(); ++it) {
		visit(*it);
	}

	DBG_PRINT(-, RepeatStmt);
	return;
}

void TypeResolver::visit(GotoStmt *gs) throw (SemanticsError) {
	DBG_PRINT(+, GotoStmt);
	assert(gs != NULL);

	DBG_PRINT(-, GotoStmt);
	return;
}

void TypeResolver::visit(GosubStmt *gs) throw (SemanticsError) {
	DBG_PRINT(+, GosubStmt);
	assert(gs != NULL);

	DBG_PRINT(-, GosubStmt);
	return;
}

void TypeResolver::visit(ReturnStmt *rs) throw (SemanticsError) {
	DBG_PRINT(+, ReturnStmt);
	assert(rs != NULL);

	assert(curFuncRetType_ != NULL);
	if (curFuncRetType_->is(Void_)) {
		if (rs->expr != NULL)
			throw SemanticsError(rs->token.getPosition(), "error: the function returns Void");

		return;
	}

	if (rs->expr == NULL)
		throw SemanticsError(rs->token.getPosition(), "error: the function should return a value");


	if (!canPromote(visit(rs->expr), curFuncRetType_))
		throw SemanticsError(rs->token.getPosition(), "error: type of the expression doesn't match the return type");

	rs->expr = insertPromoter(rs->expr, curFuncRetType_);

	DBG_PRINT(-, ReturnStmt);
	return;
}

Type *TypeResolver::visit(Expr *expr) throw (SemanticsError) {
	DBG_PRINT(+, Expr);
	assert(expr != NULL);

	Type *type = NULL;
	switch (expr->getASTType()) {
	case AST::IDENTIFIER		: type = visit(static_cast<Identifier *>(expr)); break;
	case AST::LABEL			: type = visit(static_cast<Label *>(expr)); break;

	case AST::BINARY_EXPR		: type = visit(static_cast<BinaryExpr *>(expr)); break;
	case AST::UNARY_EXPR		: type = visit(static_cast<UnaryExpr *>(expr)); break;
	case AST::STR_LITERAL_EXPR	: type = visit(static_cast<StrLiteralExpr *>(expr)); break;
	case AST::INT_LITERAL_EXPR	: type = visit(static_cast<IntLiteralExpr *>(expr)); break;
	case AST::FLOAT_LITERAL_EXPR	: type = visit(static_cast<FloatLiteralExpr *>(expr)); break;
	case AST::CHAR_LITERAL_EXPR	: type = visit(static_cast<CharLiteralExpr *>(expr)); break;
	case AST::BOOL_LITERAL_EXPR	: type = visit(static_cast<BoolLiteralExpr *>(expr)); break;
	case AST::ARRAY_LITERAL_EXPR	: type = visit(static_cast<ArrayLiteralExpr *>(expr)); break;
	case AST::FUNC_CALL_EXPR	: type = visit(static_cast<FuncCallExpr *>(expr)); break;
	case AST::CONSTRUCTOR_EXPR	: type = visit(static_cast<ConstructorExpr *>(expr)); break;
	case AST::SUBSCR_EXPR		: type = visit(static_cast<SubscrExpr *>(expr)); break;
	case AST::MEMBER_EXPR		: type = visit(static_cast<MemberExpr *>(expr)); break;
	case AST::STATIC_MEMBER_EXPR	: type = visit(static_cast<StaticMemberExpr *>(expr)); break;
	default				: assert(false);
	}

	DBG_PRINT(-, Expr);
	return type;
}

Type *TypeResolver::visit(Identifier *id) throw (SemanticsError) {
	DBG_PRINT(+, Identifier);
	assert(id != NULL);
	assert(id->symbol != NULL);

	if (id->type != NULL) {
		return id->type;
	}

	if (id->symbol->getSymbolType() == Symbol::BUILTIN_TYPE_SYMBOL
			|| id->symbol->getSymbolType() == Symbol::CLASS_SYMBOL) {
		// [class] TODO: fix this
		return NULL;
	}


	// try to resolve type of symbol
	if ((id->type = id->symbol->getType()) == NULL) {
		throw SemanticsError(id->token.getPosition(),
			std::string("error: (STUB) cannot detect type of identifier ") + id->token.getString());

	}

	// all value references returns ref!
	bool isConst = false;
	if (id->type->getTypeType() == Type::MODIFIER_TYPE) {
		ModifierType *mt = static_cast<ModifierType *>(id->type);
		isConst = mt->isConst();
	}

	id->type = new ModifierType(isConst, /* isRef = */ true, id->type->unmodify());

	DBG_PRINT(-, Identifier);
	return id->type;
}

Type *TypeResolver::visit(Label *label) throw (SemanticsError) {
	DBG_PRINT(+, Label);
	assert(label != NULL);

	DBG_PRINT(-, Label);
	return Label_;
}

Type *TypeResolver::visit(BinaryExpr *be) throw (SemanticsError) {
	DBG_PRINT(+, BinaryExpr);
	assert(be != NULL);

	Type *lhsType = visit(be->lhs);
	Type *rhsType = visit(be->rhs);
	assert(lhsType != NULL);
	assert(rhsType != NULL);

	// TODO: separate this to another function
	BinaryPromotionKey key(lhsType->unmodify(), be->token.getType(), rhsType->unmodify());

	if (binaryPromotionTable.count(key)) {
		be->type = binaryPromotionTable[key];

		assert(be != NULL);
		assert(be->lhs != NULL);
		assert(be->lhs->type != NULL);
		assert(be->rhs != NULL);
		assert(be->rhs->type != NULL);

		if (lhsType->is(lhsType->unmodify()) && rhsType->is(rhsType->unmodify())) {
			DBG_PRINT(-, BinaryExpr);
			return be->type = new ModifierType(true, false, be->type);
		}

		// insert promoters
		if (canPromote(lhsType, rhsType->unmodify(), false)) {
			// rhsType->unmodify() is wider than lhsType->unmodify().
			be->lhs = insertPromoter(be->lhs, rhsType->unmodify());
			be->rhs = insertPromoter(be->rhs, rhsType->unmodify());
		} else if (canPromote(rhsType, lhsType->unmodify(), false)) {
			// lhsType->unmodify() is wider than rhsType->unmodify().
			be->rhs = insertPromoter(be->rhs, lhsType->unmodify());
			be->lhs = insertPromoter(be->lhs, lhsType->unmodify());
		} else {
			assert(false);
		}
		DBG_PRINT(-, BinaryExpr);
		return be->type = new ModifierType(true, false, be->type);
	} else {
		throw SemanticsError(be->token.getPosition(),
			std::string("error: the operation between these two types is not allowed: ")
				+ lhsType->getTypeName() + " and " + rhsType->getTypeName());
	}

}

Type *TypeResolver::visit(UnaryExpr *ue) throw (SemanticsError) {
	DBG_PRINT(+, UnaryExpr);
	assert(ue != NULL);

	Type *rhsType = visit(ue->rhs);
	
	switch (ue->token.getType()) {
	case Token::PLUS:
	case Token::MINUS:
		if (!(rhsType->unmodify()->is(Int_)) && !(rhsType->unmodify()->is(Float_))
			&& !(rhsType->unmodify()->is(Double_)) && !(rhsType->unmodify()->is(Char_)))
			throw SemanticsError(ue->token.getPosition(), "error: right side should be numeric type");

		ue->rhs = insertPromoter(ue->rhs, new ModifierType(true, false, rhsType->unmodify()));
		DBG_PRINT(-, UnaryExpr);
		return ue->type = ue->rhs->type;
	case Token::EXCL:
		if (!(rhsType->unmodify()->is(Bool_)))
			throw SemanticsError(ue->token.getPosition(), "error: right side should be Bool");

		ue->rhs = insertPromoter(ue->rhs, new ModifierType(true, false, rhsType->unmodify()));
		DBG_PRINT(-, UnaryExpr);
		return ue->type = ue->rhs->type;
	default: ;
	}

	assert(false);
	return NULL;
}

Type *TypeResolver::visit(IntLiteralExpr *lit) throw (SemanticsError) {
	DBG_PRINT(+-, IntLiteralExpr);
	assert(lit != NULL);
	return lit->type = new ModifierType(true, false, Int_);
}

Type *TypeResolver::visit(StrLiteralExpr *lit) throw (SemanticsError) {
	DBG_PRINT(+-, StrLiteralExpr);
	assert(lit != NULL);
	return lit->type = new ModifierType(true, false, String_);
}

Type *TypeResolver::visit(CharLiteralExpr *lit) throw (SemanticsError) {
	DBG_PRINT(+-, CharLiteralExpr);
	assert(lit != NULL);
	return lit->type = new ModifierType(true, false, Char_);
}

Type *TypeResolver::visit(FloatLiteralExpr *lit) throw (SemanticsError) {
	DBG_PRINT(+-, FloatLiteralExpr);
	assert(lit != NULL);
	return lit->type = new ModifierType(true, false, Double_);
}

Type *TypeResolver::visit(BoolLiteralExpr *lit) throw (SemanticsError) {
	DBG_PRINT(+-, BoolLiteralExpr);
	assert(lit != NULL);

	return lit->type = new ModifierType(true, false, Bool_);
}

Type *TypeResolver::visit(ArrayLiteralExpr *ale) throw (SemanticsError) {
	DBG_PRINT(+, ArrayLiteralExpr);
	assert(ale != NULL);

	assert(ale->elements.size() > 0);

	Type *elemType = visit(ale->elements[0])->unmodify();
	if (elemType == NULL) {
		throw SemanticsError(ale->token.getPosition(), "error: cannot infer type of array literal");
	}
	ale->elements[0] = insertPromoter(ale->elements[0], elemType);

	for (std::vector<Expr *>::iterator it = ale->elements.begin() + 1; it != ale->elements.end(); ++it) {
		Type *curElemType = visit(*it);
		assert(curElemType != NULL);

		if (!canPromote(curElemType, elemType, false)) {
			throw SemanticsError(
				(*it)->token.getPosition(),
				std::string("error: the type of the element (")
				+ curElemType->getTypeName()
				+ std::string(") is incompatible with the type of the array literal (")
				+ elemType->getTypeName());
		}
		*it = insertPromoter(*it, elemType);
	}

	DBG_PRINT(-, ArrayLiteralExpr);
	return ale->type = new ModifierType(true, false, new ArrayType(elemType));
}

Type *TypeResolver::visit(FuncCallExpr *fce) throw (SemanticsError) {
	DBG_PRINT(+, FuncCallExpr);
	assert(fce != NULL);

	FuncType *curFuncType = NULL;
	{
		Type *tmp = visit(fce->func);
		tmp = tmp->unmodify();
		if (tmp->getTypeType() != Type::FUNC_TYPE) {
			throw SemanticsError(fce->func->token.getPosition(), "error: it is not function");
		}
		curFuncType = static_cast<FuncType *>(tmp);
	}

	for (std::vector<Expr *>::iterator it = fce->params.begin();
			it != fce->params.end(); ++it) {
		Type *argType = visit(*it);

		if (!canPromote(argType, curFuncType->getCar())) {
			throw SemanticsError((*it)->token.getPosition(),
					std::string("error: argument type (")
					+ argType->getTypeName()
					+ std::string(") doesn't match to the function's one (")
					+ curFuncType->getCar()->getTypeName()
					+ std::string(")"));
		}

		*it = insertPromoter(*it, curFuncType->getCar());

		if ((it + 1) != fce->params.end()) {
			if (curFuncType->getCdr()->getTypeType() != Type::FUNC_TYPE) {
				throw SemanticsError((*it)->token.getPosition(),
						"error: too many arguments in the function call");
			}

			curFuncType = static_cast<FuncType *>(curFuncType->getCdr());
		}
	}

	if (curFuncType->getCdr()->getTypeType() == Type::FUNC_TYPE) {
		throw SemanticsError(fce->token.getPosition(),
				"error: fewer arguments in the function call");
	}

	fce->type = curFuncType->getCdr();
	if (fce->type->is(Void_)) {
		throw SemanticsError(fce->token.getPosition(), "error: the function doesn't return a value");
	}

	DBG_PRINT(-, FuncCallExpr);
	return fce->type;
}

// TODO: STUB
Type *TypeResolver::visit(ConstructorExpr *ce) throw (SemanticsError) {
	DBG_PRINT(+, ConstructorExpr);
	assert(ce != NULL);
	assert(ce->constructor != NULL);

	for (std::vector<Expr *>::iterator it = ce->params.begin();
			it != ce->params.end(); ++it) {
		visit(*it);
	}


	// [class] TODO: check parameters to the constructor's type

	ce->type = ce->constructor->type;

	if ((ce->params.size() == 0)
		|| (ce->params.size() == 1 && ce->params[0]->type->unmodify()->is(ce->type))) {
		// Type()
		// Type(Type)
		// TODO: add optimization there from LLVMCodeGen and check the type
	} else if (ce->params.size() == 1 &&
			ce->type->is(String_) &&
			ce->params[0]->type->unmodify()->is(Int_)) {
		// String(Int)
		ce->params[0] = insertPromoter(ce->params[0], Int_);
	} else if (ce->params.size() == 1 &&
			ce->type->is(Int_) && 
			ce->params[0]->type->unmodify()->is(String_)) {
		// Int(String)
		ce->params[0] = insertPromoter(ce->params[0], String_);
	} else if (ce->params.size() == 1 &&
			ce->type->getTypeType() == Type::ARRAY_TYPE &&
			ce->params[0]->type->unmodify()->is(Int_)) {
		// [Type](...)
		ce->params[0] = insertPromoter(ce->params[0], Int_);
	} else if (ce->params.size() == 2 &&
			ce->type->getTypeType() == Type::ARRAY_TYPE &&
			ce->params[0]->type->unmodify()->is(Int_) &&
			ce->params[1]->type->unmodify()->is(
				static_cast<ArrayType *>(ce->type)->getElemType()->unmodify())) {
		ce->params[0] = insertPromoter(ce->params[0], Int_);
		ce->params[1] = insertPromoter(ce->params[1],
					static_cast<ArrayType *>(ce->type)->getElemType()->unmodify());
	} else {
		// [class] TODO: Support class
		throw SemanticsError(ce->token.getPosition(), "error: Unknown constructor type");
	}

	DBG_PRINT(-, ConstructorExpr);
	return ce->type;
}

Type *TypeResolver::visit(SubscrExpr *se) throw (SemanticsError) {
	DBG_PRINT(+, SubscrExpr);
	assert(se != NULL);

	Type *arrayType = visit(se->array);
	if (arrayType->unmodify()->getTypeType() != Type::ARRAY_TYPE)
		throw SemanticsError(se->token.getPosition(), "error: giving subscript to non-array");

	bool isConst = false;
	bool isRef = false;

	if (arrayType->getTypeType() == Type::MODIFIER_TYPE) {
		ModifierType *mt = static_cast<ModifierType *>(arrayType);
		isConst = mt->isConst();
		isRef = mt->isRef();
	}

	if (!isRef) {
		se->array = insertPromoter(se->array,
				new ModifierType(isConst, true,
					static_cast<ArrayType *>(arrayType->unmodify())->getElemType()));
	}

	Type *subscriptType = visit(se->subscript);
	if (!(subscriptType->unmodify()->is(Int_)))
		throw SemanticsError(se->token.getPosition(), "error: subscript should be Int");

	se->subscript = insertPromoter(se->subscript, Int_);

	se->type = static_cast<ArrayType *>(arrayType->unmodify())->getElemType();

	se->type = new ModifierType(isConst || !isRef, true, se->type->unmodify());

	DBG_PRINT(-, SubscrExpr);
	return se->type;
}

Type *TypeResolver::visit(MemberExpr *me) throw (SemanticsError) {
	DBG_PRINT(+, MemberExpr);
	assert(me != NULL);

	visit(me->receiver);

	if (me->receiver->type->unmodify()->getTypeType() != Type::CLASS_TYPE
		&& !(me->receiver->type->unmodify()->is(String_))
		&& me->receiver->type->unmodify()->getTypeType() != Type::ARRAY_TYPE) {
		throw SemanticsError(me->token.getPosition(),
			std::string("error: invalid left hand side type (")
			+ me->receiver->type->getTypeName() + ")");
	}

	assert(me->receiver->type->getTypeType() != Type::CLASS_TYPE && "class not supported");

	assert(me->member->getASTType() == AST::IDENTIFIER);

	if (me->receiver->type->unmodify()->is(String_)) {
		if (me->member->getString() == "length") {
			me->type = new ModifierType(true, true, Int_);
		} else {
			throw SemanticsError(me->token.getPosition(),
				std::string("error: invalid member ") + me->member->getString()
				+ " for type " + me->receiver->type->getTypeName());
		}
	} else if (me->receiver->type->unmodify()->getTypeType() == Type::ARRAY_TYPE) {
		if (me->member->getString() == "length") {
			me->type = new ModifierType(true, true, Int_);
		} else {
			throw SemanticsError(me->token.getPosition(),
				std::string("error: invalid member ") + me->member->getString()
				+ " for type " + me->receiver->type->getTypeName());
		}
	}

	DBG_PRINT(-, MemberExpr);

	return me->type;
}

Type *TypeResolver::visit(StaticMemberExpr *sme) throw (SemanticsError) {
	DBG_PRINT(+, StaticMemberExpr);
	assert(sme != NULL);

	assert(sme->receiver->type->getTypeType() == Type::NAMESPACE_TYPE
		|| sme->receiver->type->getTypeType() == Type::CLASS_TYPE);

	assert(sme->receiver->type->getTypeType() != Type::CLASS_TYPE && "class not supported");

	assert(sme->member->getASTType() == AST::IDENTIFIER);

	if (sme->receiver->type->getTypeType() == Type::NAMESPACE_TYPE) {
		NamespaceSymbol *ns = static_cast<NamespaceSymbol *>(sme->receiver->type);
		Symbol *symbol = ns->resolveMember(sme->member->getString(), sme->member->token.getPosition());
		if (symbol == NULL) {
			throw SemanticsError(sme->member->token.getPosition(),
				std::string("error : unknown member ") + sme->member->getString());
		}
		assert (symbol->getType() != NULL);

		sme->member->symbol = symbol;
		sme->member->type = symbol->getType();
		sme->type = symbol->getType();
	}

	DBG_PRINT(-, StaticMemberExpr);

	return sme->type;
}

};

