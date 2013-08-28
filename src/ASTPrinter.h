#ifndef PERYAN_AST_PRINTER_H__
#define PERYAN_AST_PRINTER_H__

#include "AST.h"

namespace Peryan {

class ASTPrinter {
private:
	ASTPrinter(const ASTPrinter&);
	ASTPrinter& operator=(const ASTPrinter&);
	bool pretty_;
	int depth_;
	// indent
	std::string ind() {
		if (pretty_) {
			return std::string("\n") + std::string(depth_, ' ');
		} else {
			return " ";
		}
	}
	void inc() {
		depth_++;
	}
	void dec() {
		depth_--;
	}
public:
	ASTPrinter(bool pretty = false) : pretty_(pretty) {}

	std::string toString(AST *ast) {
		switch (ast->getASTType()) {
		case AST::TRANS_UNIT		:
			return toString(static_cast<TransUnit *>(ast));

		case AST::TYPE_SPEC		:
		case AST::ARRAY_TYPE_SPEC	:
		case AST::FUNC_TYPE_SPEC	:
			return toString(static_cast<TypeSpec *>(ast));

		case AST::EXPR			: 
		case AST::IDENTIFIER		:
		//case AST::KEYWORD		:
		case AST::LABEL			:

		case AST::BINARY_EXPR		:
		case AST::UNARY_EXPR		:
		case AST::STR_LITERAL_EXPR	:
		case AST::INT_LITERAL_EXPR	:
		case AST::FLOAT_LITERAL_EXPR	:
		case AST::CHAR_LITERAL_EXPR	:
		case AST::BOOL_LITERAL_EXPR	:
		case AST::ARRAY_LITERAL_EXPR	:
		case AST::FUNC_CALL_EXPR	:
		case AST::CONSTRUCTOR_EXPR	:
		case AST::SUBSCR_EXPR		:
		case AST::MEMBER_EXPR		:
		case AST::REF_EXPR		:
		case AST::DEREF_EXPR		:
		case AST::FUNC_EXPR		:
			return toString(static_cast<Expr *>(ast));

		case AST::STMT			:
		case AST::FUNC_DEF_STMT		:
		case AST::VAR_DEF_STMT		:
		case AST::INST_STMT		:
		case AST::ASSIGN_STMT		:
		case AST::COMP_STMT		:
		case AST::IF_STMT		:
		case AST::REPEAT_STMT		:
		case AST::LABEL_STMT		:
		case AST::GOTO_STMT		:
		case AST::GOSUB_STMT		:
		case AST::CONTINUE_STMT		:
		case AST::BREAK_STMT		:
		case AST::RETURN_STMT		:
		case AST::EXTERN_STMT		:
			return toString(static_cast<Stmt *>(ast));
		}
	}

	std::string toString(TransUnit *tu) {
		inc();
		std::stringstream ss;
		ss<<"(TransUnit";
		for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
			ss<<ind()<<toString(*it);
		}
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(TypeSpec *ts) {
		inc();
		std::stringstream ss;

		switch (ts->getASTType()) {
		case AST::TYPE_SPEC		: ss<<"(TypeSpec"; break;
		case AST::ARRAY_TYPE_SPEC	: ss<<"(ArrayTypeSpec"; break;
		case AST::FUNC_TYPE_SPEC	: ss<<"(FuncTypeSpec"; break;
		default				: ;
		}

		if (ts->isConst)
			ss<<" const";
		if (ts->isRef)
			ss<<" ref";

		switch (ts->getASTType()) {
		case AST::TYPE_SPEC		: ss<<" \""<<ts->token.getString()<<"\""; break;
		case AST::ARRAY_TYPE_SPEC	: ss<<ind()<<toString(static_cast<ArrayTypeSpec *>(ts)->typeSpec); break;
		case AST::FUNC_TYPE_SPEC	: ss<<ind()<<toString(static_cast<FuncTypeSpec *>(ts)->lhs)
						    <<ind()<<toString(static_cast<FuncTypeSpec *>(ts)->rhs); break;
		default				: ;
		}

		ss<<")";

		dec();
		return ss.str();
	}

	std::string toString(Stmt *stmt) {
		switch (stmt->getASTType()) {
		case AST::FUNC_DEF_STMT		: return toString(static_cast<FuncDefStmt *>(stmt));
		case AST::VAR_DEF_STMT		: return toString(static_cast<VarDefStmt *>(stmt));
		case AST::INST_STMT		: return toString(static_cast<InstStmt *>(stmt));
		case AST::ASSIGN_STMT		: return toString(static_cast<AssignStmt *>(stmt));
		case AST::COMP_STMT		: return toString(static_cast<CompStmt *>(stmt));
		case AST::IF_STMT		: return toString(static_cast<IfStmt *>(stmt));
		case AST::REPEAT_STMT		: return toString(static_cast<RepeatStmt *>(stmt));
		case AST::LABEL_STMT		: return toString(static_cast<LabelStmt *>(stmt));
		case AST::GOTO_STMT		: return toString(static_cast<GotoStmt *>(stmt));
		case AST::GOSUB_STMT		: return toString(static_cast<GosubStmt*>(stmt));
		case AST::CONTINUE_STMT		: return toString(static_cast<ContinueStmt *>(stmt));
		case AST::BREAK_STMT		: return toString(static_cast<BreakStmt *>(stmt));
		case AST::RETURN_STMT		: return toString(static_cast<ReturnStmt *>(stmt));
		case AST::EXTERN_STMT		: return toString(static_cast<ExternStmt *>(stmt));
		default				: return "(Stmt)";
		}
	}

