// For detail, see the top of the SymbolResolver

#include <iostream>

#include "SymbolTable.h"
#include "TypeResolver.h"
#include "Options.h"
#include "ASTPrinter.h"


//#define DBG_PRINT(TYPE, FUNC_NAME) std::cout<<#TYPE<<#FUNC_NAME<<std::endl
#define DBG_PRINT(TYPE, FUNC_NAME)

namespace Peryan {

TypeResolver::TypeResolver(SymbolTable& symbolTable, Options& opt, WarningPrinter& wp)
	: symbolTable_(symbolTable), opt_(opt), wp_(wp)
	, Int_		(symbolTable_.Int_)
	, String_	(symbolTable_.String_)
	, Char_		(symbolTable_.Char_)
	, Float_	(symbolTable_.Float_)
	, Double_	(symbolTable_.Double_)
	, Bool_		(symbolTable_.Bool_)
	, Label_	(symbolTable_.Label_)
	, Void_		(symbolTable_.Void_)
	, rewriteWith_(NULL) {

	initPromotionTable();
	initBinaryPromotionTable();
}

// check compatibility between two type modifiers (0 means unmodified)
bool TypeResolver::canConvertModifier(Type *from, Type *to, bool isFuncParam) {
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

// defines "sub <: super" which is described in TAPL
bool TypeResolver::isSubtypeOf(Type *sub, Type *super) {
	// [class] TODO: support classes
	sub = sub->unmodify();
	super = super->unmodify();

	if (sub->is(super)) {
		// T <: T
		return true;
	} else {
		// Float <: Double
		if (sub->is(Float_) && super->is(Double_)) {
			return true;
		} else {
			return false;
		}
	}
}

bool TypeResolver::canPromote(Type *from, Type *to, Position pos, bool isFuncParam) {
	assert(from != NULL && to != NULL);

	if (isSubtypeOf(/*sub = */ from->unmodify(), /*super = */ to->unmodify())) {
		// correct
	} else if (opt_.hspCompat && promotionTable.count(PromotionKey(from->unmodify(), to->unmodify())) > 0) {
		// explicit type conversion is always prohibited in Peryan mode
		wp_.add(pos, "warning: implicit conversion from Int to Double is deprecated");
	} else {
		return false;
	}

	// From can promote to To, so we'll check modifiers
	return canConvertModifier(from, to, isFuncParam);
}

Type *TypeResolver::canPromoteBinary(Type *lhsType, Token::Type tokenType, Type* rhsType) {
	BinaryPromotionKey key(lhsType, tokenType, rhsType);

	if (binaryPromotionTable.count(key)) {
		return binaryPromotionTable[key];
	} else {
		return NULL;
	}
}

Expr *TypeResolver::insertPromoter(Expr *from, Type *toType) {
	assert(from != NULL);
	assert(from->type != NULL);
	assert(toType != NULL);

	if (from->type->is(toType))
		return from;

	if (!(from->type->unmodify()->is(toType->unmodify()))) {
		TypeSpec *ts = new TypeSpec(Token(Token::ID, toType->getTypeName(), from->token.getPosition()));
		ConstructorExpr *ce = new ConstructorExpr(from->token, ts);
		ts->type = ce->type = toType->unmodify();
		ce->params.push_back(insertPromoter(from, from->type->unmodify()));

		from = ce;
	}

	Type *fromType = from->type;
	assert(fromType != NULL);

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

void TypeResolver::initPromotionTable() {
	Type *types[8] = { Bool_, Char_, Int_, Float_, Double_, String_, Void_, Label_ };
	bool table[8][8] = {
		/*frm\to	  Bool	  Char	  Int	  Float	  Double  String  Void    Label */
		/* Bool   */	{ true	, true	, true	, true	, true	, false	, false	, false	},
		/* Char   */	{ true	, true	, true	, true	, true	, false	, false	, false	},
		/* Int	  */	{ true	, true	, true	, true	, true	, true  , false	, false	},
		/* Float  */	{ false , false	, false	, true	, false	, false	, false	, false	},
		/* Double */	{ false	, false	, true	, false	, true	, false	, false	, false	},
		/* String */	{ false	, false	, true  , false	, false	, true	, false	, false	},
		/* Void   */	{ false	, false	, false	, false	, false	, false	, true	, false	},
		/* Label  */	{ false	, false	, false	, false	, false	, false	, false	, true	}
	};

	for (int iFrom = 0; iFrom < 8; ++iFrom)
	for (int iTo = 0; iTo < 8; ++iTo)
		if (table[iFrom][iTo] == true)
			promotionTable.insert(PromotionKey(types[iFrom], types[iTo]));

	return;
}

#define REGISTER_BINARY_PROMOTION(TABLE, TOKEN_TYPES) \
	for (int iLhs = 1; iLhs < 9; ++iLhs) \
	for (int iRhs = 1; iRhs < 9; ++iRhs) \
	for (unsigned int iToken = 0; iToken < sizeof(TOKEN_TYPES) / sizeof(TOKEN_TYPES[0]); ++iToken) \
		if (TABLE[iLhs][iRhs] != NULL) \
			binaryPromotionTable[\
				BinaryPromotionKey(TABLE[iLhs][0], TOKEN_TYPES[iToken], TABLE[0][iRhs])] \
					= TABLE[iLhs][iRhs];

void TypeResolver::initBinaryPromotionTable() {
	// ^ | &
	Token::Type caretPipeAmpTokenTypes[] = { Token::CARET, Token::PIPE, Token::AMP };
	Type *caretPipeAmp[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , Bool_	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, NULL	, Int_	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};
	REGISTER_BINARY_PROMOTION(caretPipeAmp, caretPipeAmpTokenTypes);

	// = == != !
	Token::Type eqlEqeqExcleqExclTokenTypes[] = { Token::EQL, Token::EQEQ, Token::EXCLEQ, Token::EXCL};
	Type *eqlEqeqExcleqExcl[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , Bool_	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, Bool_	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, NULL	, Bool_	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, Bool_	, NULL	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, NULL	, Bool_	 , NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL	, NULL	, NULL	 , Bool_  , NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , Bool_ , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , Bool_  }
	};
	REGISTER_BINARY_PROMOTION(eqlEqeqExcleqExcl, eqlEqeqExcleqExclTokenTypes);

	// < <= > >=
	Token::Type ltLteqGtGteqTokenTypes[] = { Token::LT, Token::LTEQ, Token::GT, Token::GTEQ };
	Type *ltLteqGtGteq[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, Bool_	, Bool_	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, Bool_	, Bool_	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, Bool_	, Bool_	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, Bool_	, Bool_	 , NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};
	REGISTER_BINARY_PROMOTION(ltLteqGtGteq, ltLteqGtGteqTokenTypes);

	// << >>
	// Token::LTLT Token::GTGT
	Token::Type ltltGtgtTokenTypes[] = { Token::LTLT, Token::GTGT };
	Type *ltltGtgt[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, NULL	, Int_	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};
	REGISTER_BINARY_PROMOTION(ltltGtgt, ltltGtgtTokenTypes);

	// + - * / 

	Token::Type plusMinusStarSlashTokenTypes[] = { Token::PLUS, Token::MINUS, Token::STAR, Token::SLASH };
	Type *plusMinusStarSlash[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, Char_	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, NULL	, Int_	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, Float_, NULL	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, NULL	, Double_, NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};

	Type *hspCompatPlusMinusStarSlash[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , Int_	, Int_	, Int_	, Int_	, Int_	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, Char_	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, Int_	, Int_	, Int_	, Int_	, Int_	 , NULL	  , NULL  , NULL   },
		{ Float_, Float_, Float_, Float_, Float_, Double_, NULL	  , NULL  , NULL   },
		{ Double_,Double_,Double_,Double_,Double_,Double_, NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};
	if (opt_.hspCompat) {
		REGISTER_BINARY_PROMOTION(hspCompatPlusMinusStarSlash, plusMinusStarSlashTokenTypes);
	} else {
		REGISTER_BINARY_PROMOTION(plusMinusStarSlash, plusMinusStarSlashTokenTypes);
	}

	// %
	Token::Type percTokenTypes[] = { Token::PERC };
	Type *perc[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, NULL	, Int_	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};
	REGISTER_BINARY_PROMOTION(perc, percTokenTypes);

	// + for String
	Token::Type stringPlusTokenTypes[] = { Token::PLUS };
	Type *stringPlus[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, NULL  ,NULL	, NULL	 , String_, NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};

	Type *hspCompatStringPlus[9][9] = {
		/*lhs\rhs*/
		{ NULL	, Bool_ , Char_ , Int_	, Float_, Double_, String_, Void_ , Label_ },
		{ Bool_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Char_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Int_	, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Float_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Double_,NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ String_,NULL	, NULL	, String_,NULL	, NULL	 , String_, NULL  , NULL   },
		{ Void_ , NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   },
		{ Label_, NULL	, NULL	, NULL	, NULL	, NULL	 , NULL	  , NULL  , NULL   }
	};

	if (opt_.hspCompat) {
		REGISTER_BINARY_PROMOTION(hspCompatStringPlus, stringPlusTokenTypes);
	} else {
		REGISTER_BINARY_PROMOTION(stringPlus, stringPlusTokenTypes);
	}

	return;
}

void TypeResolver::addTypeConstraint(Type *constraint, TypeVar typeVar) {
	DBG_PRINT(+, addTypeConstraint);

	if (constraints_.count(typeVar)) {
		TypeConstraint& cur = constraints_[typeVar];
		// constraint <: lowerBound <: T <: upperBound => lowerBound = constraint
		if (isSubtypeOf(/* sub = */constraint, /* super = */cur.lowerBound)) {
			cur.lowerBound = constraint;

		// if upperBound == NULL then upperBound = constraint
		} else if (cur.upperBound == NULL) {
			cur.upperBound = constraint;

		// lowerBound <: constraint <: upperBound
		} else if (isSubtypeOf(cur.lowerBound, constraint) &&
				isSubtypeOf(constraint, cur.upperBound)) {
			// nothing to do

		// lowerBound <: T <: upperBound <: constraint => upperBound = constraint
		} else if (isSubtypeOf(cur.upperBound, constraint)) {
			cur.upperBound = constraint;

		} else {
			assert(false && "bug of isSubtypeOf");
		}
	} else {
		constraints_[typeVar] = TypeConstraint(/* lowerBound = */constraint);
	}

	DBG_PRINT(-, addTypeConstraint);
	return;
}

void TypeResolver::visit(TransUnit *tu) {
	DBG_PRINT(+, TransUnit);
	assert(tu != NULL);

	int cnt = 0;
	while (true) {
		cnt++;
		if (opt_.verbose) std::cout<<cnt<<"(st/nd/th) attempt to infer the types..."<<std::endl;
		unresolved_ = false;
		unresolvedPos_ = -1;

		incomplete_.clear();
		constraints_.clear();

		curFunc_ = NULL;

		DBG_PRINT(+, TransUnitStmts);
		for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
			(*it)->accept(this);
		}
		DBG_PRINT(-, TransUnitStmts);
		
		if (!unresolved_)
			break;

		bool changed = false;
		for (std::map<TypeVar, TypeConstraint>::iterator it = constraints_.begin();
								it != constraints_.end(); ) {
			if (incomplete_.count((*it).first)) {
				++it;
				continue;
			}

			changed = true;
			TypeVar tv = (*it).first;
			TypeConstraint& tc = (*it).second;

			if (tc.upperBound == NULL) {
				tc.upperBound = tc.lowerBound;
			}

			if (tc.takeLowerBound) {
				*tv = tc.lowerBound;
			} else {
				*tv = tc.upperBound;
			}

			constraints_.erase(it++);
		}

		if (!changed)
			break;
	}
	if (opt_.verbose) std::cout<<"TypeResolver ran "<<cnt<<" times."<<std::endl;

	if (unresolved_) {
		assert(unresolvedPos_ != -1);
		throw SemanticsError(unresolvedPos_,
				"error: cannot resolve the type of the expression, variable or function");
	}

	{
		// insert VarDefStmt for implicitly declared variables
		std::vector<Stmt *> varDefs;
		for (GlobalScope::iterator it = tu->scope->begin(); it != tu->scope->end(); ++it) {
			if ((*it)->getSymbolType() == Symbol::VAR_SYMBOL
				&& static_cast<VarSymbol *>(*it)->isImplicit) {
				VarDefStmt *vds =
					new VarDefStmt(Token(Token::KW_VAR, 0),
						new Identifier(Token(Token::ID, (*it)->getSymbolName(), 0)),
						NULL);
				vds->symbol = static_cast<VarSymbol *>(*it);
				vds->id->type = (*it)->getType();
				varDefs.push_back(vds);
			}
		}
		tu->stmts.insert(tu->stmts.begin(), varDefs.begin(), varDefs.end());
	}

	DBG_PRINT(-, TransUnit);
	return;
}

void TypeResolver::visit(FuncDefStmt *fds) {
	DBG_PRINT(+, FuncDefStmt);
	assert(fds != NULL);
	assert(fds->symbol != NULL);
	assert(fds->symbol->getType() != NULL);
	assert(fds->symbol->getType()->getTypeType() == Type::FUNC_TYPE);

	FuncSymbol *prevFunc_ = curFunc_;

	curFunc_ = static_cast<FuncSymbol *>(fds->symbol);

	assert(curFunc_->getType() != NULL);
	assert(curFunc_->getType()->getTypeType() == Type::FUNC_TYPE);

	FuncType *curFuncType = static_cast<FuncType *>(curFunc_->getType());

	std::vector<Identifier *>::iterator prmIt = fds->params.begin();
	FuncType::iterator ftIt = curFuncType->begin(Void_);

	for ( ; prmIt != fds->params.end() && ftIt != curFuncType->end(); ++prmIt, ++ftIt) {
		if (*ftIt == NULL) {
			unresolved_ = true;
			unresolvedPos_ = (*prmIt)->token.getPosition();
		}
		if (*ftIt != NULL && (*prmIt)->type == NULL) {
			(*prmIt)->symbol->setType((*prmIt)->type = *ftIt);
		}
	}

	assert(prmIt == fds->params.end());
	assert(ftIt == curFuncType->end());

	if (curFuncType->getReturnType() == NULL) {
		unresolved_ = true;
		unresolvedPos_ = fds->token.getPosition();
	}

	fds->body->accept(this);

	curFunc_ = prevFunc_;

	DBG_PRINT(-, FuncDefStmt);
	return;
}

void TypeResolver::visit(VarDefStmt *vds) {
	DBG_PRINT(+, VarDefStmt);
	assert(vds != NULL);
	assert(vds->id != NULL);

	if (vds->id->type == NULL)
		vds->id->type = vds->id->symbol->getType();

	Type *initType = NULL;
	TypeVar initTypeVar = NULL;
	if (vds->init != NULL) {
		vds->init->accept(this);
		vds->init = refresh(vds->init);
		if ((initType = vds->init->type) == NULL) {
			assert(unresolved_);
			initTypeVar = curTypeVar_;
			curTypeVar_ = NULL;
		}
	}
	
	// ex. : var foo :: String = "hi"
	if (vds->id->type != NULL && vds->init != NULL) {
		if (initType == NULL) {
			if (initTypeVar != NULL)
				addTypeConstraint(vds->id->type->unmodify(), initTypeVar);
		} else {
			assert(initType != NULL);
			assert(vds->id->type != NULL);
			if (!canPromote(initType, vds->id->type, vds->id->token.getPosition(), false)) {
				throw SemanticsError(vds->token.getPosition(),
						std::string("error: type of variable (")
						+ vds->id->type->getTypeName()
						+ std::string(") and initializer (")
						+ initType->getTypeName()
						+ std::string(") are incompatible"));
			}
			assert(vds->init->type != NULL);
			vds->init = insertPromoter(vds->init, vds->id->type);

			// cheap optimization
			//
			// rewrite such code:
			//	var str :: String = String()
			//like this:
			//	var str :: String

			if (vds->init->getASTType() == AST::CONSTRUCTOR_EXPR) {
				ConstructorExpr *ce = static_cast<ConstructorExpr *>(vds->init);
				if (ce->type->is(vds->id->type) && ce->params.size() == 0) {
					vds->init = NULL;
				}
			}
		}
	// ex. : var foo :: String
	} else if (vds->id->type != NULL && vds->init == NULL) {
		// nothing to do
		if (vds->id->type->getTypeType() == Type::MODIFIER_TYPE
			&& static_cast<ModifierType *>(vds->id->type)->isRef()) {
			throw SemanticsError(vds->token.getPosition(),
					"error: reference should be initialized at first");
		}
	// ex. : var foo = "hi"
	} else if (vds->id->type == NULL && vds->init != NULL) {
		if (initType == NULL) {
			// you have nothing to do
		} else {
			assert(initType != NULL);
			assert(initType->unmodify() != NULL);
			assert(vds->id != NULL);

			addTypeConstraint(initType->unmodify(), vds->id->symbol->getTypePtr());
			unresolved_ = true;
			unresolvedPos_ = vds->id->token.getPosition();
			curTypeVar_ = NULL;
			return;
		}
	} else if (vds->id->type == NULL && vds->init == NULL) {
		unresolved_ = true;
		unresolvedPos_ = vds->token.getPosition();
		curTypeVar_ = NULL;
		return;
	}

	if (vds->id->type != NULL && vds->id->type->getTypeType() == Type::NAMESPACE_TYPE) {
		throw SemanticsError(vds->token.getPosition(),
			std::string("error : cannot instantiate namespace type ")
			+ vds->id->type->getTypeName());
	}

	DBG_PRINT(-, VarDefStmt);
	return;
}

void TypeResolver::visit(InstStmt *is) {
	DBG_PRINT(+, InstStmt);
	assert(is != NULL);
	assert(is->inst != NULL);

	FuncType *curFuncType = NULL;
	{
		is->inst->accept(this);
		Type *instType = is->inst->type;
		if (instType == NULL) {
			assert(unresolved_);
			curTypeVar_ = NULL;
			return;
		}
		instType = instType->unmodify();
		if (instType->getTypeType() != Type::FUNC_TYPE) {
			throw SemanticsError(is->inst->token.getPosition(), "error: it is not function ("
				+ instType->getTypeName() + std::string(")"));
		}
		curFuncType = static_cast<FuncType *>(instType);
	}

	std::vector<Expr *> defaults;
	{
		Symbol *sym = NULL;
		if (is->inst->getASTType() == AST::IDENTIFIER) {
			sym = static_cast<Identifier *>(is->inst)->symbol;
		} else if (is->inst->getASTType() == AST::STATIC_MEMBER_EXPR) {
			sym = static_cast<StaticMemberExpr *>(is->inst)->member->symbol;
		}
		// TODO: make it able to deal with class members

		if (sym != NULL && sym->getSymbolType() == Symbol::FUNC_SYMBOL) {
			if (static_cast<FuncSymbol *>(sym)->defaults != NULL)
				defaults = *(static_cast<FuncSymbol *>(sym)->defaults);
		} else if (sym != NULL && sym->getSymbolType() == Symbol::EXTERN_SYMBOL) {
			if (static_cast<ExternSymbol *>(sym)->defaults != NULL)
				defaults = *(static_cast<ExternSymbol *>(sym)->defaults);
		}
	}

	assert(curFuncType != NULL);

	std::vector<Expr *>::iterator prmIt = is->params.begin();
	std::vector<Expr *>::iterator defIt = defaults.begin();
	FuncType::iterator ftIt = curFuncType->begin(Void_), ftEnd = curFuncType->end();

	while (prmIt != is->params.end() || ftIt != ftEnd) {
		if (prmIt != is->params.end() && ftIt == ftEnd) {
			// given parameters > parameters in the type
			throw SemanticsError((*prmIt)->token.getPosition(),
					"error: too many arguments in the function call");

		} else if (prmIt == is->params.end() && ftIt != ftEnd) {
			// given parameters < parameters in the type
			if (defIt != defaults.end() && *defIt != NULL) {
				// the default value exists
				is->params.push_back(*defIt);
				prmIt = is->params.end() - 1; // be careful that push_back may invalidate iterators
			} else {
				throw SemanticsError(is->token.getPosition(),
						"error: fewer arguments in the function call");
			}
		} else {
			assert(prmIt != is->params.end() && ftIt != ftEnd);

			// the given parameter is empty
			if (*prmIt == NULL) {
				// the default value exists
				if (defIt != defaults.end() && *defIt != NULL) {
					*prmIt = *defIt;
				} else {
					std::stringstream ss;
					ss<<"error: you cannot omit ";
					ss<<(prmIt - is->params.begin() + 1)<<"st/nd/th argument";
					throw SemanticsError(is->token.getPosition(), ss.str());
				}
			}
		}

		(*prmIt)->accept(this);
		*prmIt = refresh(*prmIt);

		Type *actual = (*prmIt)->type;

		// bidirectional checking
		if (actual == NULL && *ftIt == NULL) {
			// you have nothing to do
			assert(unresolved_);
			curTypeVar_ = NULL;
		} else if (actual == NULL && *ftIt != NULL) {
			assert(unresolved_);
			if (curTypeVar_ != NULL) {
				addTypeConstraint((*ftIt)->unmodify(), curTypeVar_);
				curTypeVar_ = NULL;
			}
		} else if (actual != NULL && *ftIt == NULL) {
			addTypeConstraint(actual->unmodify(), &(*ftIt));
			constraints_[&(*ftIt)].takeLowerBound = false;
			unresolved_ = true;
			curTypeVar_ = NULL;
		} else if (actual != NULL && *ftIt != NULL) {
			assert(actual != NULL);
			assert(*ftIt != NULL);
			if (!canPromote(actual, *ftIt, (*prmIt)->token.getPosition(), true)) {
				throw SemanticsError((*prmIt)->token.getPosition(),
						std::string("error: argument type (")
						+ actual->getTypeName()
						+ std::string(") doesn't match to the function's one (")
						+ (*ftIt)->getTypeName()
						+ std::string(")"));

			}

			assert((*prmIt)->type != NULL);

			*prmIt = insertPromoter(*prmIt, *ftIt);
		}

		if (prmIt != is->params.end()) ++prmIt;
		if (defIt != defaults.end()) ++defIt;
		if (ftIt != ftEnd) ++ftIt;
	}

	if (curFuncType->getReturnType() == NULL) {
		unresolved_ = true;
		curTypeVar_ = NULL;

		DBG_PRINT(-, InstStmt);
		return;
	}

	DBG_PRINT(-, InstStmt);
	return;
}

void TypeResolver::visit(AssignStmt *as) {
	DBG_PRINT(+, AssignStmt);
	assert(as != NULL);

	TypeVar typeVarLhs = NULL, typeVarRhs = NULL;

	as->lhs->accept(this);
	as->lhs = refresh(as->lhs);

	Type *lhsType = as->lhs->type;
	if (lhsType == NULL) {
		assert(unresolved_);
		typeVarLhs = curTypeVar_;
		curTypeVar_ = NULL;
	}

	Type *rhsType = NULL;

	if (as->rhs != NULL) {
		as->rhs->accept(this);
		as->rhs = refresh(as->rhs);

		rhsType = as->rhs->type;
		if (rhsType == NULL) {
			assert(unresolved_);
			typeVarRhs = curTypeVar_;
			curTypeVar_ = NULL;
		}
	}

	if (as->lhs != NULL && as->rhs != NULL) {
		if (lhsType == NULL && rhsType == NULL) {
			assert(unresolved_);
			curTypeVar_ = NULL;
			return;
		} else if (lhsType == NULL && rhsType != NULL) {
			assert(unresolved_);
			if (typeVarLhs != NULL) {
				addTypeConstraint(rhsType->unmodify(), typeVarLhs);
			}
			curTypeVar_ = NULL;
			return;
		} else if (lhsType != NULL && rhsType == NULL) {
			assert(unresolved_);
			if (typeVarRhs != NULL) {
				addTypeConstraint(lhsType->unmodify(), typeVarRhs);
			}
			curTypeVar_ = NULL;
			return;
		}
	} else if (as->lhs != NULL && as->rhs == NULL && lhsType == NULL) {
		// TODO: add type inference for ++ and --
		assert(unresolved_);
		curTypeVar_ = NULL;
		return;
	}

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
		ASTPrinter::dump(as->lhs);
		throw SemanticsError(as->token.getPosition(),
			std::string("error: left hand side ")
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
			assert(rhsType != NULL);
			assert(lhsType->unmodify() != NULL);
			if (!canPromote(rhsType, lhsType->unmodify(), as->rhs->token.getPosition(), false)) {
				throw SemanticsError(as->token.getPosition(),
					std::string("error: left hand side (")
					+ lhsType->getTypeName()
					+ std::string(") and right hand side (")
					+ rhsType->getTypeName()
					+ std::string(") should be same type"));
			}
			assert(as->rhs->type != NULL);
			as->rhs = insertPromoter(as->rhs, lhsType->unmodify());
			break;
		}
	default: assert(false);
	}

