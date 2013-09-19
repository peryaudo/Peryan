#ifndef PERYAN_AST_PRINTER_H__
#define PERYAN_AST_PRINTER_H__

#include "AST.h"
#include "ASTVisitor.h"
#include "SymbolTable.h"

namespace Peryan {

class ASTPrinter : public ASTVisitor {
private:
	ASTPrinter(const ASTPrinter&);
	ASTPrinter& operator=(const ASTPrinter&);
	bool pretty_;
	int depth_;
	bool type_;

	// indent
	void ind() {
		if (pretty_) {
			ss_<<"\n"<<std::string(depth_ * 2, ' ');
		} else {
			ss_<<" ";
		}
	}
	void inc() {
		depth_++;
	}
	void dec() {
		depth_--;
	}
	std::stringstream ss_;
public:
	ASTPrinter(bool pretty = false, bool type = false) : pretty_(pretty), depth_(0), type_(type) {}

	std::string toString(AST *ast) {
		ss_.clear();
		ast->accept(this);
		return ss_.str();
	}

	virtual void visit(TransUnit *tu) {
		inc();
		ss_<<"(TransUnit";
		for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(TypeSpec *ts) {
		inc();
		ss_<<"(TypeSpec"; 

		if (ts->isConst) ss_<<" const";
		if (ts->isRef) ss_<<" ref";

		ss_<<" \""<<ts->token.getString()<<"\")";
		dec();
		return;
	}

	virtual void visit(ArrayTypeSpec *ats) {
		inc();
		ss_<<"(ArrayTypeSpec";

		if (ats->isConst) ss_<<" const";
		if (ats->isRef) ss_<<" ref";

		ind(); ats->typeSpec->accept(this);

		ss_<<")";
		dec();
		return;
	}

	virtual void visit(FuncTypeSpec *fts) {
		inc();
		ss_<<"(FuncTypeSpec";
		ind(); fts->lhs->accept(this);
		ind(); fts->rhs->accept(this);
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(MemberTypeSpec *mts) {
		inc();
		ss_<<"(MemberTypeSpec";
		ind(); mts->lhs->accept(this);
		ind(); ss_<<mts->rhs.getString();
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(FuncDefStmt *fds) {
		inc();
		ss_<<"(FuncDefStmt ";
		fds->name->accept(this);
		for (std::vector<Identifier *>::iterator it = fds->params.begin(); it != fds->params.end(); ++it) {
			ind(); (*it)->accept(this);
		}

		ind(); fds->body->accept(this);
		ss_<<")";
		dec();

		return; 
	}

	virtual void visit(VarDefStmt *vds) {
		inc();
		ss_<<"(VarDefStmt ";
		vds->id->accept(this);

		if (vds->init != NULL) {
			ind(); vds->init->accept(this);
		}

		ss_<<")";
		dec();

		return;
	}

	virtual void visit(InstStmt *is) {
		inc();
		ss_<<"(InstStmt ";
		is->inst->accept(this);
		for (std::vector<Expr *>::iterator it = is->params.begin(); it != is->params.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(AssignStmt *as) {
		inc();
		ss_<<"(AssignStmt "<<as->token.toString();
		ind();
		as->lhs->accept(this);

		if (as->rhs != NULL) {
			ind(); as->rhs->accept(this);
		}

		ss_<<")";
		dec();

		return;
	}

	virtual void visit(CompStmt *cs) {
		inc();
		ss_<<"(CompStmt";
		for (std::vector<Stmt *>::iterator it = cs->stmts.begin(); it != cs->stmts.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(IfStmt *is) {
		inc();
		ss_<<"(IfStmt";

		for (unsigned int i = 0; i < is->ifCond.size(); ++i) {
			ind(); is->ifCond[i]->accept(this);
			ind(); is->ifThen[i]->accept(this);
		}

		if (is->elseThen != NULL) {
			ind(); is->elseThen->accept(this);
		}

		ss_<<")";

		dec();
		return;
	}

	virtual void visit(RepeatStmt *rs) {
		inc();
		ss_<<"(RepeatStmt";
		if (rs->count != NULL) {
			ind(); rs->count->accept(this);
		} else {
			ind(); ss_<<"()";
		}

		for (std::vector<Stmt *>::iterator it = rs->stmts.begin(); it != rs->stmts.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();

		return;
	}

	virtual void visit(LabelStmt *ls) {
		ss_<<"(LabelStmt "; ls->label->accept(this); ss_<<")";
		return;
	}

	virtual void visit(GotoStmt *gs) {
		ss_<<"(GotoStmt "; gs->to->accept(this); ss_<<")";
		return;
	}

	virtual void visit(GosubStmt *gs) {
		ss_<<"(GosubStmt "; gs->to->accept(this); ss_<<")";
		return;
	}

	virtual void visit(ContinueStmt *cs) {
		ss_<<"(ContinueStmt)";
		return;
	}

	virtual void visit(BreakStmt *cs) {
		ss_<<"(BreakStmt)";
		return;
	}

	virtual void visit(ReturnStmt *rs) {
		if (rs->expr != NULL) {
			ss_<<"(ReturnStmt "; rs->expr->accept(this); ss_<<")";
		} else {
			ss_<<"(ReturnStmt)";
		}
		return;
	}

	virtual void visit(ExternStmt *es) {
		ss_<<"(ExternStmt "; es->id->accept(this); ss_<<")";
		return;
	}

	virtual void visit(Identifier *id) {
		ss_<<"(Identifier \""<<id->getString()<<"\")";
		return;
	}

	virtual void visit(Label *label) {
		ss_<<"(Label \""<<label->token.getString()<<"\")";
		return;
	}

	virtual void visit(BinaryExpr *be) {
		inc();
		ss_<<"(BinaryExpr "<<be->token.toString();
		ind(); be->lhs->accept(this);
		ind(); be->rhs->accept(this);
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(UnaryExpr *ue) {
		inc();
		ss_<<"(UnaryExpr "<<ue->token.toString(); ind(); ue->rhs->accept(this); ss_<<")";
		dec();
		return;
	}

	virtual void visit(StrLiteralExpr *sle) {
		ss_<<"(StrLiteralExpr \""<<sle->token.getString()<<"\")";
		return;
	}

	virtual void visit(IntLiteralExpr *ile) {
		ss_<<"(IntLiteralExpr "<<ile->token.getInteger()<<")";
		return;
	}

	virtual void visit(FloatLiteralExpr *fle) {
		ss_<<"(FloatLiteralExpr "<<fle->token.getFloat()<<")";
		return;
	}

	virtual void visit(CharLiteralExpr *cle) {
		ss_<<"(CharLiteralExpr \'"<<cle->token.getChar()<<"\')";
		return;
	}

	virtual void visit(BoolLiteralExpr *ble) {
		if (ble->bool_) {
			ss_<<"(BoolLiteralExpr true)";
		} else {
			ss_<<"(BoolLiteralExpr false)";
		}
		return;
	}

	virtual void visit(ArrayLiteralExpr *ale) {
		inc();
		ss_<<"(ArrayLiteralExpr";
		for (std::vector<Expr *>::iterator it = ale->elements.begin(); it != ale->elements.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(FuncCallExpr *fce) {
		inc();
		ss_<<"(FuncCallExpr "; fce->func->accept(this);
		for (std::vector<Expr *>::iterator it = fce->params.begin(); it != fce->params.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(ConstructorExpr *ce) {
		inc();
		ss_<<"(ConstructorExpr "; ce->constructor->accept(this);
		for (std::vector<Expr *>::iterator it = ce->params.begin(); it != ce->params.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(SubscrExpr *se) {
		inc();
		ss_<<"(SubscrExpr"; ind(); se->array->accept(this); ind(); se->subscript->accept(this); ss_<<")";
		dec();
		return;
	}

	virtual void visit(MemberExpr *me) {
		inc();
		ss_<<"(MemberExpr"; ind(); me->receiver->accept(this); ind(); me->member->accept(this); ss_<<")";
		dec();
		return;
	}

	virtual void visit(StaticMemberExpr *sme) {
		inc();
		ss_<<"(StaticMemberExpr";
		ind(); sme->receiver->accept(this);
		ind(); sme->member->accept(this);
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(RefExpr *re) {
		ss_<<"(RefExpr "; re->refered->accept(this); ss_<<")";
		return;
	}

	virtual void visit(DerefExpr *de) {
		ss_<<"(DerefExpr "; de->derefered->accept(this); ss_<<")";
		return;
	}

	virtual void visit(FuncExpr *fe) {
		inc();
		ss_<<"(FuncExpr";
		for (std::vector<Identifier *>::iterator it = fe->params.begin(); it != fe->params.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ind(); fe->body->accept(this);
		ss_<<")";
		dec();
		return;
	}

	virtual void visit(NamespaceStmt *ns) {
		inc();
		ss_<<"(NamespaceStmt";
		for (std::vector<Stmt *>::iterator it = ns->stmts.begin(); it != ns->stmts.end(); ++it) {
			ind(); (*it)->accept(this);
		}
		ss_<<")";
		dec();
		return;
	}

	static void dump(AST *ast, bool type = true) {
		ASTPrinter printer(true, type);
		std::cerr<<printer.toString(ast)<<std::endl;
		return;
	}
};

}

#endif
