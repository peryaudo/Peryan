// the reason why only this class uses pImpl idiom is
// to remove LLVM dependecies from other part of the compiler
// and to enclosure all of them in the class.
#include <sstream>
#include <deque>

#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/PassManager.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/ValueSymbolTable.h"

#include "SymbolTable.h"
#include "AST.h"
#include "Parser.h"
#include "LLVMCodeGen.h"

//#define DBG_PRINT(TYPE, FUNC_NAME) std::cout<<#TYPE<<#FUNC_NAME<<std::endl
#define DBG_PRINT(TYPE, FUNC_NAME)

namespace Peryan {

class LLVMCodeGen::Impl {
private:
	Parser& parser_;
	std::string fileName_;

	llvm::LLVMContext& context_;
	llvm::IRBuilder<> builder_;
	llvm::Module module_;

	Type *Int_, *String_, *Char_, *Float_, *Double_, *Bool_, *Label_, *Void_;

	class Block {
	public:
		typedef enum {
			UNKNOWN_BLOCK,
			GLOBAL_BLOCK,
			COMP_BLOCK,
			FUNC_BLOCK,
			LOOP_BLOCK,
			FUNC_CALL_BLOCK
		} BlockType;

		BlockType type;
		llvm::Function *func;		// GLOBAL_BLOCK, FUNC_BLOCK
		llvm::BasicBlock *head;		// COMP_BLOCK
		llvm::BasicBlock *body;		// GLOBAL_BLOCK, COMP_BLOCK, FUNC_BLOCK, LOOP_BLOCK
		llvm::BasicBlock *end;		// GLOBAL_BLOCK, COMP_BLOCK, FUNC_BLOCK, LOOP_BLOCK
		llvm::Value *retVal;		// FUNC_BLOCK
		llvm::BasicBlock *continue_;	// LOOP_BLOCK
		llvm::BasicBlock *break_;	// LOOP_BLOCK

		Block(BlockType type)
			: type(type)
			, func(NULL)
			, head(NULL)
			, body(NULL)
			, end(NULL)
			, retVal(NULL)
			, continue_(NULL) {}
	};

	std::deque<Block> blocks;
	bool isNamespaceGlobal();
	bool isFunctionalGlobal();
	Block& getEnclosingFuncBlock();
	llvm::Function *getEnclosingFunc() {
		return getEnclosingFuncBlock().func;
	}
	Block& getEnclosingLoopBlock();

	int counter_;

	// don't remove that! (you wrote this twice!)
	std::string getUniqNumStr() {
		counter_++;
		std::stringstream ss;
		ss<<counter_;
		return ss.str();
	}

	void registerRuntimeFunctions();

	void generateFuncDecl(const std::string& name, Type *type);
	void generateGlobalVarDecl(const std::string& name, Type *type, bool isExternal);

	llvm::Type *getLLVMType(Type *type);

	void generateTransUnit(TransUnit *tu);
	void generateGlobalDecl(Scope *scope);
	void generateStmt(Stmt *stmt);
	void generateCompStmt(CompStmt *cs, bool isSimple) {
		Block tmp(Block::UNKNOWN_BLOCK);
		generateCompStmt(cs, isSimple, true, tmp);
	}
	void generateCompStmt(CompStmt *cs, bool isSimple, bool autoConnect, Block& compBlock);
	void generateFuncDefStmt(FuncDefStmt *fds);
	void generateVarDefStmt(VarDefStmt *vds);
	void generateInstStmt(InstStmt *is);
	void generateAssignStmt(AssignStmt *as);
	void generateIfStmt(IfStmt *is);
	void generateRepeatStmt(RepeatStmt *rs);

	void generateContinueStmt(ContinueStmt *cs);
	void generateBreakStmt(BreakStmt *bs);
	void generateGlobalReturnStmt(ReturnStmt *rs);
	void generateFuncReturnStmt(ReturnStmt *rs);

	void generateGotoStmt(GotoStmt *gs);
	void generateGosubStmt(GosubStmt *gs);
	void generateLabelStmt(LabelStmt *ls);

	void generateNamespaceStmt(NamespaceStmt *ns);

	llvm::Value *generateExpr(Expr *expr);
	llvm::Value *generateVarRefExpr(Identifier *id);

	llvm::Value *generateDerefExpr(DerefExpr *de);

	llvm::Value *generateBinaryExpr(BinaryExpr *be);
	llvm::Value *generateUnaryExpr(UnaryExpr *ue);

	llvm::Value *generateLabelLiteralExpr(Label *label);
	llvm::Value *generateStrLiteralExpr(StrLiteralExpr *sle);
	llvm::Value *generateIntLiteralExpr(IntLiteralExpr *ile);
	llvm::Value *generateFloatLiteralExpr(FloatLiteralExpr *fle);
	llvm::Value *generateCharLiteralExpr(CharLiteralExpr *cle);
	llvm::Value *generateBoolLiteralExpr(BoolLiteralExpr *ble);
	llvm::Value *generateArrayLiteralExpr(ArrayLiteralExpr *ale);

	llvm::Value *generateFuncCallExpr(FuncCallExpr *fce);
	llvm::Value *generateConstructorExpr(ConstructorExpr *ce);
	llvm::Value *generateSubscrExpr(SubscrExpr *se);


	void generateConstructor(llvm::Value *dest, Type *type, Expr *init);
	llvm::Value *lookup(const std::string& str);
public:
	Impl(Parser& parser, const std::string& fileName)
		: parser_(parser)
		, fileName_(fileName)
		, context_(llvm::getGlobalContext())
		, builder_(context_)
		, module_("Peryan", context_)
		, Int_		(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("Int")))
		, String_	(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("String")))
		, Char_		(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("Char")))
		, Float_	(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("Float")))
		, Double_	(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("Double")))
		, Bool_		(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("Bool")))
		, Label_	(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("Label")))
		, Void_		(static_cast<BuiltInTypeSymbol *>(parser_.getTransUnit()->scope->resolve("Void")))
		, blocks()
		, counter_(0)
	        { }

	void generate();
};

// pImpl pointer holder class

LLVMCodeGen::LLVMCodeGen(Parser& parser, const std::string& fileName)
	: impl_(new Impl(parser, fileName)) {}


void LLVMCodeGen::generate() { impl_->generate(); return; }

LLVMCodeGen::~LLVMCodeGen() { delete impl_; impl_ = NULL; }

// pImpl pointer holder class

// Implementation class

void LLVMCodeGen::Impl::registerRuntimeFunctions() {
	// the longer declarations use char* in its definition

	{
		std::vector<llvm::Type *> paramTypes;
		paramTypes.push_back(llvm::Type::getInt32Ty(context_));

		llvm::FunctionType *funcType =
			llvm::FunctionType::get(llvm::Type::getInt8Ty(context_)->getPointerTo(), paramTypes, false);

		llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "PRMalloc", &module_);
	}

	{
		std::vector<llvm::Type *> paramTypes;
		paramTypes.push_back(llvm::Type::getInt8Ty(context_)->getPointerTo());

		llvm::FunctionType *funcType =
			llvm::FunctionType::get(llvm::Type::getVoidTy(context_), paramTypes, false);

		llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "PRFree", &module_);
	}

	{
		std::vector<llvm::Type *> paramTypes;
		paramTypes.push_back(llvm::Type::getInt8Ty(context_)->getPointerTo());
		paramTypes.push_back(llvm::Type::getInt32Ty(context_));

		llvm::FunctionType *funcType =
			llvm::FunctionType::get(llvm::Type::getInt8Ty(context_)->getPointerTo(), paramTypes, false);

		llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "PRRealloc", &module_);
	}

	// for builtin String class

	{
		std::vector<llvm::Type *> paramTypes;
		paramTypes.push_back(llvm::Type::getInt8Ty(context_)->getPointerTo());

		llvm::FunctionType *funcType =
			llvm::FunctionType::get(getLLVMType(String_), paramTypes, false);

		llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "PRStringConstructorCStr", &module_);
	}

	generateFuncDecl("PRStringConstructorInt", new FuncType(Int_, String_));
	generateFuncDecl("PRStringConstructorVoid", new FuncType(Void_, String_));
	generateFuncDecl("PRStringConcatenate", new FuncType(String_, new FuncType(String_, String_)));
	generateFuncDecl("PRStringDestructor", new FuncType(String_, Void_));
	generateFuncDecl("PRStringCompare", new FuncType(String_, new FuncType(String_, Int_)));
	
	return;
}

void LLVMCodeGen::Impl::generate() {
	DBG_PRINT(+, generate);

	registerRuntimeFunctions();

	generateTransUnit(parser_.getTransUnit());

	assert(blocks.empty());

	llvm::PassManager pm;
	std::string error;
	llvm::raw_fd_ostream rawStream(fileName_.c_str(), error);

	pm.add(createPrintModulePass(&rawStream));
	pm.run(module_);

	rawStream.close();

	DBG_PRINT(-, generate);
	return;
}

