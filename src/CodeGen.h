#ifndef PERYAN_CODE_GEN_H__
#define PERYAN_CODE_GEN_H__

namespace Peryan {

class CodeGen {
private:
	CodeGen(const CodeGen&);
	CodeGen& operator=(const CodeGen&);

public:
	CodeGen() {}
	virtual void generate() = 0;
	virtual ~CodeGen() {}
};

}

#endif