	DBG_PRINT(-, AssignStmt);
	return;
}

void TypeResolver::visit(CompStmt *cs) {
	DBG_PRINT(+, CompStmt);
	assert(cs != NULL);

	for (std::vector<Stmt *>::iterator it = cs->stmts.begin();
			it != cs->stmts.end(); ++it) {
		(*it)->accept(this);
	}

	DBG_PRINT(-, CompStmt);
	return;
}

void TypeResolver::visit(NamespaceStmt *ns) {
	DBG_PRINT(+, NamespaceStmt);
	assert(ns != NULL);

	for (std::vector<Stmt *>::iterator it = ns->stmts.begin();
			it != ns->stmts.end(); ++it) {
		(*it)->accept(this);
	}

	DBG_PRINT(-, NamespaceStmt);

}

void TypeResolver::visit(IfStmt *is) {
	DBG_PRINT(+, IfStmt);
	assert(is != NULL);

	for (int i = 0, len = is->ifCond.size(); i < len; ++i) {
		is->ifCond[i]->accept(this);
		is->ifCond[i] = refresh(is->ifCond[i]);

		Type *cur = is->ifCond[i]->type;
		if (cur == NULL) {
			assert(unresolved_);
			if (curTypeVar_ != NULL) {
				addTypeConstraint(Bool_, curTypeVar_);
				curTypeVar_ = NULL;
			}
		} else {
			if (!canPromote(cur, Bool_, is->ifCond[i]->token.getPosition(), false))
				throw SemanticsError(is->token.getPosition(), "error: condition should be Bool");
			is->ifCond[i] = insertPromoter(is->ifCond[i], Bool_);
		}

		is->ifThen[i]->accept(this);
	}

	if (is->elseThen != NULL)
		is->elseThen->accept(this);

	DBG_PRINT(-, IfStmt);
	return;
}