void LLVMCodeGen::Impl::generateGlobalDecl(Scope *scope) {
	DBG_PRINT(+, generateGlobalDecl);
	assert(scope != NULL);
	// generate declaration of global functions and global variables
	for (Scope::iterator it = scope->begin(); it != scope->end(); ++it) {
		assert((*it) != NULL);
		switch ((*it)->getSymbolType()) {
		case Symbol::EXTERN_SYMBOL:
			if ((*it)->getType()->getTypeType() == Type::FUNC_TYPE) {
				generateFuncDecl((*it)->getSymbolName(), (*it)->getType());
			} else {
				generateGlobalVarDecl((*it)->getSymbolName(), (*it)->getType(), true);
			}
			break;

		case Symbol::FUNC_SYMBOL:
			generateFuncDecl((*it)->getMangledSymbolName(), (*it)->getType());
			break;

		case Symbol::VAR_SYMBOL:
			generateGlobalVarDecl((*it)->getMangledSymbolName(), (*it)->getType(), false);
			break;

		case Symbol::BUILTIN_TYPE_SYMBOL:
			// doesn't have to declare
			break;

		case Symbol::CLASS_SYMBOL:
			assert(false && "class not implemented");
			break;

		case Symbol::NAMESPACE_SYMBOL:
			generateGlobalDecl(static_cast<NamespaceSymbol *>(*it));
			break;

		default: ;
			assert(false && "unknown symbol");
		}
	}

	DBG_PRINT(-, generateGlobalDecl);
	return;
}

void LLVMCodeGen::Impl::generateTransUnit(TransUnit *tu) {
	DBG_PRINT(+, generateTransUnit);

	// generate declaration of global functions and global variables
	generateGlobalDecl(tu->scope);

	// create main function
	{
	Block block(Block::GLOBAL_BLOCK);

	llvm::FunctionType *funcType = llvm::FunctionType::get(getLLVMType(Void_), false);
	block.func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "PeryanMain", &module_);

	const std::string curNumStr = getUniqNumStr();
	block.body = llvm::BasicBlock::Create(context_, "globalBlockEntry" + curNumStr, block.func);
	block.end = llvm::BasicBlock::Create(context_, "globalBlockEnd" + curNumStr, block.func);

	blocks.push_back(block);
	}

	builder_.SetInsertPoint(blocks.back().body);

	for (std::vector<Stmt *>::iterator it = tu->stmts.begin(); it != tu->stmts.end(); ++it) {
		generateStmt(*it);
	}

	assert(blocks.back().type == Block::GLOBAL_BLOCK);

	builder_.SetInsertPoint(blocks.back().body);
	builder_.CreateBr(blocks.back().end);

	builder_.SetInsertPoint(blocks.back().end);
	builder_.CreateRetVoid();

	blocks.pop_back();

	DBG_PRINT(-, generateTransUnit);
	return;
}

// blockBody is the last basic block of the function

// explanation over isNamespaceGlobal and isFunctionalGlobal :
//
// // isNamespaceGlobal() = true, isFunctionalGlobal() = true
// {
// 	// isNamespaceGlobal = false, isFunctionalGlobal = true
// }
// func foo() :: Int {
// 	// isNamespaceGlobal = false, isFunctionalGlobal = false
// }

bool LLVMCodeGen::Impl::isNamespaceGlobal() {
	return blocks.back().type == Block::GLOBAL_BLOCK;
}

bool LLVMCodeGen::Impl::isFunctionalGlobal() {
	for (std::deque<Block>::reverse_iterator it = blocks.rbegin();
			it != blocks.rend(); ++it) {
		if ((*it).type == Block::FUNC_BLOCK)
			return false;
		if ((*it).type == Block::GLOBAL_BLOCK)
			return true;
	}

	assert(false && "no global block found");
	return true;
}

// if it is not inside function, it returns global function
LLVMCodeGen::Impl::Block& LLVMCodeGen::Impl::getEnclosingFuncBlock() {
	for (std::deque<Block>::reverse_iterator it = blocks.rbegin();
			it != blocks.rend(); ++it) {
		if ((*it).type == Block::FUNC_BLOCK || (*it).type == Block::GLOBAL_BLOCK)
			return *it;
	}
	assert(false && "no function or global block found");
	return blocks.front();
}

LLVMCodeGen::Impl::Block& LLVMCodeGen::Impl::getEnclosingLoopBlock() {
	for (std::deque<Block>::reverse_iterator it = blocks.rbegin();
			it != blocks.rend(); ++it) {
		if ((*it).type == Block::LOOP_BLOCK)
			return *it;
	}
	assert(false && "no loop block found");
	return blocks.front();

}

void LLVMCodeGen::Impl::generateStmt(Stmt *stmt) {
	DBG_PRINT(+, generateStmt);
	assert(stmt != NULL);

	switch (stmt->getASTType()) {
	case AST::COMP_STMT	:
		generateCompStmt(static_cast<CompStmt *>(stmt), true);
		break;
	case AST::FUNC_DEF_STMT	:
		assert(isNamespaceGlobal());
		generateFuncDefStmt(static_cast<FuncDefStmt *>(stmt));
		break;
	case AST::VAR_DEF_STMT	:
		generateVarDefStmt(static_cast<VarDefStmt *>(stmt));
		break;
	case AST::INST_STMT	:
		generateInstStmt(static_cast<InstStmt *>(stmt));
		break;
	case AST::ASSIGN_STMT	:
		generateAssignStmt(static_cast<AssignStmt *>(stmt));
		break;
	case AST::IF_STMT	:
		generateIfStmt(static_cast<IfStmt *>(stmt));
		break;
	case AST::REPEAT_STMT	:
		generateRepeatStmt(static_cast<RepeatStmt *>(stmt));
		break;
	case AST::LABEL_STMT	:
		generateLabelStmt(static_cast<LabelStmt *>(stmt));
		break;
	case AST::GOTO_STMT	:
		generateGotoStmt(static_cast<GotoStmt *>(stmt));
		break;
	case AST::GOSUB_STMT	:
		assert(isNamespaceGlobal());
		generateGosubStmt(static_cast<GosubStmt *>(stmt));
		break;
	case AST::CONTINUE_STMT	:
		generateContinueStmt(static_cast<ContinueStmt *>(stmt));
		break;
	case AST::BREAK_STMT	:
		generateBreakStmt(static_cast<BreakStmt *>(stmt));
		break;
	case AST::RETURN_STMT	:
		if (isFunctionalGlobal()) {
			generateGlobalReturnStmt(static_cast<ReturnStmt *>(stmt));
		} else {
			generateFuncReturnStmt(static_cast<ReturnStmt *>(stmt));
		}
		break;
	case AST::EXTERN_STMT	:
		// all extern declaration is on the global scope,
		// so that we don't need any extra operation.
		break;
	case AST::NAMESPACE_STMT:
		generateNamespaceStmt(static_cast<NamespaceStmt *>(stmt));
		break;
	default: assert(false && "unknown statement");
	}
	DBG_PRINT(-, generateStmt);
	return;
}

llvm::Type *LLVMCodeGen::Impl::getLLVMType(Type *type) {
	DBG_PRINT(+-, getLLVMType);

	if (type->getTypeType() == Type::MODIFIER_TYPE) {
		if (static_cast<ModifierType *>(type)->isRef()) {
			return getLLVMType(
				static_cast<ModifierType *>(type)->getElemType())->getPointerTo();
		} else {
			return getLLVMType(static_cast<ModifierType *>(type)->getElemType());
		}
	} else if (type->getTypeType() == Type::ARRAY_TYPE) {
		return llvm::StructType::get(
				llvm::Type::getInt32Ty(context_), // int length
				llvm::Type::getInt32Ty(context_), // int capacity
				llvm::Type::getInt32Ty(context_), // int elementSize
				getLLVMType(
					/* be careful! : recursively calling getLLVMType */
					static_cast<ArrayType *>(type)->getElemType())->getPointerTo(),
					// T* elements for [ T ]
				NULL);
	} else if (type->getTypeType() == Type::BUILTIN_TYPE) {

		     if (type->is(Int_))	return llvm::Type::getInt32Ty(context_);
		else if (type->is(String_))	return llvm::StructType::get(context_)->getPointerTo();
						// opaque*

		else if (type->is(Char_))	return llvm::Type::getInt8Ty(context_);
		else if (type->is(Float_))	return llvm::Type::getFloatPtrTy(context_);
		else if (type->is(Double_))	return llvm::Type::getDoublePtrTy(context_);
		else if (type->is(Bool_))	return llvm::Type::getInt1Ty(context_);
		else if (type->is(Label_))	return llvm::Type::getLabelTy(context_);
		else if (type->is(Void_))	return llvm::Type::getVoidTy(context_);
		else				assert(false && "unknown builtin type");

	} else {
		assert(false && "unknown type"); //unsupported
		return NULL;
	}
}

void LLVMCodeGen::Impl::generateFuncDecl(const std::string& name, Type *type) {
	DBG_PRINT(+, generateFuncDecl);
	std::vector<llvm::Type *> paramTypes;
	while (type->getTypeType() == Type::FUNC_TYPE) {
		FuncType *cur = static_cast<FuncType *>(type);
		if (cur->getCar()->is(Void_)) {
			// there's no parameter, so we have nothing to do.
			assert(cur->getCdr()->getTypeType() != Type::FUNC_TYPE);
		} else {
			paramTypes.push_back(getLLVMType(cur->getCar()));
		}

		type = cur->getCdr();
	}
	assert(type->getTypeType() == Type::BUILTIN_TYPE); // class not implemented

	llvm::FunctionType *funcType =
		llvm::FunctionType::get(getLLVMType(type), paramTypes, false);

	llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, &module_);

	DBG_PRINT(-, generateFuncDecl);
	return;
}

