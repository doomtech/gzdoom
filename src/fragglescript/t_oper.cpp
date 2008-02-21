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
//----------------------------------------------------------------------------

/*
FraggleScript is from SMMU which is under the GPL. Technically, therefore, 
combining the FraggleScript code with the non-free ZDoom code is a violation of the GPL.

As this may be a problem for you, I hereby grant an exception to my copyright on the 
SMMU source (including FraggleScript). You may use any code from SMMU in GZDoom, provided that:

    * For any binary release of the port, the source code is also made available.
    * The copyright notice is kept on any file containing my code.
*/

/* includes ************************/
#include "t_script.h"


#define evaluate_leftnright(a, b, c) {\
	left = evaluate_expression((a), (b)-1); \
	right = evaluate_expression((b)+1, (c)); }\
	

//==========================================================================
//
//
//
//==========================================================================

operator_t DFraggleThinker::operators[]=
{
	{"=",   &DFraggleThinker::OPequals,               backward},
	{"||",  &DFraggleThinker::OPor,                   forward},
	{"&&",  &DFraggleThinker::OPand,                  forward},
	{"|",   &DFraggleThinker::OPor_bin,               forward},
	{"&",   &DFraggleThinker::OPand_bin,              forward},
	{"==",  &DFraggleThinker::OPcmp,                  forward},
	{"!=",  &DFraggleThinker::OPnotcmp,               forward},
	{"<",   &DFraggleThinker::OPlessthan,             forward},
	{">",   &DFraggleThinker::OPgreaterthan,          forward},
	
	// haleyjd: Eternity Extensions
	{"<=",  &DFraggleThinker::OPlessthanorequal,      forward},
	{">=",  &DFraggleThinker::OPgreaterthanorequal,   forward},
	
	{"+",   &DFraggleThinker::OPplus,                 forward},
	{"-",   &DFraggleThinker::OPminus,                forward},
	{"*",   &DFraggleThinker::OPmultiply,             forward},
	{"/",   &DFraggleThinker::OPdivide,               forward},
	{"%",   &DFraggleThinker::OPremainder,            forward},
	{"~",   &DFraggleThinker::OPnot_bin,              forward}, // haleyjd
	{"!",   &DFraggleThinker::OPnot,                  forward},
	{"++",  &DFraggleThinker::OPincrement,            forward},
	{"--",  &DFraggleThinker::OPdecrement,            forward},
	{".",   &DFraggleThinker::OPstructure,            forward},
};

int DFraggleThinker::num_operators = sizeof(operators) / sizeof(operator_t);

/***************** logic *********************/

// = operator

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPequals(int start, int n, int stop)
{
	svariable_t *var;
	svalue_t evaluated;
	
	var = find_variable(tokens[start]);
	
	if(var)
    {
		evaluated = evaluate_expression(n+1, stop);
		setvariablevalue(var, evaluated);
    }
	else
    {
		script_error("unknown variable '%s'\n", tokens[start]);
		return nullvar;
    }
	
	return evaluated;
}


//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPor(int start, int n, int stop)
{
	svalue_t returnvar;
	int exprtrue = false;
	svalue_t eval;
	
	// if first is true, do not evaluate the second
	
	eval = evaluate_expression(start, n-1);
	
	if(intvalue(eval))
		exprtrue = true;
	else
    {
		eval = evaluate_expression(n+1, stop);
		exprtrue = !!intvalue(eval);
    }
	
	returnvar.type = svt_int;
	returnvar.value.i = exprtrue;
	return returnvar;
	
	return returnvar;
}


