/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* Global interdependent objects */

#include <C4Include.h>
#include "C4Application.h"
#include "C4Aul.h"
#include "C4Console.h"
#include <C4DefList.h>
#include "C4FullScreen.h"
#include "C4GraphicsSystem.h"
#include "C4Game.h"
#include "C4GameObjects.h"
#include "C4MouseControl.h"
#include "C4Network2.h"
#include "C4PropList.h"
#include "C4StringTable.h"

#ifdef _DEBUG
C4Set<C4PropList *> C4PropList::PropLists;
#endif
C4Set<C4PropListNumbered *> C4PropListNumbered::PropLists;
C4Set<C4PropListScript *> C4PropListScript::PropLists;
std::vector<C4PropListNumbered *> C4PropListNumbered::ShelvedPropLists;
int32_t C4PropListNumbered::EnumerationIndex = 0;
C4StringTable  Strings;
C4AulScriptEngine ScriptEngine;
C4Application  Application;
C4Console      Console;
C4FullScreen   FullScreen;
C4MouseControl MouseControl;
C4GameObjects  Objects;
C4DefList      Definitions;
// make sure C4Game reference members are initialized before Game because otherwise they're acccessed in C4Game::Default before initialization
C4GraphicsSystem GraphicsSystem;
C4Game         Game;
C4Network2     Network;

