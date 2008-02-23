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
// scripting.
//
// delayed scripts, running scripts, console cmds etc in here
// the interface between FraggleScript and the rest of the game
//
// By Simon Howard
//
// This is just one vile piece of code with so many shortcuts and
// side effects that it should be banished in HELL forever!
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

#include "r_local.h"
#include "t_script.h"
#include "p_lnspec.h"
#include "a_keys.h"
#include "d_player.h"
#include "p_spec.h"
#include "c_dispatch.h"

IMPLEMENT_POINTY_CLASS(DRunningScript)
	DECLARE_POINTER(prev)
	DECLARE_POINTER(next)
	DECLARE_POINTER(trigger)
END_POINTERS

IMPLEMENT_CLASS(DFraggleThinker)

DFraggleThinker *DFraggleThinker::ActiveThinker;

//==========================================================================
//
//
//
//==========================================================================

DFraggleThinker::DFraggleThinker(const char *scriptdata) 
{

	memset(&hub_script, 0, sizeof(hub_script));
	memset(&global_script, 0, sizeof(global_script));
	memset(&levelscript, 0, sizeof(levelscript));

	// globalscript is the root script
	global_script.parent = NULL;

	// hub script is the next level down
	hub_script.parent = &global_script;

	int i;
	
	for(i=0; i<VARIABLESLOTS; i++)
		global_script.variables[i] = hub_script.variables[i] = NULL;

	levelscript.data = strdup(scriptdata);
	levelscript.scriptnum = -1;
	levelscript.parent = &hub_script;

	memset(tokens, 0, sizeof(tokens));
	memset(tokentype, 0, sizeof(tokentype));
	num_tokens = 0;

	newvar_type = 0;
	newvar_script = NULL;
	runningscripts = new DRunningScript;

	trigger_obj = NULL;

	current_script = NULL;
	nullvar.type = svt_int;
	nullvar.value.i = 0;

	killscript = false;
	prev_section = NULL;
	linestart = NULL;
	rover = NULL;
	current_section = NULL;
	bracetype = 0;

	init_functions();
}

//==========================================================================
//
//     T_ClearScripts()
//
//      called at level start, clears all scripts
//		[CO] now it REALLY clears them!
//
//==========================================================================