//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPand(int start, int n, int stop)
{
	svalue_t returnvar;
	int exprtrue = true;
	svalue_t eval;
	
	// if first is false, do not eval second
	
	eval = evaluate_expression(start, n-1);
	
	if(!intvalue(eval) )
		exprtrue = false;
	else
    {
		eval = evaluate_expression(n+1, stop);
		exprtrue = !!intvalue(eval);
    }
	
	returnvar.type = svt_int;
	returnvar.value.i = exprtrue;
	
	return returnvar;
}

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPcmp(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;        // always an int returned
	
	if(left.type == svt_string && right.type == svt_string)
	{
		returnvar.value.i = !strcmp(left.value.s, right.value.s);
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPnotcmp(int start, int n, int stop)
{
	svalue_t returnvar;
	
	returnvar = OPcmp(start, n, stop);
	returnvar.type = svt_int;
	returnvar.value.i = !returnvar.value.i;
	
	return returnvar;
}

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPlessthan(int start, int n, int stop)
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPgreaterthan(int start, int n, int stop)
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPnot(int start, int n, int stop)
{
	svalue_t right, returnvar;
	
	right = evaluate_expression(n+1, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = !intvalue(right);
	return returnvar;
}

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPplus(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
  	if (left.type == svt_string)
    {
      	char *tmp;
      	if (right.type == svt_string)
		{
		  	tmp = (char *)malloc(strlen(left.value.s) + strlen(right.value.s) + 1);
		  	sprintf(tmp, "%s%s", left.value.s, right.value.s);
		}
      	else if (right.type == svt_fixed)
		{
	  		tmp = (char *)malloc(strlen(left.value.s) + 12);
			sprintf(tmp, "%s%4.4f", left.value.s, floatvalue(right));
		}
      	else
		{
	  		tmp = (char *)malloc(strlen(left.value.s) + 12);
	  		sprintf(tmp, "%s%i", left.value.s, intvalue(right));
		}
      	returnvar.type = svt_string;
      	returnvar.value.s = tmp;
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPminus(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	// do they mean minus as in '-1' rather than '2-1'?
	if(start == n)
	{
		// kinda hack, hehe
		left.value.i = 0; left.type = svt_int;
		right = evaluate_expression(n+1, stop);
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPmultiply(int start, int n, int stop)
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPdivide(int start, int n, int stop)
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPremainder(int start, int n, int stop)
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPor_bin(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = intvalue(left) | intvalue(right);
	return returnvar;
}


//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPand_bin(int start, int n, int stop)
{
	svalue_t left, right, returnvar;
	
	evaluate_leftnright(start, n, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = intvalue(left) & intvalue(right);
	return returnvar;
}

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPnot_bin(int start, int n, int stop)
{
	svalue_t right, returnvar;
	
	right = evaluate_expression(n+1, stop);
	
	returnvar.type = svt_int;
	returnvar.value.i = ~intvalue(right);
	return returnvar;
}


//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPincrement(int start, int n, int stop)
{
	if(start == n)          // ++n
    {
		svalue_t value;
		svariable_t *var;
		
		var = find_variable(tokens[stop]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", tokens[stop]);
			return nullvar;
		}
		value = getvariablevalue(var);
		
		// haleyjd
		if(var->Type() != svt_fixed)
		{
			value.value.i = intvalue(value) + 1;
			value.type = svt_int;
			setvariablevalue(var, value);
		}
		else
		{
			value.value.f = fixedvalue(value) + FRACUNIT;
			value.type = svt_fixed;
			setvariablevalue(var, value);
		}
		
		return value;
    }
	else if(stop == n)     // n++
    {
		svalue_t origvalue, value;
		svariable_t *var;
		
		var = find_variable(tokens[start]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", tokens[start]);
			return nullvar;
		}
		origvalue = getvariablevalue(var);
		
		// haleyjd
		if(var->Type() != svt_fixed)
		{
			value.type = svt_int;
			value.value.i = intvalue(origvalue) + 1;
			setvariablevalue(var, value);
		}
		else
		{
			value.type = svt_fixed;
			value.value.f = fixedvalue(origvalue) + FRACUNIT;
			setvariablevalue(var, value);
		}
		
		return origvalue;
    }
	
	script_error("incorrect arguments to ++ operator\n");
	return nullvar;
}

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPdecrement(int start, int n, int stop)
{
	if(start == n)          // ++n
    {
		svalue_t value;
		svariable_t *var;
		
		var = find_variable(tokens[stop]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", tokens[stop]);
			return nullvar;
		}
		value = getvariablevalue(var);
		
		// haleyjd
		if(var->Type() != svt_fixed)
		{
			value.value.i = intvalue(value) - 1;
			value.type = svt_int;
			setvariablevalue(var, value);
		}
		else
		{
			value.value.f = fixedvalue(value) - FRACUNIT;
			value.type = svt_fixed;
			setvariablevalue(var, value);
		}
		
		return value;
    }
	else if(stop == n)   // n++
    {
		svalue_t origvalue, value;
		svariable_t *var;
		
		var = find_variable(tokens[start]);
		if(!var)
		{
			script_error("unknown variable '%s'\n", tokens[start]);
			return nullvar;
		}
		origvalue = getvariablevalue(var);
		
		// haleyjd
		if(var->Type() != svt_fixed)
		{
			value.type = svt_int;
			value.value.i = intvalue(origvalue) - 1;
			setvariablevalue(var, value);
		}
		else
		{
			value.type = svt_fixed;
			value.value.f = fixedvalue(origvalue) - FRACUNIT;
			setvariablevalue(var, value);
		}
		
		return origvalue;
    }
	
	script_error("incorrect arguments to ++ operator\n");
	return nullvar;
}

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPlessthanorequal(int start, int n, int stop)
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

//==========================================================================
//
//
//
//==========================================================================

svalue_t DFraggleThinker::OPgreaterthanorequal(int start, int n, int stop)
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