void LLVMCodeGen::Impl::generateGlobalVarDecl(const std::string& name, Type *type, bool isExternal) {
	DBG_PRINT(+, generateGlobalVarDecl);
	assert(type->unmodify()->getTypeType() == Type::BUILTIN_TYPE
		|| type->unmodify()->getTypeType() == Type::ARRAY_TYPE); // class not implemented

	llvm::Constant *init = NULL;
	if (!isExternal) {
		if (type->getTypeType() == Type::MODIFIER_TYPE
			&& static_cast<ModifierType *>(type)->isRef()) {
			init = llvm::ConstantPointerNull::get(getLLVMType(type->unmodify())->getPointerTo());
		} else if (type->unmodify()->is(Int_) || type->unmodify()->is(Char_)
				|| type->unmodify()->is(Bool_)) {
			// they will be initialized again, it is just to avoid LLVM IR syntax error
			init = llvm::ConstantInt::get(getLLVMType(type), 0);
		} else if (type->unmodify()->is(String_)) {
			init = llvm::ConstantPointerNull::get(
				static_cast<llvm::PointerType *>(getLLVMType(String_)));
		} else if (type->unmodify()->getTypeType() == Type::ARRAY_TYPE) {
			// int length
			// int capacity
			// int elementSize
			// Type* elements for [ Type ]

			std::vector<llvm::Constant *> params;
			params.push_back(llvm::ConstantInt::get(getLLVMType(Int_), 0));
			params.push_back(llvm::ConstantInt::get(getLLVMType(Int_), 0));
			params.push_back(llvm::ConstantInt::get(getLLVMType(Int_), 0));
			params.push_back(llvm::ConstantPointerNull::get(
				getLLVMType(static_cast<ArrayType *>(type->unmodify())->
					getElemType())->getPointerTo()));

			init = llvm::ConstantStruct::get(
					static_cast<llvm::StructType *>(getLLVMType(type)), params);
		} else {
			std::cerr<<type->unmodify()->getTypeName()<<std::endl;
			assert(false && "unknown type");
		}
	}

	new llvm::GlobalVariable(module_, getLLVMType(type), false,
		(isExternal ? llvm::GlobalVariable::ExternalLinkage
		 	    : llvm::GlobalVariable::CommonLinkage), init, name);

	DBG_PRINT(-, generateGlobalVarDecl);
	return;
}

// if you implement C++ style cleanup, the overview is like that:
// (1) make sure which statements cause cleanup
// (2) add cleanup block at the end of the compound / function statement
//     with label variable to memory destination after cleanup
//  -  when come to these statements, register its destination to the label variable,
// (3) and jump to the cleanup block

// to realize this:
// - if you start scope, create body, end block and destination variable for each
// - if you constructed a variable add cleanup instructions at the proper end (= back().end)
// - if you came across jump statement,
//   - find out which end block you have to pass, ex. end5 end4 end3
//   - register next destination to block: end5: -> end4, end4: -> end3, end3: -> returnStmtBlock
// - if you ended scope,
//   - add switch and jump statement by registration table

// isSimple means the CompStmt is not right after function feclaration
// so if it is a simple block, we need to add new block
void LLVMCodeGen::Impl::generateCompStmt(CompStmt *cs, bool isSimple, bool autoConnect, Block& compBlock) {
	DBG_PRINT(+, generateCompStmt);

	const std::string curNumStr = getUniqNumStr();

	if (isSimple) {
		Block block(Block::COMP_BLOCK);


		llvm::Function *func = getEnclosingFunc();
		block.body = llvm::BasicBlock::Create(context_, "compBlockEntry" + curNumStr, func);
		block.head = block.body;
		block.end = llvm::BasicBlock::Create(context_, "compBlockEnd" + curNumStr, func);

		if (autoConnect) {
			// greater block's body -> the block's entry
			builder_.SetInsertPoint(blocks.back().body);
			builder_.CreateBr(block.body);
		}

		blocks.push_back(block);
	}

	for (std::vector<Stmt *>::iterator it = cs->stmts.begin(); it != cs->stmts.end(); ++it) {
		generateStmt(*it);
	}

	if (isSimple) {
		assert(blocks.back().type == Block::COMP_BLOCK);

		// the block's body -> the block's end
		builder_.SetInsertPoint(blocks.back().body);
		builder_.CreateBr(blocks.back().end);

		compBlock = blocks.back();

		// the block's end -> greater block's new body
		builder_.SetInsertPoint(blocks.back().end);
		blocks.pop_back();

		if (autoConnect) {
			llvm::Function *func = getEnclosingFunc();
			blocks.back().body = llvm::BasicBlock::Create(context_, "compBlockAfter" + curNumStr,
								func);
			builder_.CreateBr(blocks.back().body);
		}
	}

	DBG_PRINT(-, generateCompStmt);
	return;
}

void LLVMCodeGen::Impl::generateNamespaceStmt(NamespaceStmt *ns) {
	DBG_PRINT(+, generateNamespaceStmt);
	for (std::vector<Stmt *>::iterator it = ns->stmts.begin(); it != ns->stmts.end(); ++it) {
		generateStmt(*it);
	}
	DBG_PRINT(-, generateNamespaceStmt);
}

void LLVMCodeGen::Impl::generateFuncDefStmt(FuncDefStmt *fds) {
	DBG_PRINT(+, generateFuncDefStmt);

	const std::string curNumStr = getUniqNumStr();

	Type *retType = static_cast<FuncType *>(fds->symbol->getType())->getReturnType();

	{
		Block block(Block::FUNC_BLOCK);

		block.func = module_.getFunction(fds->symbol->getMangledSymbolName());
		assert(block.func != NULL);

		block.body = llvm::BasicBlock::Create(context_, "funcBlockEntry" + curNumStr, block.func);
		block.end = llvm::BasicBlock::Create(context_, "funcBlockEnd" + curNumStr, block.func);

		if (!(retType->is(Void_))) {
			// allocate a variable in the stack frame to memorize return value
			builder_.SetInsertPoint(block.body);
			block.retVal = builder_.CreateAlloca(getLLVMType(retType), 0, "$retVal");
		}

		blocks.push_back(block);
	}

	// initialization of parameters

	llvm::Function::arg_iterator llvmItr = blocks.back().func->arg_begin(),
				  llvmItrEnd = blocks.back().func->arg_end();
	std::vector<Identifier *>::iterator idItr = fds->params.begin(),
					 idItrEnd = fds->params.end();

	for (; llvmItr != llvmItrEnd && idItr != idItrEnd; ++llvmItr, ++idItr) {
		Type *curType = (*idItr)->type;

		// TODO: you have to run copy constructor there!

		assert(curType->getTypeType() == Type::BUILTIN_TYPE
			|| curType->getTypeType() == Type::MODIFIER_TYPE); // class not yet supported

		if (curType->getTypeType() == Type::BUILTIN_TYPE) {
			const std::string name = (*idItr)->symbol->getMangledSymbolName();
			const std::string originalName =
				(*idItr)->symbol->getMangledSymbolName() + std::string(".original");

			(*llvmItr).setName(originalName);

			builder_.SetInsertPoint(blocks.back().body);

			llvm::AllocaInst *allocaInst  = builder_.CreateAlloca(
					getLLVMType(curType), 0, name);

			llvm::Value *from = lookup(originalName);
			assert(from != NULL);
			builder_.CreateStore(from, allocaInst);

		} else if (curType->getTypeType() == Type::MODIFIER_TYPE) {
			(*llvmItr).setName((*idItr)->symbol->getMangledSymbolName());
		}
	}

	assert(idItr == idItrEnd && llvmItr == llvmItrEnd);

	generateCompStmt(fds->body, false);

	assert(blocks.back().type == Block::FUNC_BLOCK);

	builder_.SetInsertPoint(blocks.back().body);
	builder_.CreateBr(blocks.back().end);

	builder_.SetInsertPoint(blocks.back().end);
	if (!(retType->is(Void_))) {
		llvm::Value *retValLoaded = builder_.CreateLoad(blocks.back().retVal);
		builder_.CreateRet(retValLoaded);
	} else {
		builder_.CreateRetVoid();
	}

	blocks.pop_back();

	DBG_PRINT(-, generateFuncDefStmt);
	return;
}

