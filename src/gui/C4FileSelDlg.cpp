/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// file selection dialogs

#include "C4Include.h"
#include "gui/C4FileSelDlg.h"

#include "C4Version.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h" // only for single use of ::GraphicsResource.fctOKCancel below...

#ifdef _WIN32
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#undef MK_ALT
#include <shlobj.h>
#ifndef CSIDL_MYPICTURES
#define CSIDL_MYPICTURES 0x0027
#endif
#endif

// def 1 if gfx loading works in background thread. Right now, it doesn't
// C4Group and C4Surface just don't like it
// So for now, the loading will be done in 1/10 of the frames in OnIdle of the dialog
#define USE_BACKGROUND_THREAD_LOAD 0


// ---------------------------------------------------
// C4FileSelDlg::ListItem

C4FileSelDlg::ListItem::ListItem(const char *szFilename) : C4GUI::Control(C4Rect(0,0,0,0))
{
	if (szFilename) sFilename.Copy(szFilename); else sFilename.Clear();
}

C4FileSelDlg::ListItem::~ListItem() = default;

// ---------------------------------------------------
// C4FileSelDlg::DefaultListItem

C4FileSelDlg::DefaultListItem::DefaultListItem(const char *szFilename, bool fTruncateExtension, bool fCheckbox, bool fGrayed, C4GUI::Icons eIcon)
		: C4FileSelDlg::ListItem(szFilename), pLbl(nullptr), pCheck(nullptr), pKeyCheck(nullptr), fGrayed(fGrayed)
{
	StdStrBuf sLabel; if (szFilename) sLabel.Ref(::GetFilename(szFilename)); else sLabel.Ref(LoadResStr("IDS_CTL_NONE"));
	if (szFilename && fTruncateExtension)
	{
		sLabel.Copy();
		char *szFilename = sLabel.GrabPointer();
		RemoveExtension(szFilename);
		sLabel.Take(szFilename);
	}
	rcBounds.Hgt = ::GraphicsResource.TextFont.GetLineHeight();
	UpdateSize();
	C4GUI::ComponentAligner caMain(GetContainedClientRect(),0,0);
	int32_t iHeight = caMain.GetInnerHeight();
	if (fCheckbox)
	{
		pCheck = new C4GUI::CheckBox(caMain.GetFromLeft(iHeight), nullptr, false);
		if (fGrayed) pCheck->SetEnabled(false);
		AddElement(pCheck);
		pKeyCheck = new C4KeyBinding(C4KeyCodeEx(K_SPACE), "FileSelToggleFileActive", KEYSCOPE_Gui,
		                             new C4GUI::ControlKeyCB<ListItem>(*this, &ListItem::UserToggleCheck), C4CustomKey::PRIO_Ctrl);
	}
	C4GUI::Icon *pIco = new C4GUI::Icon(caMain.GetFromLeft(iHeight), eIcon);
	AddElement(pIco);
	pLbl = new C4GUI::Label(sLabel.getData(), caMain.GetAll(), ALeft, fGrayed ? C4GUI_CheckboxDisabledFontClr : C4GUI_CheckboxFontClr);
	AddElement(pLbl);
}

C4FileSelDlg::DefaultListItem::~DefaultListItem()
{
	if (pKeyCheck) delete pKeyCheck;
}

void C4FileSelDlg::DefaultListItem::UpdateOwnPos()
{
	BaseClass::UpdateOwnPos();
	if (!pLbl) return;
	C4GUI::ComponentAligner caMain(GetContainedClientRect(),0,0);
	caMain.GetFromLeft(caMain.GetInnerHeight()*(1+!!pCheck));
	pLbl->SetBounds(caMain.GetAll());
}

bool C4FileSelDlg::DefaultListItem::IsChecked() const
{
	return pCheck ? pCheck->GetChecked() : false;
}

