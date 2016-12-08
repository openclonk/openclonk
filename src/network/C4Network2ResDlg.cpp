/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// resource display list box

#include "C4Include.h"
#include "network/C4Network2Dialogs.h"

#include "game/C4Application.h"
#include "gui/C4GameLobby.h"
#include "game/C4FullScreen.h"
#include "network/C4Network2.h"
#include "control/C4PlayerInfo.h"
#include "network/C4Network2Players.h"
#include "graphics/C4GraphicsResource.h"

// ----------- C4Network2ResDlg::ListItem ----------------------------------------------------------------

C4Network2ResDlg::ListItem::ListItem(C4Network2ResDlg *pForResDlg, const C4Network2Res *pByRes)
		: pSaveBtn(nullptr)
{
	// init by res core (2do)
	iResID = pByRes->getResID();
	const char *szFilename = GetFilename(pByRes->getCore().getFileName());
	// get size
	int iIconSize = ::GraphicsResource.TextFont.GetLineHeight();
	int iWidth = pForResDlg->GetItemWidth();
	int iVerticalIndent = 2;
	SetBounds(C4Rect(0, 0, iWidth, iIconSize+2*iVerticalIndent));
	C4GUI::ComponentAligner ca(GetContainedClientRect(), 0,iVerticalIndent);
	// create subcomponents
	pFileIcon = new C4GUI::Icon(ca.GetFromLeft(iIconSize), C4GUI::Ico_Resource);
	pLabel = new C4GUI::Label(szFilename, iIconSize + IconLabelSpacing,iVerticalIndent, ALeft);
	pProgress = nullptr;
	// add components
	AddElement(pFileIcon); AddElement(pLabel);
	// tooltip
	SetToolTip(LoadResStr("IDS_DESC_RESOURCE"));
	// add to listbox (will eventually get moved)
	pForResDlg->AddElement(this);
	// first-time update
	Update(pByRes);
}

void C4Network2ResDlg::ListItem::Update(const C4Network2Res *pByRes)
{
	// update progress label
	iProgress = pByRes->getPresentPercent();
	if (iProgress < 100)
	{
		StdStrBuf progress;
		progress.Format("%d%%", iProgress);
		if (pProgress)
			pProgress->SetText(progress.getData());
		else
		{
			pProgress = new C4GUI::Label(progress.getData(), GetBounds().Wdt - IconLabelSpacing, 0, ARight);
			pProgress->SetToolTip(LoadResStr("IDS_NET_RESPROGRESS_DESC"));
			AddElement(pProgress);
		}
	}
	else if (pProgress) { delete pProgress; pProgress=nullptr; }
	// update disk icon
	if (IsSavePossible())
	{
		if (!pSaveBtn)
		{
			pSaveBtn = new C4GUI::CallbackButtonEx<C4Network2ResDlg::ListItem, C4GUI::IconButton>(C4GUI::Ico_Save, GetToprightCornerRect(16,16,2,1), 0, this, &ListItem::OnButtonSave);
			AddElement(pSaveBtn);
		}
	}
	else if (pSaveBtn)
		delete pSaveBtn;
}

void C4Network2ResDlg::ListItem::OnButtonSave(C4GUI::Control *pButton)
{
	LocalSaveResource(false);
}

void C4Network2ResDlg::ListItem::OnButtonSaveConfirm(C4GUI::Element *pNull)
{
	LocalSaveResource(true);
}

