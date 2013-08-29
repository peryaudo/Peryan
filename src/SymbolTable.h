#ifndef PERYAN_SYMBOL_TABLE_H__
#define PERYAN_SYMBOL_TABLE_H__

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

#include "Token.h"

namespace Peryan {

class Lexer;

class SemanticsError : public std::exception {
private:
	Position position_;
	std::string message_;
public:
	SemanticsError(Position position, std::string message)
		: std::exception(), position_(position), message_(message) {}
	Position getPosition() { return position_; }
	std::string getMessage() { return message_; }
	std::string toString(const Lexer& lexer);

	virtual ~SemanticsError() throw() {}
};

class Symbol;

class Type {
private:
	std::string name_;
public:
	typedef enum {
		TYPE,
		MODIFIER_TYPE,
		ARRAY_TYPE,
		BUILTIN_TYPE,
		FUNC_TYPE,
		NAMESPACE_TYPE,
		CLASS_TYPE
	} TypeType;

	virtual TypeType getTypeType() { return TYPE; }

	Type(const std::string& name) : name_(name) {}
	virtual std::string getTypeName() { return name_; }

	virtual bool is(Type *to) {
		return this == to;
	}

	virtual Type *unmodify() { return this; }

	virtual ~Type() {}
};

class ModifierType : public Type {
private:
	Type *elemType_;
	bool isConst_, isRef_;
public:
	ModifierType(bool isConst, bool isRef, Type *elemType)
		: Type(std::string(isConst ? "const" : "")
				+ std::string(isConst && isRef ? " " : "")
				+ std::string(isRef ? "ref" : ""))
		, elemType_(elemType)
		, isConst_(isConst), isRef_(isRef) {
			assert(isConst || isRef);
		}

	virtual TypeType getTypeType() { return MODIFIER_TYPE; }

	Type *getElemType() { return elemType_; }

	virtual std::string getTypeName() {
		return std::string(isConst_ ? "const " : "") + std::string(isRef_ ? "ref " : "")
			+ getElemType()->getTypeName();
	}
	bool isConst() { return isConst_; }
	bool isRef() { return isRef_; }

	virtual bool is(Type *to) {
		if (to->getTypeType() != MODIFIER_TYPE)
			return false;
		ModifierType *casted = static_cast<ModifierType *>(to);
		return isConst() == casted->isConst() && isRef() == casted->isRef()
			&& getElemType()->is(casted->getElemType());
	}

	virtual Type *unmodify() { return getElemType(); }
};

class ArrayType : public Type {
private:
	Type *elemType_;
public:
	ArrayType(Type *elemType) : Type("[]"), elemType_(elemType) {}

	virtual std::string getTypeName() {
		return std::string("[") + getElemType()->getTypeName() + std::string("]");
	}

	virtual TypeType getTypeType() { return ARRAY_TYPE; }

	Type *getElemType() { return elemType_; }

	virtual bool is(Type *to) {
		if (to->getTypeType() != ARRAY_TYPE)
			return false;
		return getElemType()->is(static_cast<ArrayType *>(to)->getElemType());
	}
};

class FuncType : public Type {
private:
	Type *car_, *cdr_;
public:
	FuncType(Type *car, Type *cdr) : Type("->"), car_(car), cdr_(cdr) {}

	virtual std::string getTypeName() {
		return car_->getTypeName() + std::string(" -> ") + cdr_->getTypeName();
	}

	virtual TypeType getTypeType() { return FUNC_TYPE; }
	Type *getCar() { return car_; }
	Type *getCdr() { return cdr_; }

	virtual bool is(Type *to) {
		if (to->getTypeType() != FUNC_TYPE)
			return false;

		return car_->is(static_cast<FuncType *>(to)->getCar()) &&
			cdr_->is(static_cast<FuncType *>(to)->getCdr());
	}

	Type *getReturnType() {
		Type *ret = this;
		while (ret->getTypeType() == FUNC_TYPE) {
			ret = static_cast<FuncType *>(ret)->getCdr();
			assert(ret != NULL);
		}
		return ret;
	}
};

class Scope {
private:
	Scope *enclosing_;
public:
	class iterator : public std::iterator<std::bidirectional_iterator_tag, Symbol *> {
	private:
		std::map<std::string, Symbol *>::iterator it_;
	public:
		iterator(std::map<std::string, Symbol *>::iterator it) : it_(it) {}
		Symbol* operator*() const { return (*it_).second; }
		iterator& operator++() { ++it_; return *this; }
		iterator& operator--() { --it_; return *this; }
		bool operator!=(const iterator& to) const { return it_ != to.it_; }
		bool operator==(const iterator& to) const { return it_ == to.it_; }
	};

	virtual iterator begin() = 0;
	virtual iterator end() = 0;

	typedef enum {
		SCOPE,
		BASE_SCOPE,
		GLOBAL_SCOPE,
		LOCAL_SCOPE
	} ScopeType;