llvm::Value *LLVMCodeGen::Impl::generateExpr(Expr *expr) {
	DBG_PRINT(+-, generateExpr);
	switch (expr->getASTType()) {
	case AST::IDENTIFIER		: return generateVarRefExpr(static_cast<Identifier *>(expr));

	case AST::LABEL			: return generateLabelLiteralExpr(static_cast<Label *>(expr));
	case AST::BINARY_EXPR		: return generateBinaryExpr(static_cast<BinaryExpr *>(expr));
	case AST::UNARY_EXPR		: return generateUnaryExpr(static_cast<UnaryExpr *>(expr));
	case AST::STR_LITERAL_EXPR	: return generateStrLiteralExpr(static_cast<StrLiteralExpr *>(expr));
	case AST::INT_LITERAL_EXPR	: return generateIntLiteralExpr(static_cast<IntLiteralExpr *>(expr));
	case AST::FLOAT_LITERAL_EXPR	: return generateFloatLiteralExpr(static_cast<FloatLiteralExpr *>(expr));
	case AST::CHAR_LITERAL_EXPR	: return generateCharLiteralExpr(static_cast<CharLiteralExpr *>(expr));
	case AST::BOOL_LITERAL_EXPR	: return generateBoolLiteralExpr(static_cast<BoolLiteralExpr *>(expr));
	case AST::ARRAY_LITERAL_EXPR	: return generateArrayLiteralExpr(static_cast<ArrayLiteralExpr *>(expr));
	case AST::FUNC_CALL_EXPR	: return generateFuncCallExpr(static_cast<FuncCallExpr *>(expr));
	case AST::CONSTRUCTOR_EXPR	: return generateConstructorExpr(static_cast<ConstructorExpr *>(expr));
	case AST::SUBSCR_EXPR		: return generateSubscrExpr(static_cast<SubscrExpr *>(expr));
	case AST::MEMBER_EXPR		: /*return generateMemberExpr(static_cast<MemberExpr *>(expr));*/ assert(false && "member expression not supported yet");
	case AST::REF_EXPR		: assert(false && "cannnot generate ref expr!");
	case AST::DEREF_EXPR		: return generateDerefExpr(static_cast<DerefExpr *>(expr));

	default: assert(false && "unknown expression");
	}
	return NULL;
}

llvm::Value *LLVMCodeGen::Impl::generateDerefExpr(DerefExpr *de) {
	DBG_PRINT(+, DerefExpr);
	assert(de != NULL);

	llvm::Value *derefered = generateExpr(de->derefered);
	// TODO: make it another function
	builder_.SetInsertPoint(blocks.back().body);

	DBG_PRINT(-, DerefExpr);
	return builder_.CreateLoad(derefered);
}

// TODO: rename (to avoid misunderstanding)
llvm::Value *LLVMCodeGen::Impl::generateVarRefExpr(Identifier *id) {
	DBG_PRINT(+, generateVarRefExpr);
	assert(id->symbol->getType()->getTypeType() != Type::FUNC_TYPE);

	assert(id->symbol->getSymbolType() != Symbol::EXTERN_SYMBOL); // temporarily prohibit extern variable (allows extern function)

	llvm::Value *from = lookup(id->symbol->getMangledSymbolName());
	assert(from != NULL);

	DBG_PRINT(-, generateVarRefExpr);
	return from;
}


llvm::Value *LLVMCodeGen::Impl::generateBinaryExpr(BinaryExpr *be) {
	DBG_PRINT(+-, generateBinaryExpr);
	assert(be->lhs->type->is(be->rhs->type));

	llvm::Value *lhs = generateExpr(be->lhs);
	llvm::Value *rhs = generateExpr(be->rhs);

	builder_.SetInsertPoint(blocks.back().body);

	// ^ | &
	if (be->lhs->type->is(Bool_) || be->lhs->type->is(Int_)) {
		switch (be->token.getType()) {
		case Token::CARET: return builder_.CreateXor(lhs, rhs);
		case Token::PIPE: return builder_.CreateOr(lhs, rhs);
		case Token::AMP: return builder_.CreateAnd(lhs, rhs);
		default: ;
		}
	}

	// = == != !
	if (be->lhs->type->is(Int_) || be->lhs->type->is(Char_) || be->lhs->type->is(Bool_)) {
		switch (be->token.getType()) {
		case Token::EQL:
		case Token::EQEQ:
			return builder_.CreateICmpEQ(lhs, rhs);
		case Token::EXCLEQ:
		case Token::EXCL:
			return builder_.CreateICmpNE(lhs, rhs);
		default: ;
		}
	}
	if (be->lhs->type->is(Float_) || be->lhs->type->is(Double_)) {
		switch (be->token.getType()) {
		case Token::EQL:
		case Token::EQEQ:
			return builder_.CreateFCmpOEQ(lhs, rhs);
		case Token::EXCLEQ:
		case Token::EXCL:
			return builder_.CreateFCmpONE(lhs, rhs);
		default: ;
		}
	}
	if (be->lhs->type->is(String_)) {
		switch (be->token.getType()) {
		case Token::EQL:
		case Token::EQEQ:
		case Token::EXCLEQ:
		case Token::EXCL:
			{
				std::vector<llvm::Value *> params;
				params.push_back(lhs);
				params.push_back(rhs);

				llvm::Function *func = module_.getFunction("PRStringCompare");
				assert(func != NULL);

				llvm::Value *ret = builder_.CreateCall(func, params);
				if (be->token.getType() == Token::EQL || be->token.getType() == Token::EQEQ) {
					return builder_.CreateICmpEQ(ret, llvm::ConstantInt::get(getLLVMType(Int_), 0));
				} else {
					return builder_.CreateICmpNE(ret, llvm::ConstantInt::get(getLLVMType(Int_), 0));
				}
			}
		default: ;
		}
	}

	// < <= > >=
	if (be->lhs->type->is(Int_) || be->lhs->type->is(Char_)) {
		switch (be->token.getType()) {
		case Token::LT	: return builder_.CreateICmpSLT(lhs, rhs);
		case Token::LTEQ: return builder_.CreateICmpSLE(lhs, rhs);
		case Token::GT	: return builder_.CreateICmpSGT(lhs, rhs);
		case Token::GTEQ: return builder_.CreateICmpSGE(lhs, rhs);
		default: ;
		}
	}

	if (be->lhs->type->is(Float_) || be->lhs->type->is(Double_)) {
		switch (be->token.getType()) {
		case Token::LT	: return builder_.CreateFCmpOLT(lhs, rhs);
		case Token::LTEQ: return builder_.CreateFCmpOLE(lhs, rhs);
		case Token::GT	: return builder_.CreateFCmpOGT(lhs, rhs);
		case Token::GTEQ: return builder_.CreateFCmpOGE(lhs, rhs);
		default: ;
		}
	}

	// << >>
	if (be->lhs->type->is(Int_) || be->lhs->type->is(Char_)) {
		switch (be->token.getType()) {
		case Token::LTLT: return builder_.CreateShl(lhs, rhs);
		case Token::GTGT: return builder_.CreateLShr(lhs, rhs);
		default: ;
		}
	}

	// + - * / (%)
	if (be->lhs->type->is(Int_) || be->lhs->type->is(Char_)) {
		switch (be->token.getType()) {
		case Token::PLUS: return builder_.CreateAdd(lhs, rhs);
		case Token::MINUS: return builder_.CreateSub(lhs, rhs);
		case Token::STAR: return builder_.CreateMul(lhs, rhs);
		case Token::SLASH: return builder_.CreateSDiv(lhs, rhs);
		case Token::PERC: return builder_.CreateSRem(lhs, rhs);
		default: ;
		}
	}
	if (be->lhs->type->is(Float_) || be->lhs->type->is(Double_)) {
		switch (be->token.getType()) {
		case Token::PLUS: return builder_.CreateFAdd(lhs, rhs);
		case Token::MINUS: return builder_.CreateFSub(lhs, rhs);
		case Token::STAR: return builder_.CreateFMul(lhs, rhs);
		case Token::SLASH: return builder_.CreateFDiv(lhs, rhs);
		default: ;
		}
	}

	// + for String
	if (be->lhs->type->unmodify()->is(String_) && be->token.getType() == Token::PLUS) {
		std::vector<llvm::Value *> params;
		params.push_back(lhs);
		params.push_back(rhs);

		llvm::Function *func = module_.getFunction("PRStringConcatenate");
		assert(func != NULL);

		builder_.SetInsertPoint(blocks.back().body);
		return builder_.CreateCall(func, params);
	}

	assert(false && "no matching binary expression operator");
	return NULL;
}

llvm::Value *LLVMCodeGen::Impl::generateUnaryExpr(UnaryExpr *ue) {
	DBG_PRINT(+-, generateUnaryExpr);
	llvm::Value *rhs = generateExpr(ue->rhs);

	if (ue->token.getType() == Token::PLUS) {
		assert(ue->rhs->type->is(Int_) || ue->rhs->type->is(Char_)
				|| ue->rhs->type->is(Float_) || ue->rhs->type->is(Double_));

		return generateExpr(ue->rhs);
	} else if (ue->token.getType() == Token::MINUS) {
		assert(ue->rhs->type->unmodify()->is(Int_) || ue->rhs->type->unmodify()->is(Char_)
				|| ue->rhs->type->unmodify()->is(Float_) || ue->rhs->type->unmodify()->is(Double_));

		builder_.SetInsertPoint(blocks.back().body);
		if (ue->rhs->type->unmodify()->is(Int_) || ue->rhs->type->unmodify()->is(Char_)) {
			return builder_.CreateNeg(rhs);
		} else if (ue->rhs->type->unmodify()->is(Float_) || ue->rhs->type->unmodify()->is(Double_)) {
			return builder_.CreateFNeg(rhs);
		} else {
			assert(false && "unknown type");
		}
	} else  if (ue->token.getType() == Token::EXCL) {
		assert(ue->rhs->type->unmodify()->is(Bool_));

		builder_.SetInsertPoint(blocks.back().body);
		return builder_.CreateNot(rhs);
	} else {
		assert(false && "unknown binary operand");
	}
	return NULL;
}