	std::string toString(FuncDefStmt *fds) {
		inc();
		std::stringstream ss;
		ss<<"(FuncDefStmt "<<toString(fds->name);
		for (std::vector<Identifier *>::iterator it = fds->params.begin(); it != fds->params.end(); ++it)
			ss<<ind()<<toString(*it);
		ss<<ind()<<toString(fds->body);
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(VarDefStmt *vds) {
		inc();
		std::stringstream ss;
		if (vds->init != NULL)
			ss<<"(VarDefStmt "<<toString(vds->id)<<ind()<<toString(vds->init)<<")";
		else
			ss<<"(VarDefStmt "<<toString(vds->id)<<")";
		dec();
		return ss.str();
	}

	std::string toString(InstStmt *is) {
		inc();
		std::stringstream ss;
		ss<<"(InstStmt "<<toString(is->inst);
		for (std::vector<Expr *>::iterator it = is->params.begin(); it != is->params.end(); ++it)
			ss<<ind()<<toString(*it);
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(AssignStmt *as) {
		inc();
		std::stringstream ss;
		ss<<"(AssignStmt "<<as->token.toString()<<ind()<<toString(as->lhs);
		if (as->rhs != NULL) {
			ss<<ind()<<toString(as->rhs);
		}
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(CompStmt *cs) {
		inc();
		std::stringstream ss;
		ss<<"(CompStmt";
		for (std::vector<Stmt *>::iterator it = cs->stmts.begin(); it != cs->stmts.end(); ++it)
			ss<<ind()<<toString(*it);
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(IfStmt *is) {
		inc();
		std::stringstream ss;
		ss<<"(IfStmt "<<toString(is->ifCond)<<ind()<<toString(is->ifThen);

		for (int i = 0; i < is->elseIfCond.size(); ++i)
			ss<<ind()<<toString(is->elseIfCond[i])<<ind()<<toString(is->elseIfThen[i]);

		if (is->elseThen != NULL)
			ss<<ind()<<toString(is->elseThen);

		ss<<")";

		dec();
		return ss.str();
	}

	std::string toString(RepeatStmt *rs) {
		inc();
		std::stringstream ss;
		ss<<"(RepeatStmt";
		if (rs->count != NULL) {
			ss<<ind()<<toString(rs->count);
		} else {
			ss<<ind()<<"()";
		}

		for (std::vector<Stmt *>::iterator it = rs->stmts.begin(); it != rs->stmts.end(); ++it)
			ss<<ind()<<toString(*it);
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(LabelStmt *ls) {
		std::stringstream ss;
		ss<<"(LabelStmt "<<toString(ls->label)<<")";
		return ss.str();
	}

	std::string toString(GotoStmt *gs) {
		std::stringstream ss;
		ss<<"(GotoStmt "<<toString(gs->to)<<")";
		return ss.str();
	}

	std::string toString(GosubStmt *gs) {
		std::stringstream ss;
		ss<<"(GosubStmt "<<toString(gs->to)<<")";
		return ss.str();
	}

	std::string toString(ContinueStmt *cs) {
		return std::string("(ContinueStmt)");
	}

	std::string toString(BreakStmt *cs) {
		return std::string("(BreakStmt)");
	}

	std::string toString(ReturnStmt *rs) {
		std::stringstream ss;
		if (rs->expr != NULL) {
			ss<<"(ReturnStmt "<<toString(rs->expr)<<")";
		} else {
			ss<<"(ReturnStmt)";
		}
		return ss.str();
	}

	std::string toString(ExternStmt *es) {
		std::stringstream ss;
		ss<<"(ExternStmt "<<toString(es->id)<<")";
		return ss.str();
	}

	std::string toString(Expr *expr) {
		switch (expr->getASTType()) {
		case AST::IDENTIFIER		: return toString(static_cast<Identifier *>(expr));
		//case AST::KEYWORD		:
		case AST::LABEL			: return toString(static_cast<Label *>(expr));

		case AST::BINARY_EXPR		: return toString(static_cast<BinaryExpr *>(expr));
		case AST::UNARY_EXPR		: return toString(static_cast<UnaryExpr *>(expr));
		case AST::STR_LITERAL_EXPR	: return toString(static_cast<StrLiteralExpr *>(expr));
		case AST::INT_LITERAL_EXPR	: return toString(static_cast<IntLiteralExpr *>(expr));
		case AST::FLOAT_LITERAL_EXPR	: return toString(static_cast<FloatLiteralExpr *>(expr));
		case AST::CHAR_LITERAL_EXPR	: return toString(static_cast<CharLiteralExpr *>(expr));
		case AST::BOOL_LITERAL_EXPR	: return toString(static_cast<BoolLiteralExpr *>(expr));
		case AST::ARRAY_LITERAL_EXPR	: return toString(static_cast<ArrayLiteralExpr *>(expr));
		case AST::FUNC_CALL_EXPR	: return toString(static_cast<FuncCallExpr *>(expr));
		case AST::CONSTRUCTOR_EXPR	: return toString(static_cast<ConstructorExpr *>(expr));
		case AST::SUBSCR_EXPR		: return toString(static_cast<SubscrExpr *>(expr));
		case AST::MEMBER_EXPR		: return toString(static_cast<MemberExpr *>(expr));
		case AST::REF_EXPR		: return toString(static_cast<RefExpr *>(expr));
		case AST::DEREF_EXPR		: return toString(static_cast<DerefExpr *>(expr));
		case AST::FUNC_EXPR		: return toString(static_cast<FuncExpr *>(expr));
		default				: return "(Expr)";
		}
	}

	std::string toString(Identifier *id) {
		std::stringstream ss;
		/*if (id->getASTType() == AST::KEYWORD) {
			ss<<"(Keyword \"";
		} else {*/
			ss<<"(Identifier \"";
		//}
		ss<<id->getString()<<"\")";
		return ss.str();
	}

	std::string toString(Label *label) {
		std::stringstream ss;
		ss<<"(Label \""<<label->token.getString()<<"\")";
		return ss.str();
	}

	std::string toString(BinaryExpr *be) {
		inc();
		std::stringstream ss;
		ss<<"(BinaryExpr "<<be->token.toString()<<ind()<<toString(be->lhs)<<ind()<<toString(be->rhs)<<")";
		dec();
		return ss.str();
	}

	std::string toString(UnaryExpr *ue) {
		inc();
		std::stringstream ss;
		ss<<"(UnaryExpr "<<ue->token.toString()<<ind()<<toString(ue->rhs)<<")";
		dec();
		return ss.str();
	}

	std::string toString(StrLiteralExpr *sle) {
		std::stringstream ss;
		ss<<"(StrLiteralExpr \""<<sle->token.getString()<<"\")";
		return ss.str();
	}

	std::string toString(IntLiteralExpr *ile) {
		std::stringstream ss;
		ss<<"(IntLiteralExpr "<<ile->token.getInteger()<<")";
		return ss.str();
	}

	std::string toString(FloatLiteralExpr *fle) {
		std::stringstream ss;
		ss<<"(FloatLiteralExpr "<<fle->token.getFloat()<<")";
		return ss.str();
	}

	std::string toString(CharLiteralExpr *cle) {
		std::stringstream ss;
		ss<<"(CharLiteralExpr \'"<<cle->token.getChar()<<"\')";
		return ss.str();
	}

	std::string toString(BoolLiteralExpr *ble) {
		if (ble->bool_) {
			return "(BoolLiteralExpr true)";
		} else {
			return "(BoolLiteralExpr false)";
		}
	}

	std::string toString(ArrayLiteralExpr *ale) {
		inc();
		std::stringstream ss;
		ss<<"(ArrayLiteralExpr";
		for (std::vector<Expr *>::iterator it = ale->elements.begin(); it != ale->elements.end(); ++it) {
			ss<<ind()<<toString(*it);
		}
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(FuncCallExpr *fce) {
		inc();
		std::stringstream ss;
		ss<<"(FuncCallExpr "<<toString(fce->func);
		for (std::vector<Expr *>::iterator it = fce->params.begin(); it != fce->params.end(); ++it)
			ss<<ind()<<toString(*it);
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(ConstructorExpr *ce) {
		inc();
		std::stringstream ss;
		ss<<"(ConstructorExpr "<<toString(ce->constructor);
		for (std::vector<Expr *>::iterator it = ce->params.begin(); it != ce->params.end(); ++it)
			ss<<ind()<<toString(*it);
		ss<<")";
		dec();
		return ss.str();
	}

	std::string toString(SubscrExpr *se) {
		std::stringstream ss;
		inc();
		ss<<"(SubscrExpr"<<ind()<<toString(se->array)<<ind()<<toString(se->subscript)<<")";
		dec();
		return ss.str();
	}

	std::string toString(MemberExpr *me) {
		std::stringstream ss;
		inc();
		ss<<"(MemberExpr"<<ind()<<toString(me->receiver)<<ind()<<toString(me->member)<<")";
		dec();
		return ss.str();
	}

	std::string toString(RefExpr *re) {
		std::stringstream ss;
		ss<<"(RefExpr "<<toString(re->refered)<<")";
		return ss.str();
	}

	std::string toString(DerefExpr *de) {
		std::stringstream ss;
		ss<<"(DerefExpr "<<toString(de->derefered)<<")";
		return ss.str();
	}

	std::string toString(FuncExpr *fe) {
		inc();
		std::stringstream ss;
		ss<<"(FuncExpr";
		for (std::vector<Identifier *>::iterator it = fe->params.begin(); it != fe->params.end(); ++it)
			ss<<ind()<<toString(*it);
		ss<<ind()<<toString(fe->body);
		ss<<")";
		dec();
		return ss.str();
	}
};

};

#endif
