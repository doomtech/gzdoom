/*
** ds_main
**
** new parser implementation
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

#include "templates.h"
#include "actor.h"
#include "zstring.h"
#include "sc_man.h"
#include "i_system.h"
#include "ds-parser.h"
#include "w_wad.h"
#include "a_pickups.h"
#include "ds_.h"
#include "p_lnspec.h"
#include "thingdef/thingdef_statement.h"

static int ParserTokens[TK_LastToken];

//==========================================================================
//
// Initializes the token mapping table between sc_man and Lemon.
//
//==========================================================================

void InitParserTokens()
{
	ParserTokens[';'] = DS_SEMICOLON;
	ParserTokens['{'] = DS_LBRACE;
	ParserTokens['}'] = DS_RBRACE;
	ParserTokens[TK_Class] = DS_CLASS;
	ParserTokens[':'] = DS_COLON;
	ParserTokens[TK_Const] = DS_CONST;
	ParserTokens[TK_Identifier] = DS_IDENTIFIER;
	ParserTokens['='] = DS_ASSIGN;
	ParserTokens[TK_Enum] = DS_ENUM;
	ParserTokens[','] = DS_COMMA;
	ParserTokens[TK_StringConst] = DS_STRINGCONST;
	ParserTokens[TK_Int] = DS_INT;
	ParserTokens[TK_Float] = DS_FLOAT;
	//ParserTokens[TK_Sound] = DS_SOUND;
	ParserTokens[TK_Bool] = DS_BOOL;
	ParserTokens[TK_FloatConst] = DS_FLOATCONST;
	ParserTokens[TK_IntConst] = DS_INTCONST;
	ParserTokens['?'] = DS_QUESTION;
	ParserTokens[TK_OrOr] = DS_OROR;
	ParserTokens[TK_AndAnd] = DS_ANDAND;
	ParserTokens['|'] = DS_OR;
	ParserTokens['^'] = DS_XOR;
	ParserTokens['&'] = DS_AND;
	ParserTokens[TK_Eq] = DS_EQ;
	ParserTokens[TK_Neq] = DS_NE;
	ParserTokens['>'] = DS_GT;
	ParserTokens[TK_Geq] = DS_GE;
	ParserTokens['<'] = DS_LT;
	ParserTokens[TK_Leq] = DS_LE;
	ParserTokens[TK_LShift] = DS_LSHIFT;
	ParserTokens[TK_RShift] = DS_RSHIFT;
	ParserTokens[TK_URShift] = DS_URSHIFT;
	ParserTokens['+'] = DS_PLUS;
	ParserTokens['-'] = DS_MINUS;
	ParserTokens['*'] = DS_MUL;
	ParserTokens['/'] = DS_DIVIDE;
	ParserTokens['%'] = DS_MOD;
	//ParserTokens[TK_Incr] = DS_INCR;
	//ParserTokens[TK_Decr] = DS_DECR;
	ParserTokens['!'] = DS_NOT;
	ParserTokens['~'] = DS_TILDE;
	ParserTokens[TK_DColon] = DS_DCOLON;
	ParserTokens['('] = DS_LPAREN;
	ParserTokens[')'] = DS_RPAREN;
	ParserTokens['['] = DS_LBRACKET;
	ParserTokens[']'] = DS_RBRACKET;
	ParserTokens[TK_Native] = DS_NATIVE;
	//ParserTokens[TK_DefaultProperties] = DS_DEFAULTPROPERTIES;
	ParserTokens['.'] = DS_DOT;
	ParserTokens[TK_Color] = DS_COLOR;
	//ParserTokens[TK_String] = DS_STRING;
	//ParserTokens[TK_Ellipsis] = DS_ELLIPSIS;
	//ParserTokens[TK_Void] = DS_VOID;
	//ParserTokens[TK_Vector] = DS_VECTOR;
	//ParserTokens[TK_Action] = DS_ACTION;
	//ParserTokens[TK_Private] = DS_PRIVATE;
	//ParserTokens[TK_Protected] = DS_PROTECTED;
	//ParserTokens[TK_Final] = DS_FINAL;
	ParserTokens[TK_Stop] = DS_STOP;
	ParserTokens[TK_Wait] = DS_WAIT;
	ParserTokens[TK_Fail] = DS_FAIL;
	ParserTokens[TK_Loop] = DS_LOOP;
	ParserTokens[TK_Goto] = DS_GOTO;
	ParserTokens[TK_Super] = DS_SUPER;
	//ParserTokens[TK_Bright] = DS_BRIGHT;
	//ParserTokens[TK_Offset] = DS_OFFSET;
	ParserTokens[TK_States] = DS_STATES;
	//ParserTokens[TK_State] = DS_STATE;
	//ParserTokens[TK_AddEq] = DS_ADDASSIGN;
	//ParserTokens[TK_SubEq] = DS_SUBASSIGN;
	//ParserTokens[TK_MulEq] = DS_MULASSIGN;
	//ParserTokens[TK_DivEq] = DS_DIVASSIGN;
	//ParserTokens[TK_ModEq] = DS_MODASSIGN;
	//ParserTokens[TK_AndEq] = DS_ANDASSIGN;
	//ParserTokens[TK_OrEq] = DS_ORASSIGN;
	//ParserTokens[TK_XorEq] = DS_XORASSIGN;
	//ParserTokens[TK_LShiftEq] = DS_LSHASSIGN;
	//ParserTokens[TK_RShiftEq] = DS_RSHASSIGN;
	//ParserTokens[TK_URShiftEq] = DS_URSHASSIGN;
	ParserTokens[TK_Abs] = DS_ABS;
	ParserTokens[TK_Random] = DS_RANDOM;
	ParserTokens[TK_Random2] = DS_RANDOM2;
	//ParserTokens[TK_Default] = DS_DEFAULT;
	//ParserTokens[TK_Self] = DS_SELF;
	//ParserTokens[TK_Null] = DS_NULL;
	ParserTokens[TK_True] = DS_TRUE;
	ParserTokens[TK_False] = DS_FALSE;
	//ParserTokens[TK_Name] = DS_NAME;
	ParserTokens[TK_Info] = DS_INFO;
	ParserTokens[TK_DefaultProperties] = DS_DEFAULTPROPERTIES;
}

FRandom *MakeRNG(const FToken &tok)
{
	return FRandom::StaticFindRNG(tok.NameValue().GetChars());
}

//==========================================================================
//
//
//
//==========================================================================

void DefineGlobalConstant(FsStatement *constant)
{
	constant->Resolve(FCompileContext(NULL, false, true), true);
	delete constant;
}

//==========================================================================
//
// Code to interface the new parser with the old expression evaluation
// mechanism. Will most likely be removed soon.
//
//==========================================================================

struct CodePtr
{
	FName funcname;
	TDeletingArray<FxExpression*> parameters;
};

int PrepareStateParameters(FState *state, int numparams, const PClass *cls);

static void InstallCodePtr(FState *state, CodePtr *cptr, const PClass *cls, const FScriptPosition &pos)
{
	if (cptr == NULL)
	{
		state->ActionFunc = NULL;
		state->ParameterIndex = 0;
	}
	else
	{
		int min_args, max_args;
		FCompileContext ctx(cls);
		int special = P_FindLineSpecial(cptr->funcname, &min_args, &max_args);

		if (special > 0 && min_args >= 0)
		{
			int paramindex = PrepareStateParameters(state, 6, cls);
			int i = cptr->parameters.Size();

			if (i < min_args)
			{
				pos.Message(MSG_ERROR, "Too few arguments to %s", cptr->funcname.GetChars());
				return;
			}
			if (i > max_args)
			{
				pos.Message(MSG_ERROR, "Too many arguments to %s", cptr->funcname.GetChars());
				return;
			}

			// The special's index is the first parameter
			StateParams.Set(paramindex, new FxConstant(special, pos));

			for (int j=0; j<i; j++)
			{
				StateParams.Set(paramindex+j+1, cptr->parameters[j]->CreateCast(ctx, VAL_Int));
				cptr->parameters[j] = NULL;
			}

			state->SetAction(FindGlobalActionFunction("A_CallSpecial"), false);
			return;
		}
		else
		{
			PSymbol *sym = cls->Symbols.FindSymbol (cptr->funcname, true);
			if (sym != NULL && sym->SymbolType == SYM_ActionFunction)
			{
				PSymbolActionFunction *afd = static_cast<PSymbolActionFunction *>(sym);
				state->SetAction(afd, false);

				const char *params = afd->Arguments.GetChars();
				int numparams = (int)afd->Arguments.Len();
				int excount = cptr->parameters.Size();
				bool varargs = params[numparams - 1] == '+';
				int paramindex = PrepareStateParameters(state, numparams, cls);
				int paramstart = paramindex;
				int varargcount=0;

				if (varargs)
				{
					paramindex++;
				}

				if (excount > numparams)
				{
					pos.Message(MSG_ERROR, "Too many arguments to %s", cptr->funcname.GetChars());
					return;
				}

				for(int i=0; i < excount; i++)
				{
					FxExpression *x = cptr->parameters[i];

					switch(*params++)
					{
					case 'S':
					case 's':		// Sound name
						x = x->CreateCast(ctx, VAL_Sound);
						break;

					case 'M':
					case 'm':		// Actor name
						x = x->CreateCast(ctx, VAL_Class);
						break;

					case 'T':
					case 't':		// String
						x = x->CreateCast(ctx, VAL_Name);
						break;

					case 'C':
					case 'c':		// Color
						x = x->CreateCast(ctx, VAL_Color);
						break;

					case 'L':
					case 'l':		// State label
						x = x->CreateCast(ctx, VAL_State);
						break;

					case 'X':
					case 'x':		// Number
						x = x->Resolve(ctx);
						if (x != NULL && x->ValueType != VAL_Int && x->ValueType != VAL_Float)
						{
							pos.Message(MSG_ERROR, "Numeric value expected.");
						}
						break;

					default:	// incorrect definition - shouldn't happen
						assert(false);
						x = NULL;
						break;
					}

					cptr->parameters[i] = NULL;
					StateParams.Set(paramindex+i, x);
					if (*params == '+')
					{
						varargcount++;
						params--;
					}
				}

				if (varargs)
				{
					StateParams.Set(paramstart, new FxConstant(varargcount, pos));
				}
				else if (*params != 0)
				{
					if (*params >='A' && *params <= 'Z')
					{
						pos.Message(MSG_ERROR, "Too few arguments to %s", cptr->funcname.GetChars());
					}
					else if (afd->defaultparameterindex > -1)
					{
						// copy the remaining default parameters
						StateParams.Copy(paramindex+excount, afd->defaultparameterindex+excount, 
							int(afd->Arguments.Len())-excount);
					}
				}
			}
		}
	}
}


#include "ds-parser.c"

//==========================================================================
//
//
//
//==========================================================================
void ParseDS(FScanner &sc, void *pParser, Baggage *context)
{
	while (sc.GetToken())
	{
		if (sc.TokenType == TK_Include)
		{
			sc.MustGetToken(TK_StringConst);
			// This is not using SC_Open because it can print a more useful error message when done here
			int lump = Wads.CheckNumForFullName(sc.String, true);

			if (lump == -1) 
			{
				sc.ScriptError("Lump '%s' not found", sc.String);
			}
			else
			{
				FScanner newscanner(lump);
				ParseDS(newscanner, pParser, context);
			}
		}
		else
		{

			int ptoken = ParserTokens[sc.TokenType];
			if (sc.TokenType == TK_Identifier && sc.Compare("ACTOR"))
			{
				// Problem: We need ACTOR as a keyword but the scanner cannot do that
				// because in other places ACTOR needs to be an identifier.
				ptoken = DS_ACTOR;
			}
			if (ptoken == 0)
			{
				I_FatalError("Unknown token %s found!", 
					sc.TokenName(sc.TokenType));
			}
			FToken tok;
			tok.Init(context, sc);
			context->ScriptPosition = sc;
			//DPrintf(PRINT_LOG, "Inputting token %s, %s\n", FScanner::TokenName( sc.TokenType), sc.String);
			DSParse(pParser, ptoken, tok, context);
		}
	}
}


//==========================================================================
//
//
//
//==========================================================================

void LoadDS()
{
	if (Wads.CheckNumForName("DSCRIPT") > -1)
	{

		int lastlump, lump;

		lastlump = 0;
		InitParserTokens();
		while ((lump = Wads.FindLump ("DSCRIPT", &lastlump)) != -1)
		{
			FScanner sc(lump);
			Baggage bag;
			void *pParser = DSParseAlloc(malloc);
			try
			{
				ParseDS (sc, pParser, &bag);
				// Send a terminator token to the parser.
				FToken tok;
				tok.tokentype = 0;
				tok.bag = NULL;
				DSParse(pParser, 0, tok, &bag);
			}
			catch (...)
			{
				DSParseFree(pParser, free);
				throw;
			}
			DSParseFree(pParser, free);
		}
	}
}
