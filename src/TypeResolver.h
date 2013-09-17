#ifndef PERYAN_TYPE_RESOLVER_H__
#define PERYAN_TYPE_RESOLVER_H__

#include <set>
#include <map>

#include "SymbolTable.h"
#include "Token.h"
#include "AST.h"
#include "Options.h"
#include "WarningPrinter.h"

namespace Peryan {

class TypeResolver {
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
	void visit(TransUnit *tu);
	void visit(Stmt *stmt);
	void visit(FuncDefStmt *fds);
	void visit(VarDefStmt *vds);
	void visit(InstStmt *is);
	void visit(AssignStmt *as);
	void visit(CompStmt *cs);
	void visit(IfStmt *is);
	void visit(RepeatStmt *rs);
	void visit(GotoStmt *gs);
	void visit(GosubStmt *gs);
	void visit(ReturnStmt *rs);
	void visit(NamespaceStmt *ns);
	void visit(Expr* expr);
	void visit(Identifier *id);
	void visit(Label *label);
	void visit(BinaryExpr *be);
	void visit(UnaryExpr *ue);
	void visit(IntLiteralExpr *lit);
	void visit(StrLiteralExpr *lit);
	void visit(CharLiteralExpr *lit);
	void visit(FloatLiteralExpr *lit);
	void visit(BoolLiteralExpr *lit);
	void visit(ArrayLiteralExpr *ale);
	void visit(FuncCallExpr *fce);
	void visit(ConstructorExpr *ce);
	void visit(SubscrExpr *se);
	void visit(MemberExpr *me);
	void visit(StaticMemberExpr *sme);
	void visit(DerefExpr *de);
	void visit(RefExpr *re);
};

};

#endif