void TypeResolver::visit(RepeatStmt *rs) {
	DBG_PRINT(+, RepeatStmt);
	assert(rs != NULL);

	if (rs->count != NULL) {
		rs->count->accept(this);
		rs->count = refresh(rs->count);

		Type *cur = rs->count->type;
		if (cur == NULL) {
			assert(unresolved_);
			if (curTypeVar_ != NULL) {
				addTypeConstraint(Int_, curTypeVar_);
				curTypeVar_ = NULL;
			}
			return;
		}
		if (!canPromote(cur, Int_, rs->count->token.getPosition(), false))
			throw SemanticsError(rs->token.getPosition(), "error: repeat counter should be Int");
		rs->count = insertPromoter(rs->count, Int_);
	}

	for (std::vector<Stmt *>::iterator it = rs->stmts.begin();
			it != rs->stmts.end(); ++it) {
		(*it)->accept(this);
	}

	DBG_PRINT(-, RepeatStmt);
	return;
}

void TypeResolver::visit(GotoStmt *gs) {
	DBG_PRINT(+, GotoStmt);
	assert(gs != NULL);

	DBG_PRINT(-, GotoStmt);
	return;
}

void TypeResolver::visit(GosubStmt *gs) {
	DBG_PRINT(+, GosubStmt);
	assert(gs != NULL);

	DBG_PRINT(-, GosubStmt);
	return;
}

