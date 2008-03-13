/*
** dthinker.cpp
** Implements the base class for almost anything in a level that might think
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#include "dthinker.h"
#include "stats.h"
#include "p_local.h"
#include "statnums.h"
#include "i_system.h"
#include "doomerrors.h"


static cycle_t ThinkCycles;
extern cycle_t BotSupportCycles;
extern cycle_t BotWTG;

IMPLEMENT_POINTY_CLASS (DThinker)
 DECLARE_POINTER(NextThinker)
 DECLARE_POINTER(PrevThinker)
END_POINTERS

static DThinker *NextToThink;

FThinkerList DThinker::Thinkers[MAX_STATNUM+1];
FThinkerList DThinker::FreshThinkers[MAX_STATNUM+1];
bool DThinker::bSerialOverride = false;

void FThinkerList::AddTail(DThinker *thinker)
{
	if (Sentinel == NULL)
	{
		Sentinel = new DThinker(DThinker::NO_LINK);
		Sentinel->ObjectFlags |= OF_Sentinel;
		Sentinel->NextThinker = Sentinel;
		Sentinel->PrevThinker = Sentinel;
		GC::WriteBarrier(Sentinel);
	}
	DThinker *tail = Sentinel->PrevThinker;
	assert(tail->NextThinker == Sentinel);
	thinker->PrevThinker = tail;
	thinker->NextThinker = Sentinel;
	tail->NextThinker = thinker;
	Sentinel->PrevThinker = thinker;
	GC::WriteBarrier(thinker, tail);
	GC::WriteBarrier(thinker, Sentinel);
	GC::WriteBarrier(tail, thinker);
	GC::WriteBarrier(Sentinel, thinker);
}

DThinker *FThinkerList::GetHead() const
{
	if (Sentinel == NULL || Sentinel->NextThinker == Sentinel)
	{
		return NULL;
	}
	return Sentinel->NextThinker;
}

DThinker *FThinkerList::GetTail() const
{
	if (Sentinel == NULL || Sentinel->PrevThinker == Sentinel)
	{
		return NULL;
	}
	return Sentinel->PrevThinker;
}

bool FThinkerList::IsEmpty() const
{
	return Sentinel == NULL || Sentinel->NextThinker == NULL;
}

void DThinker::SaveList(FArchive &arc, DThinker *node)
{
	if (node != NULL)
	{
		while (!(node->ObjectFlags & OF_Sentinel))
		{
			arc << node;
			node = node->NextThinker;
		}
	}
}

void DThinker::SerializeAll(FArchive &arc, bool hubLoad)
{
	DThinker *thinker;
	BYTE stat;
	int statcount;
	int i;

	// Save lists of thinkers, but not by storing the first one and letting
	// the archiver catch the rest. (Which leads to buttloads of recursion
	// and makes the file larger.) Instead, we explicitly save each thinker
	// in sequence. When restoring an archive, we also have to maintain
	// the thinker lists here instead of relying on the archiver to do it
	// for us.

	if (arc.IsStoring())
	{
		for (statcount = i = 0; i <= MAX_STATNUM; i++)
		{
			statcount += (!Thinkers[i].IsEmpty() || !FreshThinkers[i].IsEmpty());
		}
		arc << statcount;
		for (i = 0; i <= MAX_STATNUM; i++)
		{
			if (!Thinkers[i].IsEmpty() || !FreshThinkers[i].IsEmpty())
			{
				stat = i;
				arc << stat;
				SaveList(arc, Thinkers[i].GetHead());
				SaveList(arc, FreshThinkers[i].GetHead());
				thinker = NULL;
				arc << thinker;		// Save a final NULL for this list
			}
		}
	}
	else
	{
		if (hubLoad)
			DestroyMostThinkers();
		else
			DestroyAllThinkers();

		// Prevent the constructor from inserting thinkers into a list.
		bSerialOverride = true;

		try
		{
			arc << statcount;
			while (statcount > 0)
			{
				arc << stat << thinker;
				while (thinker != NULL)
				{
					// Thinkers with the OF_JustSpawned flag set go in the FreshThinkers
					// list. Anything else goes in the regular Thinkers list.
					if (thinker->ObjectFlags & OF_JustSpawned)
					{
						FreshThinkers[stat].AddTail(thinker);
					}
					else
					{
						Thinkers[stat].AddTail(thinker);
					}
					arc << thinker;
				}
				statcount--;
			}
		}
		catch (class CDoomError &)
		{
			bSerialOverride = false;
			DestroyAllThinkers();
			throw;
		}
		bSerialOverride = false;
	}
}

DThinker::DThinker (int statnum) throw()
{
	if (bSerialOverride)
	{ // The serializer will insert us into the right list
		NextThinker = NULL;
		PrevThinker = NULL;
		return;
	}

	ObjectFlags |= OF_JustSpawned;
	if ((unsigned)statnum > MAX_STATNUM)
	{
		statnum = MAX_STATNUM;
	}
	FreshThinkers[statnum].AddTail (this);
}

DThinker::DThinker(no_link_type foo) throw()
{
	foo;	// Avoid unused argument warnings.
}

DThinker::~DThinker ()
{
	assert(NextThinker == NULL && PrevThinker == NULL);
}

void DThinker::Destroy ()
{
	if (NextThinker != NULL)
	{
		Remove();
	}
	Super::Destroy();
}

void DThinker::Remove()
{
	if (this == NextToThink)
	{
		NextToThink = NextThinker;
	}
	DThinker *prev = PrevThinker;
	DThinker *next = NextThinker;
	assert(prev != NULL && next != NULL);
	prev->NextThinker = next;
	next->PrevThinker = prev;
	GC::WriteBarrier(prev, next);
	GC::WriteBarrier(next, prev);
	NextThinker = NULL;
	PrevThinker = NULL;
}

void DThinker::PostBeginPlay ()
{
}

DThinker *DThinker::FirstThinker (int statnum)
{
	DThinker *node;

	if ((unsigned)statnum > MAX_STATNUM)
	{
		statnum = MAX_STATNUM;
	}
	node = Thinkers[statnum].GetHead();
	if (node == NULL)
	{
		node = FreshThinkers[statnum].GetHead();
		if (node == NULL)
		{
			return NULL;
		}
	}
	return node;
}

void DThinker::ChangeStatNum (int statnum)
{
	FThinkerList *list;

	if ((unsigned)statnum > MAX_STATNUM)
	{
		statnum = MAX_STATNUM;
	}
	Remove();
	if ((ObjectFlags & OF_JustSpawned) && statnum >= STAT_FIRST_THINKING)
	{
		list = &FreshThinkers[statnum];
	}
	else
	{
		list = &Thinkers[statnum];
	}
	list->AddTail(this);
}

// Mark the first thinker of each list
void DThinker::MarkRoots()
{
	for (int i = 0; i <= MAX_STATNUM; ++i)
	{
		GC::Mark(Thinkers[i].Sentinel);
		GC::Mark(FreshThinkers[i].Sentinel);
	}
}

// Destroy every thinker
void DThinker::DestroyAllThinkers ()
{
	int i;

	for (i = 0; i <= MAX_STATNUM; i++)
	{
		if (i != STAT_TRAVELLING)
		{
			DestroyThinkersInList (Thinkers[i]);
			DestroyThinkersInList (FreshThinkers[i]);
		}
	}
	GC::FullGC();
}

// Destroy all thinkers except for player-controlled actors
// Players are simply removed from the list of thinkers and
// will be added back after serialization is complete.
void DThinker::DestroyMostThinkers ()
{
	int i;

	for (i = 0; i <= MAX_STATNUM; i++)
	{
		if (i != STAT_TRAVELLING)
		{
			DestroyMostThinkersInList (Thinkers[i], i);
			DestroyMostThinkersInList (FreshThinkers[i], i);
		}
	}
	GC::FullGC();
}

void DThinker::DestroyThinkersInList (FThinkerList &list)
{
	if (list.Sentinel != NULL)
	{
		DThinker *node = list.Sentinel->NextThinker;
		while (node != list.Sentinel)
		{
			DThinker *next = node->NextThinker;
			node->Destroy();
			node = next;
		}
		list.Sentinel->Destroy();
		list.Sentinel = NULL;
	}
}

void DThinker::DestroyMostThinkersInList (FThinkerList &list, int stat)
{
	if (stat != STAT_PLAYER)
	{
		DestroyThinkersInList (list);
	}
	else if (list.Sentinel != NULL)
	{
		DThinker *node = list.Sentinel->NextThinker;
		while (node != list.Sentinel)
		{
			DThinker *next = node->NextThinker;
			node->Remove();
			node = next;
		}
		list.Sentinel->Destroy();
		list.Sentinel = NULL;
	}
}

void DThinker::RunThinkers ()
{
	int i, count;

	ThinkCycles = BotSupportCycles = BotWTG = 0;

	clock (ThinkCycles);

	// Tick every thinker left from last time
	for (i = STAT_FIRST_THINKING; i <= MAX_STATNUM; ++i)
	{
		TickThinkers (&Thinkers[i], NULL);
	}

	// Keep ticking the fresh thinkers until there are no new ones.
	do
	{
		count = 0;
		for (i = STAT_FIRST_THINKING; i <= MAX_STATNUM; ++i)
		{
			count += TickThinkers (&FreshThinkers[i], &Thinkers[i]);
		}
	} while (count != 0);

	unclock (ThinkCycles);
}

int DThinker::TickThinkers (FThinkerList *list, FThinkerList *dest)
{
	int count = 0;
	DThinker *node = list->GetHead();

	if (node == NULL)
	{
		return 0;
	}

	while (node != list->Sentinel)
	{
		++count;
		NextToThink = node->NextThinker;
		if (node->ObjectFlags & OF_JustSpawned)
		{
			node->ObjectFlags &= ~OF_JustSpawned;
			if (dest != NULL)
			{ // Move thinker from this list to the destination list
				node->Remove();
				dest->AddTail(node);
			}
			node->PostBeginPlay();
		}
		else if (dest != NULL)
		{ // Move thinker from this list to the destination list
			I_Error("There is a thinker in the fresh list that has already ticked.\n");
		}

		if (!(node->ObjectFlags & OF_EuthanizeMe))
		{ // Only tick thinkers not scheduled for destruction
			node->Tick ();
		}
		node = NextToThink;
	}
	return count;
}

void DThinker::Tick ()
{
}

void DThinker::PointerSubstitution(DObject *old, DObject *notOld)
{
	// Pointer substitution must not, under any circumstances, change 
	// the linked thinker list or the game will freeze badly.
	DThinker *next = NextThinker;
	DThinker *prev = PrevThinker;
	Super::PointerSubstitution(old, notOld);
	NextThinker = next;
	PrevThinker = prev;
}

FThinkerIterator::FThinkerIterator (const PClass *type, int statnum)
{
	if ((unsigned)statnum > MAX_STATNUM)
	{
		m_Stat = STAT_FIRST_THINKING;
		m_SearchStats = true;
	}
	else
	{
		m_Stat = statnum;
		m_SearchStats = false;
	}
	m_ParentType = type;
	m_CurrThinker = DThinker::Thinkers[m_Stat].GetHead();
	m_SearchingFresh = false;
}

FThinkerIterator::FThinkerIterator (const PClass *type, int statnum, DThinker *prev)
{
	if ((unsigned)statnum > MAX_STATNUM)
	{
		m_Stat = STAT_FIRST_THINKING;
		m_SearchStats = true;
	}
	else
	{
		m_Stat = statnum;
		m_SearchStats = false;
	}
	m_ParentType = type;
	if (prev == NULL || (prev->NextThinker->ObjectFlags & OF_Sentinel))
	{
		Reinit();
	}
	else
	{
		m_CurrThinker = prev->NextThinker;
		m_SearchingFresh = false;
	}
}

void FThinkerIterator::Reinit ()
{
	m_CurrThinker = DThinker::Thinkers[m_Stat].GetHead();
	m_SearchingFresh = false;
}

DThinker *FThinkerIterator::Next ()
{
	if (m_ParentType == NULL)
	{
		return NULL;
	}
	do
	{
		do
		{
			if (m_CurrThinker != NULL)
			{
				while (!(m_CurrThinker->ObjectFlags & OF_Sentinel))
				{
					DThinker *thinker = m_CurrThinker;
					m_CurrThinker = thinker->NextThinker;
					if (thinker->IsKindOf(m_ParentType))
					{
						return thinker;
					}
				}
			}
			if ((m_SearchingFresh = !m_SearchingFresh))
			{
				m_CurrThinker = DThinker::FreshThinkers[m_Stat].GetHead();
			}
		} while (m_SearchingFresh);
		if (m_SearchStats)
		{
			m_Stat++;
			if (m_Stat > MAX_STATNUM)
			{
				m_Stat = STAT_FIRST_THINKING;
			}
		}
		m_CurrThinker = DThinker::Thinkers[m_Stat].GetHead();
		m_SearchingFresh = false;
	} while (m_SearchStats && m_Stat != STAT_FIRST_THINKING);
	return NULL;
}

ADD_STAT (think)
{
	FString out;
	out.Format ("Think time = %04.1f ms",
		SecondsPerCycle * (double)ThinkCycles * 1000);
	return out;
}
