#ifndef PERYAN_TYPE_RESOLVER_H__
#define PERYAN_TYPE_RESOLVER_H__

#include <set>
#include <map>

#include "SymbolTable.h"
#include "Token.h"
#include "AST.h"
#include "Options.h"

namespace Peryan {

class TypeResolver {
private:
	TypeResolver(const TypeResolver&);
	TypeResolver& operator=(const TypeResolver&);

private:

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

	SymbolTable& symbolTable_;

	Type *Int_, *String_, *Char_, *Float_, *Double_, *Bool_, *Label_, *Void_;

	FuncSymbol *curFunc_;

	bool canPromote(Type *from, Type *to, bool isFuncParam = false);

	bool canConvertModifier(Type *from, Type *to, bool isFuncParam = false);
	bool isSubtypeOf(Type *sub, Type *super);

	Expr *insertPromoter(Expr *from, Type *toType);

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

	std::map<BinaryPromotionKey, Type *> binaryPromotionTable;

	void initPromotionTable();
	void initBinaryPromotionTable();

	Options& opt_;
public:
	TypeResolver(SymbolTable& symbolTable, Options& opt);
	void visit(TransUnit *tu) throw (SemanticsError);
	void visit(Stmt *stmt) throw (SemanticsError);
	void visit(FuncDefStmt *fds) throw (SemanticsError);
	void visit(VarDefStmt *vds) throw (SemanticsError);
	void visit(InstStmt *is) throw (SemanticsError);
	void visit(AssignStmt *as) throw (SemanticsError);
	void visit(CompStmt *cs) throw (SemanticsError);
	void visit(IfStmt *is) throw (SemanticsError);
	void visit(RepeatStmt *rs) throw (SemanticsError);
	void visit(GotoStmt *gs) throw (SemanticsError);
	void visit(GosubStmt *gs) throw (SemanticsError);
	void visit(ReturnStmt *rs) throw (SemanticsError);
	void visit(NamespaceStmt *ns) throw (SemanticsError);
	Type *visit(Expr *expr) throw (SemanticsError);
	Type *visit(Identifier *id) throw (SemanticsError);
	Type *visit(Label *label) throw (SemanticsError);
	Type *visit(BinaryExpr *be) throw (SemanticsError);
	Type *visit(UnaryExpr *ue) throw (SemanticsError);
	Type *visit(IntLiteralExpr *lit) throw (SemanticsError);
	Type *visit(StrLiteralExpr *lit) throw (SemanticsError);
	Type *visit(CharLiteralExpr *lit) throw (SemanticsError);
	Type *visit(FloatLiteralExpr *lit) throw (SemanticsError);
	Type *visit(BoolLiteralExpr *lit) throw (SemanticsError);
	Type *visit(ArrayLiteralExpr *ale) throw (SemanticsError);
	Type *visit(FuncCallExpr *fce) throw (SemanticsError);
	Type *visit(ConstructorExpr *ce) throw (SemanticsError);
	Type *visit(SubscrExpr *se) throw (SemanticsError);
	Type *visit(MemberExpr *me) throw (SemanticsError);
	Type *visit(StaticMemberExpr *sme) throw (SemanticsError);
	Type *visit(DerefExpr *de) throw (SemanticsError);
	Type *visit(RefExpr *re) throw (SemanticsError);
};

};

#endif