llvm::Value *LLVMCodeGen::Impl::generateLabelLiteralExpr(Label *label) {
	DBG_PRINT(+, generateLabelLiteralExpr);

	llvm::Value *found = lookup(label->symbol->getMangledSymbolName());
	assert(found != NULL);

	DBG_PRINT(-, generateLabelLiteralExpr);
	return found;
}

llvm::Value *LLVMCodeGen::Impl::generateStrLiteralExpr(StrLiteralExpr *sle) {
	DBG_PRINT(+, generateStrLiteralExpr);


	builder_.SetInsertPoint(blocks.back().body);
	llvm::Value *dest = builder_.CreateAlloca(getLLVMType(sle->type), 0, "strLiteral" + getUniqNumStr());

	generateConstructor(dest, sle->type, sle);

	builder_.SetInsertPoint(blocks.back().body);
	DBG_PRINT(-, generateStrLiteralExpr);
	return builder_.CreateLoad(dest);
}

llvm::Value *LLVMCodeGen::Impl::generateIntLiteralExpr(IntLiteralExpr *ile) {
	DBG_PRINT(+-, generateIntLiteralExpr);
	return llvm::ConstantInt::get(getLLVMType(Int_), ile->integer);
}

llvm::Value *LLVMCodeGen::Impl::generateFloatLiteralExpr(FloatLiteralExpr *fle) {
	DBG_PRINT(+-, generateFloatLiteralExpr);
	return llvm::ConstantFP::get(getLLVMType(Double_), fle->float_);
}

llvm::Value *LLVMCodeGen::Impl::generateCharLiteralExpr(CharLiteralExpr *cle) {
	DBG_PRINT(+-, generateCharLiteralExpr);
	return llvm::ConstantInt::get(getLLVMType(Char_), cle->char_);
}

llvm::Value *LLVMCodeGen::Impl::generateBoolLiteralExpr(BoolLiteralExpr *ble) {
	DBG_PRINT(+-, generateBoolLiteralExpr);
	return llvm::ConstantInt::get(getLLVMType(Bool_), ble->bool_);
}

llvm::Value *LLVMCodeGen::Impl::generateArrayLiteralExpr(ArrayLiteralExpr *ale) {
	DBG_PRINT(+, generateArrayLiteralExpr);

	DBG_PRINT(-, generateArrayLiteralExpr);
	return NULL;
}

llvm::Value *LLVMCodeGen::Impl::generateFuncCallExpr(FuncCallExpr *fce) {
	DBG_PRINT(+, generateFuncCallExpr);

	Symbol *symbol = NULL;

	if (fce->func->getASTType() == AST::IDENTIFIER) {
		Identifier *id = static_cast<Identifier *>(fce->func);
		symbol = id->symbol;
	} else if (fce->func->getASTType() == AST::STATIC_MEMBER_EXPR) {
		StaticMemberExpr *sme = static_cast<StaticMemberExpr *>(fce->func);
		symbol = sme->member->symbol;
	} else {
		assert(false && "member instruction not supported yet!");
	}
	assert(symbol != NULL);

	FuncType *ft = static_cast<FuncType *>(symbol->getType());

	std::vector<llvm::Value *> params;
	for (std::vector<Expr *>::iterator it = fce->params.begin(); it != fce->params.end(); ++it) {
		if (ft->getCar()->getTypeType() == Type::MODIFIER_TYPE
			&& static_cast<ModifierType *>(ft->getCar())->isRef()) {
			params.push_back(generateExpr(*it));
		} else {
			params.push_back(generateExpr(*it));
		}

		ft = static_cast<FuncType *>(ft->getCdr());
	}

	std::string funcName;
	if (symbol->getSymbolType() == Symbol::EXTERN_SYMBOL) {
		funcName = symbol->getSymbolName();
	} else {
		funcName = symbol->getMangledSymbolName();
	}

	llvm::Function *func = module_.getFunction(funcName);
	assert(func != NULL);

	builder_.SetInsertPoint(blocks.back().body);
	DBG_PRINT(-, generateFuncCallExpr);
	return builder_.CreateCall(func, params);
}

// generateConstructorExpr and generateConstructor are completely different!
llvm::Value *LLVMCodeGen::Impl::generateConstructorExpr(ConstructorExpr *ce) {
	DBG_PRINT(+, generateConstructorExpr);

	builder_.SetInsertPoint(blocks.back().body);
	llvm::Value *dest = builder_.CreateAlloca(getLLVMType(ce->type), 0, "constructed" + getUniqNumStr());

	generateConstructor(dest, ce->type, ce);

	builder_.SetInsertPoint(blocks.back().body);
	DBG_PRINT(-, generateConstructorExpr);
	return builder_.CreateLoad(dest);
}

void LLVMCodeGen::Impl::generateVarDefStmt(VarDefStmt *vds) {
	DBG_PRINT(+, generateVarDefStmt);
	assert(vds != NULL);

	llvm::Value *to = NULL;
	if (isNamespaceGlobal()) {
		to = lookup(vds->symbol->getMangledSymbolName());
	} else {
		builder_.SetInsertPoint(blocks.back().body);
		to = builder_.CreateAlloca(getLLVMType(vds->symbol->getType()), 0,
									vds->symbol->getMangledSymbolName());
	}
	assert(to != NULL);

	generateConstructor(to, vds->symbol->getType(), vds->init);


	DBG_PRINT(-, generateVarDefStmt);
	return;
}