	virtual ScopeType getScopeType() { return SCOPE; }

	virtual std::string getScopeName() { assert(false); }

	virtual Scope *getEnclosingScope() { return enclosing_; }
	virtual Scope *getParentScope() { return getEnclosingScope(); }

	// success: false, failed: true
	virtual bool define(Symbol *symbol) = 0;
	virtual Symbol *resolve(const std::string& name, Position curPos = 0) = 0;


	virtual std::string getMangledScopeName() {
		if (getEnclosingScope() != NULL)
			return getScopeName() + std::string("$") + getEnclosingScope()->getMangledScopeName();
		else 
			return getScopeName();
	}

	Scope(Scope *enclosing) : enclosing_(enclosing) {}

	virtual ~Scope() {};
};

class Symbol {
private:
	std::string name_;
	Type *type_;
	Scope *scope_;
	Position position_;
public:
	typedef enum{
		SYMBOL,
		EXTERN_SYMBOL,
		VAR_SYMBOL,
		BUILTIN_TYPE_SYMBOL,
		SCOPED_SYMBOL,
		FUNC_SYMBOL,
		CLASS_SYMBOL,
		NAMESPACE_SYMBOL,
		LABEL_SYMBOL
	} SymbolType;

	virtual SymbolType getSymbolType() { return SYMBOL; }

	std::string getSymbolName() { return name_; }
	void setScope(Scope *scope) { scope_ = scope; }
	Type *getType() { return type_; }
	void setType(Type *type) { type_ = type; }
	Type **getTypePtr() { return &type_; }
		// breaks encapsulation, but the easiest way to support type inference...

	Position getPosition() { return position_; }

	virtual std::string getMangledSymbolName() {
		// TODO: make the rule only when the symbol is
		// - not overrided
		// - global
		// the mangled name will be just getSymbolName()
		assert(scope_ != NULL);
		return getSymbolName() + std::string("$") + scope_->getMangledScopeName();
	}

	Symbol(const std::string& name, Position position)
		: name_(name), type_(NULL), scope_(NULL), position_(position) {}
	Symbol(const std::string& name, Type *type, Position position)
		: name_(name), type_(type), scope_(NULL), position_(position) {}

	virtual ~Symbol() {}
};

class ExternSymbol : public Symbol {
public:
	virtual SymbolType getSymbolType() { return EXTERN_SYMBOL; }

	ExternSymbol(const std::string& name, Position position)
		: Symbol(name, position) {}
	ExternSymbol(const std::string& name, Type *type, Position position)
		: Symbol(name, type, position) {}
};

class VarSymbol : public Symbol {
public:
	virtual SymbolType getSymbolType() { return VAR_SYMBOL; }

	VarSymbol(const std::string& name, Position position)
		: Symbol(name, position) {}
	VarSymbol(const std::string& name, Type *type, Position position)
		: Symbol(name, type, position) {}
};

class LabelSymbol : public Symbol {
public:
	virtual SymbolType getSymbolType() { return LABEL_SYMBOL; }

	LabelSymbol(const std::string& name, Position position)
		: Symbol(name, new Type("Label"), position) {}
};

class BuiltInTypeSymbol : public Symbol, public Type {
public:
	virtual TypeType getTypeType() { return BUILTIN_TYPE; }
	virtual SymbolType getSymbolType() { return BUILTIN_TYPE_SYMBOL; }

	BuiltInTypeSymbol(const std::string& name)
		: Symbol(name, 0), Type(name) {}
};

class ScopedSymbol : public Symbol, public Scope {
public:
	virtual SymbolType getSymbolType() { return SCOPED_SYMBOL; }

	ScopedSymbol(const std::string& name, Scope *enclosing, Position position)
		: Symbol(name, position), Scope(enclosing) {}

	ScopedSymbol(const std::string& name, Type *type, Scope *enclosing, Position position)
		: Symbol(name, type, position), Scope(enclosing) {}

	virtual bool define(Symbol *symbol) {
		if (setMember(symbol->getSymbolName(), symbol))
			return true;
		symbol->setScope(this);
		return false;
	}
	// [class] TODO: make functions and definitions in class able to be used before definition
	virtual Symbol *resolve(const std::string& name, Position curPos = 0) {
		Symbol *symbol = getMember(name);
		if (symbol != NULL && symbol->getPosition() <= curPos) {
			return symbol;
		} else if (getParentScope() != NULL) {
			return getParentScope()->resolve(name, curPos);
		} else {
			return NULL;
		}
	}

	virtual Symbol *getMember(const std::string& name) = 0;
	virtual bool setMember(const std::string& name, Symbol *symbol) = 0;
};

class FuncSymbol : public ScopedSymbol {
private:
	std::map<std::string, Symbol *> args_;
public:
	virtual iterator begin() { return iterator(args_.begin()); }
	virtual iterator end() { return iterator(args_.end()); }

