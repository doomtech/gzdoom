/*
** ds_class
**
** new parser implementation - handling of class definitions
**
**---------------------------------------------------------------------------
** Copyright 2008 Christoph Oelckers
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
** 4. When not used as part of ZDoom or GZDoom, this code will be
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


#include "doomerrors.h"
#include "templates.h"
#include "v_palette.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_statement.h"
#include "thingdef/thingdef_exp.h"

//==========================================================================
//
// 
//
//==========================================================================

FsClass::FsClass(FName clsname, FName parentname, bool actordef, bool native, const FScriptPosition &pos, Baggage *bag)
{
	Position = pos;
	if (actordef || clsname == NAME_Actor)
	{
		Info = CreateNewActor(pos, clsname, parentname, NAME_None, -1, native);
		Class = Info->Class;
		ResetBaggage(bag, Class->ParentClass);
		bag->Info = Info;
		bag->Class = this;
		bag->ClassName = clsname;
	}
	else
	{
		Position.Message(MSG_FATAL, "Non-actor class definition found for class '%s'", clsname.GetChars());
	}

}

//==========================================================================
//
// 
//
//==========================================================================

void FsClass::DefineClass(Baggage &bag)
{
	FinishActor(Position, Info, bag);
	delete this;
}

//==========================================================================
//
// 
//
//==========================================================================

void FsClass::DefineConstant(FsStatement *constant)
{
	constant->Resolve(FCompileContext(Class, false, true), true);
	delete constant;
}

//==========================================================================
//
// 
//
//==========================================================================

bool FsClass::ParsePropertyParams(FPropertyInfo *prop, FPropArgs *arguments, Baggage &bag)
{
	static TArray<FPropParam> params;
	static TArray<FString> strings;

	FCompileContext ctx(Class);
	TArray<FxExpression*> &arg_expressions = arguments->arguments;
	const FString *f;
	unsigned int n=0;

	params.Clear();
	strings.Clear();
	params.Reserve(1);
	params[0].i = 0;
	if (prop->params[0] != '0')
	{
		const char * p = prop->params;
		while (*p)
		{
			FPropParam conv;
			FPropParam pref;

			conv.s = NULL;
			pref.s = NULL;
			pref.i = -1;
			FxExpression *&x = arg_expressions[n];
			switch ((*p) & 223)
			{
			case 'X':	// The expression case is handled elsewhere. This has to be an integer
			case 'I':
			case 'M':	// special case for DECORATE. Normal integer here.
				x = x->CreateCast(ctx, VAL_Int);
				if (x == NULL) return false;
				conv.i = x->EvalExpression(NULL).GetInt();
				break;

			case 'F':
				x = x->CreateCast(ctx, VAL_Float);
				if (x == NULL) return false;
				conv.f = float(x->EvalExpression(NULL).GetFloat());
				break;

			case 'Z':	// an optional string. Does not allow any numerical value.
				x = x->Resolve(ctx);
				if (x->ValueType != VAL_String)
				{
					n--;
					break;
				}
				// fall through

			case 'S':
			case 'T':
				x = x->CreateCast(ctx, VAL_String);
				if (x == NULL) return false;
				f = x->GetConstString();
				if (f != NULL)
				{
					conv.s = strings[strings.Reserve(1)] = *f;
				}
				else conv.s = "";
				break;

			case 'C':
				x = x->Resolve(ctx);
				if (x == NULL) return false;
				if (x->ValueType == VAL_Int)
				{
					if (arg_expressions.Size() <= n+2)
					{
						// Insufficient paraneters
					}
					int R=0, G=0, B=0;
					R = clamp(x->EvalExpression(NULL).GetInt(), 0, 255);

					FxExpression *&y = arg_expressions[++n];
					y = y->CreateCast(ctx, VAL_Int);
					if (y != NULL) 
					{
						G = clamp(y->EvalExpression(NULL).GetInt(), 0, 255);
					}
					FxExpression *&z = arg_expressions[++n];
					z = z->CreateCast(ctx, VAL_Int);
					if (z != NULL) 
					{
						B = clamp(z->EvalExpression(NULL).GetInt(), 0, 255);
					}
					conv.i = MAKERGB(R, G, B);
					pref.i = 0;
				}
				else
				{
					x = x->CreateCast(ctx, VAL_String);
					if (x == NULL) return false;
					f = x->GetConstString();
					if (f != NULL)
					{
						conv.s = strings[strings.Reserve(1)] = *f;
					}
					pref.i = 1;
				}
				break;

			case 'L':	// Either a number or a list of strings
				x = x->Resolve(ctx);
				if (x == NULL) return false;
				if (x->ValueType == VAL_Int)
				{
					pref.i = 0;
					conv.i = x->EvalExpression(NULL).GetInt();
				}
				else
				{
					pref.i = 1;
					params.Push(pref);
					params[0].i++;
					pref.i = -1;

					for (unsigned m=0; m < arg_expressions.Size(); m++)
					{
						FxExpression *&xx = arg_expressions[m];
						xx = xx->CreateCast(ctx, VAL_String);
						if (xx != NULL)
						{
							f = xx->GetConstString();
							if (f != NULL)
							{
								conv.s = strings[strings.Reserve(1)] = *f;
							}
							else conv.s = "";
							params.Push(conv);
							params[0].i++;
						}
					}
					goto endofparm;
				}
				break;

			default:
				assert(false);
				break;

			}
			if (pref.i != -1)
			{
				params.Push(pref);
				params[0].i++;
			}
			params.Push(conv);
			params[0].i++;
		endofparm:
			p++;
			if (*p == '_') p++;

			n++;

			if (*p == 0) 
			{
				if (n > arg_expressions.Size())
				{
					// Error
				}
				break;
			}
			else if (*p >= 'a')
			{
				if (n == arg_expressions.Size())
				{
					break;
				}
			}
		}
	}
	else
	{

	}
	// call the handler
	try
	{
		prop->Handler((AActor *)Class->Defaults, bag, &params[0]);
	}
	catch (CRecoverableError &error)
	{
		Position.Message(MSG_ERROR, "%s", error.GetMessage());
	}

	return true;
}


//==========================================================================
//
// 
//
//==========================================================================

void FsClass::AddProperty(Baggage &bag, FName name1, FName name2, FPropArgs *arguments, const FScriptPosition &pos, bool info)
{
	FString composed;
	const char *ptr;

	if (name1 == NAME_None)
	{
		ptr = name2;
	}
	else
	{
		composed << name1 << '.' << name2;
		ptr = composed;
	}

	FPropertyInfo *prop = FindProperty(ptr);

	if (prop != NULL && (prop->category == CAT_INFO) == info)
	{
		if (Class->IsDescendantOf(prop->cls))
		{
			ParsePropertyParams(prop, arguments, bag);
		}
		else
		{
			pos.Message(MSG_ERROR, "\"%s\" requires an actor of type \"%s\"\n", ptr, prop->cls->TypeName.GetChars());
		}
	}
	else
	{
		pos.Message(MSG_ERROR, "\"%s\" is an unknown actor property\n", ptr);
	}

}

//==========================================================================
//
// Special case for damage expression
//
//==========================================================================

void FsClass::AddExpressionProperty(Baggage &bag, FName name1, FName name2, FxExpression *ex, const FScriptPosition &pos)
{
	if (name1 == NAME_None && name2 == NAME_Damage)
	{
		FPropParam params[2];
		FPropertyInfo *prop = FindProperty(name2);
		FCompileContext ctx(Class);

		params[0].i = 1;
		params[1].i = 0x40000000 | StateParams.Add(ex->CreateCast(ctx, VAL_Int), Class, false);

		// call the handler
		try
		{
			prop->Handler((AActor *)Class->Defaults, bag, params);
		}
		catch (CRecoverableError &error)
		{
			pos.Message(MSG_ERROR, "%s", error.GetMessage());
		}

	}
	else
	{
		pos.Message(MSG_ERROR, "Syntax error: Expression type arguments cannot be used with '%s'\n", name2.GetChars());
	}
}

//==========================================================================
//
// 
//
//==========================================================================

void FsClass::AddFlag(FName name1, FName name2, bool on, const FScriptPosition &pos)
{
	FFlagDef *fd;

	if ( (fd = FindFlag (Class, name1.GetChars(), name2.GetChars())) )
	{
		AActor *defaults = (AActor*)Class->Defaults;
		if (fd->structoffset == -1)	// this is a deprecated flag that has been changed into a real property
		{
			HandleDeprecatedFlags(defaults, Class->ActorInfo, on, fd->flagbit);
		}
		else
		{
			DWORD * flagvar = (DWORD*) ((char*)defaults + fd->structoffset);
			if (on)
			{
				*flagvar |= fd->flagbit;
			}
			else
			{
				*flagvar &= ~fd->flagbit;
			}
		}
	}
	else
	{
		if (name1 == NAME_Actor)
		{
			pos.Message(MSG_ERROR, "\"%s\" is an unknown flag\n", name1.GetChars());
		}
		else
		{
			pos.Message(MSG_ERROR, "\"%s.%s\" is an unknown flag\n", name1.GetChars(), name2.GetChars());
		}
	}
}