// initialize allocated variable dest with init
// TODO: to run copy constructor
// TODO: to run copy constructor and destructor on assignment
// TODO: to run destructor in cleanup
void LLVMCodeGen::Impl::generateConstructor(llvm::Value *dest, Type *type, Expr *init) {
	DBG_PRINT(+, generateConstructor);
	// these optimizations should be done in TypeResolver

	// rewrite such code:
	// 	var str :: String = String("this is string!")
	//
	// like this:
	// 	var str :: String = "this is string!"

	if (init != NULL) {
		while (true) {
			if (init->getASTType() != AST::CONSTRUCTOR_EXPR)
				break;

			ConstructorExpr *ce = static_cast<ConstructorExpr *>(init);
			if (ce->type->is(type) && ce->params.size() == 1
					&& ce->params[0]->type->unmodify()->is(type->unmodify())) {
				init = ce->params[0];
			} else {
				break;
			}
		}
	}

	// rewrite such code:
	//	var str :: String = String()
	//like this:
	//	var str :: String

	if (init != NULL && init->getASTType() == AST::CONSTRUCTOR_EXPR) {
		ConstructorExpr *ce = static_cast<ConstructorExpr *>(init);
		if (ce->type->is(type) && ce->params.size() == 0) {
			init = NULL;
		}
	}

	assert(init != NULL ? init->type != NULL : true);
	assert(init != NULL ? init->type->unmodify()->is(type->unmodify()) : true);

	// generate constructor

	// the type is a reference
	if (type->getTypeType() == Type::MODIFIER_TYPE) {
		ModifierType *mt = static_cast<ModifierType *>(type);
		if (mt->isRef()) {
			assert(init != NULL);

			builder_.SetInsertPoint(blocks.back().body);
			llvm::Value *src = generateExpr(init);
			builder_.CreateStore(src, dest);
		} else {
			type = mt->getElemType();
		}
	}

	// these types are primitive
	if (type->is(Int_) || type->is(Char_) || type->is(Bool_)
		|| type->is(Float_) || type->is(Double_) || type->is(Label_)) {
		assert(!(type->is(Label_)));

		// with the type primitive, you don't have to worry about overhead
		// also, you don't have to worry about running copy constructors

		llvm::Value *src = NULL;

		if (init != NULL) {
			assert(init->getASTType() != AST::CONSTRUCTOR_EXPR);
			src = generateExpr(init);
		} else {
			if (type->is(Int_) || type->is(Char_) || type->is(Bool_)) {
				src = llvm::ConstantInt::get(getLLVMType(type), 0);
			} else if (type->is(Float_) || type->is(Double_)) {
				src = llvm::ConstantFP::get(getLLVMType(type), 0.0);
			}
		}

		assert(src != NULL);

		builder_.SetInsertPoint(blocks.back().body);
		builder_.CreateStore(src, dest);

		return;
	// these types are not primitive but also built-in type
	} else if (type->is(String_)) {
		// TODO: I'm thinking of rewriting String as a native one

		// normal constructor
		if (init != NULL && init->getASTType() == AST::STR_LITERAL_EXPR) {
			StrLiteralExpr *sle = static_cast<StrLiteralExpr *>(init);

			std::vector<llvm::Value *> params;
			params.push_back(builder_.CreateGlobalStringPtr(sle->str));

			llvm::Function *func = module_.getFunction("PRStringConstructorCStr");
			assert(func != NULL);

			builder_.SetInsertPoint(blocks.back().body);

			llvm::Value *src = builder_.CreateCall(func, params);
			builder_.CreateStore(src, dest);

			DBG_PRINT(-, generateConstructor);
			return;

		// int to string constructor
		} else if (init != NULL && init->getASTType() == AST::CONSTRUCTOR_EXPR) {
			ConstructorExpr *ce = static_cast<ConstructorExpr *>(init);
			assert(ce->params.size() == 1);
			assert(ce->params[0]->type->unmodify()->is(Int_));

			std::vector<llvm::Value *> params;
			params.push_back(generateExpr(ce->params[0]));

			llvm::Function *func = module_.getFunction("PRStringConstructorInt");
			assert(func != NULL);

			builder_.SetInsertPoint(blocks.back().body);

			llvm::Value *src = builder_.CreateCall(func, params);
			builder_.CreateStore(src, dest);

			DBG_PRINT(-, generateConstructor);
			return;

		// copy constructor
		} else if (init != NULL) {
			// TODO: in some case you have to run copy constructors there
			assert(init->getASTType() != AST::CONSTRUCTOR_EXPR);

			llvm::Value *src = generateExpr(init);

			builder_.CreateStore(src, dest);

			DBG_PRINT(-, generateConstructor);
			return;

		// normal constructor with no argument
		} else {
			llvm::Function *func = module_.getFunction("PRStringConstructorVoid");
			assert(func != NULL);

			builder_.SetInsertPoint(blocks.back().body);

			llvm::Value *src = builder_.CreateCall(func);
			builder_.CreateStore(src, dest);

			DBG_PRINT(-, generateConstructor);
			return;
		}
	} else if (type->getTypeType() == Type::ARRAY_TYPE) {

		// TODO: the case wich doesn't match this will needs copy constructor
		assert(init == NULL ? true
				    : init->getASTType() == AST::ARRAY_LITERAL_EXPR
					|| init->getASTType() == AST::CONSTRUCTOR_EXPR);

		ArrayType *at = static_cast<ArrayType *>(type);

		if (init != NULL && init->getASTType() == AST::CONSTRUCTOR_EXPR) {
			ConstructorExpr *ce = static_cast<ConstructorExpr *>(init);
			if (ce->params.size() == 0) {
			} else if (ce->params.size() == 1) {
				assert(ce->params[0]->type->unmodify()->is(Int_));
			} else if (ce->params.size() == 2){
				assert(ce->params[0]->type->unmodify()->is(Int_));
				assert(ce->params[1]->type->unmodify()->is(at->getElemType()->unmodify()));
			} else {
				assert(false && "unknown constructor parameters");
			}
		}


		const int kArrayDefaultCapacity = 1;

		// int length
		// int capacity
		// int elementSize
		// Type* elements for [ Type ]

		// Overview of the following LLVM IR code generation LLVM is like this:
		// ; [Type]* is not valid LLVM type
		//
		// %length = getelementptr [Type]* %dest, i32 0, i32 0
		// store i32 0, i32* %length
		//
		// %capacity = getelementptr [Type]* %dest, i32 0, i32 1
		// store i32 kArrayDefaultCapacity, i32* %capacity
		//
		// ; elementSize can be obtained from technique in the page:
		// ; http://nondot.org/sabre/LLVMNotes/SizeOf-OffsetOf-VariableSizedStructs.txt
		// %elementSize = getelementptr [Type]* %dest, i32 0, i32 2
		// %elementSizeTester = getelementptr Type* null, i32 1
		// %elementSizeValue = bitcast Type* %Size to i32
		// store i32 %elementSizeValue, i32* %elementSize
		//
		// %elements = getelementptr [Type]* %dest, i32 0, i32 3
		// %capacityValue = load i32* %capacity
		// %mallocedSize = mul i32 %capacityValue %elementSizeValue
		// %malloced = call i8* @PRMalloc(i32 %mallocedSize)
		// %castedMalloced = bitcast i8* %malloced to Type*
		// store Type* %castedMalloced, Type** %elements
		//
		// ; then, generateConstructor(malloced, Type, NULL) if needed

		builder_.SetInsertPoint(blocks.back().body);

		llvm::Type *lvInt = getLLVMType(Int_);
		//llvm::Type *lvArrayType = getLLVMType(at);
		llvm::Type *lvType = getLLVMType(at->getElemType());

		// %length = getelementptr [Type]* %dest, i32 0, i32 0
		// store i32 0, i32* %length
		std::vector<llvm::Value *> lengthParams;
		lengthParams.push_back(llvm::ConstantInt::get(lvInt, 0));
		lengthParams.push_back(llvm::ConstantInt::get(lvInt, 0));
		llvm::Value *length = builder_.CreateGEP(dest, lengthParams);
		builder_.CreateStore(llvm::ConstantInt::get(lvInt, 0), length);

		// %capacity = getelementptr [Type]* %dest, i32 0, i32 1
		std::vector<llvm::Value *> capacityParams;
		capacityParams.push_back(llvm::ConstantInt::get(lvInt, 0));
		capacityParams.push_back(llvm::ConstantInt::get(lvInt, 1));
		llvm::Value *capacity = builder_.CreateGEP(dest, capacityParams);

		builder_.SetInsertPoint(blocks.back().body);
		llvm::Value *capValCE = NULL;
		if (init == NULL) {
			// store i32 kArrayDefaultCapacity, i32* %capacity
			builder_.CreateStore(llvm::ConstantInt::get(lvInt, kArrayDefaultCapacity), capacity);
		} else if (init->getASTType() == AST::ARRAY_LITERAL_EXPR) {
			// store i32 ale->elements.size(), i32* %capacity
			builder_.CreateStore(llvm::ConstantInt::get(lvInt,
				static_cast<ArrayLiteralExpr *>(init)->elements.size()), capacity);
		} else if (init->getASTType() == AST::CONSTRUCTOR_EXPR) {
			ConstructorExpr *ce = static_cast<ConstructorExpr *>(init);
			if (ce->params.size() > 0) {
				capValCE = generateExpr(ce->params[0]);
				builder_.SetInsertPoint(blocks.back().body);
				builder_.CreateStore(capValCE, capacity);
			}
		} else {
			assert(false && "unknown array constructing expression");
		}
		builder_.SetInsertPoint(blocks.back().body);

		// %elementSize = getelementptr [Type]* %dest, i32 0, i32 2
		// %elementSizeTester = getelementptr Type* null, i32 1
		// %elementSizeValue = ptrtoint Type* %Size to i32
		// store i32 %elementSizeValue, i32* %elementSize
		std::vector<llvm::Value *> elementSizeParams;
		elementSizeParams.push_back(llvm::ConstantInt::get(lvInt, 0));
		elementSizeParams.push_back(llvm::ConstantInt::get(lvInt, 2));
		llvm::Value *elementSize = builder_.CreateGEP(dest, elementSizeParams);
		llvm::Value *elementSizeTester = builder_.CreateGEP(
				llvm::ConstantPointerNull::get(lvType->getPointerTo()),
				llvm::ConstantInt::get(lvInt, 2));
		llvm::Value *elementSizeValue = builder_.CreatePtrToInt(elementSizeTester, lvInt);
		builder_.CreateStore(elementSizeValue, elementSize);

		// %elements = getelementptr [Type]* %dest, i32 0, i32 3
		// %capacityValue = load i32* %capacity
		// %mallocedSize = mul i32 %capacityValue %elementSizeValue
		// %malloced = call i8* @PRMalloc(i32 %mallocedSize)
		// %castedMalloced = bitcast i8* %malloced to Type*
		// store Type* %castedMalloced, Type** %elements
		std::vector<llvm::Value *> elementsParams;
		elementsParams.push_back(llvm::ConstantInt::get(lvInt, 0));
		elementsParams.push_back(llvm::ConstantInt::get(lvInt, 3));
		llvm::Value *elements = builder_.CreateGEP(dest, elementsParams);
		llvm::Value *capacityValue = builder_.CreateLoad(capacity);
		llvm::Value *mallocedSize = builder_.CreateMul(capacityValue, elementSizeValue);
		llvm::Value *malloced = builder_.CreateCall(lookup("PRMalloc"), mallocedSize);
		llvm::Value *castedMalloced = builder_.CreateBitCast(malloced, lvType->getPointerTo());
		builder_.CreateStore(castedMalloced, elements);

		if (init != NULL && init->getASTType() == AST::ARRAY_LITERAL_EXPR) {
			ArrayLiteralExpr *ale = static_cast<ArrayLiteralExpr *>(init);

			for (int i = 0, iMax = ale->elements.size(); i < iMax; ++i) {

				// %initialized = getelementptr Type* %castedMalloced, i32 i
				llvm::Value *initialized = builder_.CreateGEP(castedMalloced, llvm::ConstantInt::get(lvInt, i));
				assert(at->getElemType()->is(ale->elements[i]->type));
				generateConstructor(initialized, at->getElemType(), ale->elements[i]);
			}
		} else if (init != NULL && init->getASTType() == AST::CONSTRUCTOR_EXPR) {
			ConstructorExpr *ce = static_cast<ConstructorExpr *>(init);
			if (ce->params.size() > 0) {
				assert(capValCE != NULL);

				const std::string curNumStr = getUniqNumStr();

				builder_.SetInsertPoint(blocks.back().body);
				llvm::Value *counter = builder_.CreateAlloca(
						getLLVMType(Int_), 0, "arrayLoopCounter" + curNumStr);
				builder_.CreateStore(llvm::ConstantInt::get(getLLVMType(Int_), 0), counter);

				// arrayLoopCond
				llvm::Function *func = getEnclosingFunc();
				llvm::BasicBlock *loopCond =
					llvm::BasicBlock::Create(context_,
							"arrayLoopCond" + curNumStr, func);
				// arrayLoopBody
				llvm::BasicBlock *loopBody =
					llvm::BasicBlock::Create(context_,
							"arrayLoopBody" + curNumStr, func);
				// arrayLoopAfter
				llvm::BasicBlock *loopAfter =
					llvm::BasicBlock::Create(context_,
							"arrayLoopAfter" + curNumStr, func);

				builder_.SetInsertPoint(blocks.back().body);
				builder_.CreateBr(loopCond);

				builder_.SetInsertPoint(loopCond);
				// if (counter < capValCE) { goto arrayLoopBody } else { goto arrayLoopAfter }
				builder_.CreateCondBr(
					builder_.CreateICmpSLT(builder_.CreateLoad(counter), capValCE),
					loopBody,
					loopAfter);

				builder_.SetInsertPoint(loopBody);
				blocks.back().body = loopBody;

				llvm::Value *initialized =
					builder_.CreateGEP(castedMalloced, builder_.CreateLoad(counter));

				if (ce->params.size() == 1) {
					generateConstructor(initialized, at->getElemType(), NULL);
				} else if (ce->params.size() == 2) {
					generateConstructor(initialized, at->getElemType(), ce->params[1]);
				} else {
					assert(false && "unknown constructor");
				}

				// counter += 1
				builder_.SetInsertPoint(blocks.back().body);
				builder_.CreateStore(
					builder_.CreateAdd(builder_.CreateLoad(counter),
							llvm::ConstantInt::get(getLLVMType(Int_), 1)), counter);
				builder_.CreateBr(loopCond);

				builder_.SetInsertPoint(loopAfter);
				blocks.back().body = loopAfter;
			}
		} else if (init != NULL) {
			assert(false && "unkown initializer");
		}

		DBG_PRINT(-, generateConstructor);
		return;
	} else {
		// [class] TODO: you have to support class there
		assert(false && "class not supported yet");
	}
}

