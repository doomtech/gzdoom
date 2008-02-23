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

#ifndef __PARSE_H__
#define __PARSE_H__

#include "m_fixed.h"
#include "actor.h"

typedef struct script_s script_t;
typedef struct operator_s operator_t;

#define T_MAXTOKENS 256
#define TOKENLENGTH 128

enum
{
  svt_string,
  svt_int,
  svt_mobj,         // a map object
  svt_function,     // functions are stored as variables
  svt_label,        // labels for goto calls are variables
  svt_const,        // const
  svt_fixed,        // haleyjd: fixed-point int - 8-17 std
  svt_arraysweredeleted,
  svt_pInt,         // pointer to game int
  svt_pMobj,        // pointer to game mobj
  svt_parraysweredeleted,
};

struct svalue_t
{
	int type;
	FString string;
	union
	{
		long i;
		char *labelptr; // goto() label
		fixed_t f;      // haleyjd: 8-17
		AActor *mobj;
	} value;

	svalue_t()
	{
		type = svt_int;
		value.i = 0;
	}

	svalue_t(const svalue_t & other)
	{
		type = other.type;
		string = other.string;
		value = other.value;
	}
};

// un-inline these two functions to save some memory
int intvalue(const svalue_t & v);
fixed_t fixedvalue(const svalue_t & v);
float floatvalue(const svalue_t & v);
const char *stringvalue(const svalue_t & v);

#include "t_vari.h"
#include "t_prepro.h"

// haleyjd: moved from t_script.h - 8-17
// 01/06/01: doubled number of allowed scripts
#define MAXSCRIPTS 257

struct script_s
{
  // script data
  
  char *data;
  int scriptnum;  // this script's number
  int len;
  
  // {} sections
  
  section_t *sections[SECTIONSLOTS];
  
  // variables:
  
  svariable_t *variables[VARIABLESLOTS];
  
  // ptr to the parent script
  // the parent script is the script above this level
  // eg. individual linetrigger scripts are children
  // of the levelscript, which is a child of the
  // global_script
  script_t *parent;

  // haleyjd: 8-17
  // child scripts.
  // levelscript holds ptrs to all of the level's scripts
  // here.
  script_t *children[MAXSCRIPTS];


  AActor *trigger;        // object which triggered this script

  bool lastiftrue;     // haleyjd: whether last "if" statement was 
                          // true or false

	svariable_t *NewVariable(const char *name, int vtype);
	svariable_t *VariableForName(const char *name);
	svariable_t *FindVariable(const char *name);
	void ClearVariables(bool complete= false);
	svariable_t *NewLabel(char *labelptr);

};

struct operator_s
{
  char *string;
  svalue_t (*handler)(int, int, int); // left, mid, right
  int direction;
};

enum
{
  forward,
  backward
};

void run_script(script_t *script);
void continue_script(script_t *script, char *continue_point);
void parse_include(char *lumpname);
void run_statement();
void script_error(char *s, ...);

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);
int find_operator_backwards(int start, int stop, char *value);

/******* tokens **********/

typedef enum
{
  name_,   // a name, eg 'count1' or 'frag'
  number,
  operator_,
  string_,
  unset,
  function          // function name
} tokentype_t;

enum    // brace types: where current_section is a { or }
{
  bracket_open,
  bracket_close
};

extern svalue_t nullvar;

extern script_t *current_script;
extern AActor *trigger_obj;
extern int killscript;

extern char *tokens[T_MAXTOKENS];
extern tokentype_t tokentype[T_MAXTOKENS];
extern int num_tokens;
extern char *rover;     // current point reached in parsing
extern char *linestart; // start of the current expression

extern section_t *current_section;
extern section_t *prev_section;
extern int bracetype;

// the global_script is the root
// script and contains only built-in
// FraggleScript variables/functions

extern script_t global_script; 
extern script_t hub_script;

#endif

