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

typedef struct sfarray_s sfarray_t;
typedef struct script_s script_t;
typedef struct svalue_s svalue_t;
typedef struct operator_s operator_t;

#define T_MAXTOKENS 256
#define TOKENLENGTH 128

struct svalue_s
{
  int type;
  union
  {
    long i;
    char *s;
    char *labelptr; // goto() label
    fixed_t f;      // haleyjd: 8-17
    AActor *mobj;
    sfarray_t *a;   // haleyjd 05/27: arrays
  } value;
};

struct sfarray_s
{
   struct sfarray_s *next; // next array in save list
   int saveindex;	   // index for saving

   unsigned int length;	   // number of values currently initialized   
   svalue_t *values;	   // array of contained values
};

// haleyjd: following macros updated to 8-17 standard

        // furthered to include support for svt_mobj

// un-inline these two functions to save some memory
int intvalue(const svalue_s & v);
fixed_t fixedvalue(const svalue_s & v);
float floatvalue(const svalue_s & v);
const char *stringvalue(const svalue_t & v);

#include "t_vari.h"

// haleyjd: moved from t_script.h - 8-17
// 01/06/01: doubled number of allowed scripts
#define MAXSCRIPTS 257

enum
{
	SECTIONSLOTS = 17
};

/***** {} sections **********/

struct section_t
{
	char *start;    // offset of starting brace {
	char *end;      // offset of ending brace   }
	int type;       // section type: for() loop, while() loop etc

	union
	{
		struct
		{
			char *loopstart;  // positioned before the while()
		} data_loop;
	} data; // data for section

	section_t *next;        // for hashing
};

enum    // section types
{
	st_empty,       // none: empty {} braces
	st_if,          // if() statement
	st_elseif,      // haleyjd: elseif()
	st_else,        // haleyjd: else()
	st_loop,        // loop
};



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
};

struct operator_s
{
  char *string;
  svalue_t (DFraggleThinker::*handler)(int, int, int); // left, mid, right
  int direction;
};

enum
{
  forward,
  backward
};

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

#endif