void LLVMCodeGen::Impl::generateInstStmt(InstStmt *is) {
	DBG_PRINT(+, generateInstStmt);

	Symbol *symbol = NULL;

	if (is->inst->getASTType() == AST::IDENTIFIER) {
		Identifier *id = static_cast<Identifier *>(is->inst);
		symbol = id->symbol;
	} else if (is->inst->getASTType() == AST::STATIC_MEMBER_EXPR) {
		StaticMemberExpr *sme = static_cast<StaticMemberExpr *>(is->inst);
		symbol = sme->member->symbol;
	} else {
		assert(false && "member instruction not supported yet!");
	}
	assert(symbol != NULL);

	FuncType *ft = static_cast<FuncType *>(symbol->getType());

	std::vector<llvm::Value *> params;
	for (std::vector<Expr *>::iterator it = is->params.begin(); it != is->params.end(); ++it) {
		params.push_back(generateExpr(*it));

		ft = static_cast<FuncType *>(ft->getCdr());
	}

	std::string funcName;
	if (symbol->getSymbolType() == Symbol::EXTERN_SYMBOL) {
		funcName = symbol->getSymbolName();
	} else {
		funcName = symbol->getMangledSymbolName();
	}

	llvm::Function *func = module_.getFunction(funcName);
	assert(func != NULL);

	builder_.SetInsertPoint(blocks.back().body);
	builder_.CreateCall(func, params);

	DBG_PRINT(-, generateInstStmt);
	return;
}

void LLVMCodeGen::Impl::generateAssignStmt(AssignStmt *as) {
	DBG_PRINT(+, generateAssignStmt);

	llvm::Value *beforePtr = generateExpr(as->lhs);
	assert(beforePtr != NULL);

	builder_.SetInsertPoint(blocks.back().body);
	llvm::Value *before = builder_.CreateLoad(beforePtr);

	llvm::Value *after = NULL;

	llvm::Value *rhs = NULL;
	if (as->rhs != NULL) {
		rhs = generateExpr(as->rhs);
	}

	builder_.SetInsertPoint(blocks.back().body);

	switch (as->token.getType()) {
	case Token::PLUSPLUS:
		after = builder_.CreateAdd(before, llvm::ConstantInt::get(getLLVMType(Int_), 1));
		break;
	case Token::MINUSMINUS:
		after = builder_.CreateSub(before, llvm::ConstantInt::get(getLLVMType(Int_), 1));
		break;
	case Token::EQL:
		assert(rhs != NULL);
		after = rhs;
		break;
	case Token::PLUSEQ:
		assert(rhs != NULL);
		if (as->lhs->type->unmodify()->is(Int_) || as->lhs->type->unmodify()->is(Char_)) {
			after = builder_.CreateAdd(before, rhs);
		} else if (as->lhs->type->unmodify()->is(Float_) || as->lhs->type->unmodify()->is(Double_)) {
			after = builder_.CreateFAdd(before, rhs);
		} else if (as->lhs->type->unmodify()->is(String_)) {
			std::vector<llvm::Value *> params;
			params.push_back(before);
			params.push_back(rhs);

			llvm::Function *func = module_.getFunction("PRStringConcatenate");
			assert(func != NULL);

			builder_.SetInsertPoint(blocks.back().body);
			after = builder_.CreateCall(func, params);
		} else {
			assert(false && "unknown type");
		}
		break;
	case Token::MINUSEQ:
		assert(rhs != NULL);
		if (as->lhs->type->unmodify()->is(Int_) || as->lhs->type->unmodify()->is(Char_)) {
			after = builder_.CreateSub(before, rhs);
		} else if (as->lhs->type->unmodify()->is(Float_) || as->lhs->type->unmodify()->is(Double_)) {
			after = builder_.CreateFSub(before, rhs);
		} else {
			assert(false && "unknown type");
		}
		break;
	case Token::STAREQ:
		assert(rhs != NULL);
		if (as->lhs->type->unmodify()->is(Int_) || as->lhs->type->unmodify()->is(Char_)) {
			after = builder_.CreateMul(before, rhs);
		} else if (as->lhs->type->unmodify()->is(Float_) || as->lhs->type->unmodify()->is(Double_)) {
			after = builder_.CreateFMul(before, rhs);
		} else {
			assert(false && "unknown type");
		}
		break;
	case Token::SLASHEQ:
		assert(rhs != NULL);
		if (as->lhs->type->unmodify()->is(Int_) || as->lhs->type->unmodify()->is(Char_)) {
			after = builder_.CreateSDiv(before, rhs);
		} else if (as->lhs->type->unmodify()->is(Float_) || as->lhs->type->unmodify()->is(Double_)) {
			after = builder_.CreateFDiv(before, rhs);
		} else {
			assert(false && "unknown type");
		}
		break;
	default: assert(false && "unknown assign operand");
	}

	assert(after != NULL);

	builder_.SetInsertPoint(blocks.back().body);

	builder_.CreateStore(after, beforePtr);

	DBG_PRINT(-, generateAssignStmt);
	return;
}

void LLVMCodeGen::Impl::generateIfStmt(IfStmt *is) {
	DBG_PRINT(+, generateIfStmt);
	const std::string curNumStr = getUniqNumStr();

	llvm::Function *func = getEnclosingFunc();

	llvm::BasicBlock *ifAfter = llvm::BasicBlock::Create(context_, "ifAfter" + curNumStr, func);

	// if
	{
		llvm::Value *ifCond = generateExpr(is->ifCond);

		llvm::BasicBlock *prevBody = blocks.back().body;

		Block block(Block::UNKNOWN_BLOCK);
		generateCompStmt(is->ifThen, true, false, block);

		// block for next else if
		blocks.back().body = llvm::BasicBlock::Create(context_, "ifElseIf" + curNumStr, func);

		// manually connect compound statement blocks
		builder_.SetInsertPoint(prevBody);
		builder_.CreateCondBr(ifCond, block.head, blocks.back().body);

		builder_.SetInsertPoint(block.end);
		builder_.CreateBr(ifAfter);
	}

	// else if
	for (int i = 0, iEnd = is->elseIfCond.size(); i < iEnd; ++i) {
		llvm::Value *elseIfCond = generateExpr(is->elseIfCond[i]);

		llvm::BasicBlock *prevBody = blocks.back().body;

		Block block(Block::UNKNOWN_BLOCK);
		generateCompStmt(is->elseIfThen[i], true, false, block);

		// block for next else if
		blocks.back().body = llvm::BasicBlock::Create(context_, "ifElseIf" + curNumStr, func);

		// manually connect compound statement blocks
		builder_.SetInsertPoint(prevBody);
		builder_.CreateCondBr(elseIfCond, block.head, blocks.back().body);

		builder_.SetInsertPoint(block.end);
		builder_.CreateBr(ifAfter);

	}

	// else
	if (is->elseThen != NULL) {
		llvm::BasicBlock *prevBody = blocks.back().body;

		Block block(Block::UNKNOWN_BLOCK);
		generateCompStmt(is->elseThen, true, false, block);

		// manually connect compound statement blocks
		builder_.SetInsertPoint(prevBody);
		builder_.CreateBr(block.head);

		builder_.SetInsertPoint(block.end);
		builder_.CreateBr(ifAfter);
	} else {
		builder_.SetInsertPoint(blocks.back().body);
		builder_.CreateBr(ifAfter);
	}

	blocks.back().body = ifAfter;

	DBG_PRINT(-, generateIfStmt);
}

