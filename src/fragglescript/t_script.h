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

#ifndef __T_SCRIPT_H__
#define __T_SCRIPT_H__

#include "p_setup.h"
#include "t_parse.h"
#include "t_vari.h"


enum waittype_e
{
    wt_none,        // not waiting
	wt_delay,       // wait for a set amount of time
	wt_tagwait,     // wait for sector to stop moving
	wt_scriptwait,  // wait for script to finish
	wt_scriptwaitpre, // haleyjd - wait for script to start
};

class DRunningScript : public DObject
{
	DECLARE_CLASS(DRunningScript, DObject)
	HAS_OBJECT_POINTERS

public:
	DRunningScript() 
	{
		script = NULL;
		wait_type = wt_none;
		wait_data = 0;
		prev = next = NULL;
		trigger = NULL;
	}
	script_t *script;
	
	// where we are
	char *savepoint;
	
	int wait_type;
	int wait_data;  // data for wait: tagnum, counter, script number etc
	
	// saved variables
	svariable_t *variables[VARIABLESLOTS];
	
	DRunningScript *prev, *next;  // for chain
	AActor *trigger;
};

//-----------------------------------------------------------------------------
//
// This thinker eliminates the need to call the Fragglescript functions from the main code
//
//-----------------------------------------------------------------------------
void T_SerializeScripts(FArchive & ar);

class DFraggleThinker : public DThinker
{
	DECLARE_CLASS(DFraggleThinker, DThinker)

	static operator_t operators[];
	static int num_operators;

	// the global script just holds all
	// the global variables
	script_t global_script;

	// the hub script holds all the variables
	// shared between levels in a hub
	script_t hub_script;

	int newvar_type;
	script_t *newvar_script;
	TArray<void *> levelpointers;

	AActor *trigger_obj;            // object which triggered script

	char *tokens[T_MAXTOKENS];
	tokentype_t tokentype[T_MAXTOKENS];
	int num_tokens;

	script_t *current_script;       // the current script
	svalue_t nullvar;      // null var for empty return
	int killscript;         // when set to true, stop the script quickly
	section_t *prev_section;       // the section from the previous statement

	char *linestart;        // start of line
	char *rover;            // current point reached in script

	section_t *current_section; // the section (if any) found in parsing the line
	int bracetype;              // bracket_open or bracket_close

	int t_argc;                     // number of arguments
	svalue_t *t_argv;               // arguments
	svalue_t t_return;              // returned value
	char * t_func;					// name of current function

	// the level script is just the stuff put in the wad,
	// which the other scripts are derivatives of
	script_t levelscript;

	DRunningScript *runningscripts;


	section_t *looping_section();
	bool CheckArgs(int cnt);
	DRunningScript *T_SaveCurrentScript();
	bool wait_finished(DRunningScript *script);
	void clear_runningscripts();

	svariable_t *find_variable(char *name);

	void init_functions();

	void T_ClearHubScript();
	svariable_t *new_variable(script_t *script, char *name, int vtype);
	svariable_t *variableforname(script_t *script, char *name);
	void clear_variables(script_t *script);
	svariable_t *add_game_int(char *name, int *var);
	svariable_t *add_game_mobj(char *name, AActor **mo);
	svariable_t *new_function(char *name, void (DFraggleThinker::*handler)() );

	FString GetFormatString(int startarg);

	void SF_Print();
	void SF_Rnd();
	void SF_Continue();
	void SF_Break();
	void SF_Goto();
	void SF_Return();
	void SF_Include();
	void SF_Input();
	void SF_Beep();
	void SF_Clock();
	void SF_ExitLevel();
	void SF_Tip();
	void SF_TimedTip();
	void SF_PlayerTip();
	void SF_Message();
	void SF_PlayerMsg();
	void SF_PlayerInGame();
	void SF_PlayerName();
	void SF_PlayerObj();
	void SF_StartScript();      // FPUKE needs to access this
	void SF_ScriptRunning();
	void SF_Wait();
	void SF_TagWait();
	void SF_ScriptWait();
	void SF_ScriptWaitPre();    // haleyjd: new wait types
	void SF_Player();
	void SF_Spawn();
	void SF_RemoveObj();
	void SF_KillObj();
	void SF_ObjX();
	void SF_ObjY();
	void SF_ObjZ();
	void SF_ObjAngle();
	void SF_Teleport();
	void SF_SilentTeleport();
	void SF_DamageObj();
	void SF_ObjSector();
	void SF_ObjHealth();
	void SF_ObjFlag();
	void SF_PushThing();
	void SF_ReactionTime();
	void SF_MobjTarget();
	void SF_MobjMomx();
	void SF_MobjMomy();
	void SF_MobjMomz();
	void SF_PointToAngle();
	void SF_PointToDist();
	void SF_SetCamera();
	void SF_ClearCamera();
	void SF_StartSound();
	void SF_StartSectorSound();
	void SF_FloorHeight();
	void SF_MoveFloor();
	void SF_CeilingHeight();
	void SF_MoveCeiling();
	void SF_LightLevel();
	void SF_FadeLight();
	void SF_FloorTexture();
	void SF_SectorColormap();
	void SF_CeilingTexture();
	void SF_ChangeHubLevel();
	void SF_StartSkill();
	void SF_OpenDoor();
	void SF_CloseDoor();
	void SF_RunCommand();
	void SF_LineTrigger();
	void SF_ChangeMusic();
	void SF_SetLineBlocking();
	void SF_SetLineMonsterBlocking();
	void SF_SetLineTexture();
	void SF_Max();
	void SF_Min();
	void SF_Abs();
	void SF_Gameskill();
	void SF_Gamemode();
	void SF_IsPlayerObj();
	void SF_PlayerKeys();
	void SF_PlayerAmmo();
	void SF_MaxPlayerAmmo();
	void SF_PlayerWeapon();
	void SF_PlayerSelectedWeapon();
	void SF_GiveInventory();
	void SF_TakeInventory();
	void SF_CheckInventory();
	void SF_SetWeapon();
	void SF_MoveCamera();
	void SF_ObjAwaken();
	void SF_AmbientSound();
	void SF_ExitSecret();
	void SF_MobjValue();
	void SF_StringValue();
	void SF_IntValue();
	void SF_FixedValue();
	void SF_SpawnExplosion();
	void SF_RadiusAttack();
	void SF_SetObjPosition();
	void SF_TestLocation();
	void SF_HealObj();  //no pain sound
	void SF_ObjDead();
	void SF_SpawnMissile();
	void SF_MapThingNumExist();
	void SF_MapThings();
	void SF_ObjState();
	void SF_LineFlag();
	void SF_PlayerAddFrag();
	void SF_SkinColor();
	void SF_PlayDemo();
	void SF_CheckCVar();
	void SF_Resurrect();
	void SF_LineAttack();
	void SF_ObjType();
	void SF_Sin();
	void SF_ASin();
	void SF_Cos();
	void SF_ACos();
	void SF_Tan();
	void SF_ATan();
	void SF_Exp();
	void SF_Log();
	void SF_Sqrt();
	void SF_Floor();
	void SF_Pow();
	void SF_NewHUPic();
	void SF_DeleteHUPic();
	void SF_ModifyHUPic();
	void SF_SetHUPicDisplay();
	void SF_SetCorona();
	void SF_Ls();
	void SF_LevelNum();
	void SF_MobjRadius();
	void SF_MobjHeight();
	void SF_ThingCount();
	void SF_SetColor();
	void SF_SpawnShot2();
	void SF_KillInSector();
	void SF_SectorType();
	void SF_SetLineTrigger();
	void SF_ChangeTag();
	void SF_WallGlow();

