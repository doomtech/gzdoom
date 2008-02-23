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

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

// It is impractical to have svariable_t being declared as a DObject so
// I have to use an intermediate class to make this subject to
// automatic pointer cleanup.

class DActorPointer : public DObject
{
	DECLARE_CLASS(DActorPointer, DObject)
	HAS_OBJECT_POINTERS

public:

	AActor * actor;

	DActorPointer()
	{
		actor=NULL;
	}

	void Serialize(FArchive & ar)
	{
		Super::Serialize(ar);
		ar << actor;
	}
};

#include "t_variable.h"


// variables

void T_ClearHubScript();

void init_variables();


// functions

svalue_t evaluate_function(int start, int stop);   // actually run a function

// arguments to handler functions

#define MAXARGS 128
extern int t_argc;
extern svalue_t *t_argv;
extern svalue_t t_return;
extern FString t_func;

#endif