void TypeResolver::visit(ReturnStmt *rs) {
	DBG_PRINT(+, ReturnStmt);
	assert(rs != NULL);

	if (curFunc_ == NULL) {
		// this is global return
		if (rs->expr != NULL)
			throw SemanticsError(rs->token.getPosition(),
				"error: global return shouldn't have return value");
		return;
	}

	FuncType *ft = static_cast<FuncType *>(curFunc_->getType());
	assert(ft != NULL);

	Type *& retType = ft->getReturnType();

	if (retType == NULL) {
		unresolved_ = true;
		unresolvedPos_  = rs->token.getPosition();

		if (rs->expr != NULL) {
			rs->expr->accept(this);
			rs->expr = refresh(rs->expr);

			if (rs->expr->type != NULL) {
				addTypeConstraint(rs->expr->type->unmodify(), &retType);
			}
		}
	} else {
		if (retType->is(Void_)) {
			if (rs->expr != NULL)
				throw SemanticsError(rs->token.getPosition(), "error: the function returns Void");

			return;
		}

		if (rs->expr == NULL)
			throw SemanticsError(rs->token.getPosition(), "error: the function should return a value");


		rs->expr->accept(this);
		rs->expr = refresh(rs->expr);

		Type *cur = rs->expr->type;
		if (cur == NULL) {
			assert(unresolved_);
			if (curTypeVar_ != NULL) {
				addTypeConstraint(retType->unmodify(), curTypeVar_);
				curTypeVar_ = NULL;
			}
		} else {
			if (!canPromote(cur, retType, rs->expr->token.getPosition()))
				throw SemanticsError(rs->token.getPosition(),
						"error: type of the expression doesn't match the return type");

			rs->expr = insertPromoter(rs->expr, retType);
		}
	}
	DBG_PRINT(-, ReturnStmt);
	return;
}