void C4FileSelDlg::DefaultListItem::SetChecked(bool fChecked)
{
	// store new state in checkbox
	if (pCheck) pCheck->SetChecked(fChecked);
}

bool C4FileSelDlg::DefaultListItem::UserToggleCheck()
{
	// toggle if possible
	if (pCheck && !IsGrayed())
	{
		pCheck->ToggleCheck(true);
		return true;
	}
	return false;
}




// ---------------------------------------------------
// C4FileSelDlg

C4FileSelDlg::C4FileSelDlg(const char *szRootPath, const char *szTitle, C4FileSel_BaseCB *pSelCallback, bool fInitElements)
		: C4GUI::Dialog(Clamp(C4GUI::GetScreenWdt()*2/3+10, 300,600), Clamp(C4GUI::GetScreenHgt()*2/3+10, 220,500), szTitle, false),
		pLocationComboBox(nullptr), pFileListBox(nullptr), pSelectionInfoBox(nullptr), btnOK(nullptr), pLocations(nullptr), iLocationCount(0), pSelection(nullptr), pSelCallback(pSelCallback)
{
	sTitle.Copy(szTitle);
	// key bindings
	pKeyRefresh = new C4KeyBinding(C4KeyCodeEx(K_F5), "FileSelReload", KEYSCOPE_Gui,
	                               new C4GUI::DlgKeyCB<C4FileSelDlg>(*this, &C4FileSelDlg::KeyRefresh), C4CustomKey::PRIO_CtrlOverride);
	pKeyEnterOverride = new C4KeyBinding(C4KeyCodeEx(K_RETURN), "FileSelConfirm", KEYSCOPE_Gui,
	                                     new C4GUI::DlgKeyCB<C4FileSelDlg>(*this, &C4FileSelDlg::KeyEnter), C4CustomKey::PRIO_CtrlOverride);
	if (fInitElements) InitElements();
	sPath.Copy(szRootPath);
}

void C4FileSelDlg::InitElements()
{
	UpdateSize();
	CStdFont *pUseFont = &(::GraphicsResource.TextFont);
	// main calcs
	bool fHasOptions = HasExtraOptions();
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,0, true);
	C4Rect rcOptions;
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(C4GUI_ButtonAreaHgt, 2*C4GUI_DefButton2Wdt+4*C4GUI_DefButton2HSpace),C4GUI_DefButton2HSpace,(C4GUI_ButtonAreaHgt-C4GUI_ButtonHgt)/2);
	if (fHasOptions) rcOptions = caMain.GetFromBottom(pUseFont->GetLineHeight() + 2*C4GUI_DefDlgSmallIndent);
	C4GUI::ComponentAligner caUpperArea(caMain.GetAll(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// create file selection area
	if (iLocationCount)
	{
		C4GUI::ComponentAligner caLocations(caUpperArea.GetFromTop(C4GUI::ComboBox::GetDefaultHeight() + 2*C4GUI_DefDlgSmallIndent), C4GUI_DefDlgIndent,C4GUI_DefDlgSmallIndent, false);
		StdStrBuf sText(LoadResStr("IDS_TEXT_LOCATION"));
		AddElement(new C4GUI::Label(sText.getData(), caLocations.GetFromLeft(pUseFont->GetTextWidth(sText.getData())), ALeft));
		pLocationComboBox = new C4GUI::ComboBox(caLocations.GetAll());
		pLocationComboBox->SetComboCB(new C4GUI::ComboBox_FillCallback<C4FileSelDlg>(this, &C4FileSelDlg::OnLocationComboFill, &C4FileSelDlg::OnLocationComboSelChange));
		pLocationComboBox->SetText(pLocations[0].sName.getData());
	}
	// create file selection area
	bool fHasPreview = HasPreviewArea();
	pFileListBox = new C4GUI::ListBox(fHasPreview ? caUpperArea.GetFromLeft(caUpperArea.GetWidth()/2) : caUpperArea.GetAll(), GetFileSelColWidth());
	pFileListBox ->SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<C4FileSelDlg>(this, &C4FileSelDlg::OnSelChange));
	pFileListBox ->SetSelectionDblClickFn(new C4GUI::CallbackHandler<C4FileSelDlg>(this, &C4FileSelDlg::OnSelDblClick));
	if (fHasPreview)
	{
		caUpperArea.ExpandLeft(C4GUI_DefDlgIndent);
		pSelectionInfoBox = new C4GUI::TextWindow(caUpperArea.GetAll());
		pSelectionInfoBox->SetDecoration(true, true, nullptr, true);
	}
	// create button area
	C4GUI::Button *btnAbort = new C4GUI::CancelButton(caButtonArea.GetFromRight(C4GUI_DefButton2Wdt));
	btnOK = new C4GUI::OKButton(caButtonArea.GetFromRight(C4GUI_DefButton2Wdt));
	// add components in tab order
	if (pLocationComboBox) AddElement(pLocationComboBox);
	AddElement(pFileListBox);
	if (pSelectionInfoBox) AddElement(pSelectionInfoBox);
	if (fHasOptions) AddExtraOptions(rcOptions);
	AddElement(btnOK);
	AddElement(btnAbort);
	SetFocus(pFileListBox, false);
	// no selection yet
	UpdateSelection();
}

