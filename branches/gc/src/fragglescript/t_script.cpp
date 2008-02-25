// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
// Copyright(C) 2005-2008 Christoph Oelckers
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

#include "r_local.h"
#include "t_script.h"
#include "p_lnspec.h"
#include "a_keys.h"
#include "d_player.h"
#include "p_spec.h"
#include "c_dispatch.h"
#include "i_system.h"


//==========================================================================
//
// global variables
// These two are the last remaining ones:
// - The global script contains static data so it must be global
// - The trigger is referenced by a global variable. However, it is set 
//   each time a script is started so that's not a problem.
//
//==========================================================================

DFsScript global_script;
AActor *trigger_obj;

//==========================================================================
//
//
//
//==========================================================================

#define DECLARE_16_POINTERS(v, i) \
	DECLARE_POINTER(v[i]) \
	DECLARE_POINTER(v[i+1]) \
	DECLARE_POINTER(v[i+2]) \
	DECLARE_POINTER(v[i+3]) \
	DECLARE_POINTER(v[i+4]) \
	DECLARE_POINTER(v[i+5]) \
	DECLARE_POINTER(v[i+6]) \
	DECLARE_POINTER(v[i+7]) \
	DECLARE_POINTER(v[i+8]) \
	DECLARE_POINTER(v[i+9]) \
	DECLARE_POINTER(v[i+10]) \
	DECLARE_POINTER(v[i+11]) \
	DECLARE_POINTER(v[i+12]) \
	DECLARE_POINTER(v[i+13]) \
	DECLARE_POINTER(v[i+14]) \
	DECLARE_POINTER(v[i+15]) \

//==========================================================================
//
//
//
//==========================================================================

IMPLEMENT_POINTY_CLASS(DFsScript)
	DECLARE_POINTER(parent)
	DECLARE_POINTER(trigger)
	DECLARE_16_POINTERS(sections, 0)
	DECLARE_POINTER(sections[16])
	DECLARE_16_POINTERS(variables, 0)
	DECLARE_16_POINTERS(children, 0)
	DECLARE_16_POINTERS(children, 16)
	DECLARE_16_POINTERS(children, 32)
	DECLARE_16_POINTERS(children, 48)
	DECLARE_16_POINTERS(children, 64)
	DECLARE_16_POINTERS(children, 80)
	DECLARE_16_POINTERS(children, 96)
	DECLARE_16_POINTERS(children, 112)
	DECLARE_16_POINTERS(children, 128)
	DECLARE_16_POINTERS(children, 144)
	DECLARE_16_POINTERS(children, 160)
	DECLARE_16_POINTERS(children, 176)
	DECLARE_16_POINTERS(children, 192)
	DECLARE_16_POINTERS(children, 208)
	DECLARE_16_POINTERS(children, 224)
	DECLARE_16_POINTERS(children, 240)
	DECLARE_POINTER(children[256])
END_POINTERS

//==========================================================================
//
//
//
//==========================================================================

