#ifndef __THINGDEF_PARSER_H
#define __THINGDEF_PARSER_H

#include "sc_man.h"
#include "name.h"
#include "zstring.h"
#include "i_system.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_exp.h"

//class ZsStatement;
class FxExpression;
//struct ZFunction;
//class ZClass;
struct FStateDefine;
struct PSymbolMethod;

//==========================================================================
//
// a single token
//
// This looks quite ugly but this is unavoidable.
// The parser uses this as part of a union so anything
// that requires a copy constructor or a destructor can't be used.
// As a result all string data is stored in an external buffer and
// only referenced by index into that buffer.
//
//==========================================================================

struct FToken
{
	int tokentype;
	Baggage *bag;
private:
	int ScriptLine;
	int ScriptNameIndex;
	union 
	{
		int intvalue;
		float floatvalue;
	} v;


public:

	// this structure intentionally doesn't have a constructor!

	void Init(Baggage *_bag, FScanner &sc)
	{
		tokentype = sc.TokenType;
		bag = _bag;
		switch(tokentype)
		{
		case TK_IntConst:
			v.intvalue = sc.Number;
			break;

		case TK_FloatConst:
			v.floatvalue = float(sc.Float);
			break;

		case TK_Identifier:
			v.intvalue = int(FName(sc.String));
			break;

		case TK_StringConst:
			v.intvalue = bag->AddString(sc.String);
			break;

		default:
			break;
		}
		FScriptPosition ScriptPosition = sc;

		ScriptLine = ScriptPosition.ScriptLine;
		ScriptNameIndex = bag->AddString(ScriptPosition.FileName);
	}

	const FToken &StringToIdentifier()
	{
		switch (tokentype)
		{
		case TK_StringConst:
			v.intvalue = int(FName(bag->GetString(v.intvalue)));
			tokentype = TK_Identifier;
			break;
		default:
			break;
		}
		return *this;
	}

	const FToken &MakeIdentifier(FName identifier)
	{
		v.intvalue = identifier;
		tokentype = TK_Identifier;
		return *this;
	}

	int IntValue() const
	{
		return v.intvalue;
	}

	float FloatValue() const
	{
		return v.floatvalue;
	}

	FName NameValue() const
	{
		return ENamedName(v.intvalue);
	}

	FString StringValue() const
	{
		return bag->GetString(v.intvalue);
	}

	FScriptPosition ScriptPosition() const
	{
		return FScriptPosition(bag->GetString(ScriptNameIndex), ScriptLine);
	}

};



/*
#ifdef _DEBUG
EXTERN_CVAR(Int, compile_log)
enum
{
	CL_PROP = 1,
	CL_STATES = 2,
	CL_RESOLVE = 4,
	CL_CODEGEN = 8,
	CL_STMT = 16,
	CL_PARSE = 32,
};

//#define CLOG(mask, code) { if (compile_log&mask) { code; } }
#define CLOG(mask, code) code

#else

#define CLOG(mask, code)

#endif
*/


#endif