C4FileSelDlg::~C4FileSelDlg()
{
	delete [] pLocations;
	if (pSelCallback) delete pSelCallback;
	delete pKeyEnterOverride;
	delete pKeyRefresh;
}

void C4FileSelDlg::OnLocationComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// Add all locations
	for (int32_t i=0; i<iLocationCount; ++i)
		pFiller->AddEntry(pLocations[i].sName.getData(), i);
}

bool C4FileSelDlg::OnLocationComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	SetCurrentLocation(idNewSelection, true);
	// No text change by caller; text alread changed by SetCurrentLocation
	return true;
}

void C4FileSelDlg::SetPath(const char *szNewPath, bool fRefresh)
{
	sPath.Copy(szNewPath);
	if (fRefresh && IsShown()) UpdateFileList();
}

void C4FileSelDlg::OnShown()
{
	BaseClass::OnShown();
	// load files
	UpdateFileList();
}

void C4FileSelDlg::UserClose(bool fOK)
{
	if (!fOK || pSelection)
	{
		// allow OK only if something is sth is selected
		Close(fOK);
	}
	else
	{
		GetScreen()->ShowErrorMessage(LoadResStr("IDS_ERR_PLEASESELECTAFILEFIRST"));
	}
}

void C4FileSelDlg::OnClosed(bool fOK)
{
	if (fOK && pSelection && pSelCallback)
		pSelCallback->OnFileSelected(pSelection->GetFilename());
	// base call: Might delete dlg
	BaseClass::OnClosed(fOK);
}

void C4FileSelDlg::OnSelDblClick(class C4GUI::Element *pEl)
{
	// item double-click: confirms with this file in single mode; toggles selection in multi mode
	if (IsMultiSelection())
	{
		ListItem *pItem = static_cast<ListItem *>(pEl);
		pItem->UserToggleCheck();
	}
	else
		UserClose(true);
}

C4FileSelDlg::ListItem *C4FileSelDlg::CreateListItem(const char *szFilename)
{
	// Default list item
	if (szFilename)
		return new DefaultListItem(szFilename, !!GetFileMask(), IsMultiSelection(), IsItemGrayed(szFilename), GetFileItemIcon());
	else
		return new DefaultListItem(nullptr, false, IsMultiSelection(), false, GetFileItemIcon());
}

