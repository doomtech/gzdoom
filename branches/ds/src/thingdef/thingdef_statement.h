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




#endif