void DFsScript::ClearChildren()
{
	int j;
	for(j=0;j<MAXSCRIPTS;j++) if (children[j])
	{
		children[j]->Destroy();
		children[j]=NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

DFsScript::DFsScript()
{
	int i;
	
	for(i=0; i<SECTIONSLOTS; i++) sections[i] = NULL;
	for(i=0; i<VARIABLESLOTS; i++) variables[i] = NULL;
	for(i=0; i<MAXSCRIPTS; i++)	children[i] = NULL;

	data = NULL;
	scriptnum = -1;
	len = 0;
	parent = NULL;
	trigger = NULL;
	lastiftrue = false;
}

//==========================================================================
//
//
//
//==========================================================================

void DFsScript::Destroy()
{
	ClearVariables(true);
	ClearSections();
	ClearChildren();
	parent = NULL;
	if (data != NULL) delete [] data;
	data = NULL;
	parent = NULL;
	trigger = NULL;
	Super::Destroy();
}

//==========================================================================
//
//
//
//==========================================================================

void DFsScript::Serialize(FArchive &arc)
{
	Super::Serialize(arc);
	// don't save a reference to the global script
	if (parent == &global_script) parent = NULL;

	arc << data << scriptnum << len << parent << trigger << lastiftrue;
	for(int i=0; i< SECTIONSLOTS; i++) arc << sections[i];
	for(int i=0; i< VARIABLESLOTS; i++) arc << variables[i];
	for(int i=0; i< MAXSCRIPTS; i++) arc << children[i];

	if (parent == NULL) parent = &global_script;
}

//==========================================================================
//
// run_script
//
// the function called by t_script.c
//
//==========================================================================

void DFsScript::ParseScript(char *position)
{
	if (position == NULL) 
	{
		lastiftrue = false;
		position = data;
	}

	// check for valid position
	if(position < data || position > data+len)
    {
		script_error("parse_script: trying to continue from point outside script!\n");
		return;
    }
	
	trigger_obj = trigger;  // set trigger
	
	FParser parse(this);
	parse.Run(position, data, data + len);
	
	// dont clear global vars!
	if(scriptnum != -1) ClearVariables();        // free variables
	
	// haleyjd
	lastiftrue = false;
}

//==========================================================================
//
// Running Scripts
//
//==========================================================================

IMPLEMENT_POINTY_CLASS(DRunningScript)
	DECLARE_POINTER(prev)
	DECLARE_POINTER(next)
	DECLARE_POINTER(trigger)
	DECLARE_16_POINTERS(variables, 0)
END_POINTERS

//==========================================================================
//
//
//
//==========================================================================

DRunningScript::DRunningScript() 
{
	prev = next = NULL;
	trigger = NULL;
	script = NULL;
	save_point = 0;
	wait_type = wait_data = 0;
	for(int i=0; i< VARIABLESLOTS; i++) variables[i] = NULL;
}

//==========================================================================
//
//
//
//==========================================================================

void DRunningScript::Destroy()
{
	int i;
	DFsVariable *current, *next;
	
	for(i=0; i<VARIABLESLOTS; i++)
    {
		current = variables[i];
		
		// go thru this chain
		while(current)
		{
			next = current->next; // save for after freeing
			current->Destroy();
			current = next; // go to next in chain
		}
		variables[i] = NULL;
    }
	Super::Destroy();
}

//==========================================================================
//
//
//
//==========================================================================

void DRunningScript::Serialize(FArchive &arc)
{
	Super::Serialize(arc);

	arc << script << save_point << wait_type << wait_data << prev << next << trigger;
	for(int i=0; i< VARIABLESLOTS; i++) arc << variables[i];
}


//==========================================================================
//
// The main thinker
//
//==========================================================================
IMPLEMENT_POINTY_CLASS(DActorPointer)
	DECLARE_POINTER(actor)
END_POINTERS

IMPLEMENT_POINTY_CLASS(DFraggleThinker)
	DECLARE_POINTER(RunningScripts)
	DECLARE_POINTER(LevelScript)
END_POINTERS

DFraggleThinker *DFraggleThinker::ActiveThinker;

//==========================================================================
//
//
//
//==========================================================================

DFraggleThinker::DFraggleThinker() 
{
	if (ActiveThinker)
	{
		I_Error ("Only one FraggleThinker is allowed to exist at a time.\nCheck your code.");
	}
	else
	{
		ActiveThinker = this;
		RunningScripts = new DRunningScript;
		LevelScript = new DFsScript;
		LevelScript->parent = &global_script;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::Destroy()
{
	DRunningScript *p = RunningScripts;
	while (p)
	{
		DRunningScript *q = p;
		p = p->next;
		q->prev = q->next = NULL;
		q->Destroy();
	}
	RunningScripts = NULL;

	LevelScript->Destroy();
	LevelScript = NULL;

	for(unsigned i=0; i<SpawnedThings.Size(); i++)
	{
		SpawnedThings[i]->Destroy();
	}
	SpawnedThings.Clear();
	ActiveThinker = NULL;
	Super::Destroy();
}

//==========================================================================
//
//
//
//==========================================================================

void DFraggleThinker::Serialize(FArchive &arc)
{
	Super::Serialize(arc);
	arc << LevelScript << RunningScripts;

	int count = SpawnedThings.Size ();
	arc << count;
	if (arc.IsLoading()) SpawnedThings.Resize(count);
	for(int i=0;i<count;i++)
	{
		arc << SpawnedThings[i];
	}

}

//==========================================================================
//
// PAUSING SCRIPTS
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
			for(current = RunningScripts->next; current; current = current->next)
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
			for(current = RunningScripts->next; current; current=current->next)
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
	
	current = RunningScripts->next;
    
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
			current->script->ParseScript (current->script->data + current->save_point);

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

void T_PreprocessScripts()
{
	DFraggleThinker *th = DFraggleThinker::ActiveThinker;
	if (th)
	{
		// run the levelscript first
		// get the other scripts
		
		// levelscript started by player 0 'superplayer'
		th->LevelScript->trigger = players[0].mo;
		
		th->LevelScript->Preprocess();
		th->LevelScript->ParseScript();
	}
}

//==========================================================================
//
//
//
//==========================================================================

static bool T_RunScript(int snum, AActor * t_trigger)
{
	DFraggleThinker *th = DFraggleThinker::ActiveThinker;
	if (th)
	{
		// [CO] It is far too dangerous to start the script right away.
		// Better queue it for execution for the next time
		// the runningscripts are checked.

		DRunningScript *runscr;
		DFsScript *script;
		int i;
		
		if(snum < 0 || snum >= MAXSCRIPTS) return false;
		script = th->LevelScript->children[snum];
		if(!script)	return false;
	
		runscr = new DRunningScript();
		runscr->script = script;
		runscr->save_point = 0; // start at beginning
		runscr->wait_type = wt_none;      // start straight away
		runscr->next = th->RunningScripts->next;
		runscr->prev = th->RunningScripts;
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
			while(runscr->variables[i] && runscr->variables[i]->type != svt_label)
				runscr->variables[i] = runscr->variables[i]->next;
		}
		// copy trigger
		runscr->trigger = t_trigger;
		return true;
	}
	return false;
}

//==========================================================================
//
//
//
//==========================================================================

static int LS_FS_Execute (line_t *ln, AActor *it, bool backSide,
	int arg0, int arg1, int arg2, int arg3, int arg4)
// FS_Execute(script#,firstsideonly,lock,msgtype)
{
	if (arg1 && ln && backSide) return false;
	if (arg2!=0 && !P_CheckKeys(it, arg2, !!arg3)) return false;
	return T_RunScript(arg0,it);
}

//==========================================================================
//
//
//
//==========================================================================

void STACK_ARGS FS_Close()
{
	int i;
	DFsVariable *current, *next;

	// we have to actually delete the global variables if we don't want
	// to get them reported as memory leaks.
	for(i=0; i<VARIABLESLOTS; i++)
    {
		current = global_script.variables[i];
		
		while(current)
		{
			next = current->next; // save for after freeing
			
			//current->ObjectFlags |= OF_YESREALLYDELETE;
			delete current;
			current = next; // go to next in chain
		}
    }
}

AT_GAME_SET(FS_Init)
{
	void init_functions();

	// I'd rather link the special here than make another source file depend on FS!
	LineSpecials[54]=LS_FS_Execute;
	init_functions();
	atterm(FS_Close);
}

//==========================================================================
//
//
//
//==========================================================================

CCMD(fpuke)
{
	int argc = argv.argc();

	if (argc < 2)
	{
		Printf (" fpuke <script>\n");
	}
	else
	{
		T_RunScript(atoi(argv[1]), players[consoleplayer].mo);
	}
}
