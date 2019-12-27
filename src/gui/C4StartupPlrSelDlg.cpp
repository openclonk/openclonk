/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
// Startup screen for non-parameterized engine start: Player selection dialog
// Also contains player creation, editing and crew management

#include "C4Include.h"
#include "gui/C4StartupPlrSelDlg.h"

#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4FileSelDlg.h"
#include "gui/C4MouseControl.h"
#include "gui/C4StartupMainDlg.h"
#include "lib/C4Random.h"
#include "lib/StdColors.h"
#include "player/C4RankSystem.h"

// font clrs
const uint32_t ClrPlayerItem   = 0xffffffff;

// Arbitrary cut-off value for player color value. This avoids pitch black
// colors which look ugly. Note that this limit is only applied in the UI,
// it's still possible to edit the Player.txt by hand.
const uint32_t PlayerColorValueLowBound = 64;

// ----- C4Utilities

StdStrBuf TimeString(int iSeconds)
{
	int iHours = iSeconds / 3600; iSeconds -= 3600*iHours;
	int iMinutes = iSeconds / 60; iSeconds -= 60*iMinutes;
	return FormatString("%02d:%02d:%02d",iHours,iMinutes,iSeconds);
}

StdStrBuf DateString(int iTime)
{
	if (!iTime) return StdStrBuf("", true);
	time_t tTime = iTime;
	struct tm *pLocalTime;
	pLocalTime=localtime(&tTime);
	return FormatString(  "%02d.%02d.%d %02d:%02d",
	                      pLocalTime->tm_mday,
	                      pLocalTime->tm_mon+1,
	                      pLocalTime->tm_year+1900,
	                      pLocalTime->tm_hour,
	                      pLocalTime->tm_min);
}

// ------------------------------------------------
// --- C4StartupPlrSelDlg::ListItem
C4StartupPlrSelDlg::ListItem::ListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBeforeElement, bool fActivated)
		: Control(C4Rect(0,0,0,0)), pCheck(nullptr), pNameLabel(nullptr), pPlrSelDlg(pForDlg), pIcon(nullptr)
{
	CStdFont &rUseFont = GraphicsResource.FontRegular;
	// calc height
	int32_t iHeight = rUseFont.GetLineHeight() + 2 * IconLabelSpacing;
	// create subcomponents
	pCheck = new C4GUI::CheckBox(C4Rect(0, 0, iHeight, iHeight), nullptr, fActivated);
	pCheck->SetOnChecked(new C4GUI::CallbackHandler<C4StartupPlrSelDlg>(pForDlg, &C4StartupPlrSelDlg::OnItemCheckChange));
	pKeyCheck = new C4KeyBinding(C4KeyCodeEx(K_SPACE), "StartupPlrSelTogglePlayerActive", KEYSCOPE_Gui,
	                             new C4GUI::ControlKeyCB<ListItem>(*this, &ListItem::KeyCheck), C4CustomKey::PRIO_Ctrl);
	pIcon = new C4GUI::Icon(C4Rect(iHeight + IconLabelSpacing, 0, iHeight, iHeight), C4GUI::Ico_Player);
	pNameLabel = new C4GUI::Label("Q", (iHeight + IconLabelSpacing)*2, IconLabelSpacing, ALeft, ClrPlayerItem, &rUseFont, false, false);
	pNameLabel->SetAutosize(false);
	// calc own bounds - use icon bounds only, because only the height is used when the item is added
	SetBounds(pIcon->GetBounds());
	// add components
	AddElement(pCheck);
	AddElement(pIcon); AddElement(pNameLabel);
	// add to listbox (will get resized horizontally and moved) - zero indent; no tree structure in this dialog
	pForListBox->InsertElement(this, pInsertBeforeElement, 0);
	// update name label width to stretch max listbox width
	C4Rect rcNameLabelBounds = pNameLabel->GetBounds();
	rcNameLabelBounds.Wdt = GetClientRect().Wdt - rcNameLabelBounds.x - IconLabelSpacing;
	pNameLabel->SetBounds(rcNameLabelBounds);
	// context menu
	SetContextHandler(new C4GUI::CBContextHandler<C4StartupPlrSelDlg::ListItem>(this, &C4StartupPlrSelDlg::ListItem::ContextMenu));
}

C4StartupPlrSelDlg::ListItem::~ListItem()
{
	delete pKeyCheck;
}

const char *C4StartupPlrSelDlg::ListItem::GetName() const
{
	// name is stored in label only
	return pNameLabel->GetText();
}

void C4StartupPlrSelDlg::ListItem::SetName(const char *szNewName)
{
	// update name in label
	pNameLabel->SetText(szNewName);
	// tooltip by name, so long names can be read via tooltip
	SetToolTip(szNewName);
}

void C4StartupPlrSelDlg::ListItem::GrabIcon(C4FacetSurface &rFromFacet)
{
	// take over icon gfx from facet - deletes them from source facet!
	if (rFromFacet.Surface)
	{
		pIcon->GetMFacet().GrabFrom(rFromFacet);
	}
	else
	{
		// reset custom icon
		// following update-call will reset to default icon
		pIcon->GetMFacet().Clear();
	}
}

void C4StartupPlrSelDlg::ListItem::SetIcon(C4GUI::Icons icoNew)
{
	pIcon->SetIcon(icoNew);
}

void C4StartupPlrSelDlg::ListItem::UpdateOwnPos()
{
	// parent for client rect
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateOwnPos();
	// reposition items
	C4GUI::ComponentAligner caBounds(GetContainedClientRect(), IconLabelSpacing, IconLabelSpacing);
	// nothing to reposition for now...
}

void C4StartupPlrSelDlg::ListItem::SetFilename(const StdStrBuf &sNewFN)
{
	// just set fn - UpdateCore-call will follow later
	Filename.Copy(sNewFN);
}

bool C4StartupPlrSelDlg::ListItem::CheckNameHotkey(const char * c)
{
	// return whether this item can be selected by entering given char:
	// first char of name must match
	// FIXME: Unicode
	if (!pNameLabel) return false;
	const char *szName = pNameLabel->GetText();
	return szName && (toupper(*szName) == toupper(c[0]));
}


// ------------------------------------------------
// --- C4StartupPlrSelDlg::PlayerListItem
C4StartupPlrSelDlg::PlayerListItem::PlayerListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBeforeElement, bool fActivated)
		: ListItem(pForDlg, pForListBox, pInsertBeforeElement, fActivated), fHasCustomIcon(false)
{
}

void C4StartupPlrSelDlg::PlayerListItem::Load(const StdStrBuf &rsFilename)
{
	int32_t iHeight = GetBounds().Hgt;
	// backup filename
	SetFilename(rsFilename);
	// load player info
	C4Group PlrGroup;
	if (!PlrGroup.Open(Config.AtUserDataPath(rsFilename.getData())))
		throw LoadError(FormatString("Error loading player file from %s: Error opening group: %s", rsFilename.getData(), PlrGroup.GetError()));
	if (!Core.Load(PlrGroup))
		throw LoadError(FormatString("Error loading player file from %s: Core data invalid or missing (Group: %s)!", rsFilename.getData(), PlrGroup.GetError()));
	// load icon
	C4FacetSurface fctIcon;
	if (PlrGroup.FindEntry(C4CFN_BigIcon) && fctIcon.Load(PlrGroup, C4CFN_BigIcon, C4FCT_Full, C4FCT_Full, false, 0))
		fHasCustomIcon = true;
	else
	{
		// no custom icon: create default by player color
		fctIcon.Create(iHeight,iHeight);
		::GraphicsResource.fctPlayerClr.DrawClr(fctIcon, true, Core.PrefColorDw);
	}
	GrabIcon(fctIcon);
	// done loading
	if (!PlrGroup.Close())
		throw LoadError(FormatString("Error loading player file from %s: Error closing group: %s", rsFilename.getData(), PlrGroup.GetError()));
	// default name
	if (!*Core.PrefName) SCopy(GetFilenameOnly(rsFilename.getData()), Core.PrefName, sizeof(Core.PrefName)-1);
	SetName(Core.PrefName);
}

C4GUI::ContextMenu *C4StartupPlrSelDlg::PlayerListItem::ContextMenu()
{
	// menu operations work on selected item only
	pPlrSelDlg->SetSelection(this);
	// context menu operations
	C4GUI::ContextMenu *pCtx = new C4GUI::ContextMenu();
	pCtx->AddItem(LoadResStr("IDS_BTN_PROPERTIES"), LoadResStr("IDS_DLGTIP_PLAYERPROPERTIES"), C4GUI::Ico_None, new C4GUI::CBMenuHandler<C4StartupPlrSelDlg>(pPlrSelDlg, &C4StartupPlrSelDlg::OnPropertyCtx));
	pCtx->AddItem(LoadResStr("IDS_BTN_DELETE"), LoadResStr("IDS_DLGTIP_PLAYERDELETE"), C4GUI::Ico_None, new C4GUI::CBMenuHandler<C4StartupPlrSelDlg>(pPlrSelDlg, &C4StartupPlrSelDlg::OnDelCtx));
	return pCtx;
}

void C4StartupPlrSelDlg::PlayerListItem::GrabCustomIcon(C4FacetSurface &fctGrabFrom)
{
	// set flag
	fHasCustomIcon = !!fctGrabFrom.Surface;
	// base class grab
	GrabIcon(fctGrabFrom);
}

void C4StartupPlrSelDlg::PlayerListItem::UpdateCore(C4PlayerInfoCore & NewCore)
{
	C4Group PlrGroup;
	if (!PlrGroup.Open(Config.AtUserDataPath(GetFilename().getData()))
	    || !NewCore.Save(PlrGroup)
	    || !PlrGroup.Close())
	{
		GetScreen()->ShowMessage(LoadResStr("IDS_FAIL_MODIFY"), "", C4GUI::Ico_Error);
		return;
	}
	Core = NewCore;
	SetName(Core.PrefName);
	// re-set non-custom icons
	if (!fHasCustomIcon)
	{
		fHasCustomIcon = false;
		int32_t iHeight = GetBounds().Hgt;
		C4FacetSurface fctIcon; fctIcon.Create(iHeight,iHeight);
		::GraphicsResource.fctPlayerClr.DrawClr(fctIcon, true, Core.PrefColorDw);
		GrabIcon(fctIcon);
	}
	// update in selection
	C4StartupPlrSelDlg *pDlg = static_cast<C4StartupPlrSelDlg *>(GetDlg());
	if (pDlg && pDlg->GetSelection() == this) pDlg->UpdateSelection();
}