void LLVMCodeGen::Impl::generateRepeatStmt(RepeatStmt *rs) {
	DBG_PRINT(+, generateRepeatStmt);

	// Flow of the process:
	//
	// (blocks.back().body -> ) repeatInit (-> repeatCond)
	// repeatCond (-> repeatBodyEntry or -> repeatAfter)
	// repeatIncr (-> repeatCond)
	// [repeatBodyEntry -> repeatBodyEnd] (-> repeatIncr)
	// repeatAfter ( = blocks.back().body)
	
	const std::string curNumStr = getUniqNumStr();

	llvm::Function *func = getEnclosingFunc();
	llvm::BasicBlock *repeatInit = llvm::BasicBlock::Create(context_, "repeatInit" + curNumStr, func);
	llvm::BasicBlock *repeatCond = llvm::BasicBlock::Create(context_, "repeatCond" + curNumStr, func);
	llvm::BasicBlock *repeatIncr = llvm::BasicBlock::Create(context_, "repeatIncr" + curNumStr, func);
	llvm::BasicBlock *repeatBodyEntry = llvm::BasicBlock::Create(context_, "repeatBodyEntry" + curNumStr, func);
	llvm::BasicBlock *repeatBodyEnd = llvm::BasicBlock::Create(context_, "repeatBodyEnd" + curNumStr, func);
	llvm::BasicBlock *repeatAfter = llvm::BasicBlock::Create(context_, "repeatAfter" + curNumStr, func);

	// begin (blocks.back().body -> ) repeatInit (-> repeatCond)

	builder_.SetInsertPoint(blocks.back().body);
	builder_.CreateBr(repeatInit);

	{
		Block block(Block::COMP_BLOCK);
		block.body = repeatInit;
		block.head = repeatInit;
		block.end = repeatAfter;

		blocks.push_back(block);
	}

	llvm::Value *cntMax = rs->count != NULL ? generateExpr(rs->count) : NULL;

	// TODO: add "isDisallowedIdentifier" in SymbolRegister

	Symbol *cntSymbol = rs->scope->resolve("cnt", rs->token.getPosition());
	assert(cntSymbol != NULL);
	assert(cntSymbol->getType()->is(Int_));

	builder_.SetInsertPoint(blocks.back().body);
	llvm::Value *cnt = builder_.CreateAlloca(
			getLLVMType(Int_), 0, cntSymbol->getMangledSymbolName());

	builder_.CreateStore(llvm::ConstantInt::get(getLLVMType(Int_), 0), cnt);

	builder_.CreateBr(repeatCond);

	assert(blocks.back().type == Block::COMP_BLOCK);
	blocks.pop_back();

	// end (blocks.back().body -> ) repeatInit (-> repeatCond)

	// begin repeatCond (-> repeatBodyEntry or -> repeatAfter)

	{
		builder_.SetInsertPoint(repeatCond);

		if (cntMax != NULL) {
			llvm::Value *cntVal = builder_.CreateLoad(cnt);
			llvm::Value *cntCmp = builder_.CreateICmpSLT(cntVal, cntMax);
			builder_.CreateCondBr(cntCmp, repeatBodyEntry, repeatAfter);
		} else {
			// it is an infinite loop
			builder_.CreateBr(repeatBodyEntry);
		}
	}

	// end repeatCond (-> repeatBodyEntry)

	// begin repeatIncr (-> repeatCond)
	
	{
		builder_.SetInsertPoint(repeatIncr);
		llvm::Value *cntVal = builder_.CreateLoad(cnt);
		llvm::Value *cntIncrVal =
			builder_.CreateAdd(cntVal,
				llvm::ConstantInt::get(getLLVMType(Int_), 1));
		builder_.CreateStore(cntIncrVal, cnt);
		builder_.CreateBr(repeatCond);
	}

	// end begin repeatIncr (-> repeatCond)

	// begin [repeatBodyEntry -> repeatBodyEnd] (-> repeatIncr)

	{
		Block block(Block::LOOP_BLOCK);
		block.body = repeatBodyEntry;
		block.end = repeatBodyEnd;
		block.continue_ = repeatIncr;
		block.break_ = repeatAfter;

		blocks.push_back(block);

		for (std::vector<Stmt *>::iterator it = rs->stmts.begin();
						it != rs->stmts.end(); ++it) {
			generateStmt(*it);
		}

		builder_.SetInsertPoint(blocks.back().body);
		builder_.CreateBr(blocks.back().end);

		builder_.SetInsertPoint(blocks.back().end);
		builder_.CreateBr(repeatIncr);

		assert(blocks.back().type == Block::LOOP_BLOCK);
		blocks.pop_back();
	}

	// end [repeatBodyEntry -> repeatBodyEnd] (-> repeatIncr)

	// begin repeatAfter ( = blocks.back().body)

	blocks.back().body = repeatAfter;

	// end repeatAfter ( = blocks.back().body)

	DBG_PRINT(-, generateRepeatStmt);
}

// TODO: make continue and break pass bodyBlockEnd
void LLVMCodeGen::Impl::generateContinueStmt(ContinueStmt *cs) {
	DBG_PRINT(+, generateContinueStmt);

	Block& loopBlock = getEnclosingLoopBlock();

	builder_.SetInsertPoint(blocks.back().body);
	builder_.CreateBr(loopBlock.continue_);

	const std::string curNumStr = getUniqNumStr();

	blocks.back().body = llvm::BasicBlock::Create(context_, "continueAfter" + curNumStr, getEnclosingFunc());

	DBG_PRINT(-, generateContinueStmt);
}

void LLVMCodeGen::Impl::generateBreakStmt(BreakStmt *bs) {
	DBG_PRINT(+, generateBreakStmt);

	Block& loopBlock = getEnclosingLoopBlock();

	builder_.SetInsertPoint(blocks.back().body);
	builder_.CreateBr(loopBlock.break_);

	const std::string curNumStr = getUniqNumStr();

	blocks.back().body = llvm::BasicBlock::Create(context_, "breakAfter" + curNumStr, getEnclosingFunc());

	DBG_PRINT(-, generateBreakStmt);
}

// TODO: after everything's completed
//
//
void LLVMCodeGen::Impl::generateGotoStmt(GotoStmt *gs) {
	assert(false && "goto statement not supported yet");
}

void LLVMCodeGen::Impl::generateLabelStmt(LabelStmt *ls) {
	assert(false && "label statement not supported yet");
}

void LLVMCodeGen::Impl::generateGosubStmt(GosubStmt *gs) {
	DBG_PRINT(+-, generateGosubStmt);
	assert(false && "gosub statement not supported yet");
}
void LLVMCodeGen::Impl::generateGlobalReturnStmt(ReturnStmt *rs) {
	DBG_PRINT(+-, generateGlobalReturnStmt);
	assert(false && "global return statement not supported yet");
}

void LLVMCodeGen::Impl::generateFuncReturnStmt(ReturnStmt *rs) {
	DBG_PRINT(+, generateFuncReturnStmt);

	llvm::Value *from = NULL;
	if (rs->expr != NULL)
		from = generateExpr(rs->expr);

	const std::string curNumStr = getUniqNumStr();

	// TODO: implement higher level unwind
	builder_.SetInsertPoint(blocks.back().body);
	Block& funcBlock = getEnclosingFuncBlock();
	if (from != NULL)
		builder_.CreateStore(from, funcBlock.retVal);
	builder_.CreateBr(funcBlock.end);
	blocks.back().body = llvm::BasicBlock::Create(context_, "returnAfter" + curNumStr, funcBlock.func);
	// little bit strange name from the viewpoint of English grammar

	DBG_PRINT(-, generateFuncReturnStmt);
	return;
}

llvm::Value *LLVMCodeGen::Impl::generateSubscrExpr(SubscrExpr *se) {
	DBG_PRINT(+, generateSubscrExpr);

	// 0: int length
	// 1: int capacity
	// 2: int elementSize
	// 3: Type* elements for [ Type ] (if Type = Int, then i32*)

	assert(se->array);
	assert(se->subscript);
	assert(se->subscript->type->unmodify()->is(Int_));

	builder_.SetInsertPoint(blocks.back().body);
	llvm::Value *array = generateExpr(se->array);
	llvm::Value *subscr = generateExpr(se->subscript);

	std::vector<llvm::Value *> elementsParams;
	elementsParams.push_back(llvm::ConstantInt::get(getLLVMType(Int_), 0));
	elementsParams.push_back(llvm::ConstantInt::get(getLLVMType(Int_), 3));
	llvm::Value *elements = builder_.CreateGEP(array, elementsParams);
	llvm::Value *elementsValue = builder_.CreateLoad(elements);
	
	std::vector<llvm::Value *> elementParams;
	elementParams.push_back(subscr);
	llvm::Value *element = builder_.CreateGEP(elementsValue, elementParams);

	DBG_PRINT(-, generateSubscrExpr);

	return element;
}

llvm::Value *LLVMCodeGen::Impl::lookup(const std::string& str) {
	{
		llvm::ValueSymbolTable& funcVST = getEnclosingFunc()->getValueSymbolTable();

		llvm::Value *found = funcVST.lookup(str);
		if (found != NULL)
			return found;
	}

	{
		llvm::ValueSymbolTable& globalVST = module_.getValueSymbolTable();

		llvm::Value *found = globalVST.lookup(str);
		if (found != NULL)
			return found;
	}

	return NULL;
}

};