void C4FileSelDlg::UpdateFileList()
{
	BeginFileListUpdate();
	// reload files
	C4GUI::Element *pEl;
	while ((pEl = pFileListBox->GetFirst())) delete pEl;
	// file items
	StdStrBuf sSearch;
	const char *szFileMask = GetFileMask();
	for (DirectoryIterator iter(sPath.getData()); *iter; ++iter)
		if (!szFileMask || WildcardListMatch(szFileMask, *iter))
			pFileListBox->AddElement(CreateListItem(*iter));
	// none-item?
	if (HasNoneItem())
	{
		pFileListBox->AddElement(CreateListItem(nullptr));
	}
	// list now done
	EndFileListUpdate();
	// path into title
	const char *szPath = sPath.getData();
	SetTitle(*szPath ? FormatString("%s [%s]", sTitle.getData(), szPath).getData() : sTitle.getData());
	// initial no-selection
	UpdateSelection();
}

void C4FileSelDlg::UpdateSelection()
{
	// update selection from list
	pSelection = static_cast<ListItem *>(pFileListBox->GetSelectedItem());
	// OK button only available if selection
	// SetEnabled would look a lot better here, but it doesn't exist yet :(
	// selection preview, if enabled
	if (pSelectionInfoBox)
	{
		// default empty
		pSelectionInfoBox->ClearText(false);
		if (!pSelection) { pSelectionInfoBox->UpdateHeight(); return; }
		// add selection description
		if (pSelection->GetFilename())
			pSelectionInfoBox->AddTextLine(pSelection->GetFilename(), &::GraphicsResource.TextFont, C4GUI_MessageFontClr, true, false);
	}
}

void C4FileSelDlg::SetSelection(const char *szNewSelection, bool fFilenameOnly)
{
	// check all selected definitions
	for (ListItem *pFileItem = static_cast<ListItem *>(pFileListBox->GetFirst()); pFileItem; pFileItem = static_cast<ListItem *>(pFileItem->GetNext()))
	{
		const char *szFileItemFilename = pFileItem->GetFilename();
		if (fFilenameOnly) szFileItemFilename = GetFilename(szFileItemFilename);
		pFileItem->SetChecked(SIsModule(szNewSelection, szFileItemFilename));
	}
}

StdStrBuf C4FileSelDlg::GetSelection(const char *szFixedSelection, bool fFilenameOnly) const
{
	StdStrBuf sResult;
	if (!IsMultiSelection())
	{
		// get single selected file for single selection dlg
		if (pSelection) sResult.Copy(fFilenameOnly ? GetFilename(pSelection->GetFilename()) : pSelection->GetFilename());
	}
	else
	{
		// force fixed selection first
		if (szFixedSelection) sResult.Append(szFixedSelection);
		//  get ';'-separated list for multi selection dlg
		for (ListItem *pFileItem = static_cast<ListItem *>(pFileListBox->GetFirst()); pFileItem; pFileItem = static_cast<ListItem *>(pFileItem->GetNext()))
			if (pFileItem->IsChecked())
			{
				const char *szAppendFilename = pFileItem->GetFilename();
				if (fFilenameOnly) szAppendFilename = GetFilename(szAppendFilename);
				// prevent adding entries twice (especially those from the fixed selection list)
				if (!SIsModule(sResult.getData(), szAppendFilename))
				{
					if (sResult.getLength()) sResult.AppendChar(';');
					sResult.Append(szAppendFilename);
				}
			}
	}
	return sResult;
}

void C4FileSelDlg::AddLocation(const char *szName, const char *szPath)
{
	// add to list
	int32_t iNewLocCount = iLocationCount+1;
	Location *pNewLocations = new Location[iNewLocCount];
	for (int32_t i=0; i<iLocationCount; ++i) pNewLocations[i] = pLocations[i];
	pNewLocations[iLocationCount].sName.Copy(szName);
	pNewLocations[iLocationCount].sPath.Copy(szPath);
	delete [] pLocations; pLocations = pNewLocations; iLocationCount = iNewLocCount;
	// first location? Then set path to this
	if (iLocationCount == 1) SetPath(szPath, false);
}