void C4StartupPlrSelDlg::PlayerListItem::SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo)
{
	// write info text for player
	pSelectionInfo->ClearText(false);
	pSelectionInfo->AddTextLine(FormatString("%s", Core.PrefName).getData(), &C4Startup::Get()->Graphics.BookFontCapt, ClrPlayerItem, false, false);
	pSelectionInfo->AddTextLine(FormatString(LoadResStr("IDS_DESC_PLAYER"), (int)Core.TotalScore, (int)Core.Rounds, (int)Core.RoundsWon, (int)Core.RoundsLost, TimeString(Core.TotalPlayingTime).getData(), Core.Comment).getData(), &C4Startup::Get()->Graphics.BookFont, ClrPlayerItem, false, false);
	if (Core.LastRound.Title[0])
		pSelectionInfo->AddTextLine(FormatString(LoadResStr("IDS_DESC_LASTGAME"),Core.LastRound.Title.getData(),DateString(Core.LastRound.Date).getData(),TimeString(Core.LastRound.Duration).getData(),(int)Core.LastRound.FinalScore).getData(), &C4Startup::Get()->Graphics.BookFont, ClrPlayerItem, false, false);
	pSelectionInfo->UpdateHeight();
}

StdStrBuf C4StartupPlrSelDlg::PlayerListItem::GetDelWarning()
{
	StdStrBuf sWarning;
	sWarning.Format(LoadResStr("IDS_MSG_DELETEPLR"), Core.PrefName);
	int32_t iPlrTime = Core.TotalPlayingTime;
	if (iPlrTime > 60*60*10)
		sWarning.Append(FormatString(LoadResStr("IDS_MSG_DELETEPLR_PLAYTIME"),
		                             TimeString(iPlrTime).getData()).getData());
	return sWarning;
}

bool C4StartupPlrSelDlg::PlayerListItem::MoveFilename(const char *szToFilename)
{
	// anything to do?
	if (ItemIdentical(GetFilename().getData(), szToFilename)) return true;
	// do it
	StdStrBuf PathFrom(Config.General.UserDataPath);
	PathFrom.Append(GetFilename());
	StdStrBuf PathTo(Config.General.UserDataPath);
	PathTo.Append(szToFilename);
	if (!MoveItem(PathFrom.getData(), PathTo.getData())) return false;
	// reflect change in class
	SetFilename(StdStrBuf(szToFilename));
	return true;
}


// ------------------------------------------------
// --- C4StartupPlrSelDlg::CrewListItem

C4StartupPlrSelDlg::CrewListItem::CrewListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, uint32_t dwPlrClr)
		: ListItem(pForDlg, pForListBox, nullptr, false), fLoaded(false), dwPlrClr(dwPlrClr), pParentGrp(nullptr)
{
	SetIcon(C4GUI::Ico_Wait);
}

void C4StartupPlrSelDlg::CrewListItem::UpdateClonkEnabled()
{
	if (!fLoaded) return;
	Core.Participation = pCheck->GetChecked();
	// immediate save of changes
	RewriteCore();
}

void C4StartupPlrSelDlg::CrewListItem::Load(C4Group &rGrp, const StdStrBuf &rsFilename)
{
	// backup filename (doesn't include path)
	SetFilename(rsFilename);
	// load core
	C4Group CrewGroup;
	if (!CrewGroup.OpenAsChild(&rGrp, rsFilename.getData()))
		throw LoadError(FormatString("Error loading crew file from %s in %s: Error opening group: %s",
		                             rsFilename.getData(), rGrp.GetFullName().getData(), CrewGroup.GetError()));
	if (!Core.Load(CrewGroup))
		throw LoadError(FormatString("Error loading crew file from %s: Core data invalid or missing (Group: %s)!",
		                             CrewGroup.GetFullName().getData(), CrewGroup.GetError()));
	ListItem::SetName(Core.Name);
	pCheck->SetChecked(!!Core.Participation);
	// load rank as icon
	C4FacetSurface fctIcon;
	if (fctIcon.Load(CrewGroup, C4CFN_ClonkRank, C4FCT_Full, C4FCT_Full, false, true))
	{
		GrabIcon(fctIcon);
	}
	else
	{
		// no custom icon: create default by rank system
		if (C4RankSystem::DrawRankSymbol(&fctIcon, Core.Rank, &::GraphicsResource.fctRank, ::GraphicsResource.iNumRanks, true))
			GrabIcon(fctIcon);
	}
	// backup group loaded from - assumes it stays valid!
	pParentGrp = &rGrp;
	// load success!
	fLoaded=true;
}

C4GUI::ContextMenu *C4StartupPlrSelDlg::CrewListItem::ContextMenu()
{
	// menu operations work on selected item only
	pPlrSelDlg->SetSelection(this);
	// context menu operations
	C4GUI::ContextMenu *pCtx = new C4GUI::ContextMenu();
	pCtx->AddItem(LoadResStr("IDS_BTN_RENAME"), LoadResStr("IDS_DESC_CREWRENAME"), C4GUI::Ico_None, new C4GUI::CBMenuHandler<C4StartupPlrSelDlg>(pPlrSelDlg, &C4StartupPlrSelDlg::OnPropertyCtx));
	pCtx->AddItem(LoadResStr("IDS_BTN_DELETE"), LoadResStr("IDS_MSG_DELETECLONK_DESC"), C4GUI::Ico_None, new C4GUI::CBMenuHandler<C4StartupPlrSelDlg>(pPlrSelDlg, &C4StartupPlrSelDlg::OnDelCtx));
	pCtx->AddItem(LoadResStr("IDS_MSG_SETDEATHMESSAGE"), LoadResStr("IDS_MSG_SETTHEMESSAGETHATAPPEARWH"), C4GUI::Ico_None, new C4GUI::CBMenuHandler<C4StartupPlrSelDlg::CrewListItem>(this, &C4StartupPlrSelDlg::CrewListItem::OnDeathMessageCtx));
	return pCtx;
}

void C4StartupPlrSelDlg::CrewListItem::OnDeathMessageCtx(C4GUI::Element *el)
{
	// Death message dialog
	C4GUI::InputDialog *pDlg;
	GetScreen()->ShowRemoveDlg(pDlg=new C4GUI::InputDialog(LoadResStr("IDS_MSG_ENTERNEWDEATHMESSAGE"), LoadResStr("IDS_MSG_SETDEATHMESSAGE"), C4GUI::Ico_Ex_Comment, new C4GUI::InputCallback<C4StartupPlrSelDlg::CrewListItem>(this, &C4StartupPlrSelDlg::CrewListItem::OnDeathMessageSet), false));
	pDlg->SetMaxText(C4MaxDeathMsg);
	pDlg->SetInputText(Core.DeathMessage);
}

void C4StartupPlrSelDlg::CrewListItem::OnDeathMessageSet(const StdStrBuf &rsNewMessage)
{
	// copy msg
	if (!rsNewMessage) *Core.DeathMessage='\0'; else SCopy(rsNewMessage.getData(), Core.DeathMessage, C4MaxDeathMsg);
	// save
	RewriteCore();
	// acoustic feedback
	C4GUI::GUISound("UI::Confirmed");
}

void C4StartupPlrSelDlg::CrewListItem::RewriteCore()
{
	if (!fLoaded) return;
	C4Group CrewGroup;
	if (!CrewGroup.OpenAsChild(pParentGrp, GetFilename().getData())
	    || !Core.Save(CrewGroup, nullptr)
	    || !CrewGroup.Close() || !pParentGrp->Save(true))
	{
		GetScreen()->ShowMessage(LoadResStr("IDS_FAIL_MODIFY"), "", C4GUI::Ico_Error);
		return;
	}
}

bool C4StartupPlrSelDlg::CrewListItem::SetName(const char *szNewName)
{
	if (!fLoaded) return false;
	// validate name
	if (!szNewName || !*szNewName) return false;
	if (SEqual(szNewName, Core.Name)) return true;
	// generate filename from new name
	char fn[_MAX_PATH_LEN];
	SCopy(szNewName, fn, _MAX_PATH);
	MakeFilenameFromTitle(fn);
	if (!*fn) return false;
	SAppend(".oci", fn, _MAX_PATH);
	// check if a rename is due
	if (!ItemIdentical(fn, GetFilename().getData()))
	{
		// check for duplicate filename
		if (pParentGrp->FindEntry(fn))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_CLONKCOLLISION"), fn);
			::pGUI->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_FAIL_RENAME"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		// OK; then rename
		if (!pParentGrp->Rename(GetFilename().getData(), fn) || !pParentGrp->Save(true))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_RENAMEFILE"), GetFilename().getData(), fn);
			::pGUI->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_FAIL_RENAME"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		const char *szConstFn = fn;
		SetFilename(StdStrBuf(szConstFn));
	}
	// update clonk name and core
	ListItem::SetName(szNewName);
	SCopy(szNewName, Core.Name, C4MaxName);
	RewriteCore();
	return true;
}

void C4StartupPlrSelDlg::CrewListItem::SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo)
{
	// write info text for player
	pSelectionInfo->ClearText(false);
	pSelectionInfo->AddTextLine(FormatString("%s %s", Core.sRankName.getData(), Core.Name).getData(), &C4Startup::Get()->Graphics.BookFontCapt, ClrPlayerItem, false, false);
	StdStrBuf sPromo;
	int32_t iNextRankExp; StdStrBuf sNextRankName;
	if (Core.GetNextRankInfo(::DefaultRanks, &iNextRankExp, &sNextRankName))
		sPromo.Format(LoadResStr("IDS_DESC_PROMO"),sNextRankName.getData(),(int) iNextRankExp);
	else
		sPromo.Copy(LoadResStr("IDS_DESC_NOPROMO"));
	pSelectionInfo->AddTextLine(FormatString(LoadResStr("IDS_DESC_OBJECT"),
	                            Core.TypeName, Core.Experience, Core.Rounds, Core.DeathCount,
	                            sPromo.getData(), TimeString(Core.TotalPlayingTime).getData(), DateString(Core.Birthday).getData()).getData(),
	                            &C4Startup::Get()->Graphics.BookFont, ClrPlayerItem, false, false);
	pSelectionInfo->UpdateHeight();
}

