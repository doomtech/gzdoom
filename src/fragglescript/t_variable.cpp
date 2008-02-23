// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2002-2008 Christoph Oelckers
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
// Variables.
//
// Variable code: create new variables, look up variables, get value,
// set value
//
// variables are stored inside the individual scripts, to allow for
// 'local' and 'global' variables. This way, individual scripts cannot
// access variables in other scripts. However, 'global' variables can
// be made which can be accessed by all scripts. These are stored inside
// a dedicated script_t which exists only to hold all of these global
// variables.
//
// functions are also stored as variables, these are kept in the global
// script so they can be accessed by all scripts. function variables
// cannot be set or changed inside the scripts themselves.
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

#include "t_script.h"



//==========================================================================
//
// returns an svalue_t holding the current
// value of a particular variable.
//
//==========================================================================

svariable_t::svariable_t(const char * _name)
{
	Name=_name;
	type=svt_int;
	value.i=0;
	next=NULL;
}

//==========================================================================
//
// returns an svalue_t holding the current
// value of a particular variable.
//
//==========================================================================

svariable_t::~svariable_t()
{
	if (type==svt_mobj) value.acp->Destroy();
}

//==========================================================================
//
// returns an svalue_t holding the current
// value of a particular variable.
//
//==========================================================================

svalue_t svariable_t::GetValue() const
{
	svalue_t returnvar;
	
	if(type == svt_pInt)
    {
		returnvar.type = svt_int;
		returnvar.value.i = *value.pI;
    }
	else if(type == svt_pMobj)
    {
		returnvar.type = svt_mobj;
		returnvar.value.mobj = *value.pMobj;
    }
	else if (type == svt_mobj)
	{
		returnvar.type = type;
		returnvar.value.mobj = value.acp->actor;
	}
	else
    {
		// copy the value (also handles fixed)
		returnvar.type = type;
		returnvar.value.i = value.i;
    }
	
	return returnvar;
}


//==========================================================================
//
// set a variable to a value from an svalue_t
//
//==========================================================================

void svariable_t::SetValue(const svalue_t &newvalue)
{
	if(killscript) return;  // protect the variables when killing script
	
	if(type == svt_const)
    {
		// const adapts to the value it is set to
		ChangeType(newvalue.type);
		
		if(type == svt_string)   // static incase a global_script var
			string = "";
    }

	switch (type)
	{
	case svt_int:
		value.i = intvalue(newvalue);
		break;

	case svt_string:
		string = newvalue.type == svt_string? newvalue.string : stringvalue(newvalue);
		break;

	case svt_fixed:
		value.fixed = fixedvalue(newvalue);
		break;
	
	case svt_mobj:
		value.acp->actor = MobjForSvalue(newvalue);
		break;
	
	case svt_pInt:
		*value.pI = intvalue(newvalue);
		break;
	
	case svt_pMobj:
		*value.pMobj = MobjForSvalue(newvalue);
		break;
	
	case svt_function:
		script_error("attempt to set function to a value\n");
		break;

	default:
		script_error("invalid variable type\n");
		break;
	}
}

//==========================================================================
//
// Archive one script variable
//
//==========================================================================

void svariable_t::Serialize(FArchive & ar)
{
	if (!ar.IsStoring()) ChangeType(svt_int);	// Just to clear old actor pointers in it!

	ar << Name << type << string;
	
	switch(type)        // store depending on type
	{
	case svt_string:
		break;

	case svt_int:
		ar << value.i;
		break;

	case svt_mobj:
		ar << value.acp;
		break;

	case svt_fixed:
		ar << value.fixed;
		break;
	}
	// later: ar << next;
}


//==========================================================================
//
// From here: variable related functions inside script_t
//
//==========================================================================

//==========================================================================
//
// create a new variable in a particular script.
// returns a pointer to the new variable.
//
//==========================================================================

svariable_t *script_t::NewVariable(const char *name, int vtype)
{
	svariable_t *newvar = new svariable_t(name);
	newvar->ChangeType(vtype);
	
	int n = variable_hash(name);
	newvar->next = variables[n];
	variables[n] = newvar;
	return newvar;
}


//==========================================================================
//
// search a particular script for a variable, which
// is returned if it exists
//
//==========================================================================

svariable_t *script_t::VariableForName(const char *name)
{
	int n = variable_hash(name);
	svariable_t *current = variables[n];
	
	while(current)
    {
		if(!strcmp(name, current->Name))        // found it?
			return current;         
		current = current->next;        // check next in chain
    }
	
	return NULL;
}


//==========================================================================
//
// find_variable checks through the current script, level script
// and global script to try to find the variable of the name wanted
//
//==========================================================================

svariable_t *script_t::FindVariable(const char *name)
{
	svariable_t *var;
	script_t *current = this;
	
	while(current)
    {
		// check this script
		if (var = current->VariableForName(name))
			return var;
		current = current->parent;    // try the parent of this one
    }
	
	return NULL;    // no variable
}


//==========================================================================
//
// free all the variables in a given script
//
//==========================================================================

void script_t::ClearVariables(bool complete)
{
	int i;
	svariable_t *current, *next;
	
	for(i=0; i<VARIABLESLOTS; i++)
    {
		current = variables[i];
		
		// go thru this chain
		while(current)
		{
			// labels are added before variables, during
			// preprocessing, so will be at the end of the chain
			// we can be sure there are no more variables to free
			if(current->Type() == svt_label && !complete) break;
			
			next = current->next; // save for after freeing
			
			delete current;
			current = next; // go to next in chain
		}
		// start of labels or NULL
		variables[i] = current;
    }
}