void C4FileSelDlg::AddCheckedLocation(const char *szName, const char *szPath)
{
	// check location
	// path must exit
	if (!szPath || !*szPath) return;
	if (!DirectoryExists(szPath)) return;
	// path must not be in list yet
	for (int32_t i=0; i<iLocationCount; ++i)
		if (ItemIdentical(szPath, pLocations[i].sPath.getData()))
			return;
	// OK; add it!
	AddLocation(szName, szPath);
}

int32_t C4FileSelDlg::GetCurrentLocationIndex() const
{
	return iCurrentLocationIndex;
}

void C4FileSelDlg::SetCurrentLocation(int32_t idx, bool fRefresh)
{
	// safety
	if (!Inside<int32_t>(idx, 0,iLocationCount)) return;
	// update ComboBox-text
	iCurrentLocationIndex = idx;
	if (pLocationComboBox) pLocationComboBox->SetText(pLocations[idx].sName.getData());
	// set new path
	SetPath(pLocations[idx].sPath.getData(), fRefresh);
}


// ---------------------------------------------------
// C4PlayerSelDlg

C4PlayerSelDlg::C4PlayerSelDlg(C4FileSel_BaseCB *pSelCallback)
		: C4FileSelDlg(Config.General.UserDataPath, LoadResStr("IDS_MSG_SELECTPLR"), pSelCallback)
{
}


// ---------------------------------------------------
// C4DefinitionSelDlg

C4DefinitionSelDlg::C4DefinitionSelDlg(C4FileSel_BaseCB *pSelCallback, const char *szFixedSelection)
		: C4FileSelDlg(Config.General.UserDataPath, FormatString(LoadResStr("IDS_MSG_SELECT"), LoadResStr("IDS_DLG_DEFINITIONS")).getData(), pSelCallback)
{
	if (szFixedSelection) sFixedSelection.Copy(szFixedSelection);
}

void C4DefinitionSelDlg::OnShown()
{
	// base call: load file list
	C4FileSelDlg::OnShown();
	// initial selection
	if (sFixedSelection) SetSelection(sFixedSelection.getData(), true);
}

bool C4DefinitionSelDlg::IsItemGrayed(const char *szFilename) const
{
	// cannot change initial selection
	if (!sFixedSelection) return false;
	return SIsModule(sFixedSelection.getData(), GetFilename(szFilename));
}

bool C4DefinitionSelDlg::SelectDefinitions(C4GUI::Screen *pOnScreen, StdStrBuf *pSelection)
{
	// let the user select definitions by showing a modal selection dialog
	C4DefinitionSelDlg *pDlg = new C4DefinitionSelDlg(nullptr, pSelection->getData());
	bool fResult;
	if ((fResult = pOnScreen->ShowModalDlg(pDlg, false)))
	{
		pSelection->Copy(pDlg->GetSelection(pSelection->getData(), true));
	}
	delete pDlg;
	return fResult;
}



// ---------------------------------------------------
// C4PortraitSelDlg::ListItem

C4PortraitSelDlg::ListItem::ListItem(const char *szFilename) : C4FileSelDlg::ListItem(szFilename)
		, fError(false), fLoaded(false)
{
	CStdFont *pUseFont = &(::GraphicsResource.MiniFont);
	// determine label text
	StdStrBuf sDisplayLabel;
	if (szFilename)
	{
		sDisplayLabel.Copy(::GetFilename(szFilename));
		::RemoveExtension(&sDisplayLabel);
	}
	else
	{
		sDisplayLabel.Ref(LoadResStr("IDS_MSG_NOPORTRAIT"));
	}
	// insert linebreaks into label text
	int32_t iLineHgt = std::max<int32_t>(pUseFont->BreakMessage(sDisplayLabel.getData(), ImagePreviewSize-6, &sFilenameLabelText, false), 1);
	// set size
	SetBounds(C4Rect(0,0,ImagePreviewSize,ImagePreviewSize+iLineHgt));
}

