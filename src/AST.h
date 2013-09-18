#ifndef PERYAN_AST_H__
#define PERYAN_AST_H__

#include <vector>
#include <sstream>

#include "Token.h"
#include "SymbolTable.h"
#include "ASTVisitor.h"

namespace Peryan {

class AST {
public:
	typedef enum {
		STMT,
		TRANS_UNIT,
		TYPE_SPEC,
		ARRAY_TYPE_SPEC,
		FUNC_TYPE_SPEC,
		MEMBER_TYPE_SPEC,
		EXPR,
		IDENTIFIER,
		BINARY_EXPR,
		UNARY_EXPR,
		STR_LITERAL_EXPR,
		INT_LITERAL_EXPR,
		FLOAT_LITERAL_EXPR,
		CHAR_LITERAL_EXPR,
		BOOL_LITERAL_EXPR,
		ARRAY_LITERAL_EXPR,
		FUNC_CALL_EXPR,
		CONSTRUCTOR_EXPR,
		SUBSCR_EXPR,
		MEMBER_EXPR,
		REF_EXPR,
		DEREF_EXPR,
		FUNC_EXPR,
		STATIC_MEMBER_EXPR,
		COMP_STMT,
		FUNC_DEF_STMT,
		VAR_DEF_STMT,
		INST_STMT,
		ASSIGN_STMT,
		IF_STMT,
		REPEAT_STMT,
		LABEL,
		LABEL_STMT,
		GOTO_STMT,
		GOSUB_STMT,
		CONTINUE_STMT,
		BREAK_STMT,
		RETURN_STMT,
		EXTERN_STMT,
		NAMESPACE_STMT
	} ASTType;

	virtual AST::ASTType getASTType() = 0;
	virtual void accept(ASTVisitor *visitor) = 0;

	Token token;

	AST() {}
	AST(const Token& token) : token(token) {}

	virtual ~AST() {};
};

class Stmt : public AST {
public:
	virtual AST::ASTType getASTType() { return STMT; }

	Stmt() : AST() {}
	Stmt(const Token& token) : AST(token) {}
};

class TransUnit : public AST {
public:
	virtual AST::ASTType getASTType() { return TRANS_UNIT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TransUnit() : AST(), scope(NULL) {}
	std::vector<Stmt *> stmts;

	GlobalScope *scope;
};

class TypeSpec : public AST {
public:
	virtual AST::ASTType getASTType() { return TYPE_SPEC; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	bool isConst;
	bool isRef;
	TypeSpec(bool isConst, bool isRef)
		: AST(), isConst(isConst), isRef(isRef), type(NULL) {}

	TypeSpec(const Token& token)
		: AST(token), isConst(false), isRef(false), type(NULL) {}

	TypeSpec(const Token& token, bool isConst, bool isRef)
		: AST(token), isConst(isConst), isRef(isRef), type(NULL) {}

	Type *type;
};

class MemberTypeSpec : public TypeSpec {
public:
	virtual AST::ASTType getASTType() { return MEMBER_TYPE_SPEC; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TypeSpec *lhs;
	Token rhs;
	MemberTypeSpec(TypeSpec *lhs, const Token& token, const Token& rhs, bool isConst, bool isRef)
		: TypeSpec(token, isConst, isRef), lhs(lhs), rhs(rhs) {}
};

class ArrayTypeSpec : public TypeSpec {
public:
	virtual AST::ASTType getASTType() { return ARRAY_TYPE_SPEC; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TypeSpec *typeSpec;

	ArrayTypeSpec(bool isConst, bool isRef, TypeSpec *typeSpec)
		: TypeSpec(isConst, isRef), typeSpec(typeSpec) {}
};


class FuncTypeSpec : public TypeSpec {
public:
	virtual AST::ASTType getASTType() { return FUNC_TYPE_SPEC; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TypeSpec *lhs, *rhs;

	FuncTypeSpec(bool isConst, bool isRef, TypeSpec *lhs, TypeSpec *rhs)
		: TypeSpec(isConst, isRef), lhs(lhs), rhs(rhs) {}
};


class Expr : public AST {
public:
	virtual AST::ASTType getASTType() { return EXPR; }

	Type *type;

	Expr() : AST(), type(NULL) {}
	Expr(const Token& token) : AST(token), type(NULL) {}
};

class Identifier : public Expr {
public:
	virtual AST::ASTType getASTType() { return IDENTIFIER; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TypeSpec *typeSpec;

	std::string getString() { return token.getString(); }

	Identifier(const Token& token) : Expr(token), typeSpec(NULL), symbol(NULL) {}

	Symbol *symbol;
};

class BinaryExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return BINARY_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *lhs, *rhs;
	BinaryExpr(Expr *lhs, const Token& token, Expr *rhs) : Expr(token), lhs(lhs), rhs(rhs) {}
};

class UnaryExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return UNARY_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *rhs;
	UnaryExpr(const Token& token, Expr *rhs) : Expr(token), rhs(rhs) {}
};

class StrLiteralExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return STR_LITERAL_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	std::string str;
	StrLiteralExpr(const Token& token) : Expr(token), str(token.getString()) {}
};

class IntLiteralExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return INT_LITERAL_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	int integer;
	IntLiteralExpr(const Token& token) : Expr(token), integer(token.getInteger()) {}
};

class FloatLiteralExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return FLOAT_LITERAL_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	double float_;
	FloatLiteralExpr(const Token& token) : Expr(token), float_(token.getFloat()) {}
};

class CharLiteralExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return CHAR_LITERAL_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	char char_;
	CharLiteralExpr(const Token& token) : Expr(token), char_(token.getChar()) {}
};

class BoolLiteralExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return BOOL_LITERAL_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	bool bool_;
	BoolLiteralExpr(const Token& token) : Expr(token), bool_(token.getType() == Token::KW_TRUE) {}
};

class ArrayLiteralExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return ARRAY_LITERAL_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	std::vector<Expr *> elements;

	ArrayLiteralExpr(const Token& token) : Expr(token) {}
};

class FuncCallExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return FUNC_CALL_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *func;
	std::vector<Expr *> params;
	FuncCallExpr(const Token& token, Expr *func) : Expr(token), func(func), partial(false) {}

	bool partial;
};

class ConstructorExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return CONSTRUCTOR_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TypeSpec *constructor;
	std::vector<Expr *> params;
	ConstructorExpr(const Token& token, TypeSpec *constructor) : Expr(token), constructor(constructor) {}
};

class RefExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return REF_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }
	
	Expr *refered;
	RefExpr(Expr *refered) : Expr(refered->token), refered(refered) {}
};

class DerefExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return DEREF_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }
	
	Expr *derefered;
	DerefExpr(Expr *derefered) : Expr(derefered->token), derefered(derefered) {}
};

class SubscrExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return SUBSCR_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *array;
	Expr *subscript;

	SubscrExpr(Expr *array, const Token& token, Expr *subscript)
		: Expr(token), array(array), subscript(subscript) {}
};

class MemberExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return MEMBER_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *receiver;
	Identifier *member;
	MemberExpr(Expr *receiver, const Token& token, Identifier *member)
		: Expr(token), receiver(receiver), member(member) {}
};

class StaticMemberExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return STATIC_MEMBER_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TypeSpec *receiver;
	Identifier *member;
	StaticMemberExpr(TypeSpec *receiver, const Token& token, Identifier *member)
		: Expr(token), receiver(receiver), member(member) {}
};

class CompStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return COMP_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	std::vector<Stmt *> stmts;
	CompStmt(const Token& token) : Stmt(token) {}

	LocalScope *scope;
};

class FuncExpr : public Expr {
public:
	virtual AST::ASTType getASTType() { return FUNC_EXPR; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	std::vector<Identifier *> params;
	TypeSpec *retTypeSpec;
	CompStmt *body;
	FuncExpr(const Token& token, CompStmt *body) : Expr(token), body(body) {}
};

class FuncDefStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return FUNC_DEF_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Identifier *name;
	std::vector<Identifier *> params;
	TypeSpec *retTypeSpec;
	CompStmt *body;
	FuncDefStmt(const Token& token, Identifier *name, CompStmt *body)
		: Stmt(token), name(name), body(body), symbol(NULL) {}

	std::vector<Expr *> defaults;

	FuncSymbol *symbol;
};

class VarDefStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return VAR_DEF_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Identifier *id;
	Expr *init;
	VarDefStmt(const Token& token, Identifier *id, Expr *init)
		: Stmt(token), id(id), init(init), symbol(NULL) {}

	VarSymbol *symbol;
};

class InstStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return INST_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *inst;
	std::vector<Expr *> params;
	InstStmt(const Token& token, Expr *inst) : Stmt(token), inst(inst) {}
};


class AssignStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return ASSIGN_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *lhs;
	Expr *rhs;
	AssignStmt(Expr *lhs, const Token& token, Expr *rhs)
		: Stmt(token), lhs(lhs), rhs(rhs) {}
};

class IfStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return IF_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	std::vector<Expr *> ifCond;
	std::vector<CompStmt *> ifThen;
	CompStmt *elseThen;
	IfStmt(const Token& token, std::vector<Expr *> ifCond, std::vector<CompStmt *> ifThen, CompStmt *elseThen)
		: Stmt(token), ifCond(ifCond), ifThen(ifThen), elseThen(elseThen) {}
};

class RepeatStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return REPEAT_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *count;
	std::vector<Stmt *> stmts;
	RepeatStmt(const Token& token, Expr *count)
		: Stmt(token), count(count), scope(NULL) {}

	LocalScope *scope;
};

class Label : public Expr {
public:
	virtual AST::ASTType getASTType() { return LABEL; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Label(const Token& token) : Expr(token), symbol(NULL) {}

	LabelSymbol *symbol;
};

class LabelStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return LABEL_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Label *label;
	LabelStmt(const Token& token, Label *label) : Stmt(token), label(label) {}
};


class GotoStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return GOTO_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Label *to;
	GotoStmt(const Token& token, Label *to) : Stmt(token), to(to) {}
};

class GosubStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return GOSUB_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Label *to;
	GosubStmt(const Token& token, Label *to) : Stmt(token), to(to) {}
};

class ContinueStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return CONTINUE_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	ContinueStmt(const Token& token) : Stmt(token) {}
};

class BreakStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return BREAK_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	BreakStmt(const Token& token) : Stmt(token) {}
};

class ReturnStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return RETURN_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Expr *expr;

	ReturnStmt(const Token& token, Expr *expr)
		: Stmt(token), expr(expr) {}
};

class ExternStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return EXTERN_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	Identifier *id;

	ExternStmt(const Token& token, Identifier *id)
		: Stmt(token), id(id), symbol(NULL) {}

	std::vector<Expr *> defaults;

	ExternSymbol *symbol;
};

class NamespaceStmt : public Stmt {
public:
	virtual AST::ASTType getASTType() { return NAMESPACE_STMT; }
	virtual void accept(ASTVisitor *visitor) { return visitor->visit(this); }

	TypeSpec *name;

	NamespaceStmt(const Token& token, TypeSpec *name)
		: Stmt(token), name(name), symbol(NULL) {}

	std::vector<Stmt *> stmts;

	NamespaceSymbol *symbol;
};

}

#endif