void TypeResolver::visit(DerefExpr *de) {
	DBG_PRINT(+, DerefExpr);
	de->derefered->accept(this);
	DBG_PRINT(-, DerefExpr);
	return;
}

void TypeResolver::visit(RefExpr *re) {
	DBG_PRINT(+, RefExpr);
	re->refered->accept(this);
	DBG_PRINT(-, RefExpr);
	return;
}

void TypeResolver::visit(Identifier *id) {
	DBG_PRINT(+, Identifier);
	assert(id != NULL);
	assert(id->symbol != NULL);

	if (id->type != NULL) {
		return;
	}

	assert(id->symbol->getSymbolType() != Symbol::BUILTIN_TYPE_SYMBOL
		&& id->symbol->getSymbolType() != Symbol::CLASS_SYMBOL);

	// try to resolve type of symbol
	if ((id->type = id->symbol->getType()) == NULL) {
		unresolved_ = true;
		unresolvedPos_ = id->token.getPosition();
		curTypeVar_ = id->symbol->getTypePtr();
		return;
	}

	// all value references returns ref!
	bool isConst = false;
	if (id->type->getTypeType() == Type::MODIFIER_TYPE) {
		ModifierType *mt = static_cast<ModifierType *>(id->type);
		isConst = mt->isConst();
	}

	id->type = new ModifierType(isConst, /* isRef = */ true, id->type->unmodify());

	DBG_PRINT(-, Identifier);
	return;
}

void TypeResolver::visit(Label *label) {
	DBG_PRINT(+, Label);
	assert(label != NULL);

	if (label->type == NULL) {
		label->type = Label_;
	}

	assert(label->symbol != NULL);

	DBG_PRINT(-, Label);
	return;
}