void C4PortraitSelDlg::ListItem::Load()
{
	if (sFilename)
	{
		// safety
		fLoaded = false;
		// load image file
		C4Group SrcGrp;
		StdStrBuf sParentPath;
		GetParentPath(sFilename.getData(), &sParentPath);
		bool fLoadError = true;
		if (SrcGrp.Open(sParentPath.getData()))
			if (fctLoadedImage.Load(SrcGrp, ::GetFilename(sFilename.getData()), C4FCT_Full, C4FCT_Full, false, 0))
			{
				// image loaded. Can only be put into facet by main thread, because those operations aren't thread safe
				fLoaded = true;
				fLoadError = false;
			}
		SrcGrp.Close();
		fError = fLoadError;
	}
}

void C4PortraitSelDlg::ListItem::DrawElement(C4TargetFacet &cgo)
{
	// Scale down newly loaded image?
	if (fLoaded)
	{
		fLoaded = false;
		if (!fctImage.CopyFromSfcMaxSize(fctLoadedImage.GetFace(), ImagePreviewSize))
			fError = true;
		fctLoadedImage.GetFace().Clear();
		fctLoadedImage.Clear();
	}
	// Draw picture
	CStdFont *pUseFont = &(::GraphicsResource.MiniFont);
	C4Facet cgoPicture(cgo.Surface, cgo.TargetX+rcBounds.x, cgo.TargetY+rcBounds.y, ImagePreviewSize, ImagePreviewSize);
	if (fError || !sFilename)
	{
		C4Facet &fctNoneImg = ::GraphicsResource.fctOKCancel;
		fctNoneImg.Draw(cgoPicture.Surface, cgoPicture.X+(cgoPicture.Wdt-fctNoneImg.Wdt)/2, cgoPicture.Y+(cgoPicture.Hgt-fctNoneImg.Hgt)/2, 1,0);
	}
	else
	{
		if (!fctImage.Surface)
		{
			// not loaded yet
			pDraw->TextOut(LoadResStr("IDS_PRC_INITIALIZE"), ::GraphicsResource.MiniFont, 1.0f, cgo.Surface, cgoPicture.X+cgoPicture.Wdt/2, cgoPicture.Y+(cgoPicture.Hgt-::GraphicsResource.MiniFont.GetLineHeight())/2, C4GUI_StatusFontClr, ACenter, false);
		}
		else
		{
			fctImage.Draw(cgoPicture);
		}
	}
	// draw filename
	pDraw->TextOut(sFilenameLabelText.getData(), *pUseFont, 1.0f, cgo.Surface, cgoPicture.X+rcBounds.Wdt/2, cgoPicture.Y+cgoPicture.Hgt, C4GUI_MessageFontClr, ACenter, false);
}


// ---------------------------------------------------
// C4PortraitSelDlg::LoaderThread

void C4PortraitSelDlg::LoaderThread::ClearLoadItems()
{
	// stop thread so list can be accessed
	Stop();
	// clear list
	LoadItems.clear();
}

void C4PortraitSelDlg::LoaderThread::AddLoadItem(ListItem *pItem)
{
	// not to be called when thread is running!
	assert(!IsStarted());
	LoadItems.push_back(pItem);
}

void C4PortraitSelDlg::LoaderThread::Execute()
{
	// list empty?
	if (!LoadItems.size())
	{
		// then we're done!
		SignalStop();
		return;
	}
	// load one item at the time
	ListItem *pLoadItem = LoadItems.front();
	pLoadItem->Load();
	LoadItems.erase(LoadItems.begin());
}

// ---------------------------------------------------
// C4PortraitSelDlg