StdStrBuf C4StartupPlrSelDlg::CrewListItem::GetDelWarning()
{
	StdStrBuf sWarning;
	sWarning.Format(LoadResStr("IDS_MSG_DELETECLONK"),
	                Core.sRankName.getData(), Core.Name, GetFilename().getData());
	int32_t iPlrTime = Core.TotalPlayingTime;
	if (iPlrTime > 60*60*10)
		sWarning.Append(static_cast<const StdStrBuf &>(FormatString(LoadResStr("IDS_MSG_DELETECLONK_PLAYTIME"), TimeString(iPlrTime).getData())));
	return sWarning;
}

void C4StartupPlrSelDlg::CrewListItem::CrewRename()
{
	if (pPlrSelDlg->pRenameEdit) return;
	// rename this entry
	pPlrSelDlg->pRenameEdit = new C4GUI::CallbackRenameEdit<C4StartupPlrSelDlg::CrewListItem, RenameParams>(pNameLabel, this, RenameParams(), &C4StartupPlrSelDlg::CrewListItem::DoRenaming, &C4StartupPlrSelDlg::CrewListItem::AbortRenaming);
}

void C4StartupPlrSelDlg::CrewListItem::AbortRenaming(RenameParams par)
{
	// no renaming
	pPlrSelDlg->pRenameEdit = nullptr;
}

C4GUI::RenameEdit::RenameResult C4StartupPlrSelDlg::CrewListItem::DoRenaming(RenameParams par, const char *szNewName)
{
	// accept if name can be set; will fail if name is invalid or already given to another Crew member
	if (!SetName(szNewName)) return C4GUI::RenameEdit::RR_Invalid;
	pPlrSelDlg->pRenameEdit = nullptr;
	// update in selection
	C4StartupPlrSelDlg *pDlg = static_cast<C4StartupPlrSelDlg *>(GetDlg());
	if (pDlg && pDlg->GetSelection() == this) pDlg->UpdateSelection();
	return C4GUI::RenameEdit::RR_Accepted;
}



// ------------------------------------------------
// --- C4StartupPlrSelDlg

C4StartupPlrSelDlg::C4StartupPlrSelDlg() : C4StartupDlg("W")
{
	// ctor
	UpdateSize(); // for clientrect

	// screen calculations
	int iButtonHeight = C4GUI_ButtonHgt;
	int iButtonXSpacing = (GetClientRect().Wdt > 700) ? GetClientRect().Wdt/58 : 2;
	int iButtonCount = 6;
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,0, true);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(std::max(caMain.GetHeight()/15, iButtonHeight)),0,0);
	rcBottomButtons = caButtonArea.GetCentered(caMain.GetWidth(), iButtonHeight);
	iBottomButtonWidth = (caButtonArea.GetWidth() - iButtonXSpacing * (iButtonCount-1)) / iButtonCount;
	C4Rect rcMain = caMain.GetAll();
	C4Rect rcPlrList = C4Rect(rcMain.Wdt/8, rcMain.Hgt/8, rcMain.Wdt*5/16, rcMain.Hgt*6/8);
	C4Rect rcInfoWindow = C4Rect(rcMain.Wdt*9/16, rcMain.Hgt/8, rcMain.Wdt*5/16, rcMain.Hgt*6/8);

	AddElement(pPlrListBox = new C4GUI::ListBox(rcPlrList));
	pPlrListBox->SetToolTip(LoadResStr("IDS_DLGTIP_PLAYERFILES"));
	pPlrListBox->SetDecoration(true, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	pPlrListBox->UpdateElementPositions();
	pPlrListBox->SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<C4StartupPlrSelDlg>(this, &C4StartupPlrSelDlg::OnSelChange));
	pPlrListBox->SetSelectionDblClickFn(new C4GUI::CallbackHandler<C4StartupPlrSelDlg>(this, &C4StartupPlrSelDlg::OnSelDblClick));
	AddElement(pSelectionInfo = new C4GUI::TextWindow(rcInfoWindow,0,0,0,100,4096,"  ",false,nullptr,0,true));
	pSelectionInfo->SetDecoration(true, true, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	pSelectionInfo->UpdateHeight();

	// bottom line buttons - positioning done in UpdateBottomButtons by UpdatePlayerList
	C4Rect rcDefault(0,0,10,10);
	AddElement(btnBack = new C4GUI::CallbackButton<C4StartupPlrSelDlg>(LoadResStr("IDS_BTN_BACK"), rcDefault, &C4StartupPlrSelDlg::OnBackBtn));
	AddElement(btnNew = new C4GUI::CallbackButton<C4StartupPlrSelDlg>(LoadResStr("IDS_BTN_NEW"), rcDefault, &C4StartupPlrSelDlg::OnNewBtn));
	btnNew->SetToolTip(LoadResStr("IDS_DLGTIP_NEWPLAYER"));
	AddElement(btnActivatePlr = new C4GUI::CallbackButton<C4StartupPlrSelDlg>(nullptr, rcDefault, &C4StartupPlrSelDlg::OnActivateBtn));
	AddElement(btnDelete = new C4GUI::CallbackButton<C4StartupPlrSelDlg>(LoadResStr("IDS_BTN_DELETE"), rcDefault, &C4StartupPlrSelDlg::OnDelBtn));
	AddElement(btnProperties = new C4GUI::CallbackButton<C4StartupPlrSelDlg>(nullptr, rcDefault, &C4StartupPlrSelDlg::OnPropertyBtn));
	AddElement(btnCrew = new C4GUI::CallbackButton<C4StartupPlrSelDlg>(LoadResStr("IDS_SELECT_CREW"), rcDefault, &C4StartupPlrSelDlg::OnCrewBtn));
	btnCrew->SetToolTip(LoadResStr("IDS_DLGTIP_PLAYERCREW"));

	// refill listboxes
	UpdatePlayerList();
	// Just safety incase listbox was empty, in which case no selection change callback will have been done:
	// Update current listbox selection to that of no selected item
	if (!pPlrListBox->GetFirst()) UpdateSelection();

	// initial focus on player list
	SetFocus(pPlrListBox, false);

	// key bindings
	C4CustomKey::CodeList keys;
	keys.emplace_back(K_BACK);
	keys.emplace_back(K_LEFT);
	keys.emplace_back(K_ESCAPE);
	if (Config.Controls.GamepadGuiControl)
	{
		ControllerKeys::Cancel(keys);
	}
	pKeyBack = new C4KeyBinding(keys, "StartupPlrSelBack", KEYSCOPE_Gui,
	                            new C4GUI::DlgKeyCB<C4StartupPlrSelDlg>(*this, &C4StartupPlrSelDlg::KeyBack), C4CustomKey::PRIO_CtrlOverride);
	pKeyProperties = new C4KeyBinding(C4KeyCodeEx(K_F2), "StartupPlrSelProp", KEYSCOPE_Gui,
	                                  new C4GUI::DlgKeyCB<C4StartupPlrSelDlg>(*this, &C4StartupPlrSelDlg::KeyProperties), C4CustomKey::PRIO_CtrlOverride);
	pKeyCrew = new C4KeyBinding(C4KeyCodeEx(K_RIGHT), "StartupPlrSelCrew", KEYSCOPE_Gui,
	                            new C4GUI::ControlKeyDlgCB<C4StartupPlrSelDlg>(pPlrListBox, *this, &C4StartupPlrSelDlg::KeyCrew), C4CustomKey::PRIO_CtrlOverride);
	pKeyDelete = new C4KeyBinding(C4KeyCodeEx(K_DELETE), "StartupPlrSelDelete", KEYSCOPE_Gui,
	                              new C4GUI::DlgKeyCB<C4StartupPlrSelDlg>(*this, &C4StartupPlrSelDlg::KeyDelete), C4CustomKey::PRIO_CtrlOverride);
	pKeyNew = new C4KeyBinding(C4KeyCodeEx(K_INSERT), "StartupPlrSelNew", KEYSCOPE_Gui,
	                           new C4GUI::DlgKeyCB<C4StartupPlrSelDlg>(*this, &C4StartupPlrSelDlg::KeyNew), C4CustomKey::PRIO_CtrlOverride);
}

C4StartupPlrSelDlg::~C4StartupPlrSelDlg()
{
	delete pKeyDelete;
	delete pKeyCrew;
	delete pKeyProperties;
	delete pKeyBack;
	delete pKeyNew;
}

void C4StartupPlrSelDlg::AbortRenaming()
{
	if (pRenameEdit) pRenameEdit->Abort();
}

void C4StartupPlrSelDlg::DrawElement(C4TargetFacet &cgo)
{
	// draw background
	typedef C4GUI::FullscreenDialog Base;
	Base::DrawElement(cgo);
}

