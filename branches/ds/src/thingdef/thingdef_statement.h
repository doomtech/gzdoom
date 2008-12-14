#ifndef __THINGDEF_STATEMENT_H
#define __THINGDEF_STATEMENT_H

#include "sc_man.h"
#include "name.h"

struct FToken;
struct FCompileContext;
struct PSymbol;
class FxExpression;
class FsFunction;

class FsStatement
{
public:
	FScriptPosition ScriptPosition;

protected:
	FsStatement();
	FsStatement(const FScriptPosition &pos);

public:
	virtual ~FsStatement();
	virtual bool Resolve(FCompileContext &ctx, bool notlocal = false);
};


//==========================================================================
//
//
//
//==========================================================================

class FsEnum;
class FsConstant : public FsStatement
{
	friend class FsEnum;
	int Type;
	FName identifier;
	FxExpression *Exp;

private:
	bool AddSymbol(FCompileContext &ctx, PSymbol *sym, bool notlocal);

public:
	FsConstant(int t, FName name, FxExpression *x, const FScriptPosition &pos);
	~FsConstant();
	bool Resolve(FCompileContext &ctx, bool notlocal = false);
};

//==========================================================================
//
//
//
//==========================================================================

class FsNativeVar : public FsStatement
{
	FtTypeExpression *Type;
	FxExpression *Subscript;
	FName identifier;

private:
	bool AddSymbol(FCompileContext &ctx, PSymbol *sym, bool notlocal);

public:
	FsNativeVar(FtTypeExpression *type, FName name, FxExpression *subscript_ex, const FScriptPosition &pos);
	~FsNativeVar();
	bool Resolve(FCompileContext &ctx, bool notlocal = false);
};

//==========================================================================
//
//
//
//==========================================================================

class FsEnum : public FsStatement
{
	TArray<FsConstant *> values;
public:
	~FsEnum();
	void Append(FsConstant *c);
	bool Resolve(FCompileContext &ctx, bool notlocal);
};


//==========================================================================
//
//
//
//==========================================================================

class FsProperty : public FsStatement
{
	FName Name;
	TArray<FxExpression *> parms;
	int current;
public:
	FString GetName();
	void Append(FxExpression *c);
	bool Resolve(FCompileContext &ctx, bool notlocal);
	int GetInt();
	int GetInt(int def);
	float GetFloat();
	float GetFloat(float def);
	fixed_t GetFixed();
	fixed_t GetFixed(fixed_t def);
	FString GetString();
	bool CheckArgs();
	bool CheckInt();
	bool CheckFloat();
	bool CheckString();
};


struct FPropArgs
{
	TDeletingArray<FxExpression *> arguments;

	void Push(FxExpression *ex)
	{
		arguments.Push(ex);
	}
};

class FsClass : public FsStatement
{
	bool ParsePropertyParams(FPropertyInfo *prop, FPropArgs *arguments, Baggage &bag);

public:

	const PClass *Class;
	FActorInfo *Info;
	FScriptPosition Position;

	FsClass(FName clsname, FName parentname, bool actordef, bool native, const FScriptPosition &pos, Baggage *bag);

	void DefineClass(Baggage &bag);
	void DefineConstant(FsStatement *c);
	void AddProperty(Baggage &bag, FName name1, FName name2, FPropArgs *arguments, const FScriptPosition &pos, bool info);
	void AddExpressionProperty(Baggage &bag, FName name1, FName name2, FxExpression *ex, const FScriptPosition &pos);
	void AddFlag(FName name1, FName name2, bool on, const FScriptPosition &pos);
	void AddFunction(Baggage &bag, FsFunction *func);

};

struct FFunctionParameter
{
	FtTypeExpression *type;
	FxExpression *defval;

	FFunctionParameter(FtTypeExpression *t, FxExpression *x)
	{
		type = t;
		defval = x;
	}

	~FFunctionParameter();
};


struct FFunctionParameterList
{
	TDeletingArray<FFunctionParameter*> params;

public:

	void AddParameter(FFunctionParameter *param)
	{
		params.Push(param);
	}
};

class FsFunction : public FsStatement
{
	FtTypeExpression *returntype;	// NULL means action function.
	FName funcname;
	FFunctionParameterList *params;
	bool varargs;

public:
	FScriptPosition ScriptPosition;

	FsFunction(const FScriptPosition &pos, FtTypeExpression *rettype, FName identifier, FFunctionParameterList *_params, bool _varargs)
	{
		ScriptPosition = pos;
		returntype = rettype;
		funcname = identifier;
		params = _params;
		varargs = _varargs;
	}

	~FsFunction()
	{
		if (returntype) delete returntype;
		if (params) delete params;
	}

	FFunctionParameterList *GetParams() {return params; }
	FName GetName() { return funcname; }
};


#endif
