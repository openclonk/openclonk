/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
// HTTP download dialog; downloads a file

#include "C4Include.h"
#include "gui/C4DownloadDlg.h"

#include "lib/C4Log.h"
#include "graphics/C4GraphicsResource.h"

C4Network2HTTPClient HTTPClient;

C4DownloadDlg::C4DownloadDlg(const char *szDLType) : C4GUI::Dialog(C4GUI_ProgressDlgWdt, 100, FormatString(LoadResStr("IDS_CTL_DL_TITLE"), szDLType).getData(), false), szError(nullptr)
{
#ifdef HAVE_WINSOCK
	fWinSock = AcquireWinSock();
#endif
	// add all elements - will be reposisioned when text is displayed
	AddElement(pIcon = new C4GUI::Icon(C4Rect(), C4GUI::Ico_NetWait));
	AddElement(pStatusLabel = new C4GUI::Label("", C4Rect(), ACenter, C4GUI_MessageFontClr, &::GraphicsResource.TextFont, false));
	pProgressBar = nullptr; // created when necessary
	AddElement(pCancelBtn = new C4GUI::CancelButton(C4Rect()));
}

C4DownloadDlg::~C4DownloadDlg()
{
#ifdef HAVE_WINSOCK
	if (fWinSock) ReleaseWinSock();
#endif
}

void C4DownloadDlg::SetStatus(const char *szNewText, int32_t iProgressPercent)
{
	// get positions
	C4GUI::ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// place icon
	pIcon->SetBounds(caMain.GetFromLeft(C4GUI_IconWdt, C4GUI_IconWdt));
	// place message label
	// use text with line breaks
	StdStrBuf sMsgBroken;
	int iMsgHeight = ::GraphicsResource.TextFont.BreakMessage(szNewText, caMain.GetInnerWidth(), &sMsgBroken, true);
	pStatusLabel->SetBounds(caMain.GetFromTop(iMsgHeight));
	pStatusLabel->SetText(sMsgBroken.getData());
	// place progress bar
	if (iProgressPercent >= 0)
	{
		if (!pProgressBar)
		{
			AddElement(pProgressBar = new C4GUI::ProgressBar(caMain.GetFromTop(C4GUI_ProgressDlgPBHgt)));
		}
		else
		{
			pProgressBar->SetBounds(caMain.GetFromTop(C4GUI_ProgressDlgPBHgt));
		}
		pProgressBar->SetProgress(iProgressPercent);
	}
	else
	{
		// no progress desired
		if (pProgressBar) { delete pProgressBar; pProgressBar = nullptr; }
	}
	// place button
	caMain.ExpandLeft(C4GUI_DefDlgIndent*2 + C4GUI_IconWdt);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromTop(C4GUI_ButtonAreaHgt), 0,0);
	pCancelBtn->SetBounds(caButtonArea.GetCentered(C4GUI_DefButtonWdt, C4GUI_ButtonHgt));
	pCancelBtn->SetToolTip(LoadResStr("IDS_DL_CANCEL"));
	// resize to actually needed size
	SetClientSize(GetClientRect().Wdt, GetClientRect().Hgt - caMain.GetHeight());
}

void C4DownloadDlg::OnIdle()
{
	// continue query process
	if (!HTTPClient.Execute())
	{
		// query aborted
		Close(false);
		return;
	}
	if (!HTTPClient.isBusy())
	{
		// download done or aborted
		Close(HTTPClient.isSuccess());
		return;
	}
	StdStrBuf sStatus; int32_t iProgress = -1;
	StdStrBuf sSize("");
	// download in progress: Update status
	if (!HTTPClient.isConnected())
	{
		// still connecting
		sStatus.Ref(LoadResStr("IDS_DL_STATUSCONNECTING"));
	}
	else
	{
		// header received?
		size_t iSize = HTTPClient.getTotalSize();
		if (!iSize)
		{
			// file size unknown: No header received.
			sStatus.Ref(LoadResStr("IDS_PRC_CONNECTED"));
		}
		else
		{
			// file size known: Download in progress
			sStatus.Ref(LoadResStr("IDS_CTL_DL_PROGRESS"));
			if (iSize <= 1024)
				sSize.Format(" (%ld Bytes)", (long)iSize);
			else if (iSize <= 1024*1024)
				sSize.Format(" (%ld KB)", (long)(iSize/1024));
			else
				sSize.Format(" (%ld MB)", (long)(iSize/1024/1024));
			iProgress = int64_t(100) * HTTPClient.getDownloadedSize() / iSize;
		}
	}
	const char *szStatusString = LoadResStr("IDS_PRC_DOWNLOADINGFILE");
	SetStatus(FormatString(szStatusString, GetFilename(HTTPClient.getRequest())).getData(), iProgress );
}

void C4DownloadDlg::UserClose(bool fOK)
{
	// user cancelled
	HTTPClient.Cancel(LoadResStr("IDS_ERR_USERCANCEL"));
}

const char *C4DownloadDlg::GetError()
{
	// own error?
	if (szError) return szError;
	// fallback to HTTP error
	return HTTPClient.GetError();
}

bool C4DownloadDlg::ShowModal(C4GUI::Screen *pScreen, const char *szURL, const char *szSaveAsFilename)
{
	// reset error
	szError = nullptr;
	// initial text
	if (!HTTPClient.Init()) return false;
	HTTPClient.SetServer(szURL);
	// show dlg
	if (!Show(pScreen, true)) return false;
	// start query
	if (!HTTPClient.Query(nullptr, true)) return false;
	// first time status update
	OnIdle();
	// cycle until query is finished or aborted
	if (!DoModal()) return false;
	// download successful: Save file
	if (!HTTPClient.getResultBin().SaveToFile(szSaveAsFilename))
	{
		// file saving failed
		szError = LoadResStr("IDS_FAIL_SAVE");
		return false;
	}
	return true;
}

bool C4DownloadDlg::DownloadFile(const char *szDLType, C4GUI::Screen *pScreen, const char *szURL, const char *szSaveAsFilename, const char *szNotFoundMessage)
{
	// log it
	LogF(LoadResStr("IDS_PRC_DOWNLOADINGFILE"), szURL);
	// show download dialog
	C4DownloadDlg *pDlg = new C4DownloadDlg(szDLType);
	if (!pDlg->ShowModal(pScreen, szURL, szSaveAsFilename))
	{
		// show an appropriate error
		const char *szError = pDlg->GetError();
		if (!szError || !*szError) szError = LoadResStr("IDS_PRC_UNKOWNERROR");
		StdStrBuf sError;
		sError.Format(LoadResStr("IDS_PRC_DOWNLOADERROR"), GetFilename(szURL), szError);
		// it's a 404: display extended message
		if (SSearch(szError, "404") && szNotFoundMessage)
			{ sError.Append("|"); sError.Append(szNotFoundMessage); }
		// display message
		pScreen->ShowMessageModal(sError.getData(), FormatString(LoadResStr("IDS_CTL_DL_TITLE"), szDLType).getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error, nullptr);
		delete pDlg;
		return false;
	}
	LogF(LoadResStr("IDS_PRC_DOWNLOADCOMPLETE"), szURL);
	delete pDlg;
	return true;
}