void TypeResolver::visit(BinaryExpr *be) {
	DBG_PRINT(+, BinaryExpr);
	assert(be != NULL);

	be->lhs->accept(this);
	be->lhs = refresh(be->lhs);

	Type *lhsType = be->lhs->type;

	be->rhs->accept(this);
	be->rhs = refresh(be->rhs);

	Type *rhsType = be->rhs->type;

	if (lhsType == NULL || rhsType == NULL) {
		assert(unresolved_);
		curTypeVar_ = NULL;
		be->type = NULL;
		return;
	}

	if ((be->type = canPromoteBinary(lhsType->unmodify(), be->token.getType(), rhsType->unmodify())) != NULL) {
		assert(be != NULL);
		assert(be->lhs != NULL);
		assert(be->lhs->type != NULL);
		assert(be->rhs != NULL);
		assert(be->rhs->type != NULL);

		if (lhsType->is(lhsType->unmodify()) && rhsType->is(rhsType->unmodify())) {
			DBG_PRINT(-, BinaryExpr);
			be->type = new ModifierType(true, false, be->type);
			return;
		}

		// in HSP, the rhs is always casted to the lhs type
		if (!opt_.hspCompat && canPromote(lhsType, rhsType->unmodify(),
					be->lhs->token.getPosition(), false)) {
			// rhsType->unmodify() is wider than lhsType->unmodify().
			be->lhs = insertPromoter(be->lhs, rhsType->unmodify());
			be->rhs = insertPromoter(be->rhs, rhsType->unmodify());
		} else if (canPromote(rhsType, lhsType->unmodify(),
					be->lhs->token.getPosition(), false)) {
			// lhsType->unmodify() is wider than rhsType->unmodify().
			be->rhs = insertPromoter(be->rhs, lhsType->unmodify());
			be->lhs = insertPromoter(be->lhs, lhsType->unmodify());
		} else {
			std::cout<<lhsType->unmodify()->getTypeName();
			std::cout<<be->token.toString();
			std::cout<<rhsType->unmodify()->getTypeName()<<std::endl;
			assert(false && "no viable cast");
		}
		DBG_PRINT(-, BinaryExpr);
		be->type = new ModifierType(true, false, be->type);
		return;
	} else {
		throw SemanticsError(be->token.getPosition(),
			std::string("error: the operation between these two types is not allowed: ")
			+ lhsType->getTypeName() + " and " + rhsType->getTypeName()
			+ "; you may use casts");
	}

}

void TypeResolver::visit(UnaryExpr *ue) {
	DBG_PRINT(+, UnaryExpr);
	assert(ue != NULL);

	ue->rhs->accept(this);
	ue->rhs = refresh(ue->rhs);

	Type *rhsType = ue->rhs->type;

	if (rhsType == NULL) {
		assert(unresolved_);
		ue->type = NULL;
		return;
	}
	
	switch (ue->token.getType()) {
	case Token::PLUS:
	case Token::MINUS:
		if (!(rhsType->unmodify()->is(Int_)) && !(rhsType->unmodify()->is(Float_))
			&& !(rhsType->unmodify()->is(Double_)) && !(rhsType->unmodify()->is(Char_)))
			throw SemanticsError(ue->token.getPosition(), "error: right side should be numeric type");

		ue->rhs = insertPromoter(ue->rhs, rhsType->unmodify());
		DBG_PRINT(-, UnaryExpr);
		ue->type = rhsType->unmodify();
		return;
	case Token::EXCL:
		if (!(rhsType->unmodify()->is(Bool_)))
			throw SemanticsError(ue->token.getPosition(), "error: right side should be Bool");

		ue->rhs = insertPromoter(ue->rhs, rhsType->unmodify());
		DBG_PRINT(-, UnaryExpr);
		ue->type = rhsType->unmodify();
		return;
	default: ;
	}

	assert(false);
	return;
}

void TypeResolver::visit(IntLiteralExpr *lit) {
	DBG_PRINT(+-, IntLiteralExpr);
	assert(lit != NULL);
	if (lit->type == NULL)
		lit->type = new ModifierType(true, false, Int_);
	return;
}

void TypeResolver::visit(StrLiteralExpr *lit) {
	DBG_PRINT(+-, StrLiteralExpr);
	assert(lit != NULL);
	if (lit->type == NULL)
		lit->type = new ModifierType(true, false, String_);
	return;
}

void TypeResolver::visit(CharLiteralExpr *lit) {
	DBG_PRINT(+-, CharLiteralExpr);
	assert(lit != NULL);
	if (lit->type == NULL)
		lit->type = new ModifierType(true, false, Char_);
	return;
}

void TypeResolver::visit(FloatLiteralExpr *lit) {
	DBG_PRINT(+-, FloatLiteralExpr);
	assert(lit != NULL);
	if (lit->type == NULL)
		lit->type = new ModifierType(true, false, Double_);
	return;
}

void TypeResolver::visit(BoolLiteralExpr *lit) {
	DBG_PRINT(+-, BoolLiteralExpr);
	assert(lit != NULL);

	if (lit->type == NULL)
		lit->type = new ModifierType(true, false, Bool_);
	return;
}

