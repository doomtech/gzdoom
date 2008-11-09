/*
** thingdef_statement.cpp
**
** Statement classes for DECORATE parser
**
**---------------------------------------------------------------------------
** Copyright 2007-2008 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------
#include "doomtype.h"
#include "dthinker.h"
#include "zstring.h"
#include "name.h"
#include "info.h"
#include "sc_man.h"
#include "doomstat.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_exp.h"
#include "thingdef/thingdef_statement.h"
#include "ds_.h"



// MACROS ------------------------------------------------------------------

//==========================================================================
//
//	FsStatement::FsStatement
//
//==========================================================================

FsStatement::FsStatement(const FScriptPosition &pos)
: ScriptPosition(pos)
{
}

FsStatement::FsStatement()
{
	ScriptPosition.FileName="";
	ScriptPosition.ScriptLine=0;
}

//==========================================================================
//
//	FsStatement::~FsStatement
//
//==========================================================================

FsStatement::~FsStatement()
{}

//==========================================================================
//
//	FsStatement::Resolve
//
//==========================================================================

bool FsStatement::Resolve(FCompileContext &ctx, bool notlocal)
{
	return false;
}

//==========================================================================
//
//	FsConstant :: FsConstant
//
//==========================================================================

FsConstant::FsConstant(int t, FName name, FxExpression *x, const FScriptPosition &pos) : FsStatement(pos)
{
	Type = t;
	Exp = x;
	identifier = name;
}

//==========================================================================
//
//	FsConstant :: AddSymbol
//
//==========================================================================

bool FsConstant::AddSymbol(FCompileContext &ctx, PSymbol *sym, bool notlocal)
{
	bool res;
	
	if (developer == 1)
	{
		if (!notlocal) ;
		else if (ctx.cls != NULL) Printf(PRINT_LOG, "Adding constant %s to class %s,", sym->SymbolName.GetChars(), ctx.cls->TypeName.GetChars());
		else Printf(PRINT_LOG, "Adding global constant %s,", sym->SymbolName.GetChars());
		
		if (sym->SymbolType == SYM_Const)
		{
			if (static_cast<PSymbolConst*>(sym)->ValueType == VAL_Int) 
				Printf(PRINT_LOG, " int, %d\n", static_cast<PSymbolConst*>(sym)->Value);
			else 
				Printf(PRINT_LOG, " float, %f\n", static_cast<PSymbolConst*>(sym)->Float);
		}
		else
		{
			//Printf(PRINT_LOG, " string, %s\n", static_cast<PSymbolStringConst*>(sym)->stringvalue);
		}
	}

	
	
	if (!notlocal)
	{
		// later
		res = false;
	}
	else if (ctx.cls != NULL)
	{
		res = !!const_cast<PClass *>(ctx.cls)->Symbols.AddSymbol(sym);
		if (!res) ScriptPosition.Message(MSG_ERROR, "Duplicate definition of '%s' in class '%s'", 
					identifier.GetChars(), ctx.cls->TypeName.GetChars());
	}
	else
	{
		res = !!GlobalSymbols.AddSymbol(sym);
		if (!res) ScriptPosition.Message(MSG_ERROR, "Duplicate definition of global symbol '%s'", 
					identifier.GetChars());
	}
	return res;
}

//==========================================================================
//
//	FsConstant :: Resolve
//
//==========================================================================

bool FsConstant::Resolve(FCompileContext &ctx, bool notlocal)
{
	Exp = Exp->Resolve(ctx);
	if (Exp == NULL) return false;

	ExpVal x = Exp->EvalExpression(NULL);
	delete Exp;
	if (Type == VAL_Int || Type == VAL_Float)
	{
		PSymbolConst *sym = new PSymbolConst(identifier);
		if (Type == VAL_Int)
		{
			sym->ValueType = VAL_Int;
			sym->Value = x.GetInt();
		}
		else
		{
			sym->ValueType = VAL_Float;
			sym->Float = x.GetFloat();
		}
		if (sym) return AddSymbol(ctx, sym, notlocal);
	}
	return false;
}



//==========================================================================
//
//	FsEnum :: Append
//
//==========================================================================

void FsEnum::Append(FsConstant *c)
{
	if (values.Size()==0) ScriptPosition = c->ScriptPosition;
	values.Push(c);
}

//==========================================================================
//
//	FsEnum :: Resolve
//
//==========================================================================

bool FsEnum::Resolve(FCompileContext &ctx, bool notlocal)
{
	bool res = true;
	int value = 0;
	for(unsigned i = 0; i < values.Size(); i++)
	{
		if (values[i]->Exp != NULL)
		{
			FxExpression *Exp = values[i]->Exp->CreateCast(ctx, VAL_Int);
			if (Exp != NULL)
			{
				Exp = Exp->Resolve(ctx);
				if (Exp != NULL)
				{
					ExpVal x = Exp->EvalExpression(NULL);
					delete Exp;
					if (x.Type == VAL_Int)
					{
						value = x.GetInt();
					}
					else res = false;
				}
				else res = false;
			}
			else res = false;
		}
		else res = false;

		PSymbolConst *sym = new PSymbolConst(values[i]->identifier);
		sym->ValueType = VAL_Int;
		sym->Value = value;
		res &= values[i]->AddSymbol(ctx, sym, notlocal);
		value++;
	}
	return res;
}