void C4StartupPlrSelDlg::UpdateBottomButtons()
{
	// bottom line buttons depend on list mode
	C4GUI::ComponentAligner caBottomButtons(rcBottomButtons,0,0);
	switch (eMode)
	{
	case PSDM_Player:
	{
		// update some buttons for player mode
		btnProperties->SetText(LoadResStr("IDS_BTN_PROPERTIES"));
		btnProperties->SetToolTip(LoadResStr("IDS_DLGTIP_PLAYERPROPERTIES"));
		btnNew->SetVisibility(true);
		btnCrew->SetVisibility(true);
		btnDelete->SetToolTip(LoadResStr("IDS_DLGTIP_PLAYERDELETE"));
		btnBack->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
		btnBack       ->SetBounds(caBottomButtons.GetGridCell(0,6,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnNew        ->SetBounds(caBottomButtons.GetGridCell(1,6,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnActivatePlr->SetBounds(caBottomButtons.GetGridCell(2,6,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnDelete     ->SetBounds(caBottomButtons.GetGridCell(3,6,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnProperties ->SetBounds(caBottomButtons.GetGridCell(4,6,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnCrew       ->SetBounds(caBottomButtons.GetGridCell(5,6,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
	}
	break;

	case PSDM_Crew:
	{
		// update some buttons for player mode
		btnProperties->SetText(LoadResStr("IDS_BTN_RENAME"));
		btnProperties->SetToolTip(LoadResStr("IDS_DESC_CREWRENAME"));
		btnNew->SetVisibility(false);
		btnCrew->SetVisibility(false);
		btnDelete->SetToolTip(LoadResStr("IDS_MSG_DELETECLONK_DESC"));
		btnBack->SetToolTip(LoadResStr("IDS_MSG_BACKTOPLAYERDLG"));
		btnBack       ->SetBounds(caBottomButtons.GetGridCell(0,4,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnActivatePlr->SetBounds(caBottomButtons.GetGridCell(1,4,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnDelete     ->SetBounds(caBottomButtons.GetGridCell(2,4,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
		btnProperties ->SetBounds(caBottomButtons.GetGridCell(3,4,0,1,iBottomButtonWidth,C4GUI_ButtonHgt,true));
	}
	break;
	};
}

void C4StartupPlrSelDlg::UpdatePlayerList()
{
	// something has changed!
	AbortRenaming();
	// refill pPlrListBox with players in player folder or crew
	// clear old items
	C4GUI::Element *pEl;
	while ((pEl = pPlrListBox->GetFirst())) delete pEl;
	// update command buttons
	UpdateBottomButtons();
	// create new
	switch (eMode)
	{
	case PSDM_Player:
	{
		SetTitle(LoadResStrNoAmp("IDS_DLG_PLAYERSELECTION"));
		// player mode: insert all players
		const char *szFn;
		StdStrBuf sSearchPath(Config.General.UserDataPath);
		PlayerListItem *pFirstActivatedPlrItem=nullptr, *pFirstDeactivatedPlrItem=nullptr, *pPlrItem=nullptr;
		for (DirectoryIterator i(sSearchPath.getData()); (szFn=*i); i++)
		{
			szFn = Config.AtRelativePath(szFn);
			if (*GetFilename(szFn) == '.') continue; // ignore ".", ".." and private files (".*")
			if (!WildcardMatch(C4CFN_PlayerFiles, GetFilename(szFn))) continue;
			bool fIsParticipating = !!SIsModule(Config.General.Participants, szFn, nullptr, false);
			pPlrItem = new PlayerListItem(this, pPlrListBox, nullptr, fIsParticipating);
			try
			{
				pPlrItem->Load(StdStrBuf(szFn));
			}
			catch (ListItem::LoadError & e)
			{
				// invalid player: ignore but log error message
				DebugLog(e.getData());
				delete pPlrItem;
				continue;
			}
			if (fIsParticipating)
			{
				if (!pFirstActivatedPlrItem) pFirstActivatedPlrItem = pPlrItem;
			}
			else if (!pFirstDeactivatedPlrItem) pFirstDeactivatedPlrItem = pPlrItem;
		}
		// select first element; prefer activated player
		if (!(pPlrItem = pFirstActivatedPlrItem))
			pPlrItem = pFirstDeactivatedPlrItem;
		if (pPlrItem)
			pPlrListBox->SelectEntry(pPlrItem, false);
		// re-validate Game.PlayerFilename
		UpdateActivatedPlayers();
		break;
	}

	case PSDM_Crew:
	{
		SetTitle(FormatString("%s %s", LoadResStrNoAmp("IDS_CTL_CREW"), CurrPlayer.Core.PrefName).getData());
		// crew mode: Insert complete crew of player (2do: sort)
		bool fSucc; char szFn[_MAX_PATH_LEN];
		for (fSucc=CurrPlayer.Grp.FindEntry(C4CFN_ObjectInfoFiles, szFn); fSucc; fSucc=CurrPlayer.Grp.FindNextEntry(C4CFN_ObjectInfoFiles, szFn, nullptr, true))
		{
			CrewListItem *pCrewItem = new CrewListItem(this, pPlrListBox, CurrPlayer.Core.PrefColorDw);
			try
			{
				pCrewItem->Load(CurrPlayer.Grp, StdStrBuf(szFn));
			}
			catch (ListItem::LoadError & e)
			{
				// invalid player: ignore but log error message
				DebugLog(e.getData());
				delete pCrewItem;
				continue;
			}
		}
		// resort crew by type and experience
		ResortCrew();
		pPlrListBox->SelectFirstEntry(false);
		break;
	}
	}
}

C4StartupPlrSelDlg::ListItem *C4StartupPlrSelDlg::GetSelection()
{
	// get selected item: There's only instances of PlrListItem in this list
	return static_cast<ListItem *>(pPlrListBox->GetSelectedItem());
}

void C4StartupPlrSelDlg::SetSelection(ListItem *pNewItem)
{
	// update selection in listbox
	pPlrListBox->SelectEntry(pNewItem, false);
}

void C4StartupPlrSelDlg::UpdateSelection()
{
	// something has changed!
	AbortRenaming();
	// get currently selected player
	ListItem *pSel = GetSelection();
	// button text 'activate' if current player is deactivated; 'deactivate' otherwise
	if (pSel && pSel->IsActivated())
	{
		btnActivatePlr->SetText(LoadResStr("IDS_BTN_DEACTIVATE"));
		btnActivatePlr->SetToolTip(FormatString(LoadResStr("IDS_MSG_NOPARTICIPATE_DESC"), pSel->GetName()).getData());
	}
	else
	{
		btnActivatePlr->SetText(LoadResStr("IDS_BTN_ACTIVATE"));
		btnActivatePlr->SetToolTip(FormatString(LoadResStr("IDS_MSG_PARTICIPATE_DESC"), pSel ? pSel->GetName() : "").getData());
	}
	// no item selected?
	if (!pSel)
	{
		pSelectionInfo->ClearText(true);
		// 2do: disable buttons
		return;
	}
	// info text for selection
	pSel->SetSelectionInfo(pSelectionInfo);
}

void C4StartupPlrSelDlg::OnItemCheckChange(C4GUI::Element *pCheckBox)
{
	switch (eMode)
	{
	case PSDM_Player:
		// Deselect all other players
		for (ListItem* pEl = static_cast<ListItem*>(pPlrListBox->GetFirst()); pEl != nullptr; pEl = pEl->GetNext())
			if (pCheckBox && pEl != pCheckBox->GetParent())
				pEl->SetActivated(false);
		// update Config.General.Participants
		UpdateActivatedPlayers();
		break;
	case PSDM_Crew:
		// update affected crew item
		if (pCheckBox) static_cast<CrewListItem *>(pCheckBox->GetParent())->UpdateClonkEnabled();
		break;
	}
	// update player selection text
	UpdateSelection();
}

void C4StartupPlrSelDlg::UpdateActivatedPlayers()
{
	assert(eMode == PSDM_Player);
	// refill Config.General.Participants-list
	*Config.General.Participants = '\0';
	for (ListItem *pPlrItem = static_cast<ListItem *>(pPlrListBox->GetFirst()); pPlrItem; pPlrItem = pPlrItem->GetNext())
		if (pPlrItem->IsActivated())
		{
			const char *szAddFilename = pPlrItem->GetFilename().getData();
			if (std::strlen(Config.General.Participants) + 1 + std::strlen(szAddFilename) < sizeof(Config.General.Participants))
				SAddModule(Config.General.Participants, szAddFilename);
			else
			{
				pPlrItem->SetActivated(false);
				GetScreen()->ShowMessage(FormatString(LoadResStr("IDS_ERR_PLAYERSTOOLONG"), pPlrItem->GetName()).getData(), LoadResStr("IDS_ERR_TITLE"), C4GUI::Ico_Error);
			}
		}
}

void C4StartupPlrSelDlg::OnActivateBtn(C4GUI::Control *btn)
{
	// toggle activation state of current item
	// get currently selected player
	ListItem *pSel = GetSelection();
	if (!pSel) return;
	pSel->SetActivated(!pSel->IsActivated());
	// update stuff
	OnItemCheckChange(pSel->GetCheckBox());
}

void C4StartupPlrSelDlg::DoBack()
{
	switch (eMode)
	{
	case PSDM_Player:
	{
		// back 2 main
		C4Startup::Get()->SwitchDialog(C4Startup::SDID_Back);
		break;
	}

	case PSDM_Crew:
		// back 2 player list
		SetPlayerMode();
		break;
	}
}

void C4StartupPlrSelDlg::OnNewBtn(C4GUI::Control *btn)
{
	if (eMode != PSDM_Player) return;
	C4GUI::Dialog *pDlg;
	GetScreen()->ShowRemoveDlg(pDlg=new C4StartupPlrPropertiesDlg(nullptr, this));
	pDlg->SetPos(std::min<int32_t>(GetBounds().Wdt/10, GetBounds().Wdt - pDlg->GetBounds().Wdt), std::min<int32_t>(GetBounds().Hgt/4, GetBounds().Hgt - pDlg->GetBounds().Hgt));
}

bool C4StartupPlrSelDlg::CheckPlayerName(const StdStrBuf &Playername, StdStrBuf &Filename, const StdStrBuf *pPrevFilename, bool fWarnEmpty)
{
	// must not be empty
	if (!Playername.getLength())
	{
		if (fWarnEmpty) C4GUI::Screen::GetScreenS()->ShowMessage(LoadResStr("IDS_ERR_PLRNAME_EMPTY"), "", C4GUI::Ico_Error);
		return false;
	}
	// generate valid filename
	Filename.Copy(Playername);
	// Slashes in Filenames are no good
	SReplaceChar(Filename.getMData(), '\\', '_');
	SReplaceChar(Filename.getMData(), '/', '_');
	SReplaceChar(Filename.getMData(), ':', '_');
	SReplaceChar(Filename.getMData(), '*', '_');
	SReplaceChar(Filename.getMData(), '?', '_');
	SReplaceChar(Filename.getMData(), '"', '_');
	SReplaceChar(Filename.getMData(), '<', '_');
	SReplaceChar(Filename.getMData(), '>', '_');
	SReplaceChar(Filename.getMData(), '|', '_');
	if (*Filename.getData() == '.') *Filename.getMData() = '_';
	Filename.Append(".ocp");
	StdStrBuf Path(Config.General.UserDataPath); // start at local path

	Path.Append(Filename);
	// validity check: Must not exist yet if renamed
	if (!pPrevFilename || !ItemIdentical(Path.getData(), Config.AtUserDataPath(pPrevFilename->getData()))) if (ItemExists(Path.getData()))
	{
		C4GUI::Screen::GetScreenS()->ShowMessage(FormatString(LoadResStr("IDS_ERR_PLRNAME_TAKEN"),
		    Playername.getData()).getData(), "", C4GUI::Ico_Error);
		return false;
	}

	return true;
}

void C4StartupPlrSelDlg::OnCrewBtn(C4GUI::Control *btn)
{
	// display crew for activated player
	if (eMode != PSDM_Player) return;
	PlayerListItem *pSel = static_cast<PlayerListItem *>(GetSelection());
	if (!pSel) return;
	SetCrewMode(pSel);
}

void C4StartupPlrSelDlg::SetPlayerMode()
{
	// change view to listing players
	C4GUI::GUISound("UI::Close");
	StdStrBuf LastPlrFilename;
	LastPlrFilename.Copy(static_cast<const StdStrBuf &>(CurrPlayer.Grp.GetFullName()));
	CurrPlayer.Grp.Close();
	eMode = PSDM_Player;
	UpdatePlayerList();
	SelectItem(LastPlrFilename, false);
	UpdateSelection();
}

void C4StartupPlrSelDlg::SetCrewMode(PlayerListItem *pSel)
{
	// change view to listing crew of a player
	CurrPlayer.Core = pSel->GetCore();

	StdStrBuf Path(Config.General.UserDataPath); // start at local path
//  Path.Append(Config.General.PlayerPath);
	Path.Append(pSel->GetFilename());

	if (!CurrPlayer.Grp.Open(Path.getData())) return;
	if (!CurrPlayer.Grp.FindEntry(C4CFN_ObjectInfoFiles))
	{
		StdCopyStrBuf strCrew(FormatString("%s %s", LoadResStrNoAmp("IDS_CTL_CREW"), CurrPlayer.Core.PrefName));
		// player has no crew!
		GetScreen()->ShowMessage(FormatString(LoadResStr("IDS_ERR_PLRNOCREW"),
		                                      CurrPlayer.Core.PrefName).getData(),
		                         strCrew.getData(), C4GUI::Ico_Player);
		return;
	}
	C4GUI::GUISound("UI::Open");
	eMode = PSDM_Crew;
	UpdatePlayerList();
	UpdateSelection();
}

void C4StartupPlrSelDlg::OnDelBtn(C4GUI::Control *btn)
{
	// something has changed!
	AbortRenaming();
	// delete selected player
	ListItem *pSel = GetSelection();
	if (!pSel) return;
	StdStrBuf sWarning; sWarning.Take(pSel->GetDelWarning());
	GetScreen()->ShowRemoveDlg(new C4GUI::ConfirmationDialog(sWarning.getData(), LoadResStr("IDS_BTN_DELETE"),
	                           new C4GUI::CallbackHandlerExPar<C4StartupPlrSelDlg, ListItem *>(this, &C4StartupPlrSelDlg::OnDelBtnConfirm, pSel), C4GUI::MessageDialog::btnYesNo));
}

void C4StartupPlrSelDlg::OnDelBtnConfirm(ListItem *pSel)
{
	StdStrBuf Path;

	switch (eMode)
	{
	case PSDM_Player:
		Path.Append(Config.General.UserDataPath); // start at local path
		Path.Append(pSel->GetFilename());
		if (!C4Group_DeleteItem(Path.getData()))
		{
			StdStrBuf sMsg; sMsg.Copy(LoadResStr("IDS_FAIL_DELETE"));
			GetScreen()->ShowMessage(sMsg.getData(), LoadResStr("IDS_DLG_CLEAR"), C4GUI::Ico_Error);
		}
		break;

	case PSDM_Crew:
		if (!CurrPlayer.Grp.DeleteEntry(pSel->GetFilename().getData()))
		{
			StdStrBuf sMsg; sMsg.Copy(LoadResStr("IDS_FAIL_DELETE"));
			GetScreen()->ShowMessage(sMsg.getData(), LoadResStr("IDS_DLG_CLEAR"), C4GUI::Ico_Error);
		}
		break;
	}
	// update buttons 'n stuff
	UpdatePlayerList();
}

void C4StartupPlrSelDlg::SelectItem(const StdStrBuf &Filename, bool fActivate)
{
	// find item
	for (ListItem *pPlrItem = static_cast<ListItem *>(pPlrListBox->GetFirst()); pPlrItem; pPlrItem = pPlrItem->GetNext())
		if (ItemIdentical(pPlrItem->GetFilename().getData(), Filename.getData()))
		{
			// select it
			pPlrListBox->SelectEntry(pPlrItem, false);
			// activate it
			if (fActivate)
			{
				pPlrItem->SetActivated(true);
				// player activation updates
				OnItemCheckChange(pPlrItem->GetCheckBox());
			}
			// max one
			return;
		}
}

void C4StartupPlrSelDlg::OnPropertyBtn(C4GUI::Control *btn)
{
	// something has changed!
	AbortRenaming();
	switch (eMode)
	{
	case PSDM_Player:
	{
		// show property dialog for selected player
		PlayerListItem *pSel = static_cast<PlayerListItem *>(GetSelection());
		if (!pSel) return;
		C4GUI::Dialog *pDlg;
		GetScreen()->ShowRemoveDlg(pDlg=new C4StartupPlrPropertiesDlg(pSel, this));
		pDlg->SetPos(std::min<int32_t>(GetBounds().Wdt/10, GetBounds().Wdt - pDlg->GetBounds().Wdt),
		             (GetBounds().Hgt - pDlg->GetBounds().Hgt) / 2);
	}
	break;

	case PSDM_Crew:
		// rename crew
		CrewListItem *pSel = static_cast<CrewListItem *>(GetSelection());
		if (!pSel) return;
		pSel->CrewRename();
		break;
	}
}


/* -- Crew sorting -- */

struct C4StartupPlrSelDlg_CrewSortDataEntry
{
	int32_t iMaxExp;
	C4ID idType;

	C4StartupPlrSelDlg_CrewSortDataEntry(int32_t iMaxExp, C4ID idType) : iMaxExp(iMaxExp), idType(idType) {}
};

class C4StartupPlrSelDlg_CrewSortDataMatchType
{
	C4ID idType;

public:
	C4StartupPlrSelDlg_CrewSortDataMatchType(C4ID idType) : idType(idType) {}
	bool operator()(C4StartupPlrSelDlg_CrewSortDataEntry Check) { return Check.idType == idType; }
};

typedef std::vector<C4StartupPlrSelDlg_CrewSortDataEntry> C4StartupPlrSelDlg_CrewSortData;

int32_t C4StartupPlrSelDlg::CrewSortFunc(const C4GUI::Element *pEl1, const C4GUI::Element *pEl2, void *par)
{
	const auto *pItem1 = static_cast<const CrewListItem *>(pEl1);
	const auto *pItem2 = static_cast<const CrewListItem *>(pEl2);
	C4StartupPlrSelDlg_CrewSortData &rSortData = *static_cast<C4StartupPlrSelDlg_CrewSortData *>(par);
	C4StartupPlrSelDlg_CrewSortData::iterator i = std::find_if(rSortData.begin(), rSortData.end(), C4StartupPlrSelDlg_CrewSortDataMatchType(pItem1->GetCore().id)),
	    j = std::find_if(rSortData.begin(), rSortData.end(), C4StartupPlrSelDlg_CrewSortDataMatchType(pItem2->GetCore().id));
	// primary sort: By Clonk type, where high exp Clonk types are sorted atop low exp Clonk types
	if (i != j)
	{
		if (i == rSortData.end()) return -1; else if (j == rSortData.end()) return +1; // can't really happen
		return (*i).iMaxExp - (*j).iMaxExp;
	}
	// secondary: By experience
	return pItem1->GetCore().Experience - pItem2->GetCore().Experience;
}

void C4StartupPlrSelDlg::ResortCrew()
{
	assert(eMode == PSDM_Crew);
	// create a list of Clonk types and their respective maximum experience
	C4StartupPlrSelDlg_CrewSortData SortData;
	for (CrewListItem *pCrewItem = static_cast<CrewListItem *>(pPlrListBox->GetFirst()); pCrewItem; pCrewItem = pCrewItem->GetNext())
	{
		C4StartupPlrSelDlg_CrewSortData::iterator i = std::find_if(SortData.begin(), SortData.end(), C4StartupPlrSelDlg_CrewSortDataMatchType(pCrewItem->GetCore().id));
		if (i == SortData.end())
			SortData.emplace_back(pCrewItem->GetCore().Experience, pCrewItem->GetCore().id);
		else
			(*i).iMaxExp = std::max<int32_t>((*i).iMaxExp, pCrewItem->GetCore().Experience);
	}
	pPlrListBox->SortElements(&CrewSortFunc, &SortData);
}

// ------------------------------------------------
// --- Player color HSV chooser
class C4StartupPlrColorPickerDlg : public C4GUI::Dialog
{
public:
	C4StartupPlrColorPickerDlg(C4PlayerInfoCore *plrcore);

protected:
	// Event handler
	void OnClosed(bool commit) override;

private:
	class Picker : public C4GUI::Control
	{
	public:
		Picker(const C4Rect &bounds);

		// Set/retrieve current color value
		void SetColor(uint32_t rgb);
		uint32_t GetColor() const;
	
	protected:
		// Event handlers, overridden from C4GUI::Control
		void DrawElement(C4TargetFacet &cgo) override;
		void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override;
		void DoDragging(C4GUI::CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam) override;
	
	private:
		static const unsigned int HSPickerCursorSize = 5;
		static const unsigned int VPickerCursorSize = 7;
		C4FacetSurface hsFacet, vFacet; // chooser backgrounds
		C4Rect hsPickerRect, vPickerRect;
		C4GUI::Picture *flagPreview, *crewPreview;
		uint32_t hsv; // current color
		enum {
			PS_Idle, // user isn't dragging anything
			PS_IdleDragging, // user started the drag on empty space
			PS_DragHS, // user started the drag over the HS picker
			PS_DragV // user started the drag over the V picker
		} state;

		bool HandleMouseDown(int32_t x, int32_t y);
		void UpdateVFacet(uint32_t h, uint32_t s);
		void UpdatePreview();
	};

	C4PlayerInfoCore *plrcore;
	Picker *picker;

	static uint32_t HSV2RGB(uint32_t hsv)
	{
		float h = GetRedValue(hsv) / 255.f * 6.f;
		float s = GetGreenValue(hsv) / 255.f;
		float v = GetBlueValue(hsv) / 255.f;

		float chroma = s * v;
		float x = chroma * (1.f - std::abs(std::fmod(h, 2.f) - 1.f));

		float r = 0;
		float g = 0;
		float b = 0;

		switch (static_cast<int>(h))
		{
		case 0: case 6:
			r = chroma; g = x; break;
		case 1:
			r = x; g = chroma; break;
		case 2:
			g = chroma; b = x; break;
		case 3:
			g = x; b = chroma; break;
		case 4:
			b = chroma; r = x; break;
		case 5:
			b = x; r = chroma; break;
		}
		r += v-chroma;
		g += v-chroma;
		b += v-chroma;

		return RGBA(r * 255.f, g * 255.f, b * 255.f, hsv >> 24);
	}
	static uint32_t RGB2HSV(uint32_t rgb)
	{
		float r = GetRedValue(rgb) / 255.f;
		float g = GetGreenValue(rgb) / 255.f;
		float b = GetBlueValue(rgb) / 255.f;

		float min = std::min(r, std::min(g, b));
		float max = std::max(r, std::max(g, b));

		float chroma = max - min;
	
		float hue = 0;
		if (r == max)
			hue = std::fmod((g-b) / chroma, 6.f);
		else if (g == max)
			hue = (b-r) / chroma + 2.f;
		else
			hue = (r-g) / chroma + 4.f;

		float h = hue / 6.f;
		float s = max == 0 ? 0.f : chroma / max;
		float v = max;

		return RGBA(h * 255.f, s * 255.f, v * 255.f, rgb >> 24);
	}
};

C4StartupPlrColorPickerDlg::C4StartupPlrColorPickerDlg(C4PlayerInfoCore *plrcore)
	: Dialog(400, 296 + C4GUI_ButtonAreaHgt, LoadResStr("IDS_DLG_PLAYERCOLORSELECTION"), false), plrcore(plrcore)
{
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 1, true);

	picker = new Picker(caMain.GetFromTop(280));
	picker->SetColor(plrcore->PrefColorDw);
	AddElement(picker);

	// buttons
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0, 0);
	caButtonArea = C4GUI::ComponentAligner(caButtonArea.GetCentered(2*128 + 4*8, C4GUI_ButtonAreaHgt), 8, 8);
	C4GUI::Button *cancelButton = new C4GUI::CancelButton(caButtonArea.GetFromRight(128));
	C4GUI::Button *okButton = new C4GUI::OKButton(caButtonArea.GetFromRight(128));
	AddElement(okButton);
	AddElement(cancelButton);
}

void C4StartupPlrColorPickerDlg::OnClosed(bool commit)
{
	// Write chosen color back to player core
	if (commit)
		plrcore->PrefColorDw = picker->GetColor();
}

C4StartupPlrColorPickerDlg::Picker::Picker(const C4Rect &bounds)
	: Control(bounds), state(PS_Idle)
{
	C4GUI::ComponentAligner caMain(bounds, 8, 8, true);
	caMain.ExpandBottom(-(caMain.GetInnerHeight() - 256));
	hsPickerRect = caMain.GetFromLeft(256);
	vPickerRect = caMain.GetFromLeft(16 + VPickerCursorSize);
	vPickerRect.Hgt = 256 - PlayerColorValueLowBound;

	C4Facet &flagPreviewPic = ::GraphicsResource.fctFlagClr;
	int preview_width = std::min<int>(flagPreviewPic.Wdt, caMain.GetInnerWidth());
	flagPreview = new C4GUI::Picture(caMain.GetFromTop(flagPreviewPic.GetHeightByWidth(preview_width), preview_width), true);
	flagPreview->SetFacet(flagPreviewPic);
	AddElement(flagPreview);

	C4Facet &crewPreviewPic = ::GraphicsResource.fctCrewClr;
	preview_width = std::min<int>(crewPreviewPic.Wdt, caMain.GetInnerWidth());
	crewPreview = new C4GUI::Picture(caMain.GetFromTop(crewPreviewPic.GetHeightByWidth(preview_width), preview_width), true);
	crewPreview->SetFacet(crewPreviewPic);
	AddElement(crewPreview);
	
	// Pre-draw the H+S chooser background, it never changes anyway
	hsFacet.Create(hsPickerRect.Wdt, hsPickerRect.Hgt);
	hsFacet.Surface->Lock();
	for (int y = 0; y < hsFacet.Hgt; ++y)
		for (int x = 0; x < hsFacet.Wdt; ++x)
			hsFacet.Surface->SetPixDw(x, y, HSV2RGB(C4RGB(x, 255-y, 255)));
	hsFacet.Surface->Unlock();

	vFacet.Create(vPickerRect.Wdt - VPickerCursorSize, vPickerRect.Hgt);
	UpdateVFacet(255, 255);
}

void C4StartupPlrColorPickerDlg::Picker::UpdateVFacet(uint32_t h, uint32_t s)
{
	// Draw the V chooser background according to current H+S values
	vFacet.Surface->Lock();
	for (int y = 0; y < vPickerRect.Hgt; ++y)
		for (int x = 0; x < vFacet.Wdt; ++x)
			vFacet.Surface->SetPixDw(x, y, HSV2RGB(C4RGB(h, s, 255-y)));
	vFacet.Surface->Unlock();
}

void C4StartupPlrColorPickerDlg::Picker::UpdatePreview()
{
	flagPreview->SetDrawColor(HSV2RGB(hsv));
	crewPreview->SetDrawColor(HSV2RGB(hsv));
}

void C4StartupPlrColorPickerDlg::Picker::SetColor(uint32_t rgb)
{
	hsv = RGB2HSV(rgb);
	UpdateVFacet(GetRedValue(hsv), GetGreenValue(hsv));
	UpdatePreview();
}

uint32_t C4StartupPlrColorPickerDlg::Picker::GetColor() const
{
	return HSV2RGB(hsv);
}

void C4StartupPlrColorPickerDlg::Picker::DrawElement(C4TargetFacet &cgo)
{
	// H+S chooser background
	C4Facet cgoPicker(cgo.Surface, cgo.TargetX + hsPickerRect.x, cgo.TargetY + hsPickerRect.y, hsPickerRect.Wdt, hsPickerRect.Hgt);
	hsFacet.Draw(cgoPicker.Surface, cgoPicker.X, cgoPicker.Y);
	// H+S cursor
	cgoPicker.Wdt = cgoPicker.Hgt = HSPickerCursorSize;
	cgoPicker.X += GetRedValue(hsv) - cgoPicker.Wdt / 2;
	cgoPicker.Y += 255 - GetGreenValue(hsv) - cgoPicker.Hgt / 2;
	pDraw->DrawLineDw(cgoPicker.Surface, cgoPicker.X, cgoPicker.Y, cgoPicker.X + cgoPicker.Wdt, cgoPicker.Y + cgoPicker.Hgt, C4RGB(0, 0, 0));
	pDraw->DrawLineDw(cgoPicker.Surface, cgoPicker.X + cgoPicker.Wdt, cgoPicker.Y, cgoPicker.X, cgoPicker.Y + cgoPicker.Hgt, C4RGB(0, 0, 0));

	// V chooser background
	cgoPicker.Set(cgo.Surface, cgo.TargetX + vPickerRect.x + VPickerCursorSize, cgo.TargetY + vPickerRect.y, vPickerRect.Wdt - VPickerCursorSize, vPickerRect.Hgt);
	vFacet.Draw(cgoPicker.Surface, cgoPicker.X, cgoPicker.Y);
	// V cursor
	cgoPicker.Wdt = cgoPicker.Hgt = VPickerCursorSize;
	cgoPicker.X -= cgoPicker.Wdt / 2 + 1;
	cgoPicker.Y += 255 - GetBlueValue(hsv) - cgoPicker.Hgt / 2;
	for (int i = 0; i < cgoPicker.Hgt / 2 + 1; ++i)
		pDraw->DrawLineDw(cgoPicker.Surface, cgoPicker.X + i, cgoPicker.Y + i, cgoPicker.X + i, cgoPicker.Y + cgoPicker.Hgt - i, C4RGB(255, 255, 255));
}

bool C4StartupPlrColorPickerDlg::Picker::HandleMouseDown(int32_t x, int32_t y)
{
	if (state == PS_IdleDragging)
	{
		// User is dragging something that is neither of the pickers. Ignore.
		return false;
	}
	// Check if a drag starts or was originally started over a picker
	else if (state == PS_DragHS || (state == PS_Idle && hsPickerRect.Contains(x, y)))
	{
		int h = Clamp(x - hsPickerRect.x, 0, hsPickerRect.Wdt - 1);
		assert(Inside(h, 0, 255));
		int s = 255 - Clamp(y - hsPickerRect.y, 0, hsPickerRect.Hgt - 1);
		assert(Inside(s, 0, 255));
		hsv = C4RGB(h, s, GetBlueValue(hsv));
		UpdateVFacet(h, s);
		UpdatePreview();
		state = PS_DragHS;
		return true;
	}
	else if (state == PS_DragV || (state == PS_Idle && vPickerRect.Contains(x, y)))
	{
		int v = 255 - Clamp(y - vPickerRect.y, 0, vPickerRect.Hgt - 1);
		assert(Inside(v, 0, 255));
		hsv = (hsv & 0xFFFFFF00) | v;
		UpdatePreview();
		state = PS_DragV;
		return true;
	}
	else
	{
		// Drag started outside of all picker areas; ignore movement until user releases mouse button.
		state = PS_IdleDragging;
		return false;
	}
}

void C4StartupPlrColorPickerDlg::Picker::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);

	if (!rMouse.IsLDown()) state = PS_Idle;
	if (rMouse.pDragElement) return;
	if (rMouse.IsLDown())
	{
		if (HandleMouseDown(iX, iY))
		{
			rMouse.pDragElement = this;
			C4GUI::GUISound("UI::Select");
		}
		else
		{
			rMouse.pDragElement = nullptr;
		}
	}
}

void C4StartupPlrColorPickerDlg::Picker::DoDragging(C4GUI::CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	HandleMouseDown(iX, iY);
}

/* ---- Player property dlg ---- */

C4StartupPlrPropertiesDlg::C4StartupPlrPropertiesDlg(C4StartupPlrSelDlg::PlayerListItem * pForPlayer, C4StartupPlrSelDlg *pParentDlg)
		: Dialog(C4Startup::Get()->Graphics.fctPlrPropBG.Wdt, C4Startup::Get()->Graphics.fctPlrPropBG.Hgt, "", false), pMainDlg(pParentDlg), pForPlayer(pForPlayer), fClearBigIcon(false)
{
	if (pForPlayer)
	{
		// edit existing player
		C4P = pForPlayer->GetCore();
	}
	else
	{
		// create new player: Use default C4P values, with a few exceptions
		// FIXME: Use Player, not Clonkranks
		C4P.Default(&::DefaultRanks);
		// Set name, color, comment
		SCopy(LoadResStr("IDS_PLR_NEWCOMMENT"), C4P.Comment, C4MaxComment);
		C4P.PrefColor = UnsyncedRandom(8);
		C4P.PrefColorDw = C4P.GetPrefColorValue(C4P.PrefColor);
		C4P.OldPrefControlStyle = 1;
		C4P.OldPrefAutoContextMenu = 1;
		C4P.OldPrefControl = 0;
	}
	const int32_t BetweenElementDist = 2;
	// use black fonts here
	CStdFont *pUseFont = &C4Startup::Get()->Graphics.BookFont;
	CStdFont *pSmallFont = &C4Startup::Get()->Graphics.BookSmallFont;
	// get positions
	UpdateSize();
	C4GUI::ComponentAligner caMain(GetClientRect(), 0, 1, true);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt), 0,0);
	// dlg title
	const char *szTitle;
	if (pForPlayer)
	{
		szTitle = LoadResStr("IDS_DLG_PLAYER2");
	}
	else
	{
		szTitle = LoadResStr("IDS_PLR_NEWPLAYER");
	}
	C4GUI::Label *pLbl = new C4GUI::Label(szTitle, caMain.GetFromTop(pUseFont->GetLineHeight()), ALeft, C4StartupFontClr, pUseFont, false);
	AddElement(pLbl);
	caMain.ExpandTop(-BetweenElementDist);
	// place name label
	AddElement(new C4GUI::Label(LoadResStr("IDS_CTL_NAME2"), caMain.GetFromTop(pSmallFont->GetLineHeight()), ALeft, C4StartupFontClr, pSmallFont, false));
	// place name edit
	pNameEdit = new C4GUI::Edit(caMain.GetFromTop(C4GUI::Edit::GetCustomEditHeight(pUseFont)));
	pNameEdit->SetFont(pUseFont);
	pNameEdit->SetColors(C4StartupEditBGColor, C4StartupFontClr, C4StartupEditBorderColor);
	pNameEdit->InsertText(C4P.PrefName, false);
	pNameEdit->SetMaxText(C4MaxName);
	AddElement(pNameEdit);
	SetFocus(pNameEdit, false);
	caMain.ExpandTop(-BetweenElementDist);

	int32_t iControlPicSize = C4GUI::ArrowButton::GetDefaultHeight(); // GetGridCell(0,3,0,1,-1,-1,false,2)
	int32_t label_hgt = pSmallFont->GetLineHeight();

	// place color label
	C4GUI::ComponentAligner caColorArea(caMain.GetFromTop(iControlPicSize + BetweenElementDist + label_hgt), 2, 0);
	C4GUI::ComponentAligner caPictureArea(caColorArea.GetFromRight(iControlPicSize, iControlPicSize + BetweenElementDist + label_hgt), 2,0);
	caColorArea.ExpandLeft(2);
	AddElement(new C4GUI::Label(FormatString("%s:", LoadResStr("IDS_CTL_COLOR")).getData(), caColorArea.GetFromTop(label_hgt), ALeft, C4StartupFontClr, pSmallFont, false));
	caColorArea.ExpandTop(-BetweenElementDist);
	// place picture label
	AddElement(new C4GUI::Label(LoadResStr("IDS_CTL_PICTURE"), caPictureArea.GetFromTop(label_hgt), ALeft, C4StartupFontClr, pSmallFont, false));
	caPictureArea.ExpandTop(-BetweenElementDist);
	// place color controls
	C4GUI::Button *pBtn; const char *szTip;
	szTip = LoadResStr("IDS_DLGTIP_PLAYERCOLORS");
	AddElement(pBtn = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::ArrowButton>(C4GUI::ArrowButton::Left, caColorArea.GetFromLeft(C4GUI::ArrowButton::GetDefaultWidth()), &C4StartupPlrPropertiesDlg::OnClrChangeLeft));
	pBtn->SetToolTip(szTip);
	C4Facet &rfctClrPreviewPic = ::GraphicsResource.fctFlagClr;
	pClrPreview = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::IconButton>(C4GUI::Ico_None, caColorArea.GetFromLeft(rfctClrPreviewPic.GetWidthByHeight(caColorArea.GetHeight())), 'C', &C4StartupPlrPropertiesDlg::OnClrChangeCustom);
	pClrPreview->SetFacet(rfctClrPreviewPic);
	AddElement(pClrPreview);
	AddElement(pBtn = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::ArrowButton>(C4GUI::ArrowButton::Right, caColorArea.GetFromLeft(C4GUI::ArrowButton::GetDefaultWidth()), &C4StartupPlrPropertiesDlg::OnClrChangeRight));
	pBtn->SetToolTip(szTip);
	if (!C4P.PrefColorDw) C4P.PrefColorDw=0xff;
	// Place picture controls
	AddElement(pPictureBtn = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::IconButton>(C4GUI::Ico_Player, caPictureArea.GetAll(), 'P' /* 2do */, &C4StartupPlrPropertiesDlg::OnPictureBtn));
	pPictureBtn->SetToolTip(LoadResStr("IDS_DESC_SELECTAPICTUREANDORLOBBYI"));
	UpdateBigIcon();
	UpdatePlayerColor(true);
	caMain.ExpandTop(-BetweenElementDist);
	// place control label
	C4GUI::ComponentAligner caControlArea(caMain.GetFromTop(iControlPicSize + label_hgt + BetweenElementDist), 0,0, false);
	C4GUI::ComponentAligner caSkinArea(caControlArea.GetFromRight(iControlPicSize + label_hgt + BetweenElementDist), 0,0, false);
	AddElement(new C4GUI::Label(FormatString("%s:", LoadResStr("IDS_CTL_CONTROL")).getData(), caControlArea.GetFromTop(label_hgt), ALeft, C4StartupFontClr, pSmallFont, false));
	caControlArea.ExpandTop(-BetweenElementDist);
	// place clonk style label
	AddElement(new C4GUI::Label(LoadResStr("IDS_CTL_CLONKSKIN"), caSkinArea.GetFromTop(label_hgt), ALeft, C4StartupFontClr, pSmallFont, false));
	caSkinArea.ExpandTop(-BetweenElementDist);
	// place control controls
	C4GUI::ComponentAligner caControl(caControlArea.GetFromTop(iControlPicSize), 2,0);
	szTip = LoadResStr("IDS_DLGTIP_PLAYERCONTROL");
	AddElement(pBtn = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::ArrowButton>(C4GUI::ArrowButton::Left, caControl.GetFromLeft(C4GUI::ArrowButton::GetDefaultWidth()), &C4StartupPlrPropertiesDlg::OnCtrlChangeLeft));
	pBtn->SetToolTip(szTip);
	caControl.ExpandBottom(label_hgt); C4Rect ctrl_name_rect = caControl.GetFromBottom(label_hgt);
	C4Facet &rfctCtrlPic = ::GraphicsResource.fctKeyboard; // UpdatePlayerControl() will alternatively set fctGamepad
	AddElement(pCtrlImg = new C4GUI::Picture(caControl.GetFromLeft(rfctCtrlPic.GetWidthByHeight(caControl.GetHeight())), true));
	pCtrlImg->SetToolTip(szTip);
	AddElement(pBtn = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::ArrowButton>(C4GUI::ArrowButton::Right, caControl.GetFromLeft(C4GUI::ArrowButton::GetDefaultWidth()), &C4StartupPlrPropertiesDlg::OnCtrlChangeRight));
	pBtn->SetToolTip(szTip);
	caControl.ExpandLeft(-10);
	C4P.OldPrefControl = Clamp<int32_t>(C4P.OldPrefControl, 0, C4MaxControlSet-1);
	ctrl_name_lbl = new C4GUI::Label("CtrlName", ctrl_name_rect, ALeft, C4StartupFontClr, pSmallFont, false, false, true);
	AddElement(ctrl_name_lbl);
	UpdatePlayerControl();

	C4GUI::ComponentAligner caSkin(caSkinArea.GetFromTop(iControlPicSize), 2,0);
	szTip = LoadResStr("IDS_DLGTIP_PLAYERCREWSKIN");
	AddElement(pBtn = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::ArrowButton>(C4GUI::ArrowButton::Left, caSkin.GetFromLeft(C4GUI::ArrowButton::GetDefaultWidth()), &C4StartupPlrPropertiesDlg::OnSkinChangeLeft));
	pBtn->SetToolTip(szTip);
	C4Facet rfctSkinPic = ::GraphicsResource.fctClonkSkin.GetPhase(0);
	AddElement(pSkinImg = new C4GUI::Picture(caSkin.GetFromLeft(rfctSkinPic.GetWidthByHeight(caSkin.GetHeight())), true));
	pSkinImg->SetToolTip(szTip);
	pSkinImg->SetFacet(::GraphicsResource.fctClonkSkin.GetPhase(0));
	AddElement(pBtn = new C4GUI::CallbackButton<C4StartupPlrPropertiesDlg, C4GUI::ArrowButton>(C4GUI::ArrowButton::Right, caSkin.GetFromLeft(C4GUI::ArrowButton::GetDefaultWidth()), &C4StartupPlrPropertiesDlg::OnSkinChangeRight));
	pBtn->SetToolTip(szTip);
	caSkin.ExpandLeft(-10);
	UpdatePlayerSkin();

	caMain.ExpandTop(-BetweenElementDist);
	// place buttons
	// OK
	C4GUI::Button *pBtnOK = new C4GUI::OKIconButton(C4Rect(147-GetMarginLeft(), 295+35-GetMarginTop(), 54, 33), C4GUI::Ico_None);
	AddElement(pBtnOK);
	// Cancel
	C4GUI::Button *pBtnAbort = new C4GUI::CancelIconButton(C4Rect(317-GetMarginLeft(), 16-GetMarginTop(), 21, 21), C4GUI::Ico_None);
	AddElement(pBtnAbort);
	// when called from player selection screen: input dlg always closed in the end
	// otherwise, modal proc will delete
	if (pMainDlg) SetDelOnClose();
}

void C4StartupPlrPropertiesDlg::DrawElement(C4TargetFacet &cgo)
{
	C4Startup::Get()->Graphics.fctPlrPropBG.Draw(cgo.Surface, rcBounds.x+cgo.TargetX, rcBounds.y+cgo.TargetY);
}

bool IsColorConflict(DWORD dwClr1, DWORD dwClr2);

void C4StartupPlrPropertiesDlg::UpdatePlayerColor(bool fUpdateSliders)
{
	C4P.PrefColorDw = C4P.PrefColorDw | 0xFF000000; // Ensure full opacity
	pClrPreview->SetColor(C4P.PrefColorDw);
	pPictureBtn->SetColor(C4P.PrefColorDw);
}

void C4StartupPlrPropertiesDlg::OnClrChangeLeft(C4GUI::Control *pBtn)
{
	// previous standard color in list
	C4P.PrefColor = C4P.PrefColor ? C4P.PrefColor - 1 : 11;
	C4P.PrefColorDw = C4PlayerInfoCore::GetPrefColorValue(C4P.PrefColor);
	UpdatePlayerColor(true);
}
void C4StartupPlrPropertiesDlg::OnClrChangeCustom(C4GUI::Control *pBtn)
{
	GetScreen()->ShowModalDlg(new C4StartupPlrColorPickerDlg(&C4P));
	UpdatePlayerColor(true);
}

void C4StartupPlrPropertiesDlg::OnClrChangeRight(C4GUI::Control *pBtn)
{
	// next standard color in list
	C4P.PrefColor = (C4P.PrefColor + 1) % 12;
	C4P.PrefColorDw = C4PlayerInfoCore::GetPrefColorValue(C4P.PrefColor);
	UpdatePlayerColor(true);
}

void C4StartupPlrPropertiesDlg::UpdatePlayerControl()
{
	C4PlayerControlAssignmentSet *control_set = Game.PlayerControlUserAssignmentSets.GetSetByName(C4P.PrefControl.getData());
	if (!control_set) control_set = Game.PlayerControlUserAssignmentSets.GetDefaultSet();
	// update keyboard image of selected control
	C4Facet fctCtrlPic;
	if (control_set) fctCtrlPic = control_set->GetPicture();
	pCtrlImg->SetFacet(fctCtrlPic);
	if (control_set)
		ctrl_name_lbl->SetText(control_set->GetGUIName());
	else
		ctrl_name_lbl->SetText("???");
}

void C4StartupPlrPropertiesDlg::OnCtrlChangeLeft(C4GUI::Control *pBtn)
{
	// previous control set in list
	C4PlayerControlAssignmentSet *control_set = Game.PlayerControlUserAssignmentSets.GetSetByName(C4P.PrefControl.getData());
	int32_t index = Game.PlayerControlUserAssignmentSets.GetSetIndex(control_set);
	if (index < 0) index = 0; // defined control set not found - probably an old CR player file
	if (!index--) index = Game.PlayerControlUserAssignmentSets.GetSetCount() - 1;
	control_set = Game.PlayerControlUserAssignmentSets.GetSetByIndex(index);
	if (control_set) C4P.PrefControl = control_set->GetName();
	UpdatePlayerControl();
}

void C4StartupPlrPropertiesDlg::OnCtrlChangeRight(C4GUI::Control *pBtn)
{
	// next control set in list
	C4PlayerControlAssignmentSet *control_set = Game.PlayerControlUserAssignmentSets.GetSetByName(C4P.PrefControl.getData());
	int32_t index = Game.PlayerControlUserAssignmentSets.GetSetIndex(control_set);
	if (index < 0) index = 0; // defined control set not found - probably an old CR player file
	if (++index >= int32_t(Game.PlayerControlUserAssignmentSets.GetSetCount())) index = 0;
	control_set = Game.PlayerControlUserAssignmentSets.GetSetByIndex(index);
	if (control_set) C4P.PrefControl = control_set->GetName();
	UpdatePlayerControl();
}

void C4StartupPlrPropertiesDlg::UpdatePlayerSkin()
{
	pSkinImg->SetFacet(::GraphicsResource.fctClonkSkin.GetPhase(C4P.PrefClonkSkin));
}

void C4StartupPlrPropertiesDlg::OnSkinChangeLeft(C4GUI::Control *pBtn)
{
	// previous skin in list
	C4P.PrefClonkSkin = C4P.PrefClonkSkin ? C4P.PrefClonkSkin - 1 : 3;
	UpdatePlayerSkin();
}
void C4StartupPlrPropertiesDlg::OnSkinChangeRight(C4GUI::Control *pBtn)
{
	// next skin in list
	C4P.PrefClonkSkin = (C4P.PrefClonkSkin + 1) % 4;
	UpdatePlayerSkin();
}

void C4StartupPlrPropertiesDlg::UserClose(bool fOK)
{
	// check name validity
	if (fOK)
	{
		StdStrBuf PlrName(pNameEdit->GetText()), Filename;
		if (!C4StartupPlrSelDlg::CheckPlayerName(PlrName, Filename, pForPlayer ? &pForPlayer->GetFilename() : nullptr, true)) return;

		// Warn that gamepad controls are still unfinished.
		C4PlayerControlAssignmentSet *control_set = Game.PlayerControlUserAssignmentSets.GetSetByName(C4P.PrefControl.getData());
		if (control_set && control_set->HasGamepad())
		{
			GetScreen()->ShowMessageModal(
					LoadResStr("IDS_DLG_GAMEPADEXPERIMENTAL"),
					LoadResStr("IDS_DLG_GAMEPADEXPTITLE"),
					C4GUI::MessageDialog::btnOK,
					C4GUI::Ico_Gamepad
			);
		}
	}
	Close(fOK);
}

void C4StartupPlrPropertiesDlg::OnClosed(bool fOK)
{
	if (fOK)
	{
		// store selected data if desired
		StdStrBuf PlrName(pNameEdit->GetText()), Filename;
		if (C4StartupPlrSelDlg::CheckPlayerName(PlrName, Filename, pForPlayer ? &pForPlayer->GetFilename() : nullptr, true))
		{
			SCopy(PlrName.getData(), C4P.PrefName, C4MaxName);
			C4Group PlrGroup;
			bool fSucc=false;
			// existant player: update file
			if (pForPlayer)
			{
				if (!pForPlayer->MoveFilename(Filename.getData()))
					GetScreen()->ShowMessage(LoadResStr("IDS_FAIL_RENAME"), "", C4GUI::Ico_Error);
				// update bigicon
				if (fClearBigIcon || fctNewBigIcon.Surface)
				{
					C4Group PlrGroup;
					if (PlrGroup.Open(Config.AtUserDataPath(Filename.getData())))
					{
						if (fClearBigIcon || fctNewBigIcon.Surface) PlrGroup.Delete(C4CFN_BigIcon);
						if (fctNewBigIcon.Surface) fctNewBigIcon.GetFace().SavePNG(PlrGroup, C4CFN_BigIcon);
						if (PlrGroup.Close()) fSucc = true;
						if (fClearBigIcon || fctNewBigIcon.Surface) pForPlayer->GrabCustomIcon(fctNewBigIcon);
					}
				}
				else
				{
					fSucc = true;
				}
				pForPlayer->UpdateCore(C4P);
				// player may have been activated: Make sure any new filename is reflected in participants list
				if (pMainDlg) pMainDlg->UpdateActivatedPlayers();
			}
			else
			{
				// NewPlayer: Open new player group
				if (PlrGroup.Open(Config.AtUserDataPath(Filename.getData()), true))
				{
					// Do not overwrite (should have been caught earlier anyway)
					if (PlrGroup.FindEntry(C4CFN_PlayerInfoCore)) return;
					// Save info core
					C4P.Save(PlrGroup);
					// Add BigIcon
					if (fctNewBigIcon.Surface)
					{
						fctNewBigIcon.GetFace().SavePNG(PlrGroup, C4CFN_BigIcon);
					}
					// Close group
					if (PlrGroup.Close()) fSucc=true;
					// update activate button text
					if (pMainDlg)
					{
						pMainDlg->UpdatePlayerList();
						pMainDlg->SelectItem(Filename, true);
					}
					else
					{
						// no main player selection dialog: This means that this dlg was shown as a creation dialog from the main startup dlg
						// Just set the newly created player as current selection
						SCopy(Config.AtRelativePath(Filename.getData()), Config.General.Participants, sizeof Config.General.Participants);
					}
				}
			}
			if (!fSucc) GetScreen()->ShowErrorMessage(PlrGroup.GetError());
		}
	}
	// Make the dialog go away
	Dialog::OnClosed(fOK);
}

bool C4StartupPlrPropertiesDlg::SetNewPicture(C4Surface &srcSfc, C4FacetSurface *trgFct, int32_t iMaxSize, bool fColorize)
{
	if (fColorize)
	{
		C4Surface srcSfcClr;
		if (!srcSfcClr.CreateColorByOwner(&srcSfc)) return false;
		return trgFct->CopyFromSfcMaxSize(srcSfcClr, iMaxSize, C4P.PrefColorDw);
	}
	else
	{
		return trgFct->CopyFromSfcMaxSize(srcSfc, iMaxSize);
	}
}

void C4StartupPlrPropertiesDlg::SetNewPicture(const char *szFromFilename)
{
	if (!szFromFilename)
	{
		// If szFromFilename==nullptr, clear bigicon
		fClearBigIcon = true;
		fctNewBigIcon.Clear();
	}
	else
	{
		// else set new bigicon by loading and scaling if necessary.
		C4Surface sfcNewPic;
		C4Group SrcGrp;
		StdStrBuf sParentPath;
		GetParentPath(szFromFilename, &sParentPath);
		bool fSucc = false;
		if (SrcGrp.Open(sParentPath.getData()))
		{
			if (sfcNewPic.Load(SrcGrp, GetFilename(szFromFilename), false, false, 0))
			{
				fSucc = true;
				if (!SetNewPicture(sfcNewPic, &fctNewBigIcon, C4MaxBigIconSize, true)) fSucc = false;
			}
		}
		if (!fSucc)
		{
			// error!
			GetScreen()->ShowErrorMessage(FormatString(LoadResStr("IDS_PRC_NOGFXFILE"), szFromFilename, SrcGrp.GetError()).getData());
		}
	}
	// update icon
	UpdateBigIcon();
}

void C4StartupPlrPropertiesDlg::OnPictureBtn(C4GUI::Control *pBtn)
{
	StdStrBuf sNewPic;
	if (C4PortraitSelDlg::SelectPortrait(GetScreen(), &sNewPic))
	{
		SetNewPicture(sNewPic.getData());
	}
}

void C4StartupPlrPropertiesDlg::UpdateBigIcon()
{
	// new icon?
	bool fHasIcon = false;
	if (fctNewBigIcon.Surface)
	{
		pPictureBtn->SetFacet(fctNewBigIcon);
		fHasIcon = true;
	}
	// old icon in existing player?
	else if (!fClearBigIcon && pForPlayer)
	{
		C4Group PlrGroup;
		if (PlrGroup.Open(Config.AtUserDataPath(pForPlayer->GetFilename().getData())))
		{
			if (PlrGroup.FindEntry(C4CFN_BigIcon))
			{
				if (fctOldBigIcon.Load(PlrGroup, C4CFN_BigIcon, C4FCT_Full, C4FCT_Full, false, 0))
				{
					pPictureBtn->SetFacet(fctOldBigIcon);
					fHasIcon = true;
				}
			}
		}
	}
	// no icon: Set default
	if (!fHasIcon)
	{
		pPictureBtn->SetFacet(::GraphicsResource.fctPlayerClr);
	}
}