	svalue_t OPequals(int, int, int);           // =

	svalue_t OPplus(int, int, int);             // +
	svalue_t OPminus(int, int, int);            // -
	svalue_t OPmultiply(int, int, int);         // *
	svalue_t OPdivide(int, int, int);           // /
	svalue_t OPremainder(int, int, int);        // %

	svalue_t OPor(int, int, int);               // ||
	svalue_t OPand(int, int, int);              // &&
	svalue_t OPnot(int, int, int);              // !

	svalue_t OPor_bin(int, int, int);           // |
	svalue_t OPand_bin(int, int, int);          // &
	svalue_t OPnot_bin(int, int, int);          // ~

	svalue_t OPcmp(int, int, int);              // ==
	svalue_t OPnotcmp(int, int, int);           // !=
	svalue_t OPlessthan(int, int, int);         // <
	svalue_t OPgreaterthan(int, int, int);      // >

	svalue_t OPincrement(int, int, int);        // ++
	svalue_t OPdecrement(int, int, int);        // --

	svalue_t OPstructure(int, int, int);    // in t_vari.c

	// haleyjd: Eternity operator extensions
	// sorely missing compound comparison operators
	svalue_t OPlessthanorequal(int, int, int);     // <=
	svalue_t OPgreaterthanorequal(int, int, int);  // >=

	svalue_t evaluate_expression(int start, int stop);
	svalue_t evaluate_function(int start, int stop);

	void spec_script();
	void spec_brace();
	bool spec_if();
	bool spec_elseif(bool lastif);
	void spec_else(bool lastif);
	void spec_while();
	void spec_for();                 // for() loop
	bool spec_variable();

	int find_operator(int start, int stop, char *value);
	int find_operator_backwards(int start, int stop, char *value);
	

	void create_variable(int start, int stop);

	void parse_var_line(int start);
	void parse_data(char *data, char *end);
	void parse_script();
	void parse_include(char *lumpname);
	void continue_script(script_t *script, char *continue_point);
	void run_script(script_t *script);

	void run_statement();

	void clear_script();
	section_t *new_section(char *brace);
	section_t *find_section_start(char *brace);
	section_t *find_section_end(char *brace);
	svariable_t *new_label(char *labelptr);
	char *process_find_char(char *data, char find);
	void dry_run_script();
	void preprocess(script_t *script);

	void next_token();
	void get_tokens(char *s);
	void add_char(char c);
	void print_tokens();
	void script_error(char *s, ...);

	svalue_t simple_evaluate(int n);
	void pointless_brackets(int *start, int *stop);

	svalue_t getvariablevalue(svariable_t *v);
	void setvariablevalue(svariable_t *v, svalue_t newvalue);

	void ArchiveRunningScript(FArchive & ar,DRunningScript *rs);
	DRunningScript *UnArchiveRunningScript(FArchive & ar);
	void ArchiveRunningScripts(FArchive & ar);
	void UnArchiveRunningScripts(FArchive & ar);
	void ArchiveSpawnedThings(FArchive & ar);

public:

	DFraggleThinker() {} 
	DFraggleThinker(const char *scriptdata);

	void PreprocessScripts();
	void RunScript(int snum, AActor * t_trigger);

	void Destroy();
	void Serialize(FArchive & arc);
	void Tick();
};


#include "t_fs.h"

AActor *MobjForSvalue(svalue_t svalue);

// savegame related stuff

extern TArray<DActorPointer*> SpawnedThings;

void FS_EmulateCmd(char * string);

#endif