C4PortraitSelDlg::C4PortraitSelDlg(C4FileSel_BaseCB *pSelCallback)
		: C4FileSelDlg(Config.General.SystemDataPath, FormatString(LoadResStr("IDS_MSG_SELECT"), LoadResStr("IDS_TYPE_PORTRAIT")).getData(), pSelCallback, false)
{
	char path[_MAX_PATH_LEN];
	// add common picture locations
	StdStrBuf strLocation;
	SCopy(Config.General.UserDataPath, path, _MAX_PATH); TruncateBackslash(path);
	strLocation.Format("%s %s", C4ENGINECAPTION, LoadResStr("IDS_TEXT_USERPATH"));
	AddLocation(strLocation.getData(), path);
	SCopy(Config.General.SystemDataPath, path, _MAX_PATH); TruncateBackslash(path);
	strLocation.Format("%s %s", C4ENGINECAPTION, LoadResStr("IDS_TEXT_PROGRAMDIRECTORY"));
	AddCheckedLocation(strLocation.getData(), path);
#ifdef _WIN32
	wchar_t wpath[MAX_PATH+1];
	if (SHGetSpecialFolderPathW(nullptr, wpath, CSIDL_PERSONAL, false)) AddCheckedLocation(LoadResStr("IDS_TEXT_MYDOCUMENTS"), StdStrBuf(wpath).getData());
	if (SHGetSpecialFolderPathW(nullptr, wpath, CSIDL_MYPICTURES, false)) AddCheckedLocation(LoadResStr("IDS_TEXT_MYPICTURES"), StdStrBuf(wpath).getData());
	if (SHGetSpecialFolderPathW(nullptr, wpath, CSIDL_DESKTOPDIRECTORY, false)) AddCheckedLocation(LoadResStr("IDS_TEXT_DESKTOP"), StdStrBuf(wpath).getData());
#endif
#ifdef __APPLE__
	AddCheckedLocation(LoadResStr("IDS_TEXT_HOME"), getenv("HOME"));
#else
	AddCheckedLocation(LoadResStr("IDS_TEXT_HOMEFOLDER"), getenv("HOME"));
#endif
#ifndef _WIN32
	sprintf(path, "%s%c%s", getenv("HOME"), (char)DirectorySeparator, (const char *)"Desktop");
	AddCheckedLocation(LoadResStr("IDS_TEXT_DESKTOP"), path);
#endif
	// build dialog
	InitElements();
	// select last location
	SetCurrentLocation(Config.Startup.LastPortraitFolderIdx, false);
}

void C4PortraitSelDlg::OnClosed(bool fOK)
{
	// remember location
	Config.Startup.LastPortraitFolderIdx = GetCurrentLocationIndex();
	// inherited
	C4FileSelDlg::OnClosed(fOK);
}

C4FileSelDlg::ListItem *C4PortraitSelDlg::CreateListItem(const char *szFilename)
{
	// use own list item type
	ListItem *pNew = new ListItem(szFilename);;
	// schedule image loading
	ImageLoader.AddLoadItem(pNew);
	return pNew;
}

void C4PortraitSelDlg::BeginFileListUpdate()
{
	// new file list. Stop loading current
	ImageLoader.ClearLoadItems();
}

void C4PortraitSelDlg::EndFileListUpdate()
{
#if USE_BACKGROUND_THREAD_LOAD
	// Begin loading images
	ImageLoader.Start();
#endif
}

void C4PortraitSelDlg::OnIdle()
{
#if !USE_BACKGROUND_THREAD_LOAD
	// no multithreading? Workaround for image loading then...
	static int32_t i = 0;
	if (!(i++%10)) ImageLoader.Execute();
#endif
}

bool C4PortraitSelDlg::SelectPortrait(C4GUI::Screen *pOnScreen, StdStrBuf *pSelection)
{
	// let the user select a portrait by showing a modal selection dialog
	C4PortraitSelDlg *pDlg = new C4PortraitSelDlg(nullptr);
	bool fResult;
	if ((fResult = pOnScreen->ShowModalDlg(pDlg, false)))
	{
		pSelection->Take(pDlg->GetSelection(nullptr, false));
	}
	delete pDlg;
	return fResult;
}
