/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* Play midis using mci */

#include <Standard.h>

#ifdef HAVE_MIDI_H
#include <mmsystem.h>
#include <midi.h>
#include <stdio.h>

BOOL PlayMidi(const char *sFileName, HWND appWnd)
	{
  char buf[256];
  sprintf(buf, "open \"%s\" type sequencer alias ITSMYMUSIC", sFileName);
	if (mciSendString("close all", NULL, 0, NULL) != 0)
    return FALSE;
  if (mciSendString(buf, NULL, 0, NULL) != 0)
    return FALSE;
  if (mciSendString("play ITSMYMUSIC from 0 notify", NULL, 0, appWnd) != 0)
    return FALSE;
  return TRUE;
	}

BOOL PauseMidi()
	{
	if (mciSendString("stop ITSMYMUSIC", NULL, 0, NULL) != 0) return FALSE;
	return TRUE;
	}

BOOL ResumeMidi(HWND appWnd)
	{
	if (mciSendString("play ITSMYMUSIC notify", NULL, 0, appWnd) != 0) return FALSE;
	return TRUE;
	}

BOOL StopMidi()
	{
	if (mciSendString("close all", NULL, 0, NULL) != 0) return FALSE;
	return TRUE;
	}

BOOL ReplayMidi(HWND appWnd)
	{
	if (mciSendString("play ITSMYMUSIC from 0 notify", NULL, 0, appWnd) != 0) return FALSE;
	return TRUE;
	}
#endif //HAVE_MIDI_H
