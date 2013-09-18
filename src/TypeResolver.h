#ifndef PERYAN_TYPE_RESOLVER_H__
#define PERYAN_TYPE_RESOLVER_H__

#include <set>
#include <map>

#include "SymbolTable.h"
#include "Token.h"
#include "AST.h"
#include "ASTVisitor.h"
#include "Options.h"
#include "WarningPrinter.h"

namespace Peryan {

class TypeResolver : public ASTVisitor {
private:
	TypeResolver(const TypeResolver&);
	TypeResolver& operator=(const TypeResolver&);

	SymbolTable& symbolTable_;
	Options& opt_;
	WarningPrinter& wp_;

	bool changed_, unresolved_;
	Position unresolvedPos_;

	typedef Type** TypeVar;

	class TypeConstraint {
	public:
		// lowerBound <: T <: upperBound ( = by default, NULL)
		Type *lowerBound;
		Type *upperBound;
		bool takeLowerBound;
		// true for function return type and variable
		// false for function parameters
		TypeConstraint() : lowerBound(NULL), upperBound(NULL), takeLowerBound(true) {}
		TypeConstraint(Type *lowerBound)
			: lowerBound(lowerBound), upperBound(NULL), takeLowerBound(true) {}
	};

	std::map<TypeVar, TypeConstraint> constraints_;
	std::set<TypeVar> incomplete_;
	TypeVar curTypeVar_;
	void addTypeConstraint(Type *constraint, TypeVar typeVar);


	Type *Int_, *String_, *Char_, *Float_, *Double_, *Bool_, *Label_, *Void_;

	FuncSymbol *curFunc_;

	bool canPromote(Type *from, Type *to, Position pos, bool isFuncParam = false);
	Type *canPromoteBinary(Type *lhsType, Token::Type tokenType, Type* rhsType);

	bool canConvertModifier(Type *from, Type *to, bool isFuncParam = false);
	bool isSubtypeOf(Type *sub, Type *super);

	Expr *insertPromoter(Expr *from, Type *toType);

	class PromotionKey {
	public:
		Type *from, *to;
		PromotionKey(Type *from, Type *to) : from(from), to(to) {}

		bool operator<(const PromotionKey& p) const {
			if (from != p.from) {
				return from < p.from;
			} else {
				return to < p.to;
			}
		}
	};

	class BinaryPromotionKey {
	public:
		Type *from, *to;
		Token::Type type;
		BinaryPromotionKey(Type *from, Token::Type type, Type *to)
			: from(from), to(to), type(type) {}

		// it seems verbose but I think it's better than doing it with hard-reading way
		bool operator<(const BinaryPromotionKey& p) const {
			if (from != p.from) {
				return from < p.from;
			} else {
				if (type != p.type) {
					return type < p.type;
				} else {
					return to < p.to;
				}
			}
		}
	};

	std::set<PromotionKey> promotionTable;
	std::map<BinaryPromotionKey, Type *> binaryPromotionTable;

	void initPromotionTable();

	void initBinaryPromotionTable();

	Expr *rewriteWith_;
	Expr *refresh(Expr *from) {
		if (rewriteWith_ == NULL) {
			return from;
		} else {
			from = rewriteWith_;
			rewriteWith_ = NULL;
			return from;
		}
	}
	void rewrite(Expr *rewriteWith) {
		rewriteWith_ = rewriteWith;
		return;
	}
public:
	TypeResolver(SymbolTable& symbolTable, Options& opt, WarningPrinter& wp);
	virtual void visit(TransUnit *tu);
	virtual void visit(FuncDefStmt *fds);
	virtual void visit(VarDefStmt *vds);
	virtual void visit(InstStmt *is);
	virtual void visit(AssignStmt *as);
	virtual void visit(CompStmt *cs);
	virtual void visit(IfStmt *is);
	virtual void visit(RepeatStmt *rs);
	virtual void visit(GotoStmt *gs);
	virtual void visit(GosubStmt *gs);
	virtual void visit(ReturnStmt *rs);
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
	virtual void visit(ConstructorExpr *ce);
	virtual void visit(SubscrExpr *se);
	virtual void visit(MemberExpr *me);
	virtual void visit(StaticMemberExpr *sme);
	virtual void visit(DerefExpr *de);
	virtual void visit(RefExpr *re);

	virtual void visit(LabelStmt *ls)	{ return; }
	virtual void visit(ExternStmt *es)	{ return; }
	virtual void visit(ContinueStmt *cs)	{ return; }
	virtual void visit(BreakStmt *bs)	{ return; }
	virtual void visit(TypeSpec *ts)	{ return; }
	virtual void visit(ArrayTypeSpec *ats)	{ return; }
	virtual void visit(FuncTypeSpec *fts)	{ return; }
	virtual void visit(MemberTypeSpec *mts)	{ return; }
	virtual void visit(FuncExpr *de)	{ return; }
};

};

#endif
