#ifndef __THINGDEF_STATEMENT_H
#define __THINGDEF_STATEMENT_H

#include "sc_man.h"
#include "name.h"

struct FToken;
struct FCompileContext;
struct PSymbol;
class FxExpression;

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
	const PClass *Class;
	FActorInfo *Info;
	FScriptPosition Position;

	bool ParsePropertyParams(FPropertyInfo *prop, FPropArgs *arguments, Baggage &bag);

public:

	FsClass(FName clsname, FName parentname, bool actordef, bool native, const FScriptPosition &pos, Baggage *bag);

	void DefineClass(Baggage &bag);
	void DefineConstant(FsStatement *c);
	void AddProperty(Baggage &bag, FName name1, FName name2, FPropArgs *arguments, const FScriptPosition &pos, bool info);
	void AddExpressionProperty(Baggage &bag, FName name1, FName name2, FxExpression *ex, const FScriptPosition &pos);
	void AddFlag(FName name1, FName name2, bool on, const FScriptPosition &pos);

};




#endif
