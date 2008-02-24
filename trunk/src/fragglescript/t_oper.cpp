// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Operators
//
// Handler code for all the operators. The 'other half'
// of the parsing.
//
// By Simon Howard
//
//---------------------------------------------------------------------------
//
// FraggleScript is from SMMU which is under the GPL. Technically, 
// therefore, combining the FraggleScript code with the non-free 
// ZDoom code is a violation of the GPL.
//
// As this may be a problem for you, I hereby grant an exception to my 
// copyright on the SMMU source (including FraggleScript). You may use 
// any code from SMMU in GZDoom, provided that:
//
//    * For any binary release of the port, the source code is also made 
//      available.
//    * The copyright notice is kept on any file containing my code.
//
//

/* includes ************************/
#include "t_script.h"


#define evaluate_leftnright(a, b, c) {\
	left = EvaluateExpression((a), (b)-1); \
	right = EvaluateExpression((b)+1, (c)); }\
	

FParser::operator_t FParser::operators[]=
{
	{"=",   &FParser::OPequals,               backward},
	{"||",  &FParser::OPor,                   forward},
	{"&&",  &FParser::OPand,                  forward},
	{"|",   &FParser::OPor_bin,               forward},
	{"&",   &FParser::OPand_bin,              forward},
	{"==",  &FParser::OPcmp,                  forward},
	{"!=",  &FParser::OPnotcmp,               forward},
	{"<",   &FParser::OPlessthan,             forward},
	{">",   &FParser::OPgreaterthan,          forward},
	{"<=",  &FParser::OPlessthanorequal,      forward},
	{">=",  &FParser::OPgreaterthanorequal,   forward},
	
	{"+",   &FParser::OPplus,                 forward},
	{"-",   &FParser::OPminus,                forward},
	{"*",   &FParser::OPmultiply,             forward},
	{"/",   &FParser::OPdivide,               forward},
	{"%",   &FParser::OPremainder,            forward},
	{"~",   &FParser::OPnot_bin,              forward}, // haleyjd
	{"!",   &FParser::OPnot,                  forward},
	{"++",  &FParser::OPincrement,            forward},
	{"--",  &FParser::OPdecrement,            forward},
	{".",   &FParser::OPstructure,            forward},
};

int FParser::num_operators = sizeof(FParser::operators) / sizeof(FParser::operator_t);

/***************** logic *********************/

// = operator

svalue_t FParser::OPequals(int start, int n, int stop)
{
	DFsVariable *var;
	svalue_t evaluated;
	
	var = Script->FindVariable(Tokens[start]);
	
	if(var)
    {
		evaluated = EvaluateExpression(n+1, stop);
		var->SetValue (evaluated);
    }
	else
    {
		script_error("unknown variable '%s'\n", Tokens[start]);
		return nullvar;
    }
	
	return evaluated;
}


svalue_t FParser::OPor(int start, int n, int stop)
{
	svalue_t returnvar;
	int exprtrue = false;
	svalue_t eval;
	
	// if first is true, do not evaluate the second
	
	eval = EvaluateExpression(start, n-1);
	
	if(intvalue(eval))
		exprtrue = true;
	else
    {
		eval = EvaluateExpression(n+1, stop);
		exprtrue = !!intvalue(eval);
    }
	
	returnvar.type = svt_int;
	returnvar.value.i = exprtrue;
	return returnvar;
}


svalue_t FParser::OPand(int start, int n, int stop)
{
	svalue_t returnvar;
	int exprtrue = true;
	svalue_t eval;
	
	// if first is false, do not eval second
	
	eval = EvaluateExpression(start, n-1);
	
	if(!intvalue(eval) )
		exprtrue = false;
	else
    {
		eval = EvaluateExpression(n+1, stop);
		exprtrue = !!intvalue(eval);
    }
	
	returnvar.type = svt_int;
	returnvar.value.i = exprtrue;
	
	return returnvar;
}

// haleyjd: reformatted as per 8-17
svalue_t FParser::OPcmp(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;        // always an int returned
	
	if(left.type == svt_string && right.type == svt_string)
	{
		returnvar.value.i = !strcmp(left.string, right.string);
		return returnvar;
	}
	
	// haleyjd: direct mobj comparison when both are mobj
	if(left.type == svt_mobj && right.type == svt_mobj)
	{
		// we can safely assume reference equivalency for
		// AActor's in all cases since they are static for the
		// duration of a level
		returnvar.value.i = (left.value.mobj == right.value.mobj);
		return returnvar;
	}
	
	if(left.type == svt_fixed || right.type == svt_fixed)
	{
		returnvar.value.i = (fixedvalue(left) == fixedvalue(right));
		return returnvar;
	}
	
	returnvar.value.i = (intvalue(left) == intvalue(right));
	
	return returnvar;
	
}

