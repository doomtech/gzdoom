// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: s_sound.c,v 1.3 1998/01/05 16:26:08 pekangas Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include "m_alloc.h"

#include "i_system.h"
#include "i_sound.h"
#include "i_music.h"
#include "i_cd.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "s_playlist.h"
#include "c_dispatch.h"
#include "m_random.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "doomstat.h"
#include "cmdlib.h"
#include "v_video.h"
#include "v_text.h"
#include "a_sharedglobal.h"
#include "gstrings.h"
#include "gi.h"
#include "templates.h"
#include "zstring.h"
#include "timidity/timidity.h"

// MACROS ------------------------------------------------------------------

#ifdef NeXT
// NeXT doesn't need a binary flag in open call
#define O_BINARY 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef FIXED2FLOAT
#define FIXED2FLOAT(f)			(((float)(f))/(float)65536)
#endif

#define NORM_PITCH				128
#define NORM_PRIORITY			64
#define NORM_SEP				0

#define S_PITCH_PERTURB 		1
#define S_STEREO_SWING			0.75

// TYPES -------------------------------------------------------------------

struct MusPlayingInfo
{
	FString name;
	MusInfo *handle;
	int   baseorder;
	bool  loop;
};

enum
{
	SOURCE_None,		// Sound is always on top of the listener.
	SOURCE_Actor,		// Sound is coming from an actor.
	SOURCE_Sector,		// Sound is coming from a sector.
	SOURCE_Polyobj,		// Sound is coming from a polyobject.
	SOURCE_Unattached,	// Sound is not attached to any particular emitter.
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

extern float S_GetMusicVolume (const char *music);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static bool S_CheckSoundLimit(sfxinfo_t *sfx, const FVector3 &pos, int near_limit);
static void S_ActivatePlayList(bool goBack);
static void CalcPosVel(const FSoundChan *chan, FVector3 *pos, FVector3 *vel);
static void CalcPosVel(int type, const AActor *actor, const sector_t *sector, const FPolyObj *poly,
	const float pt[3], int channel, int chanflags, FVector3 *pos, FVector3 *vel);
static void CalcSectorSoundOrg(const sector_t *sec, int channum, fixed_t *x, fixed_t *y, fixed_t *z);
static void CalcPolyobjSoundOrg(const FPolyObj *poly, fixed_t *x, fixed_t *y, fixed_t *z);
static FSoundChan *S_StartSound(AActor *mover, const sector_t *sec, const FPolyObj *poly,
	const FVector3 *pt, int channel, FSoundID sound_id, float volume, float attenuation);
static sfxinfo_t *S_LoadSound(sfxinfo_t *sfx);

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static bool		MusicPaused;		// whether music is paused
static MusPlayingInfo mus_playing;	// music currently being played
static FString	 LastSong;			// last music that was played
static FPlayList *PlayList;
static int		RestartEvictionsAt;	// do not restart evicted channels before this level.time

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int sfx_empty;

FSoundChan *Channels;
FSoundChan *FreeChannels;

int S_RolloffType;
float S_MinDistance;
float S_MaxDistanceOrRolloffFactor;
BYTE *S_SoundCurve;
int S_SoundCurveSize;

CVAR (Bool, snd_surround, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)	// [RH] Use surround sounds?
FBoolCVar noisedebug ("noise", false, 0);	// [RH] Print sound debugging info?
CVAR (Int, snd_channels, 32, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)	// number of channels available
CVAR (Bool, snd_flipstereo, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

// CODE --------------------------------------------------------------------

//==========================================================================
//
// S_NoiseDebug
//
// [RH] Print sound debug info. Called by status bar.
//==========================================================================

void S_NoiseDebug (void)
{
	FSoundChan *chan;
	FVector3 listener;
	FVector3 origin;
	int y, color;

	y = 32 * CleanYfac;
	screen->DrawText (CR_YELLOW, 0, y, "*** SOUND DEBUG INFO ***", TAG_DONE);
	y += 8;

	screen->DrawText (CR_GOLD, 0, y, "name", TAG_DONE);
	screen->DrawText (CR_GOLD, 70, y, "x", TAG_DONE);
	screen->DrawText (CR_GOLD, 120, y, "y", TAG_DONE);
	screen->DrawText (CR_GOLD, 170, y, "z", TAG_DONE);
	screen->DrawText (CR_GOLD, 220, y, "vol", TAG_DONE);
	screen->DrawText (CR_GOLD, 260, y, "dist", TAG_DONE);
	screen->DrawText (CR_GOLD, 300, y, "chan", TAG_DONE);
	screen->DrawText (CR_GOLD, 340, y, "flags", TAG_DONE);
	y += 8;

	if (Channels == NULL)
	{
		return;
	}

	listener.X = FIXED2FLOAT(players[consoleplayer].camera->x);
	listener.Y = FIXED2FLOAT(players[consoleplayer].camera->z);
	listener.Z = FIXED2FLOAT(players[consoleplayer].camera->y);

	// Display the oldest channel first.
	for (chan = Channels; chan->NextChan != NULL; chan = chan->NextChan)
	{ }
	while (y < SCREENHEIGHT - 16)
	{
		char temp[32];

		CalcPosVel(chan, &origin, NULL);
		color = (chan->ChanFlags & CHAN_LOOP) ? CR_BROWN : CR_GREY;

		// Name
		Wads.GetLumpName (temp, chan->SfxInfo->lumpnum);
		temp[8] = 0;
		screen->DrawText (color, 0, y, temp, TAG_DONE);

		if (!(chan->ChanFlags & CHAN_IS3D))
		{
			screen->DrawText(color, 70, y, "---", TAG_DONE);	// X
			screen->DrawText(color, 120, y, "---", TAG_DONE);	// Y
			screen->DrawText(color, 170, y, "---", TAG_DONE);	// Z
			screen->DrawText(color, 260, y, "---", TAG_DONE);	// Distance
		}
		else
		{
			// X coordinate
			mysnprintf (temp, countof(temp), "%.0f", origin.X);
			screen->DrawText (color, 70, y, temp, TAG_DONE);

			// Y coordinate
			mysnprintf (temp, countof(temp), "%.0f", origin.Z);
			screen->DrawText (color, 120, y, temp, TAG_DONE);

			// Z coordinate
			mysnprintf (temp, countof(temp), "%.0f", origin.Y);
			screen->DrawText (color, 170, y, temp, TAG_DONE);

			// Distance
			if (chan->DistanceScale > 0)
			{
				mysnprintf (temp, countof(temp), "%.0f", (origin - listener).Length());
				screen->DrawText (color, 260, y, temp, TAG_DONE);
			}
			else
			{
				screen->DrawText (color, 260, y, "---", TAG_DONE);
			}
		}

		// Volume
		mysnprintf (temp, countof(temp), "%.2g", chan->Volume);
		screen->DrawText (color, 220, y, temp, TAG_DONE);

		// Channel
		mysnprintf (temp, countof(temp), "%d", chan->EntChannel);
		screen->DrawText (color, 300, y, temp, TAG_DONE);

		// Flags
		mysnprintf (temp, countof(temp), "%s3%sZ%sU%sM%sN%sA%sL%sE",
			(chan->ChanFlags & CHAN_IS3D)			? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK,
			(chan->ChanFlags & CHAN_LISTENERZ)		? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK,
			(chan->ChanFlags & CHAN_UI)				? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK,
			(chan->ChanFlags & CHAN_MAYBE_LOCAL)	? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK,
			(chan->ChanFlags & CHAN_NOPAUSE)		? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK,
			(chan->ChanFlags & CHAN_AREA)			? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK,
			(chan->ChanFlags & CHAN_LOOP)			? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK,
			(chan->ChanFlags & CHAN_EVICTED)		? TEXTCOLOR_GREEN : TEXTCOLOR_BLACK);
		screen->DrawText (color, 340, y, temp, TAG_DONE);

		y += 8;
		if (chan->PrevChan == &Channels)
		{
			break;
		}
		chan = (FSoundChan *)((size_t)chan->PrevChan - myoffsetof(FSoundChan, NextChan));
	}
	BorderNeedRefresh = screen->GetPageCount ();
}

static FString LastLocalSndInfo;
static FString LastLocalSndSeq;
void S_AddLocalSndInfo(int lump);

//==========================================================================
//
// S_Init
//
// Initializes sound stuff, including volume. Sets channels, SFX and
// music volume, allocates channel buffer, and sets S_sfx lookup.
//==========================================================================

void S_Init ()
{
	int curvelump;

	atterm (S_Shutdown);

	// remove old data (S_Init can be called multiple times!)
	if (S_SoundCurve != NULL)
	{
		delete[] S_SoundCurve;
	}

	// Heretic and Hexen have sound curve lookup tables. Doom does not.
	curvelump = Wads.CheckNumForName ("SNDCURVE");
	if (curvelump >= 0)
	{
		S_SoundCurveSize = Wads.LumpLength (curvelump);
		S_SoundCurve = new BYTE[S_SoundCurveSize];
		Wads.ReadLump(curvelump, S_SoundCurve);
	}

	// Free all channels for use.
	while (Channels != NULL)
	{
		S_ReturnChannel(Channels);
	}

	// no sounds are playing, and they are not paused
	MusicPaused = false;
}

//==========================================================================
//
// S_InitData
//
//==========================================================================

void S_InitData ()
{
	LastLocalSndInfo = LastLocalSndSeq = "";
	S_ParseSndInfo ();
	S_ParseSndSeq (-1);
	S_ParseReverbDef ();
}

//==========================================================================
//
// S_Shutdown
//
//==========================================================================

void S_Shutdown ()
{
	FSoundChan *chan, *next;

	while (Channels != NULL)
	{
		GSnd->StopSound(Channels);
	}
	GSnd->UpdateSounds();
	for (chan = FreeChannels; chan != NULL; chan = next)
	{
		next = chan->NextChan;
		delete chan;
	}
	FreeChannels = NULL;

	if (S_SoundCurve != NULL)
	{
		delete[] S_SoundCurve;
		S_SoundCurve = NULL;
	}
	if (PlayList != NULL)
	{
		delete PlayList;
		PlayList = NULL;
	}
}

//==========================================================================
//
// S_Start
//
// Per level startup code. Kills playing sounds at start of level
// and starts new music.
//==========================================================================

void S_Start ()
{
	if (GSnd)
	{
		// kill all playing sounds at start of level (trust me - a good idea)
		S_StopAllChannels();
		
		// Check for local sound definitions. Only reload if they differ
		// from the previous ones.
		const char *LocalSndInfo;
		const char *LocalSndSeq;
		
		// To be certain better check whether level is valid!
		if (level.info && level.info->soundinfo)
		{
			LocalSndInfo = level.info->soundinfo;
		}
		else
		{
			LocalSndInfo = "";
		}

		if (level.info && level.info->sndseq)
		{
			LocalSndSeq  = level.info->sndseq;
		}
		else
		{
			LocalSndSeq  = "";
		}

		bool parse_ss = false;

		// This level uses a different local SNDINFO
		if (LastLocalSndInfo.CompareNoCase(LocalSndInfo) != 0 || !level.info)
		{
			// First delete the old sound list
			for(unsigned i = 1; i < S_sfx.Size(); i++) 
			{
				GSnd->UnloadSound(&S_sfx[i]);
			}
			
			// Parse the global SNDINFO
			S_ParseSndInfo();
		
			if (*LocalSndInfo)
			{
				// Now parse the local SNDINFO
				int j = Wads.CheckNumForFullName(LocalSndInfo, true);
				if (j>=0) S_AddLocalSndInfo(j);
			}

			// Also reload the SNDSEQ if the SNDINFO was replaced!
			parse_ss = true;
		}
		else if (LastLocalSndSeq.CompareNoCase(LocalSndSeq) != 0)
		{
			parse_ss = true;
		}
		if (parse_ss)
		{
			S_ParseSndSeq(*LocalSndSeq? Wads.CheckNumForFullName(LocalSndSeq, true) : -1);
		}
		else
		
		LastLocalSndInfo = LocalSndInfo;
		LastLocalSndSeq = LocalSndSeq;
	}

	// stop the old music if it has been paused.
	// This ensures that the new music is started from the beginning
	// if it's the same as the last one and it has been paused.
	if (MusicPaused) S_StopMusic(true);

	// start new music for the level
	MusicPaused = false;

	// [RH] This is a lot simpler now.
	if (!savegamerestore)
	{
		if (level.cdtrack == 0 || !S_ChangeCDMusic (level.cdtrack, level.cdid))
			S_ChangeMusic (level.music, level.musicorder);
	}
}

//==========================================================================
//
// S_PrecacheLevel
//
// Like R_PrecacheLevel, but for sounds.
//
//==========================================================================

void S_PrecacheLevel ()
{
	unsigned int i;

	if (GSnd)
	{
		for (i = 0; i < S_sfx.Size(); ++i)
		{
			S_sfx[i].bUsed = false;
		}

		AActor *actor;
		TThinkerIterator<AActor> iterator;

		while ( (actor = iterator.Next ()) != NULL )
		{
			S_sfx[actor->SeeSound].bUsed = true;
			S_sfx[actor->AttackSound].bUsed = true;
			S_sfx[actor->PainSound].bUsed = true;
			S_sfx[actor->DeathSound].bUsed = true;
			S_sfx[actor->ActiveSound].bUsed = true;
			S_sfx[actor->UseSound].bUsed = true;
		}

		for (i = 1; i < S_sfx.Size(); ++i)
		{
			if (S_sfx[i].bUsed)
			{
				S_CacheSound (&S_sfx[i]);
			}
		}
		for (i = 1; i < S_sfx.Size(); ++i)
		{
			if (!S_sfx[i].bUsed && S_sfx[i].link == sfxinfo_t::NO_LINK)
			{
				GSnd->UnloadSound (&S_sfx[i]);
			}
		}
	}
}

//==========================================================================
//
// S_CacheSound
//
//==========================================================================

void S_CacheSound (sfxinfo_t *sfx)
{
	if (GSnd)
	{
		if (sfx->bPlayerReserve)
		{
			return;
		}
		else if (sfx->bRandomHeader)
		{
			S_CacheRandomSound (sfx);
		}
		else
		{
			while (sfx->link != sfxinfo_t::NO_LINK)
			{
				sfx = &S_sfx[sfx->link];
			}
			sfx->bUsed = true;
			GSnd->LoadSound (sfx);
		}
	}
}

//==========================================================================
//
// S_GetChannel
//
// Returns a free channel for the system sound interface.
//
//==========================================================================

FSoundChan *S_GetChannel(void *syschan)
{
	FSoundChan *chan;

	if (FreeChannels != NULL)
	{
		chan = FreeChannels;
		S_UnlinkChannel(chan);
	}
	else
	{
		chan = new FSoundChan;
		memset(chan, 0, sizeof(*chan));
	}
	S_LinkChannel(chan, &Channels);
	chan->SysChannel = syschan;
	return chan;
}

//==========================================================================
//
// S_ReturnChannel
//
// Returns a channel to the free pool.
//
//==========================================================================

void S_ReturnChannel(FSoundChan *chan)
{
	if (chan->SourceType == SOURCE_Actor && chan->Actor != NULL)
	{
		chan->Actor->SoundChans &= ~(1 << chan->EntChannel);
	}
	S_UnlinkChannel(chan);
	memset(chan, 0, sizeof(*chan));
	S_LinkChannel(chan, &FreeChannels);
}

//==========================================================================
//
// S_UnlinkChannel
//
//==========================================================================

void S_UnlinkChannel(FSoundChan *chan)
{
	*(chan->PrevChan) = chan->NextChan;
	if (chan->NextChan != NULL)
	{
		chan->NextChan->PrevChan = chan->PrevChan;
	}
}

//==========================================================================
//
// S_LinkChannel
//
//==========================================================================

void S_LinkChannel(FSoundChan *chan, FSoundChan **head)
{
	chan->NextChan = *head;
	if (chan->NextChan != NULL)
	{
		chan->NextChan->PrevChan = &chan->NextChan;
	}
	*head = chan;
	chan->PrevChan = head;
}

// [RH] Split S_StartSoundAtVolume into multiple parts so that sounds can
//		be specified both by id and by name. Also borrowed some stuff from
//		Hexen and parameters from Quake.

//==========================================================================
//
// CalcPosVel
//
// Retrieves a sound's position and velocity for 3D sounds. This version
// is for an already playing sound.
//
//=========================================================================

static void CalcPosVel(const FSoundChan *chan, FVector3 *pos, FVector3 *vel)
{
	CalcPosVel(chan->SourceType, chan->Actor, chan->Sector, chan->Poly, chan->Point,
		chan->EntChannel, chan->ChanFlags, pos, vel);
}

//=========================================================================
//
// CalcPosVel
//
// This version is for sounds that haven't started yet so have no channel.
//
//=========================================================================

static void CalcPosVel(int type, const AActor *actor, const sector_t *sector,
	const FPolyObj *poly, const float pt[3], int channum, int chanflags, FVector3 *pos, FVector3 *vel)
{
	if (pos != NULL)
	{
		fixed_t x, y, z;

		if (players[consoleplayer].camera != NULL)
		{
			x = players[consoleplayer].camera->x;
			y = players[consoleplayer].camera->z;
			z = players[consoleplayer].camera->y;
		}
		else
		{
			z = y = x = 0;
		}

		switch (type)
		{
		case SOURCE_None:
		default:
			break;

		case SOURCE_Actor:
			if (actor != NULL)
			{
				x = actor->x;
				y = actor->z;
				z = actor->y;
			}
			break;

		case SOURCE_Sector:
			if (chanflags & CHAN_AREA)
			{
				CalcSectorSoundOrg(sector, channum, &x, &z, &y);
			}
			else
			{
				x = sector->soundorg[0];
				z = sector->soundorg[1];
				chanflags |= CHAN_LISTENERZ;
			}
			break;

		case SOURCE_Polyobj:
			CalcPolyobjSoundOrg(poly, &x, &z, &y);
			break;

		case SOURCE_Unattached:
			pos->X = pt[0];
			pos->Y = !(chanflags & CHAN_LISTENERZ) ? pt[1] : FIXED2FLOAT(y);
			pos->Z = pt[2];
			break;
		}
		if (type != SOURCE_Unattached)
		{
			if ((chanflags & CHAN_LISTENERZ) && players[consoleplayer].camera != NULL)
			{
				y = players[consoleplayer].camera != NULL ? players[consoleplayer].camera->z : 0;
			}
			pos->X = FIXED2FLOAT(x);
			pos->Y = FIXED2FLOAT(y);
			pos->Z = FIXED2FLOAT(z);
		}
	}
	if (vel != NULL)
	{
		// Only actors maintain velocity information.
		if (type == SOURCE_Actor)
		{
			if (actor != NULL)
			{
				vel->X = FIXED2FLOAT(actor->momx) * TICRATE;
				vel->Y = FIXED2FLOAT(actor->momz) * TICRATE;
				vel->Z = FIXED2FLOAT(actor->momy) * TICRATE;
			}
		}
		else
		{
			vel->Zero();
		}
	}
}

//==========================================================================
//
// CalcSectorSoundOrg
//
// Returns the perceived sound origin for a sector. If the listener is
// inside the sector, then the origin is their location. Otherwise, the
// origin is from the nearest wall on the sector.
//
//==========================================================================

static void CalcSectorSoundOrg(const sector_t *sec, int channum, fixed_t *x, fixed_t *y, fixed_t *z)
{
	if (!(i_compatflags & COMPATF_SECTORSOUNDS))
	{
		// Are we inside the sector? If yes, the closest point is the one we're on.
		if (P_PointInSector(*x, *y) == sec)
		{
			*x = players[consoleplayer].camera->x;
			*y = players[consoleplayer].camera->y;
		}
		else
		{
			// Find the closest point on the sector's boundary lines and use
			// that as the perceived origin of the sound.
			sec->ClosestPoint(*x, *y, *x, *y);
		}
	}
	else
	{
		*x = sec->soundorg[0];
		*y = sec->soundorg[1];
	}

	// Set sound vertical position based on channel.
	if (channum == CHAN_FLOOR)
	{
		*z = MIN(sec->floorplane.ZatPoint(*x, *y), *z);
	}
	else if (channum == CHAN_CEILING)
	{
		*z = MAX(sec->ceilingplane.ZatPoint(*x, *y), *z);
	}
	else if (channum == CHAN_INTERIOR)
	{
		*z = clamp(*z, sec->floorplane.ZatPoint(*x, *y), sec->ceilingplane.ZatPoint(*x, *y));
	}
}

//==========================================================================
//
// CalcPolySoundOrg
//
// Returns the perceived sound origin for a polyobject. This is similar to
// CalcSectorSoundOrg, except there is no special case for being "inside"
// a polyobject, so the sound literally comes from the polyobject's walls.
// Vertical position of the sound always comes from the visible wall.
//
//==========================================================================

static void CalcPolyobjSoundOrg(const FPolyObj *poly, fixed_t *x, fixed_t *y, fixed_t *z)
{
	seg_t *seg;
	sector_t *sec;

	PO_ClosestPoint(poly, *x, *y, *x, *y, &seg);
	sec = seg->frontsector;
	*z = clamp(*z, sec->floorplane.ZatPoint(*x, *y), sec->ceilingplane.ZatPoint(*x, *y));
}

//==========================================================================
//
// S_StartSound
//
// 0 attenuation means full volume over whole level.
// 0 < attenuation means to scale the distance by that amount when
//		calculating volume.
//
//==========================================================================

static FSoundChan *S_StartSound(AActor *actor, const sector_t *sec, const FPolyObj *poly,
	const FVector3 *pt, int channel, FSoundID sound_id, float volume, float attenuation)
{
	sfxinfo_t *sfx;
	int chanflags;
	int basepriority;
	int org_id;
	int pitch;
	FSoundChan *chan;
	FVector3 pos, vel;

	if (sound_id <= 0 || volume <= 0)
		return NULL;

	int type;

	if (actor != NULL)
	{
		type = SOURCE_Actor;
	}
	else if (sec != NULL)
	{
		type = SOURCE_Sector;
	}
	else if (poly != NULL)
	{
		type = SOURCE_Polyobj;
	}
	else if (pt != NULL)
	{
		type = SOURCE_Unattached;
	}
	else
	{
		type = SOURCE_None;
	}

	org_id = sound_id;
	chanflags = channel & ~7;
	channel &= 7;

	CalcPosVel(type, actor, sec, poly, &pt->X, channel, chanflags, &pos, &vel);

	if (i_compatflags & COMPATF_MAGICSILENCE)
	{ // For people who just can't play without a silent BFG.
		channel = CHAN_WEAPON;
	}
	else if ((chanflags & CHAN_MAYBE_LOCAL) && (i_compatflags & COMPATF_SILENTPICKUP))
	{
		if (actor != NULL && actor != players[consoleplayer].camera)
		{
			return NULL;
		}
	}

	sfx = &S_sfx[sound_id];

	// Scale volume according to SNDINFO data.
	volume = MIN(volume * sfx->Volume, 1.f);
	if (volume <= 0)
		return NULL;

	// When resolving a link we do not want to get the NearLimit of
	// the referenced sound so some additional checks are required
	int near_limit = sfx->NearLimit;

	// Resolve player sounds, random sounds, and aliases
	while (sfx->link != sfxinfo_t::NO_LINK)
	{
		if (sfx->bPlayerReserve)
		{
			sound_id = FSoundID(S_FindSkinnedSound (actor, sound_id));
			near_limit = S_sfx[sound_id].NearLimit;
		}
		else if (sfx->bRandomHeader)
		{
			sound_id = FSoundID(S_PickReplacement (sound_id));
			if (near_limit < 0) near_limit = S_sfx[sound_id].NearLimit;
		}
		else
		{
			sound_id = FSoundID(sfx->link);
			if (near_limit < 0) near_limit = S_sfx[sound_id].NearLimit;
		}
		sfx = &S_sfx[sound_id];
	}

	// If this is a singular sound, don't play it if it's already playing.
	if (sfx->bSingular && S_CheckSingular(sound_id))
	{
		chanflags |= CHAN_EVICTED;
	}

	// If the sound is unpositioned or comes from the listener, it is
	// never limited.
	if (type == SOURCE_None || actor == players[consoleplayer].camera)
	{
		near_limit = 0;
	}

	// If this sound doesn't like playing near itself, don't play it if
	// that's what would happen.
	if (near_limit > 0 && S_CheckSoundLimit(sfx, pos, near_limit))
	{
		chanflags |= CHAN_EVICTED;
	}

	// If the sound is blocked and not looped, return now. If the sound
	// is blocked and looped, pretend to play it so that it can
	// eventually play for real.
	if ((chanflags & (CHAN_EVICTED | CHAN_LOOP)) == CHAN_EVICTED)
	{
		return NULL;
	}

	// Make sure the sound is loaded.
	sfx = S_LoadSound(sfx);

	// The empty sound never plays.
	if (sfx->lumpnum == sfx_empty)
	{
		return NULL;
	}

	// Select priority.
	if (type == SOURCE_None || actor == players[consoleplayer].camera)
	{
		basepriority = 40;
	}
	else
	{
		basepriority = 0;
	}

	if (actor != NULL && channel == CHAN_AUTO)
	{ // Select a channel that isn't already playing something.
		BYTE mask = actor->SoundChans;

		// Try channel 0 first, then travel from channel 7 down.
		if ((mask & 1) == 0)
		{
			channel = 0;
		}
		else
		{
			for (channel = 7; channel > 0; --channel, mask <<= 1)
			{
				if ((mask & 0x80) == 0)
				{
					break;
				}
			}
			if (channel == 0)
			{ // Crap. No free channels.
				return NULL;
			}
		}
	}

	// If this actor is already playing something on the selected channel, stop it.
	if (type != SOURCE_None && ((actor == NULL && channel != CHAN_AUTO) || (actor != NULL && actor->SoundChans & (1 << channel))))
	{
		for (chan = Channels; chan != NULL; chan = chan->NextChan)
		{
			if (chan->SourceType == type && chan->EntChannel == channel)
			{
				bool foundit;

				switch (type)
				{
				case SOURCE_Actor:		foundit = (chan->Actor == actor);	break;
				case SOURCE_Sector:		foundit = (chan->Sector == sec);	break;
				case SOURCE_Polyobj:	foundit = (chan->Poly == poly);		break;
				case SOURCE_Unattached:	foundit = (chan->Point[0] == pt->X && chan->Point[2] == pt->Z && chan->Point[1] == pt->Y);		break;
				default:				foundit = false;					break;
				}
				if (foundit)
				{
					GSnd->StopSound(chan);
					break;
				}
			}
		}
	}

	// Vary the sfx pitches.
	if (sfx->PitchMask != 0)
	{
		pitch = NORM_PITCH - (M_Random() & sfx->PitchMask) + (M_Random() & sfx->PitchMask);
	}
	else
	{
		pitch = NORM_PITCH;
	}

	if (chanflags & CHAN_EVICTED)
	{
		chan = NULL;
	}
	else if (attenuation > 0)
	{
		chan = GSnd->StartSound3D (sfx, volume, attenuation, pitch, basepriority, pos, vel, sec, channel, chanflags, NULL);
	}
	else
	{
		chan = GSnd->StartSound (sfx, volume, pitch, chanflags, NULL);
	}
	if (chan == NULL && (chanflags & CHAN_LOOP))
	{
		chan = S_GetChannel(NULL);
		chanflags |= CHAN_EVICTED;
	}
	if (attenuation > 0)
	{
		chanflags |= CHAN_IS3D | CHAN_JUSTSTARTED;
	}
	else
	{
		chanflags |= CHAN_LISTENERZ | CHAN_JUSTSTARTED;
	}
	if (chan != NULL)
	{
		chan->SoundID = sound_id;
		chan->OrgID = FSoundID(org_id);
		chan->SfxInfo = sfx;
		chan->EntChannel = channel;
		chan->Volume = volume;
		chan->ChanFlags |= chanflags;
		chan->NearLimit = near_limit;
		chan->Pitch = pitch;
		chan->Priority = basepriority;
		chan->DistanceScale = attenuation;
		chan->SourceType = type;
		switch (type)
		{
		case SOURCE_Actor:		chan->Actor = actor;	actor->SoundChans |= 1 << channel;	break;
		case SOURCE_Sector:		chan->Sector = sec;		break;
		case SOURCE_Polyobj:	chan->Poly = poly;		break;
		case SOURCE_Unattached:	chan->Point[0] = pt->X; chan->Point[1] = pt->Y; chan->Point[2] = pt->Z;	break;
		default:										break;
		}
	}
	return chan;
}

//==========================================================================
//
// S_RestartSound
//
// Attempts to restart looping sounds that were evicted from their channels.
//
//==========================================================================

void S_RestartSound(FSoundChan *chan)
{
	assert(chan->ChanFlags & CHAN_EVICTED);
	assert(chan->SfxInfo != NULL);

	FSoundChan *ochan;
	sfxinfo_t *sfx = chan->SfxInfo;

	// If this is a singular sound, don't play it if it's already playing.
	if (sfx->bSingular && S_CheckSingular(chan->SoundID))
		return;

	sfx = S_LoadSound(sfx);

	// The empty sound never plays.
	if (sfx->lumpnum == sfx_empty)
	{
		return;
	}

	if (chan->ChanFlags & CHAN_IS3D)
	{
		FVector3 pos, vel;

		CalcPosVel(chan, &pos, &vel);

		// If this sound doesn't like playing near itself, don't play it if
		// that's what would happen.
		if (chan->NearLimit > 0 && S_CheckSoundLimit(&S_sfx[chan->SoundID], pos, chan->NearLimit))
		{
			return;
		}

		ochan = GSnd->StartSound3D(sfx, chan->Volume, chan->DistanceScale, chan->Pitch,
			chan->Priority, pos, vel, chan->Sector, chan->EntChannel, chan->ChanFlags, chan);
	}
	else
	{
		ochan = GSnd->StartSound(chan->SfxInfo, chan->Volume, chan->Pitch, chan->ChanFlags, chan);
	}
	assert(ochan == NULL || ochan == chan);
	if (ochan != NULL)
	{
		ochan->ChanFlags &= ~CHAN_EVICTED;
		// When called from the savegame loader, the actor's SoundChans
		// flags will be cleared. During normal gameplay, they should still
		// be set.
		if (ochan->SourceType == SOURCE_Actor)
		{
			if (ochan->Actor != NULL) ochan->Actor->SoundChans |= 1 << ochan->EntChannel;
		}
	}
}

//==========================================================================
//
// S_Sound - Unpositioned version
//
//==========================================================================

void S_Sound (int channel, FSoundID sound_id, float volume, float attenuation)
{
	S_StartSound (NULL, NULL, NULL, NULL, channel, sound_id, volume, attenuation);
}

//==========================================================================
//
// S_Sound - An actor is source
//
//==========================================================================

void S_Sound (AActor *ent, int channel, FSoundID sound_id, float volume, float attenuation)
{
	if (ent == NULL || ent->Sector->Flags & SECF_SILENT)
		return;
	S_StartSound (ent, NULL, NULL, NULL, channel, sound_id, volume, attenuation);
}

//==========================================================================
//
// S_Sound - A polyobject is source
//
//==========================================================================

void S_Sound (const FPolyObj *poly, int channel, FSoundID sound_id, float volume, float attenuation)
{
	S_StartSound (NULL, NULL, poly, NULL, channel, sound_id, volume, attenuation);
}

//==========================================================================
//
// S_Sound - A point is source
//
//==========================================================================

void S_Sound (fixed_t x, fixed_t y, fixed_t z, int channel, FSoundID sound_id, float volume, float attenuation)
{
	FVector3 pt(FIXED2FLOAT(x), FIXED2FLOAT(z), FIXED2FLOAT(y));
	S_StartSound (NULL, NULL, NULL, &pt, channel, sound_id, volume, attenuation);
}

//==========================================================================
//
// S_Sound - An entire sector is source
//
//==========================================================================

void S_Sound (const sector_t *sec, int channel, FSoundID sfxid, float volume, float attenuation)
{
	S_StartSound (NULL, sec, NULL, NULL, channel, sfxid, volume, attenuation);
}

//==========================================================================
//
// S_LoadSound
//
// Returns a pointer to the sfxinfo with the actual sound data.
//
//==========================================================================

sfxinfo_t *S_LoadSound(sfxinfo_t *sfx)
{
	if (sfx->data == NULL)
	{
		GSnd->LoadSound (sfx);
		if (sfx->link != sfxinfo_t::NO_LINK)
		{
			sfx = &S_sfx[sfx->link];
		}
	}
	return sfx;
}

//==========================================================================
//
// S_CheckSingular
//
// Returns true if a copy of this sound is already playing.
//
//==========================================================================

bool S_CheckSingular(int sound_id)
{
	for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
	{
		if (chan->OrgID == sound_id)
		{
			return true;
		}
	}
	return false;
}

//==========================================================================
//
// S_CheckSoundLimit
//
// Limits the number of nearby copies of a sound that can play near
// each other. If there are NearLimit instances of this sound already
// playing within 256 units of the new sound, the new sound will not
// start.
//
//==========================================================================

bool S_CheckSoundLimit(sfxinfo_t *sfx, const FVector3 &pos, int near_limit)
{
	FSoundChan *chan;
	int count;
	
	for (chan = Channels, count = 0; chan != NULL && count < near_limit; chan = chan->NextChan)
	{
		if (!(chan->ChanFlags & CHAN_EVICTED) && chan->SfxInfo == sfx)
		{
			FVector3 chanorigin;

			CalcPosVel(chan, &chanorigin, NULL);
			if ((chanorigin - pos).LengthSquared() <= 256.0*256.0)
			{
				count++;
			}
		}
	}
	return count >= near_limit;
}

//==========================================================================
//
// S_StopSound
//
// Stops an unpositioned sound from playing on a specific channel.
//
//==========================================================================

void S_StopSound (int channel)
{
	for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
	{
		if (chan->SourceType == SOURCE_None &&
			(chan->EntChannel == channel || (i_compatflags & COMPATF_MAGICSILENCE)))
		{
			GSnd->StopSound(chan);
		}
	}
}

//==========================================================================
//
// S_StopSound
//
// Stops a sound from a single actor from playing on a specific channel.
//
//==========================================================================

void S_StopSound (AActor *actor, int channel)
{
	// No need to search every channel if we know it's not playing anything.
	if (actor != NULL && actor->SoundChans & (1 << channel))
	{
		for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
		{
			if (chan->SourceType == SOURCE_Actor &&
				chan->Actor == actor &&
				(chan->EntChannel == channel || (i_compatflags & COMPATF_MAGICSILENCE)))
			{
				GSnd->StopSound(chan);
			}
		}
	}
}

//==========================================================================
//
// S_StopSound
//
// Stops a sound from a single sector from playing on a specific channel.
//
//==========================================================================

void S_StopSound (const sector_t *sec, int channel)
{
	for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
	{
		if (chan->SourceType == SOURCE_Sector &&
			chan->Sector == sec &&
			(chan->EntChannel == channel || (i_compatflags & COMPATF_MAGICSILENCE)))
		{
			GSnd->StopSound(chan);
		}
	}
}

//==========================================================================
//
// S_StopSound
//
// Stops a sound from a single polyobject from playing on a specific channel.
//
//==========================================================================

void S_StopSound (const FPolyObj *poly, int channel)
{
	for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
	{
		if (chan->SourceType == SOURCE_Polyobj &&
			chan->Poly == poly &&
			(chan->EntChannel == channel || (i_compatflags & COMPATF_MAGICSILENCE)))
		{
			GSnd->StopSound(chan);
		}
	}
}

//==========================================================================
//
// S_StopAllChannels
//
//==========================================================================

void S_StopAllChannels ()
{
	SN_StopAllSequences();
	while (Channels != NULL)
	{
		GSnd->StopSound(Channels);
	}
	GSnd->UpdateSounds();
}

//==========================================================================
//
// S_RelinkSound
//
// Moves all the sounds from one thing to another. If the destination is
// NULL, then the sound becomes a positioned sound.
//==========================================================================

void S_RelinkSound (AActor *from, AActor *to)
{
	if (from == NULL)
		return;

	for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
	{
		if (chan->SourceType == SOURCE_Actor && chan->Actor == from)
		{
			if (to != NULL)
			{
				chan->Actor = to;
			}
			else if (!(chan->ChanFlags & CHAN_LOOP))
			{
				chan->SourceType = SOURCE_Unattached;
				chan->Point[0] = FIXED2FLOAT(from->x);
				chan->Point[1] = FIXED2FLOAT(from->z);
				chan->Point[2] = FIXED2FLOAT(from->y);
			}
			else
			{
				GSnd->StopSound(chan);
			}
		}
	}
}

//==========================================================================
//
// S_GetSoundPlayingInfo
//
// Is a sound being played by a specific emitter?
//==========================================================================

bool S_GetSoundPlayingInfo (const AActor *actor, int sound_id)
{
	if (sound_id > 0)
	{
		for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
		{
			if (chan->OrgID == sound_id &&
				chan->SourceType == SOURCE_Actor &&
				chan->Actor == actor)
			{
				return true;
			}
		}
	}
	return false;
}

bool S_GetSoundPlayingInfo (const sector_t *sec, int sound_id)
{
	if (sound_id > 0)
	{
		for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
		{
			if (chan->OrgID == sound_id &&
				chan->SourceType == SOURCE_Sector &&
				chan->Sector == sec)
			{
				return true;
			}
		}
	}
	return false;
}

bool S_GetSoundPlayingInfo (const FPolyObj *poly, int sound_id)
{
	if (sound_id > 0)
	{
		for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
		{
			if (chan->OrgID == sound_id &&
				chan->SourceType == SOURCE_Polyobj &&
				chan->Poly == poly)
			{
				return true;
			}
		}
	}
	return false;
}

//==========================================================================
//
// S_IsActorPlayingSomething
//
//==========================================================================

bool S_IsActorPlayingSomething (AActor *actor, int channel, int sound_id)
{
	if (actor->SoundChans == 0)
	{
		return false;
	}

	if (i_compatflags & COMPATF_MAGICSILENCE)
	{
		channel = 0;
	}

	for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
	{
		if (chan->SourceType == SOURCE_Actor && chan->Actor == actor)
		{
			if (channel == 0 || chan->EntChannel == channel)
			{
				return sound_id <= 0 || chan->OrgID == sound_id;
			}
		}
	}
	return false;
}

//==========================================================================
//
// S_PauseSound
//
// Stop music and sound effects, during game PAUSE.
//==========================================================================

void S_PauseSound (bool notmusic)
{
	if (!notmusic && mus_playing.handle && !MusicPaused)
	{
		I_PauseSong (mus_playing.handle);
		MusicPaused = true;
	}
	GSnd->SetSfxPaused (true, 0);
}

//==========================================================================
//
// S_ResumeSound
//
// Resume music and sound effects, after game PAUSE.
//==========================================================================

void S_ResumeSound ()
{
	if (mus_playing.handle && MusicPaused)
	{
		I_ResumeSong (mus_playing.handle);
		MusicPaused = false;
	}
	GSnd->SetSfxPaused (false, 0);
}

//==========================================================================
//
// S_EvictAllChannels
//
// Forcibly evicts all channels so that there are none playing, but all
// information needed to restart them is retained.
//
//==========================================================================

void S_EvictAllChannels()
{
	FSoundChan *chan, *next;

	for (chan = Channels; chan != NULL; chan = next)
	{
		next = chan->NextChan;

		if (!(chan->ChanFlags & CHAN_EVICTED))
		{
			chan->ChanFlags |= CHAN_EVICTED;
			if (chan->SysChannel != NULL)
			{
				GSnd->StopSound(chan);
			}
			assert(chan->NextChan == next);
		}
	}
}

//==========================================================================
//
// S_RestoreEvictedChannel
//
// Recursive helper for S_RestoreEvictedChannels().
//
//==========================================================================

void S_RestoreEvictedChannel(FSoundChan *chan)
{
	if (chan == NULL)
	{
		return;
	}
	S_RestoreEvictedChannel(chan->NextChan);
	if (chan->ChanFlags & CHAN_EVICTED)
	{
		S_RestartSound(chan);
		if (!(chan->ChanFlags & CHAN_LOOP))
		{
			if (chan->ChanFlags & CHAN_EVICTED)
			{ // Still evicted and not looping? Forget about it.
				S_ReturnChannel(chan);
			}
			else if (!(chan->ChanFlags & CHAN_JUSTSTARTED))
			{ // Should this sound become evicted again, it's okay to forget about it.
				chan->ChanFlags |= CHAN_FORGETTABLE;
			}
		}
	}
}

//==========================================================================
//
// S_RestoreEvictedChannels
//
// Restarts as many evicted channels as possible. Any channels that could
// not be started and are not looping are moved to the free pool.
//
//==========================================================================

void S_RestoreEvictedChannels()
{
	// Restart channels in the same order they were originally played.
	S_RestoreEvictedChannel(Channels);
}

//==========================================================================
//
// S_UpdateSounds
//
// Updates music & sounds
//==========================================================================

void S_UpdateSounds (void *listener_p)
{
	FVector3 pos, vel;

	I_UpdateMusic();

	// [RH] Update music and/or playlist. I_QrySongPlaying() must be called
	// to attempt to reconnect to broken net streams and to advance the
	// playlist when the current song finishes.
	if (mus_playing.handle != NULL &&
		!I_QrySongPlaying(mus_playing.handle) &&
		PlayList)
	{
		PlayList->Advance();
		S_ActivatePlayList(false);
	}

	for (FSoundChan *chan = Channels; chan != NULL; chan = chan->NextChan)
	{
		if ((chan->ChanFlags & (CHAN_EVICTED | CHAN_IS3D)) == CHAN_IS3D)
		{
			CalcPosVel(chan, &pos, &vel);
			GSnd->UpdateSoundParams3D(chan, pos, vel);
		}
		chan->ChanFlags &= ~CHAN_JUSTSTARTED;
	}

	SN_UpdateActiveSequences();

	GSnd->UpdateListener();
	GSnd->UpdateSounds();

	if (level.time >= RestartEvictionsAt)
	{
		RestartEvictionsAt = 0;
		S_RestoreEvictedChannels();
	}
}

//==========================================================================
//
// (FArchive &) << (FSoundID &)
//
//==========================================================================

FArchive &operator<<(FArchive &arc, FSoundID &sid)
{
	if (arc.IsStoring())
	{
		arc.WriteName((const char *)sid);
	}
	else
	{
		sid = arc.ReadName();
	}
	return arc;
}

//==========================================================================
//
// (FArchive &) << (FSoundChan &)
//
//==========================================================================

static FArchive &operator<<(FArchive &arc, FSoundChan &chan)
{
	arc << chan.SourceType;
	switch (chan.SourceType)
	{
	case SOURCE_None:								break;
	case SOURCE_Actor:		arc << chan.Actor;		break;
	case SOURCE_Sector:		arc << chan.Sector;		break;
	case SOURCE_Polyobj:	arc << chan.Poly;		break;
	case SOURCE_Unattached:	arc << chan.Point[0] << chan.Point[1] << chan.Point[2];	break;
	default:				I_Error("Unknown sound source type %d\n", chan.SourceType);	break;
	}
	arc << chan.SoundID;
	arc << chan.OrgID;
	arc << chan.Volume;
	arc << chan.DistanceScale;
	arc << chan.Pitch;
	arc << chan.ChanFlags;
	arc << chan.EntChannel;
	arc << chan.Priority;
	arc << chan.NearLimit;
	arc << chan.StartTime;

	if (arc.IsLoading())
	{
		chan.SfxInfo = &S_sfx[chan.SoundID];
	}
	return arc;
}

//==========================================================================
//
// S_SerializeSounds
//
//==========================================================================

void S_SerializeSounds(FArchive &arc)
{
	FSoundChan *chan;

	GSnd->Sync(true);

	if (arc.IsStoring())
	{
		TArray<FSoundChan *> chans;

		// Count channels and accumulate them so we can store them in
		// reverse order. That way, they will be in the same order when
		// reloaded later as they are now.
		for (chan = Channels; chan != NULL; chan = chan->NextChan)
		{
			// If the sound is forgettable, this is as good a time as
			// any to forget about it. And if it's a UI sound, it shouldn't
			// be stored in the savegame.
			if (!(chan->ChanFlags & (CHAN_FORGETTABLE | CHAN_UI)))
			{
				chans.Push(chan);
			}
		}

		arc.WriteCount(chans.Size());

		for (unsigned int i = chans.Size(); i-- != 0; )
		{
			// Replace start time with sample position.
			QWORD start = chans[i]->StartTime.AsOne;
			chans[i]->StartTime.AsOne = GSnd ? GSnd->GetPosition(chans[i]) : 0;
			arc << *chans[i];
			chans[i]->StartTime.AsOne = start;
		}
	}
	else
	{
		unsigned int count;

		S_StopAllChannels();
		count = arc.ReadCount();
		for (unsigned int i = 0; i < count; ++i)
		{
			chan = S_GetChannel(NULL);
			arc << *chan;
			// Sounds always start out evicted when restored from a save.
			chan->ChanFlags |= CHAN_EVICTED | CHAN_ABSTIME;
		}
		// The two tic delay is to make sure any screenwipes have finished.
		// This needs to be two because the game is run for one tic before
		// the wipe so that it can produce a screen to wipe to. So if we
		// only waited one tic to restart the sounds, they would start
		// playing before the wipe, and depending on the synchronization
		// between the main thread and the mixer thread at the time, the
		// sounds might be heard briefly before pausing for the wipe.
		RestartEvictionsAt = level.time + 2;
	}
	DSeqNode::SerializeSequences(arc);
	GSnd->Sync(false);
	GSnd->UpdateSounds();
}

//==========================================================================
//
// S_ActivatePlayList
//
// Plays the next song in the playlist. If no songs in the playlist can be
// played, then it is deleted.
//==========================================================================

void S_ActivatePlayList (bool goBack)
{
	int startpos, pos;

	startpos = pos = PlayList->GetPosition ();
	S_StopMusic (true);
	while (!S_ChangeMusic (PlayList->GetSong (pos), 0, false, true))
	{
		pos = goBack ? PlayList->Backup () : PlayList->Advance ();
		if (pos == startpos)
		{
			delete PlayList;
			PlayList = NULL;
			Printf ("Cannot play anything in the playlist.\n");
			return;
		}
	}
}

//==========================================================================
//
// S_ChangeCDMusic
//
// Starts a CD track as music.
//==========================================================================

bool S_ChangeCDMusic (int track, unsigned int id, bool looping)
{
	char temp[32];

	if (id != 0)
	{
		mysnprintf (temp, countof(temp), ",CD,%d,%x", track, id);
	}
	else
	{
		mysnprintf (temp, countof(temp), ",CD,%d", track);
	}
	return S_ChangeMusic (temp, 0, looping);
}

//==========================================================================
//
// S_StartMusic
//
// Starts some music with the given name.
//==========================================================================

bool S_StartMusic (const char *m_id)
{
	return S_ChangeMusic (m_id, 0, false);
}

//==========================================================================
//
// S_ChangeMusic
//
// Starts playing a music, possibly looping.
//
// [RH] If music is a MOD, starts it at position order. If name is of the
// format ",CD,<track>,[cd id]" song is a CD track, and if [cd id] is
// specified, it will only be played if the specified CD is in a drive.
//==========================================================================

TArray<char> musiccache;

bool S_ChangeMusic (const char *musicname, int order, bool looping, bool force)
{
	if (!force && PlayList)
	{ // Don't change if a playlist is active
		return false;
	}

	// allow specifying "*" as a placeholder to play the level's default music.
	if (musicname != NULL && !strcmp(musicname, "*"))
	{
		if (gamestate == GS_LEVEL || gamestate == GS_TITLELEVEL)
		{
			musicname = level.music;
			order = level.musicorder;
		}
		else
		{
			musicname = NULL;
		}
	}

	if (musicname == NULL || musicname[0] == 0)
	{
		// Don't choke if the map doesn't have a song attached
		S_StopMusic (true);
		return false;
	}
	
	if (!mus_playing.name.IsEmpty() && stricmp (mus_playing.name, musicname) == 0)
	{
		if (order != mus_playing.baseorder)
		{
			if (mus_playing.handle->SetSubsong(order))
			{
				mus_playing.baseorder = order;
			}
		}
		return true;
	}

	if (strnicmp (musicname, ",CD,", 4) == 0)
	{
		int track = strtoul (musicname+4, NULL, 0);
		const char *more = strchr (musicname+4, ',');
		unsigned int id = 0;

		if (more != NULL)
		{
			id = strtoul (more+1, NULL, 16);
		}
		S_StopMusic (true);
		mus_playing.handle = I_RegisterCDSong (track, id);
	}
	else
	{
		int lumpnum = -1;
		int offset = 0, length = 0;
		int device = MDEV_DEFAULT;
		MusInfo *handle = NULL;

		int *devp = MidiDevices.CheckKey(FName(musicname));
		if (devp != NULL) device = *devp;

		// Strip off any leading file:// component.
		if (strncmp(musicname, "file://", 7) == 0)
		{
			musicname += 7;
		}

		if (!FileExists (musicname))
		{
			if ((lumpnum = Wads.CheckNumForFullName (musicname, true, ns_music)) == -1)
			{
				if (strstr(musicname, "://") > musicname)
				{
					// Looks like a URL; try it as such.
					handle = I_RegisterURLSong(musicname);
					if (handle == NULL)
					{
						Printf ("Could not open \"%s\"\n", musicname);
						return false;
					}
				}
				else
				{
					Printf ("Music \"%s\" not found\n", musicname);
					return false;
				}
			}
			if (handle == NULL)
			{
				if (!Wads.IsUncompressedFile(lumpnum))
				{
					// We must cache the music data and use it from memory.

					// shut down old music before reallocating and overwriting the cache!
					S_StopMusic (true);

					offset = -1;							// this tells the low level code that the music 
															// is being used from memory
					length = Wads.LumpLength (lumpnum);
					if (length == 0)
					{
						return false;
					}
					musiccache.Resize(length);
					Wads.ReadLump(lumpnum, &musiccache[0]);
				}
				else
				{
					offset = Wads.GetLumpOffset (lumpnum);
					length = Wads.LumpLength (lumpnum);
					if (length == 0)
					{
						return false;
					}
				}
			}
		}

		// shutdown old music
		S_StopMusic (true);

		// Just record it if volume is 0
		if (snd_musicvolume <= 0)
		{
			mus_playing.loop = looping;
			mus_playing.name = "";
			mus_playing.baseorder = order;
			LastSong = musicname;
			return true;
		}

		// load & register it
		if (handle != NULL)
		{
			mus_playing.handle = handle;
		}
		else if (offset != -1)
		{
			mus_playing.handle = I_RegisterSong (lumpnum != -1 ?
				Wads.GetWadFullName (Wads.GetLumpFile (lumpnum)) :
				musicname, NULL, offset, length, device);
		}
		else
		{
			mus_playing.handle = I_RegisterSong (NULL, &musiccache[0], -1, length, device);
		}
	}

	mus_playing.loop = looping;
	mus_playing.name = musicname;
	mus_playing.baseorder = 0;
	LastSong = "";

	if (mus_playing.handle != 0)
	{ // play it
		I_PlaySong (mus_playing.handle, looping, S_GetMusicVolume (musicname), order);
		mus_playing.baseorder = order;
		return true;
	}
	return false;
}

//==========================================================================
//
// S_RestartMusic
//
// Must only be called from snd_reset in i_sound.cpp!
//==========================================================================

void S_RestartMusic ()
{
	if (!LastSong.IsEmpty())
	{
		FString song = LastSong;
		LastSong = "";
		S_ChangeMusic (song, mus_playing.baseorder, mus_playing.loop, true);
	}
}

//==========================================================================
//
// S_GetMusic
//
//==========================================================================

int S_GetMusic (char **name)
{
	int order;

	if (mus_playing.name)
	{
		*name = copystring (mus_playing.name);
		order = mus_playing.baseorder;
	}
	else
	{
		*name = NULL;
		order = 0;
	}
	return order;
}

//==========================================================================
//
// S_StopMusic
//
//==========================================================================

void S_StopMusic (bool force)
{
	// [RH] Don't stop if a playlist is active.
	if ((force || PlayList == NULL) && !mus_playing.name.IsEmpty())
	{
		if (MusicPaused)
			I_ResumeSong(mus_playing.handle);

		I_StopSong(mus_playing.handle);
		I_UnRegisterSong(mus_playing.handle);

		LastSong = mus_playing.name;
		mus_playing.name = "";
		mus_playing.handle = 0;
	}
}

//==========================================================================
//
// CCMD playsound
//
//==========================================================================

CCMD (playsound)
{
	if (argv.argc() > 1)
	{
		S_Sound (CHAN_AUTO | CHAN_UI, argv[1], 1.f, ATTN_NONE);
	}
}

//==========================================================================
//
// CCMD idmus
//
//==========================================================================

CCMD (idmus)
{
	level_info_t *info;
	FString map;
	int l;

	if (argv.argc() > 1)
	{
		if (gameinfo.flags & GI_MAPxx)
		{
			l = atoi (argv[1]);
			if (l <= 99)
			{
				map = CalcMapName (0, l);
			}
			else
			{
				Printf ("%s\n", GStrings("STSTR_NOMUS"));
				return;
			}
		}
		else
		{
			map = CalcMapName (argv[1][0] - '0', argv[1][1] - '0');
		}

		if ( (info = FindLevelInfo (map)) )
		{
			if (info->music)
			{
				S_ChangeMusic (info->music, info->musicorder);
				Printf ("%s\n", GStrings("STSTR_MUS"));
			}
		}
		else
		{
			Printf ("%s\n", GStrings("STSTR_NOMUS"));
		}
	}
}

//==========================================================================
//
// CCMD changemus
//
//==========================================================================

CCMD (changemus)
{
	if (argv.argc() > 1)
	{
		if (PlayList)
		{
			delete PlayList;
			PlayList = NULL;
		}
		S_ChangeMusic (argv[1], argv.argc() > 2 ? atoi (argv[2]) : 0);
	}
}

//==========================================================================
//
// CCMD stopmus
//
//==========================================================================

CCMD (stopmus)
{
	if (PlayList)
	{
		delete PlayList;
		PlayList = NULL;
	}
	S_StopMusic (false);
}

//==========================================================================
//
// CCMD cd_play
//
// Plays a specified track, or the entire CD if no track is specified.
//==========================================================================

CCMD (cd_play)
{
	char musname[16];

	if (argv.argc() == 1)
	{
		strcpy (musname, ",CD,");
	}
	else
	{
		mysnprintf (musname, countof(musname), ",CD,%d", atoi(argv[1]));
	}
	S_ChangeMusic (musname, 0, true);
}

//==========================================================================
//
// CCMD cd_stop
//
//==========================================================================

CCMD (cd_stop)
{
	CD_Stop ();
}

//==========================================================================
//
// CCMD cd_eject
//
//==========================================================================

CCMD (cd_eject)
{
	CD_Eject ();
}

//==========================================================================
//
// CCMD cd_close
//
//==========================================================================

CCMD (cd_close)
{
	CD_UnEject ();
}

//==========================================================================
//
// CCMD cd_pause
//
//==========================================================================

CCMD (cd_pause)
{
	CD_Pause ();
}

//==========================================================================
//
// CCMD cd_resume
//
//==========================================================================

CCMD (cd_resume)
{
	CD_Resume ();
}

//==========================================================================
//
// CCMD playlist
//
//==========================================================================

CCMD (playlist)
{
	int argc = argv.argc();

	if (argc < 2 || argc > 3)
	{
		Printf ("playlist <playlist.m3u> [<position>|shuffle]\n");
	}
	else
	{
		if (PlayList != NULL)
		{
			PlayList->ChangeList (argv[1]);
		}
		else
		{
			PlayList = new FPlayList (argv[1]);
		}
		if (PlayList->GetNumSongs () == 0)
		{
			delete PlayList;
			PlayList = NULL;
		}
		else
		{
			if (argc == 3)
			{
				if (stricmp (argv[2], "shuffle") == 0)
				{
					PlayList->Shuffle ();
				}
				else
				{
					PlayList->SetPosition (atoi (argv[2]));
				}
			}
			S_ActivatePlayList (false);
		}
	}
}

//==========================================================================
//
// CCMD playlistpos
//
//==========================================================================

static bool CheckForPlaylist ()
{
	if (PlayList == NULL)
	{
		Printf ("No playlist is playing.\n");
		return false;
	}
	return true;
}

CCMD (playlistpos)
{
	if (CheckForPlaylist() && argv.argc() > 1)
	{
		PlayList->SetPosition (atoi (argv[1]) - 1);
		S_ActivatePlayList (false);
	}
}

//==========================================================================
//
// CCMD playlistnext
//
//==========================================================================

CCMD (playlistnext)
{
	if (CheckForPlaylist())
	{
		PlayList->Advance ();
		S_ActivatePlayList (false);
	}
}

//==========================================================================
//
// CCMD playlistprev
//
//==========================================================================

CCMD (playlistprev)
{
	if (CheckForPlaylist())
	{
		PlayList->Backup ();
		S_ActivatePlayList (true);
	}
}

//==========================================================================
//
// CCMD playliststatus
//
//==========================================================================

CCMD (playliststatus)
{
	if (CheckForPlaylist ())
	{
		Printf ("Song %d of %d:\n%s\n",
			PlayList->GetPosition () + 1,
			PlayList->GetNumSongs (),
			PlayList->GetSong (PlayList->GetPosition ()));
	}
}

//==========================================================================
//
// CCMD cachesound <sound name>
//
//==========================================================================

CCMD (cachesound)
{
	if (argv.argc() < 2)
	{
		Printf ("Usage: cachesound <sound> ...\n");
		return;
	}
	for (int i = 1; i < argv.argc(); ++i)
	{
		FSoundID sfxnum = argv[i];
		if (sfxnum != 0)
		{
			S_CacheSound (&S_sfx[sfxnum]);
		}
	}
}