void DFraggleThinker::Destroy()
{
	DRunningScript *current;
	int i,j;
	void * p;

	if (ActiveThinker == this) ActiveThinker = NULL;

	for(unsigned int i=0;i<SpawnedThings.Size();i++)
	{
		SpawnedThings[i]->Destroy();
	}
	SpawnedThings.Clear();

	// clear all old variables in the levelscript
	for(i=0; i<VARIABLESLOTS; i++)
    {
		svariable_t * var=levelscript.variables[i];
		while(var)
		{
			svariable_t *next = var->next;
			delete var;
			var = next;
		}
		levelscript.variables[i] = NULL;
    }
	for(i=0;i<SECTIONSLOTS;i++)
	{
		section_t * var=levelscript.sections[i];
		while(var)
		{
			section_t *next = var->next;
			delete var;
			var = next;
		}
		levelscript.sections[i] = NULL;
	}

    
	// abort all runningscripts and copy their data back to their owner
	// to make deletion easier
	current = runningscripts->next;
	while(current)
	{
		// copy out the script variables from the runningscript
		for(i=0; i<VARIABLESLOTS; i++) current->script->variables[i] = current->variables[i];
		
		DRunningScript *next = current->next;   // save before freeing
		current->Destroy();
		current = next;   // continue to next in chain
	}
	runningscripts->Destroy();

	for(j=0;j<MAXSCRIPTS;j++) if (levelscript.children[j])
	{
		for(i=0; i<VARIABLESLOTS; i++)
		{
			svariable_t * var=levelscript.children[j]->variables[i];
			while(var)
			{
				svariable_t *next = var->next;
				delete var;
				var = next;
			}
		}
		for(i=0;i<SECTIONSLOTS;i++)
		{
			section_t * var=levelscript.children[j]->sections[i];
			while(var)
			{
				section_t *next = var->next;
				delete var;
				var = next;
			}
		}
		free(levelscript.children[j]->data);
		delete levelscript.children[j];
		levelscript.children[j]=NULL;
	}

	if (levelscript.data != NULL) free(levelscript.data);
	levelscript.data=NULL;

	// clear all old variables in the global script
	// This also takes care of any defined functions.
	for(i=0; i<VARIABLESLOTS; i++)
	{
		svariable_t * var=global_script.variables[i];
		while(var)
		{
			svariable_t *next = var->next;
			delete var;
			var = next;
		}
		global_script.variables[i] = NULL;
	}
	for(i=0;i<SECTIONSLOTS;i++)
	{
		section_t * var=global_script.sections[i];
		while(var)
		{
			section_t *next = var->next;
			delete var;
			var = next;
		}
		global_script.sections[i] = NULL;
	}
	while (levelpointers.Pop(p)) free(p);
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::PreprocessScripts()
{
	// run the levelscript first
	// get the other scripts
	
	// levelscript started by player 0 'superplayer'
	levelscript.trigger = players[0].mo;
	
	preprocess(&levelscript);
	run_script(&levelscript);
}


//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::RunScript(int snum, AActor * t_trigger)
{
	// [CO] It is far too dangerous to start the script right away.
	// Better queue it for execution for the next time
	// the runningscripts are checked.

	DRunningScript *runscr;
	script_t *script;
	int i;
	
	if(snum < 0 || snum >= MAXSCRIPTS) return;
	script = levelscript.children[snum];
	if(!script)	return;
	
	runscr = new DRunningScript();
	runscr->script = script;
	runscr->savepoint = script->data; // start at beginning
	runscr->wait_type = wt_none;      // start straight away
	runscr->next = runningscripts->next;
	runscr->prev = runningscripts;
	runscr->prev->next = runscr;
	if(runscr->next)
		runscr->next->prev = runscr;
	
	// save the script variables 
	for(i=0; i<VARIABLESLOTS; i++)
    {
		runscr->variables[i] = script->variables[i];
		
		// in case we are starting another current_script:
		// remove all the variables from the script variable list
		// we only start with the basic labels
		while(runscr->variables[i] && runscr->variables[i]->Type() != svt_label)
			runscr->variables[i] = runscr->variables[i]->next;
    }
	// copy trigger
	runscr->trigger = t_trigger;

}

// console scripting debugging commands



//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::clear_runningscripts()
{
	DRunningScript *runscr, *next;
	
	runscr = runningscripts->next;
	
	// free the whole chain
	while(runscr)
    {
		next = runscr->next;
		runscr->Destroy();
		runscr = next;
    }
	runningscripts->next = NULL;
}



//==========================================================================
//
//
//
//==========================================================================

bool DFraggleThinker::wait_finished(DRunningScript *script)
{
	switch(script->wait_type)
    {
    case wt_none: return true;        // uh? hehe
    case wt_scriptwait:               // waiting for script to finish
		{
			DRunningScript *current;
			for(current = runningscripts->next; current; current = current->next)
			{
				if(current == script) continue;  // ignore this script
				if(current->script->scriptnum == script->wait_data)
					return false;        // script still running
			}
			return true;        // can continue now
		}
		
    case wt_delay:                          // just count down
		{
			return --script->wait_data <= 0;
		}
		
    case wt_tagwait:
		{
			int secnum = -1;
			
			while ((secnum = P_FindSectorFromTag(script->wait_data, secnum)) >= 0)
			{
				sector_t *sec = &sectors[secnum];
				if(sec->floordata || sec->ceilingdata || sec->lightingdata)
					return false;        // not finished
			}
			return true;
		}
		
    case wt_scriptwaitpre: // haleyjd - wait for script to start
		{
			DRunningScript *current;
			for(current=runningscripts->next; current; current=current->next)
			{
				if(current == script) continue;  // ignore this script
				if(current->script->scriptnum == script->wait_data)
					return true;        // script is now running
			}
			return false; // no running instances found
		}
		
    default: return true;
    }
	
	return false;
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::Tick()
{
	DRunningScript *current, *next;
	int i;
	
	current = runningscripts->next;
    
	while(current)
	{
		if(wait_finished(current))
		{
			// copy out the script variables from the
			// runningscript
			
			for(i=0; i<VARIABLESLOTS; i++)
				current->script->variables[i] = current->variables[i];
			current->script->trigger = current->trigger; // copy trigger
			
			// unhook from chain 
			current->prev->next = current->next;
			if(current->next) current->next->prev = current->prev;
			next = current->next;   // save before freeing
			
			// continue the script
			continue_script(current->script, current->savepoint);

			// free
			current->Destroy();
		}
		else
			next = current->next;
		current = next;   // continue to next in chain
	}
}

//==========================================================================
//
//
//
//==========================================================================

DRunningScript *DFraggleThinker::T_SaveCurrentScript()
{
	DRunningScript *runscr;
	int i;

	runscr = new DRunningScript();
	runscr->script = current_script;
	runscr->savepoint = rover;
	
	// leave to other functions to set wait_type: default to wt_none
	runscr->wait_type = wt_none;
	
	// hook into chain at start
	
	runscr->next = runningscripts->next;
	runscr->prev = runningscripts;
	runscr->prev->next = runscr;
	if(runscr->next)
		runscr->next->prev = runscr;
	
	// save the script variables 
	for(i=0; i<VARIABLESLOTS; i++)
    {
		runscr->variables[i] = current_script->variables[i];
		
		// remove all the variables from the script variable list
		// to prevent them being removed when the script stops
		
		while(current_script->variables[i] &&
			current_script->variables[i]->Type() != svt_label)
			current_script->variables[i] =
			current_script->variables[i]->next;
    }
	runscr->trigger = current_script->trigger;      // save trigger
	
	killscript = true;      // stop the script
	
	return runscr;
}

//==========================================================================
//
// looping section. using the rover, find the highest level
// loop we are currently in and return the section_t for it.
//
//==========================================================================

section_t *DFraggleThinker::looping_section()
{
	section_t *best = NULL;         // highest level loop we're in
	// that has been found so far
	int n;
	
	// check thru all the hashchains
	
	for(n=0; n<SECTIONSLOTS; n++)
    {
		section_t *current = current_script->sections[n];
		
		// check all the sections in this hashchain
		while(current)
		{
			// a loop?
			
			if(current->type == st_loop)
				// check to see if it's a loop that we're inside
				if(rover >= current->start && rover <= current->end)
				{
					// a higher nesting level than the best one so far?
					if(!best || (current->start > best->start))
						best = current;     // save it
				}
				current = current->next;
		}
    }
	
	return best;    // return the best one found
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_Wait()
{
	DRunningScript *runscr;
	
	if(t_argc != 1)
    {
		script_error("incorrect arguments to function\n");
		return;
    }
	
	runscr = T_SaveCurrentScript();
	
	runscr->wait_type = wt_delay;
	
	runscr->wait_data = (intvalue(t_argv[0]) * TICRATE) / 100;
}

//==========================================================================
//
// wait for sector with particular tag to stop moving
//
//==========================================================================

void DFraggleThinker::SF_TagWait()
{
	DRunningScript *runscr;
	
	if(t_argc != 1)
    {
		script_error("incorrect arguments to function\n");
		return;
    }
	
	runscr = T_SaveCurrentScript();
	
	runscr->wait_type = wt_tagwait;
	runscr->wait_data = intvalue(t_argv[0]);
}

//==========================================================================
//
// wait for a script to finish
//
//==========================================================================

void DFraggleThinker::SF_ScriptWait()
{
	DRunningScript *runscr;
	
	if(t_argc != 1)
    {
		script_error("insufficient arguments to function\n");
		return;
    }
	
	runscr = T_SaveCurrentScript();
	
	runscr->wait_type = wt_scriptwait;
	runscr->wait_data = intvalue(t_argv[0]);
}

//==========================================================================
//
// haleyjd 05/23/01: wait for a script to start (zdoom-inspired)
//
//==========================================================================

void DFraggleThinker::SF_ScriptWaitPre()
{
	DRunningScript *runscr;
	
	if(t_argc != 1)
	{
		script_error("insufficient arguments to function\n");
		return;
	}
	
	runscr = T_SaveCurrentScript();
	runscr->wait_type = wt_scriptwaitpre;
	runscr->wait_data = intvalue(t_argv[0]);
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_StartScript()
{
	DRunningScript *runscr;
	script_t *script;
	int i, snum;
	
	if(t_argc != 1)
    {
		script_error("incorrect arguments to function\n");
		return;
    }
	
	snum = intvalue(t_argv[0]);

	if(snum < 0 || snum >= MAXSCRIPTS)
	{
		script_error("script number %d out of range\n",snum);
		return;
	}
	
	script = levelscript.children[snum];
	
	if(!script)
    {
		script_error("script %i not defined\n", snum);
    }
	
	runscr = new  DRunningScript();
	runscr->script = script;
	runscr->savepoint = script->data; // start at beginning
	runscr->wait_type = wt_none;      // start straight away
	
	// hook into chain at start
	
	// haleyjd: restructured
	runscr->next = runningscripts->next;
	runscr->prev = runningscripts;
	runscr->prev->next = runscr;
	if(runscr->next)
		runscr->next->prev = runscr;
	
	// save the script variables 
	for(i=0; i<VARIABLESLOTS; i++)
    {
		runscr->variables[i] = script->variables[i];
		
		// in case we are starting another current_script:
		// remove all the variables from the script variable list
		// we only start with the basic labels
		while(runscr->variables[i] &&
			runscr->variables[i]->Type() != svt_label)
			runscr->variables[i] =
			runscr->variables[i]->next;
    }
	// copy trigger
	runscr->trigger = current_script->trigger;
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::SF_ScriptRunning()
{
	DRunningScript *current;
	int snum = 0;
	
	if(t_argc < 1)
    {
		script_error("not enough arguments to function\n");
		return;
    }
	
	snum = intvalue(t_argv[0]);  
	
	for(current=runningscripts->next; current; current=current->next)
    {
		if(current->script->scriptnum == snum)
		{
			// script found so return
			t_return.type = svt_int;
			t_return.value.i = 1;
			return;
		}
    }
	
	// script not found
	t_return.type = svt_int;
	t_return.value.i = 0;
}


/*********************
ADD SCRIPT
*********************/

// when the level is first loaded, all the
// scripts are simply stored in the levelscript.
// before the level starts, this script is
// preprocessed and run like any other. This allows
// the individual scripts to be derived from the
// levelscript. When the interpreter detects the
// 'script' keyword this function is called

// haleyjd: significant reformatting in 8-17

void DFraggleThinker::spec_script()
{
	int scriptnum;
	int datasize;
	script_t *script;
	
	scriptnum = 0;
	
	if(!current_section)
    {
		script_error("need seperators for script\n");
		return;
    }
	
	// presume that the first token is "script"
	
	if(num_tokens < 2)
    {
		script_error("need script number\n");
		return;
    }
	
	scriptnum = intvalue(evaluate_expression(1, num_tokens-1));
	
	if(scriptnum < 0)
    {
		script_error("invalid script number\n");
		return;
    }
	
	script = new script_t;
	
	// add to scripts list of parent
	current_script->children[scriptnum] = script;
	
	// copy script data
	// workout script size: -2 to ignore { and }
	datasize = (int)(current_section->end - current_section->start - 2);
	
	// alloc extra 10 for safety
	script->data = (char *)malloc(datasize+10);
	
	// copy from parent script (levelscript) 
	// ignore first char which is {
	memcpy(script->data, current_section->start+1, datasize);
	
	// tack on a 0 to end the string
	script->data[datasize] = '\0';
	
	script->scriptnum = scriptnum;
	script->parent = current_script; // remember parent
	
	// preprocess the script now
	preprocess(script);
    
	// restore current_script: usefully stored in new script
	current_script = script->parent;
	
	// rover may also be changed, but is changed below anyway
	
	// we dont want to run the script, only add it
	// jump past the script in parsing
	
	rover = current_section->end + 1;
}


//==========================================================================
//
//
//
//==========================================================================
AActor *DFraggleThinker::MobjForSvalue(svalue_t svalue)
{
	int intval;

	if(svalue.type == svt_mobj) 
	{
		// Inventory items in the player's inventory have to be considered non-present.
		if (svalue.value.mobj != NULL && 
			svalue.value.mobj->IsKindOf(RUNTIME_CLASS(AInventory)) && 
			static_cast<AInventory*>(svalue.value.mobj)->Owner != NULL)
		{
			return NULL;
		}

		return svalue.value.mobj;
	}
	else
	{
		// this requires some creativity. We use the intvalue
		// as the thing number of a thing in the level.
		intval = intvalue(svalue);
		
		if(intval < 0 || intval >= (int)SpawnedThings.Size())
		{ 
			return NULL;
		}
		// Inventory items in the player's inventory have to be considered non-present.
		if (SpawnedThings[intval]->actor != NULL &&
			SpawnedThings[intval]->actor->IsKindOf(RUNTIME_CLASS(AInventory)) && 
			static_cast<AInventory*>(SpawnedThings[intval]->actor)->Owner != NULL)
		{
			return NULL;
		}

		return SpawnedThings[intval]->actor;
	}
}

