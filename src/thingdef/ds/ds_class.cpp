/*
** ds_class
**
** new parser implementation - handling of class definitions
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


#include "thingdef/thingdef.h"
#include "thingdef/thingdef_statement.h"
#include "thingdef/thingdef_exp.h"

//==========================================================================
//
// 
//
//==========================================================================

FsClass::FsClass(FName clsname, FName parentname, bool actordef, bool native, const FScriptPosition &pos, Baggage *bag)
{
	Position = pos;
	if (actordef || clsname == NAME_Actor)
	{
		Info = CreateNewActor(pos, clsname, parentname, NAME_None, -1, native);
		Class = Info->Class;
		bag->Class = this;
		bag->ClassName = clsname;
	}
	else
	{
		Position.Message(MSG_FATAL, "Non-actor class definition found for class '%s'", clsname.GetChars());
	}

}

//==========================================================================
//
// 
//
//==========================================================================

void FsClass::DefineClass(Baggage &bag)
{
	FinishActor(Position, Info, bag);
	delete this;
}

//==========================================================================
//
// 
//
//==========================================================================

void FsClass::DefineConstant(FsStatement *constant)
{
	constant->Resolve(FCompileContext(Class, false, true), true);
	delete constant;
}