	FuncSymbol(const std::string& name, Scope *parent, Position position)
		: ScopedSymbol(name, parent, position) {}

	virtual SymbolType getSymbolType() { return FUNC_SYMBOL; }

	virtual Symbol *getMember(const std::string& name) {
		if (args_.count(name)) {
			return args_[name];
		} else {
			return NULL;
		}
	}

	virtual bool setMember(const std::string& name, Symbol *symbol) {
		if (args_.count(name))
			return true;
		args_[name] = symbol;

		return false;
	}

	virtual std::string getScopeName() { return getSymbolName(); }
};

class ClassSymbol : public ScopedSymbol { 
public:
	virtual SymbolType getSymbolType() { return CLASS_SYMBOL; }
}; // STUB!!!

class NamespaceSymbol : public ScopedSymbol, public Type {
private:
	std::map<std::string, Symbol *> members_;
public:
	NamespaceSymbol(const std::string& name, Scope *parent, Position position)
		: ScopedSymbol(name, parent, position), Type(name) {}

	virtual SymbolType getSymbolType() { return NAMESPACE_SYMBOL; }
	virtual TypeType getTypeType() { return NAMESPACE_TYPE; }

	virtual iterator begin() { return iterator(members_.begin()); }
	virtual iterator end() { return iterator(members_.end()); }

	virtual Symbol *getMember(const std::string& name) {
		if (members_.count(name)) {
			return members_[name];
		} else {
			return NULL;
		}
	}

	virtual bool setMember(const std::string& name, Symbol *symbol) {
		if (members_.count(name))
			return true;
		members_[name] = symbol;

		return false;
	}

	virtual std::string getScopeName() { return getSymbolName(); }

	virtual Symbol *resolveMember(const std::string& name, Position curPos = 0) {
		if (members_.count(name) && members_[name]->getPosition() <= curPos) {
			return members_[name];
		} else {
			// don't unwind to the parent scope because it is member
			return NULL;
		}
	}
};

class BaseScope : public Scope {
private:
	std::map<std::string, Symbol *> symbols_;
public:
	virtual iterator begin() { return iterator(symbols_.begin()); }
	virtual iterator end() { return iterator(symbols_.end()); }

	BaseScope(Scope *parent) : Scope(parent) {}

	virtual ScopeType getScopeType() { return BASE_SCOPE; }

	virtual bool define(Symbol *symbol) {
		if (symbols_.count(symbol->getSymbolName()))
			return true;
		symbols_[symbol->getSymbolName()] = symbol;
		symbol->setScope(this);
		return false;
	}

	virtual Symbol *resolve(const std::string& name, Position curPos = 0) {
		if (symbols_.count(name) && symbols_[name]->getPosition() <= curPos) {
			return symbols_[name];
		} else if (getParentScope() != NULL) {
			return getParentScope()->resolve(name, curPos);
		} else {
			return NULL;
		}
	}
};

class GlobalScope : public BaseScope {
public:
	GlobalScope() : BaseScope(NULL) {}

	virtual ScopeType getScopeType() { return GLOBAL_SCOPE; }

	virtual std::string getScopeName() { return "global"; }
};

class LocalScope : public BaseScope {
private:
	std::string name_;
public:
	LocalScope(Scope *parent, Position position) : BaseScope(parent) {
		std::stringstream ss;
		ss<<"local"<<position;
		name_ = ss.str();
	}

	virtual ScopeType getScopeType() { return LOCAL_SCOPE; }

	virtual std::string getScopeName() { return name_; }
};

// type constraint class for type inference
// generally it will be *s == *t after unification
class TypeConst {
public:
	Type **s, **t;
	TypeConst(Type **s, Type **t) : s(s), t(t) {}
};


class SymbolTable {
private:
	SymbolTable(const SymbolTable&);
	SymbolTable& operator=(const SymbolTable&);

	GlobalScope *global_;

	// I think it can be rewritten by Union-Find or something
	std::vector<TypeConst> typeConstSet_;
public:
	SymbolTable() {
		global_ = new GlobalScope();

		global_->define(new BuiltInTypeSymbol("Int"));
		global_->define(new BuiltInTypeSymbol("String"));
		global_->define(new BuiltInTypeSymbol("Char"));
		global_->define(new BuiltInTypeSymbol("Float"));
		global_->define(new BuiltInTypeSymbol("Double"));
		global_->define(new BuiltInTypeSymbol("Bool"));
		global_->define(new BuiltInTypeSymbol("Void"));
		global_->define(new BuiltInTypeSymbol("Label"));
	}

	GlobalScope *getGlobalScope() { return global_; }

	void addTypeConst(Type **s, Type **t) {
		typeConstSet_.push_back(TypeConst(s, t));
		return;
	}
};

};

#endif