svalue_t FParser::OPnotcmp(int start, int n, int stop)
{
	svalue_t returnvar;
	
	returnvar = OPcmp(start, n, stop);
	returnvar.type = svt_int;
	returnvar.value.i = !returnvar.value.i;
	
	return returnvar;
}

svalue_t FParser::OPlessthan(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	returnvar.type = svt_int;
	
	// haleyjd: 8-17
	if(left.type == svt_fixed || right.type == svt_fixed)
		returnvar.value.i = (fixedvalue(left) < fixedvalue(right));
	else
		returnvar.value.i = (intvalue(left) < intvalue(right));
	
	return returnvar;
}

svalue_t FParser::OPgreaterthan(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	// haleyjd: 8-17
	returnvar.type = svt_int;
	if(left.type == svt_fixed || right.type == svt_fixed)
		returnvar.value.i = (fixedvalue(left) > fixedvalue(right));
	else
		returnvar.value.i = (intvalue(left) > intvalue(right));
	
	return returnvar;
}

svalue_t FParser::OPnot(int start, int n, int stop)
{
	svalue_t right, returnvar;
	
	right = EvaluateExpression(n+1, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = !intvalue(right);
	return returnvar;
}

/************** simple math ***************/

svalue_t FParser::OPplus(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
  	if (left.type == svt_string)
    {
      	if (right.type == svt_string)
		{
			returnvar.string.Format("%s%s", left.string.GetChars(), right.string.GetChars());
		}
      	else if (right.type == svt_fixed)
		{
			returnvar.string.Format("%s%4.4f", left.string.GetChars(), floatvalue(right));
		}
      	else
		{
	  		returnvar.string.Format("%s%i", left.string.GetChars(), intvalue(right));
		}
      	returnvar.type = svt_string;
    }
	// haleyjd: 8-17
	else if(left.type == svt_fixed || right.type == svt_fixed)
	{
		returnvar.type = svt_fixed;
		returnvar.value.f = fixedvalue(left) + fixedvalue(right);
	}
	else
	{
		returnvar.type = svt_int;
		returnvar.value.i = intvalue(left) + intvalue(right);
	}
	
	return returnvar;
}

svalue_t FParser::OPminus(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	// do they mean minus as in '-1' rather than '2-1'?
	if(start == n)
	{
		// kinda hack, hehe
		left.value.i = 0; left.type = svt_int;
		right = EvaluateExpression(n+1, stop);
	}
	else
		evaluate_leftnright(start, n, stop);
	
	// haleyjd: 8-17
	if(left.type == svt_fixed || right.type == svt_fixed)
	{
		returnvar.type = svt_fixed;
		returnvar.value.f = fixedvalue(left) - fixedvalue(right);
	}
	else
	{
		returnvar.type = svt_int;
		returnvar.value.i = intvalue(left) - intvalue(right);
	}
	
	return returnvar;
}

svalue_t FParser::OPmultiply(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	// haleyjd: 8-17
	if(left.type == svt_fixed || right.type == svt_fixed)
	{
		returnvar.type = svt_fixed;
		returnvar.value.f = FixedMul(fixedvalue(left), fixedvalue(right));
	}
	else
	{
		returnvar.type = svt_int;
		returnvar.value.i = intvalue(left) * intvalue(right);
	}
	
	return returnvar;
}

svalue_t FParser::OPdivide(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	// haleyjd: 8-17
	if(left.type == svt_fixed || right.type == svt_fixed)
	{
		fixed_t fr;
		
		if((fr = fixedvalue(right)) == 0)
			script_error("divide by zero\n");
		else
		{
			returnvar.type = svt_fixed;
			returnvar.value.f = FixedDiv(fixedvalue(left), fr);
		}
	}
	else
	{
		int ir;
		
		if(!(ir = intvalue(right)))
			script_error("divide by zero\n");
		else
		{
			returnvar.type = svt_int;
			returnvar.value.i = intvalue(left) / ir;
		}
	}
	
	return returnvar;
}

svalue_t FParser::OPremainder(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	int ir;
	
	evaluate_leftnright(start, n, stop);
	
	if(!(ir = intvalue(right)))
		script_error("divide by zero\n");
	else
    {
		returnvar.type = svt_int;
		returnvar.value.i = intvalue(left) % ir;
    }
	return returnvar;
}

/********** binary operators **************/

// binary or |

svalue_t FParser::OPor_bin(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = intvalue(left) | intvalue(right);
	return returnvar;
}


// binary and &

svalue_t FParser::OPand_bin(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = intvalue(left) & intvalue(right);
	return returnvar;
}

// haleyjd: binary invert - ~
svalue_t FParser::OPnot_bin(int start, int n, int stop)
{
	svalue_t right, returnvar;
	
	right = EvaluateExpression(n+1, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = ~intvalue(right);
	return returnvar;
}


// ++
svalue_t FParser::OPincrement(int start, int n, int stop)
{
	if(start == n)          // ++n
    {
		svalue_t value;
		DFsVariable *var;
		
		var = Script->FindVariable(Tokens[stop]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", Tokens[stop]);
			return nullvar;
		}
		value = var->GetValue();
		
		// haleyjd
		if(var->type != svt_fixed)
		{
			value.value.i = intvalue(value) + 1;
			value.type = svt_int;
			var->SetValue (value);
		}
		else
		{
			value.value.f = fixedvalue(value) + FRACUNIT;
			value.type = svt_fixed;
			var->SetValue (value);
		}
		
		return value;
    }
	else if(stop == n)     // n++
    {
		svalue_t origvalue, value;
		DFsVariable *var;
		
		var = Script->FindVariable(Tokens[start]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", Tokens[start]);
			return nullvar;
		}
		origvalue = var->GetValue();
		
		// haleyjd
		if(var->type != svt_fixed)
		{
			value.type = svt_int;
			value.value.i = intvalue(origvalue) + 1;
			var->SetValue (value);
		}
		else
		{
			value.type = svt_fixed;
			value.value.f = fixedvalue(origvalue) + FRACUNIT;
			var->SetValue (value);
		}
		
		return origvalue;
    }
	
	script_error("incorrect arguments to ++ operator\n");
	return nullvar;
}

// --
svalue_t FParser::OPdecrement(int start, int n, int stop)
{
	if(start == n)          // ++n
    {
		svalue_t value;
		DFsVariable *var;
		
		var = Script->FindVariable(Tokens[stop]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", Tokens[stop]);
			return nullvar;
		}
		value = var->GetValue();
		
		// haleyjd
		if(var->type != svt_fixed)
		{
			value.value.i = intvalue(value) - 1;
			value.type = svt_int;
			var->SetValue (value);
		}
		else
		{
			value.value.f = fixedvalue(value) - FRACUNIT;
			value.type = svt_fixed;
			var->SetValue (value);
		}
		
		return value;
    }
	else if(stop == n)   // n++
    {
		svalue_t origvalue, value;
		DFsVariable *var;
		
		var = Script->FindVariable(Tokens[start]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", Tokens[start]);
			return nullvar;
		}
		origvalue = var->GetValue();
		
		// haleyjd
		if(var->type != svt_fixed)
		{
			value.type = svt_int;
			value.value.i = intvalue(origvalue) - 1;
			var->SetValue (value);
		}
		else
		{
			value.type = svt_fixed;
			value.value.f = fixedvalue(origvalue) - FRACUNIT;
			var->SetValue (value);
		}
		
		return origvalue;
    }
	
	script_error("incorrect arguments to ++ operator\n");
	return nullvar;
}

// haleyjd: Eternity extensions

svalue_t FParser::OPlessthanorequal(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;
	
	if(left.type == svt_fixed || right.type == svt_fixed)
		returnvar.value.i = (fixedvalue(left) <= fixedvalue(right));
	else
		returnvar.value.i = (intvalue(left) <= intvalue(right));
	return returnvar;
}

svalue_t FParser::OPgreaterthanorequal(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;
	
	if(left.type == svt_fixed || right.type == svt_fixed)
		returnvar.value.i = (fixedvalue(left) >= fixedvalue(right));
	else
		returnvar.value.i = (intvalue(left) >= intvalue(right));
	return returnvar;
}