void C4Network2ResDlg::ListItem::LocalSaveResource(bool fDoOverwrite)
{
	// get associated resource
	C4Network2Res::Ref pRes = GetRefRes();
	if (!pRes) return;
	const char *szResFile = pRes->getFile();
	StdCopyStrBuf strErrCopyFile(LoadResStr("IDS_NET_ERR_COPYFILE"));
	if (!pRes->isTempFile())
	{
		GetScreen()->ShowMessage(LoadResStr("IDS_NET_ERR_COPYFILE_LOCAL"), strErrCopyFile.getData(), C4GUI::Ico_Error);
		return;
	}
	const char *szFilename = GetFilename(pRes->getCore().getFileName());
	/*  const char *szSpecialPath = "";
	  if (WildcardMatch(C4CFN_PlayerFiles, szFilename))
	    // write players to player path
	    szSpecialPath = Config.General.PlayerPath;
	*/
	const char *szTarget = Config.AtUserDataPath(szFilename);
	if (!fDoOverwrite && ItemExists(szTarget))
	{
		// show a confirmation dlg, asking whether the resource should be overwritten
		GetScreen()->ShowRemoveDlg(new C4GUI::ConfirmationDialog(
		                             FormatString(LoadResStr("IDS_NET_RES_SAVE_OVERWRITE"), GetFilename(szTarget)).getData(), LoadResStr("IDS_NET_RES_SAVE"),
		                             new C4GUI::CallbackHandler<C4Network2ResDlg::ListItem>(this, &C4Network2ResDlg::ListItem::OnButtonSaveConfirm), C4GUI::MessageDialog::btnYesNo));
		return;
	}
	if (!C4Group_CopyItem(szResFile, szTarget))
		GetScreen()->ShowMessage(strErrCopyFile.getData(), strErrCopyFile.getData(), C4GUI::Ico_Error);
	else
	{
		GetScreen()->ShowMessage(FormatString(LoadResStr("IDS_NET_RES_SAVED_DESC"), GetFilename(szTarget)).getData(),
		                         LoadResStr("IDS_NET_RES_SAVED"), C4GUI::Ico_Save);
	}
}

C4Network2Res::Ref C4Network2ResDlg::ListItem::GetRefRes()
{
	// forward to network reslist
	return ::Network.ResList.getRefRes(iResID);
}

bool C4Network2ResDlg::ListItem::IsSavePossible()
{
	// check resource
	bool fCanSave = false;
	C4Network2Res::Ref pRes = GetRefRes();
	if (!pRes) return false;
	// temp files from network folder only
	if (pRes->isTempFile())
	{
		// check type
		C4Network2ResType eType = pRes->getType();
		if ((eType == NRT_Player && Config.Lobby.AllowPlayerSave) || eType == NRT_Scenario || eType == NRT_Definitions)
			// check complete
			if (!pRes->isLoading())
				// save OK
				fCanSave = true;
	}
	return fCanSave;
}


// ----------- C4Network2ResDlg -----------------------------------------------------------------------

C4Network2ResDlg::C4Network2ResDlg(const C4Rect &rcBounds, bool fActive) : ListBox(rcBounds)
{
	// 2do
	// initially active?
	if (fActive) Activate();
}

void C4Network2ResDlg::Activate()
{
	// create timer if necessary
	Application.Add(this);
	// force an update
	Update();
}

void C4Network2ResDlg::Deactivate()
{
	// release timer if set
	Application.Remove(this);
}

void C4Network2ResDlg::Update()
{
	// check through own resources and current res list
	ListItem *pItem = static_cast<ListItem *>(pClientWindow->GetFirst()), *pNext;
	C4Network2Res *pRes; int iResID=-1;
	while ((pRes = ::Network.ResList.getRefNextRes(++iResID)))
	{
		iResID = pRes->getResID();
		// resource checking: deleted resource(s) present?
		while (pItem && (pItem->GetResID() < iResID))
		{
			pNext = static_cast<ListItem *>(pItem->GetNext());
			delete pItem; pItem = pNext;
		}
		// same resource present for update?
		if (pItem && pItem->GetResID() == iResID)
		{
			pItem->Update(pRes);
			pItem = static_cast<ListItem *>(pItem->GetNext());
		}
		else
			// not present: insert (or add if pItem=nullptr)
			InsertElement(new ListItem(this, pRes), pItem);
	}
	// del trailing items
	while (pItem)
	{
		pNext = static_cast<ListItem *>(pItem->GetNext());
		delete pItem; pItem = pNext;
	}
}