void TypeResolver::visit(ArrayLiteralExpr *ale) {
	DBG_PRINT(+, ArrayLiteralExpr);
	assert(ale != NULL);

	if (ale->type != NULL)
		return;

	assert(ale->elements.size() > 0);

	// TODO: fix them to take the biggest type of the elements

	ale->elements[0]->accept(this);
	ale->elements[0] = refresh(ale->elements[0]);

	Type *elemType = ale->elements[0]->type->unmodify();
	if (elemType == NULL) {
		unresolved_ = true;
		unresolvedPos_ = ale->elements[0]->token.getPosition();
		ale->type = NULL;
		return;
		// throw SemanticsError(ale->token.getPosition(), "error: cannot infer type of array literal");
	}
	ale->elements[0] = insertPromoter(ale->elements[0], elemType);

	for (std::vector<Expr *>::iterator it = ale->elements.begin() + 1; it != ale->elements.end(); ++it) {
		(*it)->accept(this);
		*it = refresh(*it);

		Type *curElemType = (*it)->type;
		if (curElemType == NULL) {
			assert(unresolved_);
			if (curTypeVar_ != NULL) {
				addTypeConstraint(elemType->unmodify(), curTypeVar_);
				curTypeVar_ = NULL;
			}
			ale->type = NULL;
			return;
		}

		if (!canPromote(curElemType, elemType, (*it)->token.getPosition(), false)) {
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
	ale->type = new ModifierType(true, false, new ArrayType(elemType));
	return;
}

void TypeResolver::visit(FuncCallExpr *fce) {
	DBG_PRINT(+, FuncCallExpr);
	assert(fce != NULL);

	FuncType *curFuncType = NULL;
	{
		fce->func->accept(this);
		fce->func = refresh(fce->func);

		Type *tmp = fce->func->type;
		if (tmp == NULL) {
			assert(unresolved_);
			curTypeVar_ = NULL;
			fce->type = NULL;
			return;
		}
		tmp = tmp->unmodify();
		assert (tmp != NULL);
		if (tmp->getTypeType() != Type::FUNC_TYPE) {
			throw SemanticsError(fce->func->token.getPosition(), "error: it is not function");
		}
		curFuncType = static_cast<FuncType *>(tmp);
	}

	std::vector<Expr *>::iterator prmIt = fce->params.begin(), prmEnd = fce->params.end();
	FuncType::iterator ftIt = curFuncType->begin(Void_), ftEnd = curFuncType->end();
	for (; prmIt != prmEnd && ftIt != ftEnd; ++prmIt, ++ftIt) {
		(*prmIt)->accept(this);
		*prmIt = refresh(*prmIt);
		Type *argType = (*prmIt)->type;

		if (argType == NULL && *ftIt == NULL) {
			// you have nothing to do
			assert(unresolved_);
			curTypeVar_ = NULL;
		} else if (argType == NULL && *ftIt != NULL) {
			assert(unresolved_);
			if (curTypeVar_ != NULL) {
				addTypeConstraint((*ftIt)->unmodify(), curTypeVar_);
				curTypeVar_ = NULL;
			}
		} else if (argType != NULL && *ftIt == NULL) {
			addTypeConstraint(argType->unmodify(), &(*ftIt));
			constraints_[&(*ftIt)].takeLowerBound = false;
			unresolved_ = true;
			curTypeVar_ = NULL;
		} else if (argType != NULL && *ftIt != NULL) {
			if (!canPromote(argType, *ftIt, (*prmIt)->token.getPosition())) {
				throw SemanticsError((*prmIt)->token.getPosition(),
						std::string("error: argument type (")
						+ argType->getTypeName()
						+ std::string(") doesn't match to the function's one (")
						+ (*ftIt)->getTypeName()
						+ std::string(")"));
			}

			*prmIt = insertPromoter(*prmIt, *ftIt);
		}
	}

	if (ftIt != ftEnd) {
		throw SemanticsError(fce->token.getPosition(),
				"error: fewer arguments in the function call");
	}

	if (prmIt != prmEnd) {
		throw SemanticsError((*prmIt)->token.getPosition(),
				"error: too many arguments in the function call");
	}

	if (curFuncType->getReturnType() == NULL) {
		unresolved_ = true;
		unresolvedPos_ = fce->func->token.getPosition();
		curTypeVar_ = &(curFuncType->getReturnType());

		DBG_PRINT(-, FuncCallExpr);
		fce->type = NULL;
	} else {
		fce->type = curFuncType->getReturnType();
	}

	if (fce->type != NULL && fce->type->is(Void_)) {
		throw SemanticsError(fce->token.getPosition(), "error: the function doesn't return a value");
	}

	DBG_PRINT(-, FuncCallExpr);
	return;
}

void TypeResolver::visit(ConstructorExpr *ce) {
	DBG_PRINT(+, ConstructorExpr);
	assert(ce != NULL);
	assert(ce->constructor != NULL);

	for (std::vector<Expr *>::iterator it = ce->params.begin();
			it != ce->params.end(); ++it) {
		(*it)->accept(this);
		*it = refresh(*it);

		if ((*it)->type == NULL) {
			assert(unresolved_);
			curTypeVar_ = NULL;
			ce->type = NULL;
			return;
		}
	}

	ce->type = ce->constructor->type;

	assert(ce->type != NULL);

	if ((ce->params.size() == 0)
		|| (ce->params.size() == 1 && ce->params[0]->type->unmodify()->is(ce->type))) {
		// Type()
		// Type(Type)
		// TODO: add optimization there from LLVMCodeGen and check the type
	} else if (ce->params.size() == 1 &&
			ce->type->getTypeType() == Type::BUILTIN_TYPE &&
			promotionTable.count(PromotionKey(ce->params[0]->type->unmodify(), ce->type)) > 0) {
		// TypeA(TypeB) where TypeB can promote to TypeA
		ce->params[0] = insertPromoter(ce->params[0], ce->params[0]->type->unmodify());
	} else if (ce->params.size() == 1 &&
			ce->type->getTypeType() == Type::ARRAY_TYPE &&
			ce->params[0]->type->unmodify()->is(Int_)) {
		// [Type](length)
		ce->params[0] = insertPromoter(ce->params[0], Int_);
	} else if (ce->params.size() == 2 &&
			ce->type->getTypeType() == Type::ARRAY_TYPE &&
			ce->params[0]->type->unmodify()->is(Int_) &&
			ce->params[1]->type->unmodify()->is(
				static_cast<ArrayType *>(ce->type)->getElemType()->unmodify())) {
		// [Type](length, initializer)
		ce->params[0] = insertPromoter(ce->params[0], Int_);
		ce->params[1] = insertPromoter(ce->params[1],
					static_cast<ArrayType *>(ce->type)->getElemType()->unmodify());
	} else {
		// [class] TODO: Support class
		throw SemanticsError(ce->token.getPosition(),
			std::string("error: Unknown constructor type (")
			+ ce->type->getTypeName()
			+ ")");
	}

	// cheap optimization
	//
	// rewrite such code:
	// 	var str :: String = String("this is string!")
	//
	// like this:
	// 	var str :: String = "this is string!"

	{
		Expr *rewriteWith = ce;
		while (true) {
			if (rewriteWith->getASTType() != AST::CONSTRUCTOR_EXPR)
				break;

			ConstructorExpr *ce = static_cast<ConstructorExpr *>(rewriteWith);

			if (ce->type->getTypeType() != Type::BUILTIN_TYPE)
				break;

			if (ce->params.size() == 1
					&& ce->params[0]->type->unmodify()->is(ce->type->unmodify())) {
				rewriteWith = ce->params[0];
			} else {
				break;
			}
		}

		rewrite(rewriteWith);
	}

	DBG_PRINT(-, ConstructorExpr);
	return;
}

void TypeResolver::visit(SubscrExpr *se) {
	DBG_PRINT(+, SubscrExpr);
	assert(se != NULL);

	if (se->type != NULL)
		return;

	se->array->accept(this);
	se->array = refresh(se->array);

	Type *arrayType = (se->array)->type;
	if (arrayType == NULL) {
		assert(unresolved_);
		curTypeVar_ = NULL;
		se->type = NULL;
		return;
	}

	if (arrayType->unmodify()->getTypeType() != Type::ARRAY_TYPE)
		throw SemanticsError(se->token.getPosition(), "error: giving subscript to non-array type");

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

	se->subscript->accept(this);
	se->subscript = refresh(se->subscript);

	Type *subscriptType = (se->subscript)->type;
	if (subscriptType == NULL) {
		assert(unresolved_);
		if (curTypeVar_ != NULL) {
			addTypeConstraint(Int_, curTypeVar_);
			curTypeVar_ = NULL;
		}
		se->type = NULL;
		return;
	}
	if (!(subscriptType->unmodify()->is(Int_)))
		throw SemanticsError(se->token.getPosition(), "error: subscript should be Int");

	se->subscript = insertPromoter(se->subscript, Int_);

	se->type = static_cast<ArrayType *>(arrayType->unmodify())->getElemType();

	se->type = new ModifierType(isConst || !isRef, true, se->type->unmodify());

	DBG_PRINT(-, SubscrExpr);
	return;
}

void TypeResolver::visit(MemberExpr *me) {
	DBG_PRINT(+, MemberExpr);
	assert(me != NULL);

	me->receiver->accept(this);
	me->receiver = refresh(me->receiver);

	if (me->receiver->type == NULL) {
		assert(unresolved_);

		if (opt_.hspCompat) {
			// in HSP compatible mode, the member expression means reference to the array element
			if (curTypeVar_ != NULL && me->type != NULL) {
				*curTypeVar_ = new ArrayType(me->type);
				curTypeVar_ = NULL;
				me->type = NULL;
			} else {
				curTypeVar_ = &(me->type);
			}
		} else {
			// TODO: there will be the most important part of the type inference
			curTypeVar_ = NULL;
		}
		me->type = NULL;
		return;
	}

	if (me->receiver->type->unmodify()->getTypeType() != Type::CLASS_TYPE
		&& !(me->receiver->type->unmodify()->is(String_))
		&& me->receiver->type->unmodify()->getTypeType() != Type::ARRAY_TYPE) {
		throw SemanticsError(me->token.getPosition(),
			std::string("error: invalid receiver type (")
			+ me->receiver->type->getTypeName() + ")");
	}

	if (me->receiver->type->getTypeType() != Type::MODIFIER_TYPE || !(static_cast<ModifierType *>(me->receiver->type)->isRef())) {
		Type *unmodified = me->receiver->type;
		bool isConst = false;
		if (unmodified->getTypeType() == Type::MODIFIER_TYPE) {
			isConst = static_cast<ModifierType *>(unmodified)->isConst();
			unmodified = static_cast<ModifierType *>(unmodified)->getElemType();
		}
		me->receiver = insertPromoter(me->receiver, new ModifierType(isConst, true, unmodified));
	}

	assert(me->receiver->type->unmodify()->getTypeType() != Type::CLASS_TYPE && "class not supported");

	assert(me->member->getASTType() == AST::IDENTIFIER);

	if (me->receiver->type->unmodify()->is(String_)) {
		if (me->member->getString() == "length") {
			me->type = new ModifierType(true, false, Int_);
		} else {
			throw SemanticsError(me->token.getPosition(),
				std::string("error: invalid member \"") + me->member->getString()
				+ "\" for type " + me->receiver->type->getTypeName());
		}
	} else if (me->receiver->type->unmodify()->getTypeType() == Type::ARRAY_TYPE) {
		if (me->member->getString() == "length") {
			me->type = new ModifierType(true, true, Int_);
		} else if (me->member->getString() == "resize") {
			me->type = new FuncType(Int_, Void_);
		} else {
			if (opt_.hspCompat) {
				// rewrite the whole expression as an array reference
				Expr *array = me->receiver;
				Expr *subscr = me->member;
				Token token = me->token;

				Expr *rewriteWith = new SubscrExpr(array, token, subscr);
				rewriteWith->type = NULL;

				rewriteWith->accept(this);
				rewriteWith = refresh(rewriteWith);
				rewrite(rewriteWith);

				return;
			} else {
				throw SemanticsError(me->token.getPosition(),
					std::string("error: invalid member \"") + me->member->getString()
					+ "\" for type " + me->receiver->type->getTypeName());
			}
		}
	}

	DBG_PRINT(-, MemberExpr);

	return;
}

void TypeResolver::visit(StaticMemberExpr *sme) {
	DBG_PRINT(+, StaticMemberExpr);
	assert(sme != NULL);

	assert(sme->receiver->type->getTypeType() == Type::NAMESPACE_TYPE
		|| sme->receiver->type->getTypeType() == Type::CLASS_TYPE);

	assert(sme->receiver->type->getTypeType() != Type::CLASS_TYPE && "class not supported");

	assert(sme->member->getASTType() == AST::IDENTIFIER);

	if (sme->member->symbol == NULL) {
		if (sme->receiver->type->getTypeType() == Type::NAMESPACE_TYPE) {
			NamespaceSymbol *ns = static_cast<NamespaceSymbol *>(sme->receiver->type);
			Symbol *symbol = ns->resolveMember(sme->member->getString(), sme->member->token.getPosition());
			if (symbol == NULL) {
				throw SemanticsError(sme->member->token.getPosition(),
					std::string("error : unknown member ") + sme->member->getString());
			}

			sme->member->symbol = symbol;
		}
	}

	if (sme->member->symbol->getType() == NULL) {
		unresolved_ = true;
		unresolvedPos_ = sme->member->token.getPosition();
		curTypeVar_ = sme->member->symbol->getTypePtr();
		sme->type = NULL;
	} else {
		sme->member->type = sme->member->symbol->getType();
		sme->type = sme->member->symbol->getType();
	}

	DBG_PRINT(-, StaticMemberExpr);
	return;
}

}

