#ifndef PERYAN_LLVM_CODE_GEN_H__
#define PERYAN_LLVM_CODE_GEN_H__

#include "CodeGen.h"

namespace Peryan {

class Parser;

class LLVMCodeGen : public CodeGen {
private:
	class Impl;
	Impl *impl_;
public:
	LLVMCodeGen(Parser& parser, const std::string& fileName);

	virtual void generate();

	virtual ~LLVMCodeGen();
};

}

#endif

