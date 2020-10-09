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
// Startup screen for non-parameterized engine start: Scenario selection dialog

#include "C4Include.h"
#include "gui/C4StartupScenSelDlg.h"

#include "c4group/C4ComponentHost.h"
#include "c4group/C4Components.h"
#include "game/C4Application.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4FileSelDlg.h"
#include "gui/C4GameDialogs.h"
#include "gui/C4GameOptions.h"
#include "gui/C4MouseControl.h"
#include "gui/C4StartupMainDlg.h"
#include "gui/C4StartupNetDlg.h"
#include "network/C4Network2Dialogs.h"

// singleton
C4StartupScenSelDlg *C4StartupScenSelDlg::pInstance=nullptr;


// ----------------------------------------------------------------
// Map folder data

void C4MapFolderData::Scenario::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt( sFilename,        "File"         , StdStrBuf()));
	pComp->Value(mkNamingAdapt( sBaseImage,       "BaseImage"    , StdStrBuf()));
	pComp->Value(mkNamingAdapt( sOverlayImage,    "OverlayImage" , StdStrBuf()));
	pComp->Value(mkNamingAdapt( rcOverlayPos,     "Area",          C4Rect()));
	pComp->Value(mkNamingAdapt( sTitle,           "Title"        , StdStrBuf()));
	pComp->Value(mkNamingAdapt( iTitleFontSize,   "TitleFontSize", 20));
	pComp->Value(mkNamingAdapt( dwTitleInactClr,  "TitleColorInactive", 0x7fffffffu));
	pComp->Value(mkNamingAdapt( dwTitleActClr,    "TitleColorActive",   0x0fffffffu));
	pComp->Value(mkNamingAdapt( iTitleOffX,       "TitleOffX",     0));
	pComp->Value(mkNamingAdapt( iTitleOffY,       "TitleOffY",     0));
	pComp->Value(mkNamingAdapt( byTitleAlign,     "TitleAlign",    ACenter));
	pComp->Value(mkNamingAdapt( fTitleBookFont,   "TitleUseBookFont", true));
	pComp->Value(mkNamingAdapt( fImgDump,         "ImageDump",     false)); // developer help
}

void C4MapFolderData::AccessGfx::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt( sPassword,        "Access",        StdStrBuf()));
	pComp->Value(mkNamingAdapt( sOverlayImage,    "OverlayImage" , StdStrBuf()));
	pComp->Value(mkNamingAdapt( rcOverlayPos,     "Area",          C4Rect()));
}

C4MapFolderData::MapPic::MapPic(const FLOAT_RECT &rcfBounds, const C4Facet &rfct) : C4GUI::Picture(C4Rect(rcfBounds), false), rcfBounds(rcfBounds)
{
	// ctor
	SetFacet(rfct);
	SetToolTip(LoadResStr("IDS_MSG_MAP_DESC"));
}

void C4MapFolderData::MapPic::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	typedef C4GUI::Picture Parent;
	Parent::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
	// input: mouse movement or buttons - deselect everything if clicked
	if (iButton == C4MC_Button_LeftDown && C4StartupScenSelDlg::pInstance)
	{
		C4StartupScenSelDlg::pInstance->DeselectAll();
	}
}

void C4MapFolderData::MapPic::DrawElement(C4TargetFacet &cgo)
{
	// get drawing bounds
	float x0 = rcfBounds.left + cgo.TargetX, y0 = rcfBounds.top + cgo.TargetY;
	// draw the image
	GetFacet().DrawXFloat(cgo.Surface, x0, y0, rcfBounds.right-rcfBounds.left, rcfBounds.bottom-rcfBounds.top);
}

void C4MapFolderData::Clear()
{
	fCoordinatesAdjusted = false;
	fctBackgroundPicture.Clear();
	pScenarioFolder = nullptr;
	pSelectedEntry = nullptr;
	pSelectionInfoBox = nullptr;
	rcScenInfoArea.Set(0,0,0,0);
	MinResX=MinResY=0;
	fUseFullscreenMap=false;
	int i;
	for (i=0; i<iScenCount; ++i) delete ppScenList[i];
	iScenCount=0;
	delete [] ppScenList; ppScenList=nullptr;
	for (i=0; i<iAccessGfxCount; ++i) delete ppAccessGfxList[i];
	iAccessGfxCount=0;
	delete [] ppAccessGfxList; ppAccessGfxList=nullptr;
	pMainDlg = nullptr;
}

bool C4MapFolderData::Load(C4Group &hGroup, C4ScenarioListLoader::Folder *pScenLoaderFolder)
{
	// clear previous
	Clear();
	// load localization info
	C4LangStringTable LangTable;
	bool fHasLangTable = C4Language::LoadComponentHost(&LangTable, hGroup, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
	// load core data
	StdStrBuf Buf;
	if (!hGroup.LoadEntryString(C4CFN_MapFolderData, &Buf)) return false;
	if (fHasLangTable) LangTable.ReplaceStrings(Buf);
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "FolderMap"), Buf, C4CFN_MapFolderData)) return false;
	// check resolution requirement
	if (MinResX && MinResX>C4GUI::GetScreenWdt()) return false;
	if (MinResY && MinResY>C4GUI::GetScreenHgt()) return false;
	// load images
	if (!fctBackgroundPicture.Load(hGroup, C4CFN_MapFolderBG, C4FCT_Full, C4FCT_Full, false, 0))
	{
		DebugLogF(R"(C4MapFolderData::Load(%s): Could not load background graphic "%s")", hGroup.GetName(), C4CFN_MapFolderBG);
		return false;
	}
	int i;
	for (i=0; i<iScenCount; ++i)
	{
		// init scenario entry stuff
		Scenario *pScen = ppScenList[i];
		pScen->pScenEntry = pScenLoaderFolder->FindEntryByName(pScen->sFilename.getData());
		pScen->pBtn = nullptr;
		pScen->sTitle.Replace("TITLE", pScen->pScenEntry ? pScen->pScenEntry->GetName().getData() : "<c ff0000>ERROR</c>" /* scenario not loaded; title cannot be referenced */);
		// developer image dump
		if (pScen->fImgDump)
		{
			C4FacetSurface fctDump; bool fSuccess=false;
			if (fctDump.Create(pScen->rcOverlayPos.Wdt, pScen->rcOverlayPos.Hgt, C4FCT_Full, C4FCT_Full))
			{
				pDraw->Blit(fctBackgroundPicture.Surface,
				              (float) pScen->rcOverlayPos.x, (float) pScen->rcOverlayPos.y,
				              (float) pScen->rcOverlayPos.Wdt, (float) pScen->rcOverlayPos.Hgt,
				              fctDump.Surface,
				              0, 0,
				              fctDump.Wdt, fctDump.Hgt);
				fSuccess = fctDump.Surface->SavePNG(pScen->sBaseImage.getData(), true, false, false);
			}
			if (!fSuccess)
				DebugLogF(R"(C4MapFolderData::Load(%s): Could not dump graphic "%s")", hGroup.GetName(), pScen->sBaseImage.getData());
			continue;
		}
		// load images
		if (pScen->sBaseImage.getLength()>0) if (!pScen->fctBase.Load(hGroup, pScen->sBaseImage.getData(), C4FCT_Full, C4FCT_Full, false, 0))
			{
				DebugLogF(R"(C4MapFolderData::Load(%s): Could not load base graphic "%s")", hGroup.GetName(), pScen->sBaseImage.getData());
				return false;
			}
		if (pScen->sOverlayImage.getLength()>0) if (!pScen->fctOverlay.Load(hGroup, pScen->sOverlayImage.getData(), C4FCT_Full, C4FCT_Full, false, 0))
			{
				DebugLogF(R"(C4MapFolderData::Load(%s): Could not load graphic "%s")", hGroup.GetName(), pScen->sOverlayImage.getData());
				return false;
			}
	}
	for (i=0; i<iAccessGfxCount; ++i)
	{
		AccessGfx *pGfx= ppAccessGfxList[i];
		if (pGfx->sOverlayImage.getLength()>0) if (!pGfx->fctOverlay.Load(hGroup, pGfx->sOverlayImage.getData(), C4FCT_Full, C4FCT_Full, false, 0))
			{
				DebugLogF(R"(C4MapFolderData::Load(%s): Could not load graphic "%s")", hGroup.GetName(), pGfx->sOverlayImage.getData());
				return false;
			}
	}
	// all loaded
	pScenarioFolder = pScenLoaderFolder;
	return true;
}

void C4MapFolderData::CompileFunc(StdCompiler *pComp)
{
	// core values
	pComp->Value(mkNamingAdapt( rcScenInfoArea, "ScenInfoArea", C4Rect(0,0,0,0)));
	pComp->Value(mkNamingAdapt( MinResX,        "MinResX",    0));
	pComp->Value(mkNamingAdapt( MinResY,        "MinResY",    0));
	pComp->Value(mkNamingAdapt( fUseFullscreenMap,"FullscreenBG",    false));
	// compile scenario list
	int32_t iOldScenCount = iScenCount;
	pComp->Value(mkNamingCountAdapt(iScenCount,  "Scenario"));
	if (pComp->isDeserializer())
	{
		while (iOldScenCount--) delete ppScenList[iOldScenCount];
		delete [] ppScenList;
		if (iScenCount)
		{
			ppScenList = new Scenario *[iScenCount];
			memset(ppScenList, 0, sizeof(Scenario *)*iScenCount);
		}
		else
			ppScenList = nullptr;
	}
	if (iScenCount)
	{
		mkPtrAdaptNoNull(*ppScenList);
		pComp->Value(mkNamingAdapt(mkArrayAdaptMap(ppScenList, iScenCount, mkPtrAdaptNoNull<Scenario>), "Scenario"));
	}
	// compile access gfx list
	int32_t iOldAccesGfxCount = iAccessGfxCount;
	pComp->Value(mkNamingCountAdapt(iAccessGfxCount,  "AccessGfx"));
	if (pComp->isDeserializer())
	{
		while (iOldAccesGfxCount--) delete ppAccessGfxList[iOldAccesGfxCount];
		delete [] ppAccessGfxList;
		if (iAccessGfxCount)
		{
			ppAccessGfxList = new AccessGfx *[iAccessGfxCount];
			memset(ppAccessGfxList, 0, sizeof(AccessGfx *)*iAccessGfxCount);
		}
		else
			ppAccessGfxList = nullptr;
	}
	if (iAccessGfxCount)
	{
		mkPtrAdaptNoNull(*ppAccessGfxList);
		pComp->Value(mkNamingAdapt(mkArrayAdaptMap(ppAccessGfxList, iAccessGfxCount, mkPtrAdaptNoNull<AccessGfx>), "AccessGfx"));
	}
}

void C4MapFolderData::ConvertFacet2ScreenCoord(const C4Rect &rc, FLOAT_RECT *pfrc, float fBGZoomX, float fBGZoomY, int iOffX, int iOffY)
{
	pfrc->left = (fBGZoomX * rc.x) + iOffX;
	pfrc->top = (fBGZoomY * rc.y) + iOffY;
	pfrc->right = pfrc->left + (fBGZoomX * rc.Wdt);
	pfrc->bottom = pfrc->top + (fBGZoomY * rc.Hgt);
}

void C4MapFolderData::ConvertFacet2ScreenCoord(int32_t *piValue, float fBGZoom, int iOff)
{
	*piValue = int32_t(floorf(fBGZoom * *piValue + 0.5f)) + iOff;
}

void C4MapFolderData::ConvertFacet2ScreenCoord(C4Rect &rcMapArea, bool fAspect)
{
	if (!fctBackgroundPicture.Wdt || !fctBackgroundPicture.Hgt) return; // invalid BG - should not happen
	// get zoom of background image
	float fBGZoomX = 1.0f, fBGZoomY = 1.0f; int iOffX=0, iOffY=0;
	if (fAspect)
	{
		if (fctBackgroundPicture.Wdt * rcMapArea.Hgt > rcMapArea.Wdt * fctBackgroundPicture.Hgt)
		{
			// background image is limited by width
			fBGZoomX = fBGZoomY = (float) rcMapArea.Wdt / fctBackgroundPicture.Wdt;
			iOffY = std::max<int>(0, (int)(rcMapArea.Hgt - (fBGZoomX * fctBackgroundPicture.Hgt)))/2;
		}
		else
		{
			// background image is limited by height
			fBGZoomX = fBGZoomY = (float) rcMapArea.Hgt / fctBackgroundPicture.Hgt;
			iOffX = std::max<int>(0, (int)(rcMapArea.Wdt - (fBGZoomY * fctBackgroundPicture.Wdt)))/2;
		}
	}
	else
	{
		// do not keep aspect: Independant X and Y zoom
		fBGZoomX = (float) rcMapArea.Wdt / fctBackgroundPicture.Wdt;;
		fBGZoomY = (float) rcMapArea.Hgt / fctBackgroundPicture.Hgt;;
	}
	iOffX -= rcMapArea.x; iOffY -= rcMapArea.y;
	C4Rect rcBG; rcBG.Set(0,0,fctBackgroundPicture.Wdt, fctBackgroundPicture.Hgt);
	ConvertFacet2ScreenCoord(rcBG, &rcfBG, fBGZoomX, fBGZoomY, iOffX, iOffY);
	// default for scenario info area: 1/3rd of right area
	if (!rcScenInfoArea.Wdt)
		rcScenInfoArea.Set((int32_t)(fctBackgroundPicture.Wdt*2/3), (int32_t)(fctBackgroundPicture.Hgt/16), (int32_t)(fctBackgroundPicture.Wdt/3), (int32_t)(fctBackgroundPicture.Hgt*7/8));
	// assume all facet coordinates are referring to background image zoom; convert them to screen coordinates by applying zoom and offset
	FLOAT_RECT rcfScenInfoArea;
	ConvertFacet2ScreenCoord(rcScenInfoArea, &rcfScenInfoArea, fBGZoomX, fBGZoomY, iOffX, iOffY);
	rcScenInfoArea.x = (int32_t) rcfScenInfoArea.left; rcScenInfoArea.y = (int32_t) rcfScenInfoArea.top;
	rcScenInfoArea.Wdt = (int32_t) (rcfScenInfoArea.right - rcfScenInfoArea.left);
	rcScenInfoArea.Hgt = (int32_t) (rcfScenInfoArea.bottom - rcfScenInfoArea.top);
	int i;
	for (i=0; i<iScenCount; ++i)
	{
		Scenario *pScen = ppScenList[i];
		ConvertFacet2ScreenCoord(pScen->rcOverlayPos, &(pScen->rcfOverlayPos), fBGZoomX, fBGZoomY, iOffX, iOffY);
		// title sizes
		ConvertFacet2ScreenCoord(&(pScen->iTitleFontSize), fBGZoomY, 0);
		// title position: Relative to title rect; so do not add offset here
		ConvertFacet2ScreenCoord(&(pScen->iTitleOffX), fBGZoomX, 0);
		ConvertFacet2ScreenCoord(&(pScen->iTitleOffY), fBGZoomY, 0);
	}
	for (i=0; i<iAccessGfxCount; ++i) ConvertFacet2ScreenCoord(ppAccessGfxList[i]->rcOverlayPos, &(ppAccessGfxList[i]->rcfOverlayPos), fBGZoomX, fBGZoomY, iOffX, iOffY);
	// done
	fCoordinatesAdjusted = true;
}

void C4MapFolderData::CreateGUIElements(C4StartupScenSelDlg *pMainDlg, C4GUI::Window &rContainer)
{
	this->pMainDlg = pMainDlg;
	// convert all coordinates to match the container sizes
	// do this only once; assume container won't change between loads
	if (!fCoordinatesAdjusted)
	{
		if (!fUseFullscreenMap)
			ConvertFacet2ScreenCoord(rContainer.GetClientRect(), true);
		else
		{
			C4Rect rcMapRect = pMainDlg->GetBounds();
			rContainer.ClientPos2ScreenPos(rcMapRect.x, rcMapRect.y);
			ConvertFacet2ScreenCoord(rcMapRect, false);
		}
	}
	// empty any previous stuff in container
	while (rContainer.GetFirst()) delete rContainer.GetFirst();
	// create background image
	if (!fUseFullscreenMap)
		rContainer.AddElement(new MapPic(rcfBG, fctBackgroundPicture));
	else
	{
		pMainDlg->SetBackground(&fctBackgroundPicture);
	}
	// create mission access overlays
	int i;
	for (i=0; i<iAccessGfxCount; ++i)
	{
		AccessGfx *pGfx = ppAccessGfxList[i];
		const char *szPassword = pGfx->sPassword.getData();
		if (!szPassword || !*szPassword || SIsModule(Config.General.MissionAccess, szPassword))
		{
			// ACCESS TO GFX GRANTED: draw it
			rContainer.AddElement(new MapPic(pGfx->rcfOverlayPos, pGfx->fctOverlay));
		}
	}
	// create buttons for scenarios
	C4GUI::Button *pBtnFirst = nullptr;
	for (i=0; i<iScenCount; ++i)
	{
		Scenario *pScen = ppScenList[i];
		if (pScen->pScenEntry && !pScen->pScenEntry->HasMissionAccess())
		{
			// no access to this scenario: Do not create a button at all; not even base image. The scenario is "invisible".
		}
		else
		{
			C4GUI::CallbackButtonEx<C4StartupScenSelDlg, C4GUI::FacetButton> *pBtn = new C4GUI::CallbackButtonEx<C4StartupScenSelDlg, C4GUI::FacetButton>
			(pScen->fctBase, pScen->fctOverlay, pScen->rcfOverlayPos, 0, pMainDlg, &C4StartupScenSelDlg::OnButtonScenario);
			ppScenList[i]->pBtn = pBtn;
			if (pScen->pScenEntry)
				pBtn->SetToolTip(FormatString(LoadResStr("IDS_MSG_MAP_STARTSCEN"), pScen->pScenEntry->GetName().getData()).getData());
			if (pScen->sTitle.getLength()>0)
			{
				pBtn->SetText(pScen->sTitle.getData());
				pBtn->SetTextColors(pScen->dwTitleInactClr, pScen->dwTitleActClr);
				pBtn->SetTextPos(pScen->iTitleOffX, pScen->iTitleOffY, pScen->byTitleAlign);
				CStdFont *pUseFont; float fFontZoom=1.0f;
				if (pScen->fTitleBookFont)
					pUseFont = &(C4Startup::Get()->Graphics.GetBlackFontByHeight(pScen->iTitleFontSize, &fFontZoom));
				else
					pUseFont = &(::GraphicsResource.GetFontByHeight(pScen->iTitleFontSize, &fFontZoom));
				if (Inside<float>(fFontZoom, 0.8f, 1.25f)) fFontZoom = 1.0f; // some tolerance for font zoom
				pBtn->SetTextFont(pUseFont, fFontZoom);
			}
			rContainer.AddElement(pBtn);
			if (!pBtnFirst) pBtnFirst = pBtn;
		}
	}
	// create scenario info listbox
	pSelectionInfoBox = new C4GUI::TextWindow(rcScenInfoArea,
	    C4StartupScenSel_TitlePictureWdt+2*C4StartupScenSel_TitleOverlayMargin, C4StartupScenSel_TitlePictureHgt+2*C4StartupScenSel_TitleOverlayMargin,
	    C4StartupScenSel_TitlePicturePadding, 100, 4096, nullptr, true, &C4Startup::Get()->Graphics.fctScenSelTitleOverlay, C4StartupScenSel_TitleOverlayMargin);
	pSelectionInfoBox->SetDecoration(false, false, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	rContainer.AddElement(pSelectionInfoBox);
}

void C4MapFolderData::OnButtonScenario(C4GUI::Control *pEl)
{
	// get associated scenario entry
	int i;
	for (i=0; i<iScenCount; ++i)
		if (pEl == ppScenList[i]->pBtn)
			break;
	if (i == iScenCount) return;
	// select the associated entry
	pSelectedEntry = ppScenList[i]->pScenEntry;
}

void C4MapFolderData::ResetSelection()
{
	pSelectedEntry = nullptr;
}



// ----------------------------------------------------------------
// Scenario list loader

// ------------------------------------
// Entry

C4ScenarioListLoader::Entry::Entry(class C4ScenarioListLoader *pLoader, Folder *pParent) : pLoader(pLoader), pNext(nullptr), pParent(pParent), fBaseLoaded(false), fExLoaded(false)
{
	// ctor: Put into parent tree node
	if (pParent)
	{
		pNext = pParent->pFirst;
		pParent->pFirst = this;
	}
	iIconIndex = -1;
	iDifficulty = 0;
	iFolderIndex = 0;
}

C4ScenarioListLoader::Entry::~Entry()
{
	// dtor: unlink from parent list (MUST be in there)
	if (pParent)
	{
		Entry **ppCheck = &(pParent->pFirst);
		while (*ppCheck != this)
		{
			ppCheck = &(*ppCheck)->pNext;
		}
		*ppCheck = pNext;
	}
}


bool C4ScenarioListLoader::Entry::Load(C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx)
{
	// nothing to do if already loaded
	if (fBaseLoaded && (fExLoaded || !fLoadEx)) return true;
	C4Group Group;
	// group specified: Load as child
	if (pFromGrp)
	{
		assert(psFilename);
		if (!Group.OpenAsChild(pFromGrp, psFilename->getData())) return false;
		// set FN by complete entry name
		this->sFilename.Take(Group.GetFullName());
	}
	else
	{
		// set FN by complete entry name
		if (psFilename) this->sFilename.Copy(*psFilename);
		// no parent group: Direct load from filename
		if (!Group.Open(sFilename.getData())) return false;
	}
	// okay; load standard stuff from group
	bool fNameLoaded=false, fIconLoaded=false;
	if (fBaseLoaded)
	{
		fNameLoaded = fIconLoaded = true;
	}
	else
	{
		// Set default name as filename without extension
		sName.Copy(GetFilename(sFilename.getData()));
		char *szBuf = sName.GrabPointer();
		RemoveExtension(szBuf);
		sName.Take(szBuf);
		// load entry specific stuff that's in the front of the group
		if (!LoadCustomPre(Group))
			return false;
		// Load entry name
		C4ComponentHost DefNames;
		if (C4Language::LoadComponentHost(&DefNames, Group, C4CFN_Title, Config.General.LanguageEx))
			if (DefNames.GetLanguageString(Config.General.LanguageEx, sName))
				fNameLoaded = true;
		// load entry icon
		if (Group.FindEntry(C4CFN_IconPNG) && fctIcon.Load(Group, C4CFN_IconPNG, C4FCT_Full, C4FCT_Full, false, 0))
			fIconLoaded = true;
		else
		{
			C4FacetSurface fctTemp;
			if (Group.FindEntry(C4CFN_ScenarioIcon) && fctTemp.Load(Group, C4CFN_ScenarioIcon, C4FCT_Full, C4FCT_Full, true, 0))
			{
				// old style icon: Blit it on a pieace of paper
				fctTemp.Surface->Lock();
				for (int y=0; y<fctTemp.Hgt; ++y)
					for (int x=0; x<fctTemp.Wdt; ++x)
					{
						uint32_t dwPix = fctTemp.Surface->GetPixDw(x,y, false);
						// transparency has some tolerance...
						if (Inside<uint8_t>(dwPix & 0xff, 0xb8, 0xff))
							if (Inside<uint8_t>((dwPix>>0x08) & 0xff, 0x00, 0x0f))
								if (Inside<uint8_t>((dwPix>>0x10) & 0xff, 0xb8, 0xff))
									fctTemp.Surface->SetPixDw(x,y,0x00ffffff);
					}
				fctTemp.Surface->Unlock();
				int iIconSize = C4Startup::Get()->Graphics.fctScenSelIcons.Hgt;
				fctIcon.Create(iIconSize, iIconSize, C4FCT_Full, C4FCT_Full);
				C4Startup::Get()->Graphics.fctScenSelIcons.GetPhase(C4StartupScenSel_DefaultIcon_OldIconBG).Draw(fctIcon);
				fctTemp.Draw(fctIcon.Surface, (fctIcon.Wdt-fctTemp.Wdt)/2, (fctIcon.Hgt-fctTemp.Hgt)/2);
				fctTemp.Clear();
				fIconLoaded = true;
			}
		}
		// load any entryx-type-specific custom data (e.g. fallbacks for scenario title, and icon)
		if (!LoadCustom(Group, fNameLoaded, fIconLoaded)) return false;
		fBaseLoaded = true;
	}
	// load extended stuff: title picture
	if (fLoadEx && !fExLoaded)
	{
		// load desc
		C4ComponentHost DefDesc;
		if (C4Language::LoadComponentHost(&DefDesc, Group, C4CFN_ScenarioDesc, Config.General.LanguageEx))
		{
			sDesc.Copy(DefDesc.GetData());
		}
		// load title
		fctTitle.Load(Group, C4CFN_ScenarioTitle,C4FCT_Full,C4FCT_Full,false,true);
		fExLoaded = true;
		// load version
		Group.LoadEntryString(C4CFN_Version, &sVersion);
	}
	// done, success
	return true;
}

// helper func: Recursive check whether a directory contains a .ocs or .ocf file
bool DirContainsScenarios(const char *szDir)
{
	// Ignore object and group folders to avoid descending e.g. deep into unpacked Objects.ocd
	if (WildcardMatch(C4CFN_DefFiles, szDir) || WildcardMatch(C4CFN_GenericGroupFiles, szDir))
	{
		return false;
	}
	// create iterator on free store to avoid stack overflow with deeply recursed folders
	DirectoryIterator *pIter = new DirectoryIterator(szDir);
	const char *szChildFilename;
	for (; (szChildFilename = **pIter); ++*pIter)
	{
		// Ignore directory navigation entries and CVS folders
		if (C4Group_TestIgnore(szChildFilename)) continue;
		if (WildcardMatch(C4CFN_ScenarioFiles, szChildFilename) || WildcardMatch(C4CFN_FolderFiles, szChildFilename)) break;
		if (DirectoryExists(szChildFilename))
			if (DirContainsScenarios(szChildFilename))
				break;
	}
	delete pIter;
	// return true if loop was broken, in which case a matching entry was found
	return !!szChildFilename;
}

C4ScenarioListLoader::Entry *C4ScenarioListLoader::Entry::CreateEntryForFile(const StdStrBuf &sFilename, C4ScenarioListLoader *pLoader, Folder *pParent)
{
	// determine entry type by file type
	const char *szFilename = sFilename.getData();
	if (!szFilename || !*szFilename) return nullptr;
	if (WildcardMatch(C4CFN_ScenarioFiles, sFilename.getData())) return new Scenario(pLoader, pParent);
	if (WildcardMatch(C4CFN_FolderFiles, sFilename.getData())) return new SubFolder(pLoader, pParent);
	// regular, open folder (C4Group-packed folders without extensions are not regarded, because they could contain anything!)
	const char *szExt = GetExtension(szFilename);
	if ((!szExt || !*szExt) && DirectoryExists(sFilename.getData()))
	{
		// do not open folders in the mod directory (and the dir itself - thus match minus the separator),
		// as contained files will be discovered anyway
		const char * modsDirectoryPrefix = Config.General.ModsDataPath;
		if (std::strncmp(szFilename, modsDirectoryPrefix, std::strlen(modsDirectoryPrefix) - 1) == 0)
			return nullptr;
		// open folders only if they contain a scenario or folder
		if (DirContainsScenarios(szFilename))
			return new RegularFolder(pLoader, pParent);
	}
	// type not recognized
	return nullptr;
}

bool C4ScenarioListLoader::Entry::RenameTo(const char *szNewName)
{
	// change name+filename
	// some name sanity validation
	if (!szNewName || !*szNewName) return false;
	if (SEqual(szNewName, sName.getData())) return true;
	char fn[_MAX_PATH_LEN];
	SCopy(szNewName, fn, _MAX_PATH);
	// generate new file name
	MakeFilenameFromTitle(fn);
	if (!*fn) return false;
	const char *szExt = GetDefaultExtension();
	if (szExt) { SAppend(".", fn, _MAX_PATH); SAppend(szExt, fn, _MAX_PATH); }
	char fullfn[_MAX_PATH_LEN];
	SCopy(sFilename.getData(), fullfn, _MAX_PATH);
	char *fullfn_fn = GetFilename(fullfn);
	SCopy(fn, fullfn_fn, _MAX_PATH - (fullfn_fn - fullfn));
	StdCopyStrBuf strErr(LoadResStr("IDS_FAIL_RENAME"));
	// check if a rename is due
	if (!ItemIdentical(sFilename.getData(), fullfn))
	{
		// check for duplicate filename
		if (ItemExists(fullfn))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_FILEEXISTS"), fullfn);
			::pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		// OK; then rename
		if (!C4Group_MoveItem(sFilename.getData(), fullfn, true))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_RENAMEFILE"), sFilename.getData(), fullfn);
			::pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		sFilename.Copy(fullfn);
	}
	// update real name in group, if this is a group
	if (C4Group_IsGroup(fullfn))
	{
		C4Group Grp;
		if (!Grp.Open(fullfn))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_OPENFILE"), sFilename.getData(), Grp.GetError());
			::pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		if (!Grp.Delete(C4CFN_Title))
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_DELOLDTITLE"), sFilename.getData(), Grp.GetError());
			::pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
		if (!SetTitleInGroup(Grp, szNewName)) return false;
		if (!Grp.Close())
		{
			StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_WRITENEWTITLE"), sFilename.getData(), Grp.GetError());
			::pGUI->ShowMessageModal(sMsg.getData(), strErr.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return false;
		}
	}
	// update title
	sName.Copy(szNewName);
	// done
	return true;
}

bool C4ScenarioListLoader::Entry::SetTitleInGroup(C4Group &rGrp, const char *szNewTitle)
{
	// default for group files: Create a title text file and set the title in there
	// no title needed if filename is sufficient - except for scenarios, where a Scenario.txt could overwrite the title
	if (!IsScenario())
	{
		StdStrBuf sNameByFile; sNameByFile.Copy(GetFilename(sFilename.getData()));
		char *szBuf = sNameByFile.GrabPointer();
		RemoveExtension(szBuf);
		sNameByFile.Take(szBuf);
		if (SEqual(szNewTitle, sNameByFile.getData())) return true;
	}
	// okay, make a title
	StdStrBuf sTitle; sTitle.Format("%s:%s", Config.General.Language, szNewTitle);
	if (!rGrp.Add(C4CFN_WriteTitle, sTitle, false, true))
	{
		StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_ERR_ERRORADDINGNEWTITLEFORFIL"), sFilename.getData(), rGrp.GetError());
		::pGUI->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_FAIL_RENAME"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		return false;
	}
	return true;
}


// ------------------------------------
// Scenario

bool C4ScenarioListLoader::Scenario::LoadCustomPre(C4Group &rGrp)
{
	// load scenario core first
	StdStrBuf sFileContents;
	if (!rGrp.LoadEntryString(C4CFN_ScenarioCore, &sFileContents)) return false;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkParAdapt(C4S, false), sFileContents, (rGrp.GetFullName() + DirSep C4CFN_ScenarioCore).getData()))
		return false;
	// Mission access
	fNoMissionAccess = (!C4S.Head.MissionAccess.empty() && !SIsModule(Config.General.MissionAccess, C4S.Head.MissionAccess.c_str()));
	// Localized parameter definitions. needed for achievements and parameter input boxes.
	// Only show them for "real" scenarios
	if (!C4S.Head.SaveGame && !C4S.Head.Replay)
	{
		// Skipping ahead in regular reading list, so keep other entries in memory
		rGrp.PreCacheEntries(C4CFN_AnyScriptStringTbl, true);
		rGrp.PreCacheEntries(C4CFN_ScenarioParameterDefs, true);
		C4LangStringTable ScenarioLangStringTable;
		C4Language::LoadComponentHost(&ScenarioLangStringTable, rGrp, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
		ParameterDefs.Load(rGrp, &ScenarioLangStringTable);
		// achievement images: Loaded from this entry and parent folder
		nAchievements = 0;
		const C4ScenarioParameterDefs *deflists[] = { pParent ? pParent->GetAchievementDefs() : nullptr, &ParameterDefs };
		for (auto deflist : deflists)
		{
			if (!deflist) continue;
			const C4ScenarioParameterDef *def; size_t idx=0;
			while ((def = deflist->GetParameterDefByIndex(idx++)))
			{
				if (def->IsAchievement())
				{
					int32_t val = pLoader->GetAchievements().GetValueByID(C4ScenarioParameters::AddFilename2ID(rGrp.GetFullName().getData(), def->GetID()).getData(), def->GetDefault());
					if (val)
					{
						// player has this achievement - find graphics for it
						const char *achievement_gfx = def->GetAchievement();
						StdStrBuf sAchievementFilename(C4CFN_Achievements);
						sAchievementFilename.Replace("*", achievement_gfx);
						// look in scenario
						if (!fctAchievements[nAchievements].Load(rGrp, sAchievementFilename.getData(), C4FCT_Height, C4FCT_Full, false, true))
						{
							// look in parent folder
							const C4FacetSurface *fct = nullptr;
							const C4AchievementGraphics *parent_achv_gfx;
							if (pParent && (parent_achv_gfx = pParent->GetAchievementGfx())) fct = parent_achv_gfx->FindByName(achievement_gfx);
							// look in main gfx group file
							if (!fct) fct = ::GraphicsResource.Achievements.FindByName(achievement_gfx);
							if (!fct) continue; // achievement graphics not found :(
							fctAchievements[nAchievements].Set((const C4Facet &)*fct);
						}
						// section by achievement index (1-based, since zero means no achievement)
						if (val>1) fctAchievements[nAchievements].X += fctAchievements[nAchievements].Wdt * (val-1);
						// description for this achievement is taken from option
						const C4ScenarioParameterDef::Option *opt = def->GetOptionByValue(val);
						if (opt) sAchievementDescriptions[nAchievements] = opt->Description;
						// keep track of achievement count
						++nAchievements;
						if (nAchievements == C4StartupScenSel_MaxAchievements) break;
					}
				}
			}
		}
	}
	return true;
}

bool C4ScenarioListLoader::Scenario::LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded)
{
	// icon fallback: Standard scenario icon
	if (!fIconLoaded)
	{
		iIconIndex = C4S.Head.Icon;
		fctIcon.Set(C4Startup::Get()->Graphics.fctScenSelIcons.GetSection(C4S.Head.Icon));
	}
	// scenario name fallback to core
	if (!fNameLoaded)
		sName = C4S.Head.Title;
	// difficulty: Set only for regular rounds (not savegame or record) to avoid bogus sorting
	if (!C4S.Head.SaveGame && !C4S.Head.Replay)
		iDifficulty = C4S.Head.Difficulty;
	else
		iDifficulty = 0;
	// minimum required player count
	iMinPlrCount = C4S.GetMinPlayer();
	return true;
}

bool C4ScenarioListLoader::Scenario::GetAchievement(int32_t idx, C4Facet *out_facet, const char **out_description)
{
	// return true and fill output parameters if player got the indexed achievement
	if (idx < 0 || idx >= nAchievements) return false;
	*out_facet = fctAchievements[idx];
	*out_description = sAchievementDescriptions[idx].getData();
	return true;
}

bool C4ScenarioListLoader::Scenario::Start()
{
	// gogo!
	if (!(C4StartupScenSelDlg::pInstance)) return false;
	return (C4StartupScenSelDlg::pInstance)->StartScenario(this);
}

bool C4ScenarioListLoader::Scenario::CanOpen(StdStrBuf &sErrOut, bool &CanHide)
{
	// safety
	C4StartupScenSelDlg *pDlg = C4StartupScenSelDlg::pInstance;
	if (!pDlg) return false;
	// check mission access
	if (!HasMissionAccess())
	{
		sErrOut.Copy(LoadResStr("IDS_PRC_NOMISSIONACCESS"));
		return false;
	}
	// replay
	if (C4S.Head.Replay)
	{
		// replays can currently not be launched in network mode
		if (pDlg->IsNetworkStart())
		{
			sErrOut.Copy(LoadResStr("IDS_PRC_NONETREPLAY"));
			return false;
		}
	}
	// regular game
	else
	{
		// check player count
		int32_t iPlrCount = SModuleCount(Config.General.Participants);
		int32_t iMaxPlrCount = C4S.Head.MaxPlayer;
		if (C4S.Head.SaveGame)
		{
			// Some scenarios have adjusted MaxPlayerCount to 0 after starting to prevent future joins
			// make sure it's possible to start the savegame anyway
			iMaxPlrCount = std::max<int32_t>(iMinPlrCount, iMaxPlrCount);

			// <Sven2> Savegames store a lot of internal stuff. If you updated clonk in the meantime, many things tend to break
			if (C4S.Head.C4XVer[0] != C4XVER1 || C4S.Head.C4XVer[1] != C4XVER2)
			{
				// Only show a warning to let players try it anyways.
				sErrOut.Format(LoadResStr("IDS_MSG_SAVEGAMEVERSIONMISMATCH"), C4S.Head.C4XVer[0], C4S.Head.C4XVer[1]);
				CanHide = false;
			}
		}
		// normal scenarios: At least one player except in network mode, where it is possible to wait for the additional players
		// Melees need at least two
		if ((iPlrCount < iMinPlrCount))
		{
			if (pDlg->IsNetworkStart())
			{
				// network game: Players may yet join in lobby
				// only issue a warning for too few players (by setting the error but not returning false here)
				sErrOut.Format(LoadResStr("IDS_MSG_TOOFEWPLAYERSNET"), (int) iMinPlrCount);
				CanHide = true;
			}
			else
			{
				// for regular games, this is a fatal no-start-cause
				sErrOut.Format(LoadResStr("IDS_MSG_TOOFEWPLAYERS"), (int) iMinPlrCount);
				return false;
			}
		}
		// scenarios (both normal and savegame) may also impose a maximum player restriction
		if (iPlrCount > iMaxPlrCount)
		{
			sErrOut.Format(LoadResStr("IDS_MSG_TOOMANYPLAYERS"), (int) C4S.Head.MaxPlayer);
			return false;
		}
	}
	// Okay, start!
	return true;
}

StdStrBuf C4ScenarioListLoader::Scenario::GetOpenText()
{
	return StdCopyStrBuf(LoadResStr("IDS_BTN_STARTGAME"));
}

StdStrBuf C4ScenarioListLoader::Scenario::GetOpenTooltip()
{
	return StdCopyStrBuf(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));
}


// ------------------------------------
// Folder

C4ScenarioListLoader::Folder::~Folder()
{
	if (pMapData) delete pMapData;
	ClearChildren();
}

bool C4ScenarioListLoader::Folder::Start()
{
	// open as subfolder
	if (!C4StartupScenSelDlg::pInstance) return false;
	return C4StartupScenSelDlg::pInstance->OpenFolder(this);
}
int
#ifdef _MSC_VER
__cdecl
#endif
EntrySortFunc(const void *pEl1, const void *pEl2)
{
	C4ScenarioListLoader::Entry *pEntry1 = *(C4ScenarioListLoader::Entry * const *) pEl1, *pEntry2 = *(C4ScenarioListLoader::Entry * const *) pEl2;
	// sort folders before scenarios
	bool fS1 = !pEntry1->GetIsFolder(), fS2 = !pEntry2->GetIsFolder();
	if (fS1 != fS2) return fS1-fS2;
	// sort by folder index (undefined index 0 goes to the end)
	if (!Config.Startup.AlphabeticalSorting) if (pEntry1->GetFolderIndex() || pEntry2->GetFolderIndex())
		{
			if (!pEntry1->GetFolderIndex()) return +1;
			if (!pEntry2->GetFolderIndex()) return -1;
			int32_t iDiff = pEntry1->GetFolderIndex() - pEntry2->GetFolderIndex();
			if (iDiff) return iDiff;
		}
	// sort by numbered standard scenario icons
	if (Inside(pEntry1->GetIconIndex(), 2, 11))
	{
		int32_t iDiff = pEntry1->GetIconIndex() - pEntry2->GetIconIndex();
		if (iDiff) return iDiff;
	}
	// sort by difficulty (undefined difficulty goes to the end)
	if (!Config.Startup.AlphabeticalSorting) if (pEntry1->GetDifficulty() || pEntry2->GetDifficulty())
		{
			if (!pEntry1->GetDifficulty()) return +1;
			if (!pEntry2->GetDifficulty()) return -1;
			int32_t iDiff = pEntry1->GetDifficulty() - pEntry2->GetDifficulty();
			if (iDiff) return iDiff;
		}
	// otherwise, sort by name
	return stricmp(pEntry1->GetName().getData(), pEntry2->GetName().getData());
}

uint32_t C4ScenarioListLoader::Folder::GetEntryCount() const
{
	uint32_t iCount = 0;
	for (Entry *i = pFirst; i; i = i->pNext) ++iCount;
	return iCount;
}

void C4ScenarioListLoader::Folder::Sort()
{
	// use C-Library-QSort on a buffer of entry pointers; then re-link list
	if (!pFirst) return;
	uint32_t iCount,i;
	Entry **ppEntries = new Entry *[i = iCount = GetEntryCount()], **ppI, *pI=pFirst, **ppIThis;
	for (ppI = ppEntries; i--; pI = pI->pNext) *ppI++ = pI;
	qsort(ppEntries, iCount, sizeof(Entry *), &EntrySortFunc);
	ppIThis = &pFirst;
	for (ppI = ppEntries; iCount--; ppIThis = &((*ppIThis)->pNext)) *ppIThis = *ppI++;
	*ppIThis = nullptr;
	delete [] ppEntries;
}

void C4ScenarioListLoader::Folder::ClearChildren()
{
	// folder deletion: del all the tree non-recursively
	Folder *pDelFolder = this, *pCheckFolder;
	for (;;)
	{
		// delete all children as long as they are not folders
		Entry *pChild;
		while ((pChild = pDelFolder->pFirst))
			if ((pCheckFolder = pChild->GetIsFolder()))
				// child entry if folder: Continue delete in there
				pDelFolder = pCheckFolder;
			else
				// regular child entry: del it
				// destructor of child will remove it from list
				delete pChild;
		// this emptied: Done!
		if (pDelFolder == this) break;
		// deepest child recursion reached: Travel up folders
		pDelFolder = (pCheckFolder = pDelFolder)->pParent;
		assert(pDelFolder);
		delete pCheckFolder;
	}
}

bool C4ScenarioListLoader::Folder::LoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx, bool fReload)
{
	// contents already loaded?
	if (fContentsLoaded && !fReload) return true;
	// clear previous
	if (pMapData) { delete pMapData; pMapData = nullptr; }
	// if filename is not given, assume it's been loaded in this entry
	if (!psFilename) psFilename = &this->sFilename; else this->sFilename = *psFilename;
	// nothing loaded: Load now
	if (!DoLoadContents(pLoader, pFromGrp, *psFilename, fLoadEx)) return false;
	// sort loaded stuff by name
	Sort();
	return true;
}

C4ScenarioListLoader::Entry *C4ScenarioListLoader::Folder::FindEntryByName(const char *szFilename) const
{
	// do a case-insensitive filename comparison
	for (Entry *pEntry = pFirst; pEntry; pEntry = pEntry->GetNext())
		if (SEqualNoCase(szFilename, GetFilename(pEntry->GetEntryFilename().getData())))
			return pEntry;
	// nothing found
	return nullptr;
}

StdStrBuf C4ScenarioListLoader::Folder::GetOpenText()
{
	return StdCopyStrBuf(LoadResStr("IDS_BTN_OPEN"));
}

StdStrBuf C4ScenarioListLoader::Folder::GetOpenTooltip()
{
	return StdCopyStrBuf(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));
}

bool C4ScenarioListLoader::Folder::IsGrayed()
{
	return false;
}

bool C4ScenarioListLoader::Folder::LoadCustomPre(C4Group &rGrp)
{
	// load folder core if available
	StdStrBuf sFileContents;
	if (rGrp.LoadEntryString(C4CFN_FolderCore, &sFileContents))
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(C4F, sFileContents, (rGrp.GetFullName() + DirSep C4CFN_FolderCore).getData()))
			return false;
	return true;
}

// ------------------------------------
// SubFolder

bool C4ScenarioListLoader::SubFolder::LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded)
{
	// default icon fallback
	if (!fIconLoaded)
	{
		if(WildcardMatch(C4CFN_Savegames, GetFilename(sFilename.getData()))) iIconIndex = C4StartupScenSel_DefaultIcon_SavegamesFolder;
		else iIconIndex = C4StartupScenSel_DefaultIcon_Folder;
		fctIcon.Set(C4Startup::Get()->Graphics.fctScenSelIcons.GetSection(iIconIndex));	
	}
	// folder index
	iFolderIndex = C4F.Head.Index;
	return true;
}

bool C4ScenarioListLoader::SubFolder::DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx)
{
	assert(pLoader);
	// clear any previous
	ClearChildren();
	// group specified: Load as child
	C4Group Group;
	if (pFromGrp)
	{
		if (!Group.OpenAsChild(pFromGrp, sFilename.getData())) return false;
	}
	else
		// no parent group: Direct load from filename
		if (!Group.Open(sFilename.getData())) return false;
	// Load achievement data contained scenarios can fall back to
	C4LangStringTable FolderLangStringTable;
	C4Language::LoadComponentHost(&FolderLangStringTable, Group, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
	AchievementDefs.Load(Group, &FolderLangStringTable);
	AchievementGfx.Init(Group);
	// get number of entries, to estimate progress
	const char *szC4CFN_ScenarioFiles = C4CFN_ScenarioFiles; // assign values for constant comparison
	const char *szSearchMask; int32_t iEntryCount=0;
	for (szSearchMask = szC4CFN_ScenarioFiles; szSearchMask;)
	{
		Group.ResetSearch();
		while (Group.FindNextEntry(szSearchMask)) ++iEntryCount;
		// next search mask
		if (szSearchMask == szC4CFN_ScenarioFiles)
			szSearchMask = C4CFN_FolderFiles;
		else
			szSearchMask = nullptr;
	}
	// initial progress estimate
	if (!pLoader->DoProcessCallback(0, iEntryCount, nullptr)) return false;
	// iterate through group contents
	char ChildFilename[_MAX_FNAME_LEN]; StdStrBuf sChildFilename; int32_t iLoadCount=0;
	for (szSearchMask = szC4CFN_ScenarioFiles; szSearchMask;)
	{
		Group.ResetSearch();
		while (Group.FindNextEntry(szSearchMask, ChildFilename))
		{
			// mark progress
			if (!pLoader->DoProcessCallback(iLoadCount, iEntryCount, ChildFilename)) return false;
			sChildFilename.Ref(ChildFilename);
			// okay; create this item
			Entry *pNewEntry = Entry::CreateEntryForFile(sChildFilename, pLoader, this);
			if (pNewEntry)
			{
				// ...and load it
				if (!pNewEntry->Load(&Group, &sChildFilename, fLoadEx))
				{
					DebugLogF(R"(Error loading entry "%s" in SubFolder "%s"!)", sChildFilename.getData(), Group.GetFullName().getData());
					delete pNewEntry;
				}
			}
			++iLoadCount;
		}
		// next search mask
		if (szSearchMask == szC4CFN_ScenarioFiles)
			szSearchMask = C4CFN_FolderFiles;
		else
			szSearchMask = nullptr;
	}
	// load map folder data
	if (Group.FindEntry(C4CFN_MapFolderData))
	{
		pMapData = new C4MapFolderData();
		if (!pMapData->Load(Group, this))
		{
			// load error :(
			delete pMapData;
			pMapData = nullptr;
		}
	}
	// done, success
	fContentsLoaded = true;
	return true;
}


// ------------------------------------
// RegularFolder

C4ScenarioListLoader::RegularFolder::~RegularFolder() = default;

bool C4ScenarioListLoader::RegularFolder::LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded)
{
	// default icon fallback
	if (!fIconLoaded)
		fctIcon.Set(C4Startup::Get()->Graphics.fctScenSelIcons.GetSection(C4StartupScenSel_DefaultIcon_WinFolder));
	// folder index
	iFolderIndex = C4F.Head.Index;
	return true;
}

bool C4ScenarioListLoader::RegularFolder::DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx)
{
	// clear any previous
	ClearChildren();
	// regular folders must exist and not be within group!
	assert(!pFromGrp);
	if (sFilename.getData() && sFilename[0])
		Merge(sFilename.getData());

	// get number of entries, to estimate progress
	int32_t iCountLoaded=0, iCountTotal=0;
	NameList::iterator it;
	for (it = contents.begin(); it != contents.end(); ++it)
	{
		if (!DirectoryExists(it->c_str())) continue;
		DirectoryIterator DirIter(it->c_str());
		const char *szChildFilename;
		for (; (szChildFilename = *DirIter); ++DirIter)
		{
			if (!*szChildFilename || *GetFilename(szChildFilename)=='.') continue;
			++iCountTotal;
		}
	}
	// initial progress estimate
	if (!pLoader->DoProcessCallback(iCountLoaded, iCountTotal, nullptr)) return false;

	// do actual loading of files
	std::set<std::string> names;
	const char *szChildFilename;
	for (it = contents.begin(); it != contents.end(); ++it)
	{
		if (!pLoader->DoProcessCallback(iCountLoaded, iCountTotal, GetFilename(it->c_str()))) return false;
		for (DirectoryIterator DirIter(it->c_str()); (szChildFilename = *DirIter); ++DirIter)
		{
			StdStrBuf sChildFilename(szChildFilename);
			szChildFilename = GetFilename(szChildFilename);
			// progress callback
			if (!pLoader->DoProcessCallback(iCountLoaded, iCountTotal, szChildFilename)) return false;
			// Ignore directory navigation entries and CVS folders
			if (C4Group_TestIgnore(szChildFilename)) continue;
			if (names.find(szChildFilename) != names.end()) continue;
			names.insert(szChildFilename);
			// filename okay; create this item
			Entry *pNewEntry = Entry::CreateEntryForFile(sChildFilename, pLoader, this);
			if (pNewEntry)
			{
				// ...and load it
				if (!pNewEntry->Load(nullptr, &sChildFilename, fLoadEx))
				{
					DebugLogF(R"(Error loading entry "%s" in Folder "%s"!)", szChildFilename, it->c_str());
					delete pNewEntry;
				}
			}
			++iCountLoaded;
		}
	}
	// done, success
	fContentsLoaded = true;
	return true;
}

void C4ScenarioListLoader::RegularFolder::Merge(const char *szPath)
{
	contents.emplace_back(szPath);
}

// ------------------------------------
// C4ScenarioListLoader

C4ScenarioListLoader::C4ScenarioListLoader(const C4ScenarioParameters &Achievements) : Achievements(Achievements), pRootFolder(nullptr), pCurrFolder(nullptr),
		iLoading(0), iProgress(0), iMaxProgress(0), fAbortThis(false), fAbortPrevious(false)
{
}

C4ScenarioListLoader::~C4ScenarioListLoader()
{
	if (pRootFolder) delete pRootFolder;
}

bool C4ScenarioListLoader::BeginActivity(bool fAbortPrevious)
{
	// if previous activities were running, stop them first if desired
	if (iLoading && fAbortPrevious)
		this->fAbortPrevious = true;
	// mark this activity
	++iLoading;
	// progress of activity not yet decided
	iProgress = iMaxProgress = 0;
	current_load_info.Clear();
	// okay; start activity
	return true;
}

void C4ScenarioListLoader::EndActivity()
{
	assert(iLoading);
	if (!--iLoading)
	{
		// last activity done: Reset any flags
		fAbortThis = false;
		fAbortPrevious = false;
		iProgress = iMaxProgress = 0;
		current_load_info.Clear();
	}
	else
	{
		// child activity done: Transfer abort flag for next activity
		fAbortThis = fAbortPrevious;
	}
}

bool C4ScenarioListLoader::DoProcessCallback(int32_t iProgress, int32_t iMaxProgress, const char *current_load_info)
{
	this->iProgress = iProgress;
	this->iMaxProgress = iMaxProgress;
	this->current_load_info.Copy(current_load_info);
	// callback to dialog
	if (C4StartupScenSelDlg::pInstance) C4StartupScenSelDlg::pInstance->ProcessCallback();
	// process callback - abort at a few ugly circumstances...
	// schedule with 1ms delay to force event processing
	// (delay 0 would be nice, but isn't supported properly by our Windows implementation of ScheduleProcs)
	if (!Application.ScheduleProcs(1) // WM_QUIT message?
	    || !C4StartupScenSelDlg::pInstance // host dialog removed?
	    || !C4StartupScenSelDlg::pInstance->IsShown() // host dialog closed?
	   ) return false;
	// and also abort if flagged
	return !fAbortThis;
}

bool C4ScenarioListLoader::Load(const StdStrBuf &sRootFolder)
{
	// (unthreaded) loading of all entries in root folder
	if (!BeginActivity(true)) return false;
	if (pRootFolder) { delete pRootFolder; pRootFolder = nullptr; }
	pCurrFolder = pRootFolder = new RegularFolder(this, nullptr);
	// Load regular game data if no explicit path specified
	if(!sRootFolder.getData())
		for(const auto & iter : Reloc)
			pRootFolder->Merge(iter.strBuf.getData());
	bool fSuccess = pRootFolder->LoadContents(this, nullptr, &sRootFolder, false, false);
	EndActivity();
	return fSuccess;
}

bool C4ScenarioListLoader::Load(Folder *pSpecifiedFolder, bool fReload)
{
	// call safety
	if (!pRootFolder || !pSpecifiedFolder) return false;
	// set new current and load it
	if (!BeginActivity(true)) return false;
	pCurrFolder = pSpecifiedFolder;
	bool fSuccess = pCurrFolder->LoadContents(this, nullptr, nullptr, false, fReload);
	EndActivity();
	return fSuccess;
}

bool C4ScenarioListLoader::LoadExtended(Entry *pEntry)
{
	// call safety
	if (!pRootFolder || !pEntry) return false;
	// load info of selection
	if (!BeginActivity(false)) return false;
	bool fSuccess = pEntry->Load(nullptr, nullptr, true);
	EndActivity();
	return fSuccess;
}

bool C4ScenarioListLoader::FolderBack()
{
	// call safety
	if (!pRootFolder || !pCurrFolder) return false;
	// already in root: Can't go up
	if (pCurrFolder == pRootFolder) return false;
	// otherwise, up one level
	return Load(pCurrFolder->GetParent(), false);
}

bool C4ScenarioListLoader::ReloadCurrent()
{
	// call safety
	if (!pRootFolder || !pCurrFolder) return false;
	// reload current
	return Load(pCurrFolder, true);
}






// ----------------------------------------------------------------
// Scenario selection GUI


// font clrs
const uint32_t ClrScenarioItem   = 0xff000000,
                                   ClrScenarioItemXtra = 0x7f000000,
                                                         ClrScenarioItemDisabled = 0x7f000000;

// ------------------------------------------------
// --- C4StartupScenSelDlg::ScenListItem
C4StartupScenSelDlg::ScenListItem::ScenListItem(C4GUI::ListBox *pForListBox, C4ScenarioListLoader::Entry *pForEntry, C4GUI::Element *pInsertBeforeElement)
		: pIcon(nullptr), pNameLabel(nullptr), pScenListEntry(pForEntry)
{
	assert(pScenListEntry);
	CStdFont &rUseFont = C4Startup::Get()->Graphics.BookFont;
	StdStrBuf sIgnore; bool bIgnore;
	bool fEnabled = pScenListEntry->CanOpen(sIgnore, bIgnore) && !pScenListEntry->IsGrayed();
	// calc height
	int32_t iHeight = rUseFont.GetLineHeight() + 2 * IconLabelSpacing;
	// create subcomponents
	pIcon = new C4GUI::Picture(C4Rect(0, 0, iHeight, iHeight), true);
	pIcon->SetFacet(pScenListEntry->GetIconFacet());
	pNameLabel = new C4GUI::Label(pScenListEntry->GetName().getData(), iHeight + IconLabelSpacing, IconLabelSpacing, ALeft, fEnabled ? ClrScenarioItem : ClrScenarioItemDisabled, &rUseFont, false, false);
	// achievement components
	for (int32_t i=0; i<C4StartupScenSel_MaxAchievements; ++i)
	{
		C4Facet fct; const char *desc;
		if (pForEntry->GetAchievement(i, &fct, &desc))
		{
			ppAchievements[i] = new C4GUI::Picture(C4Rect(iHeight * (i+2), 0, iHeight, iHeight), true); // position will be adjusted later
			ppAchievements[i]->SetFacet(fct);
			ppAchievements[i]->SetToolTip(desc);
		}
		else
		{
			ppAchievements[i] = nullptr;
		}
	}
	// calc own bounds - use icon bounds only, because only the height is used when the item is added
	SetBounds(pIcon->GetBounds());
	// add components
	AddElement(pIcon); AddElement(pNameLabel);
	for (auto & ppAchievement : ppAchievements) if (ppAchievement) AddElement(ppAchievement);
	// tooltip by name, so long names can be read via tooltip
	SetToolTip(pScenListEntry->GetName().getData());
	// add to listbox (will get resized horizontally and moved) - zero indent; no tree structure in this dialog
	pForListBox->InsertElement(this, pInsertBeforeElement, 0);
	// update name label width to reflect new horizontal size
	// name label width must be set so rename edit will take its size
	pNameLabel->SetAutosize(false);
	C4Rect rcNLB = pNameLabel->GetBounds(); rcNLB.Wdt = GetClientRect().Wdt - rcNLB.x - IconLabelSpacing;
	pNameLabel->SetBounds(rcNLB);
}

void C4StartupScenSelDlg::ScenListItem::UpdateOwnPos()
{
	// parent for client rect
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateOwnPos();
	// reposition achievement items
	C4GUI::ComponentAligner caBounds(GetContainedClientRect(), IconLabelSpacing, IconLabelSpacing);
	for (auto & ppAchievement : ppAchievements) if (ppAchievement)
	{
		ppAchievement->SetBounds(caBounds.GetFromRight(caBounds.GetHeight()));
	}
}

void C4StartupScenSelDlg::ScenListItem::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	// double-click opens/starts item - currently processed by ListBox already!
	// inherited processing
	typedef C4GUI::Window BaseClass;
	BaseClass::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

bool C4StartupScenSelDlg::ScenListItem::CheckNameHotkey(const char * c)
{
	// return whether this item can be selected by entering given char:
	// first char of name must match
	// FIXME: make unicode-ready
	if (!pScenListEntry) return false;
	const char *szName = pScenListEntry->GetName().getData();
	return szName && (toupper(*szName) == toupper(c[0]));
}

bool C4StartupScenSelDlg::ScenListItem::KeyRename()
{
	// rename this entry
	C4StartupScenSelDlg::pInstance->StartRenaming(new C4GUI::CallbackRenameEdit<C4StartupScenSelDlg::ScenListItem, RenameParams>(pNameLabel, this, RenameParams(), &C4StartupScenSelDlg::ScenListItem::DoRenaming, &C4StartupScenSelDlg::ScenListItem::AbortRenaming));
	return true;
}

void C4StartupScenSelDlg::ScenListItem::AbortRenaming(RenameParams par)
{
	// no renaming
	C4StartupScenSelDlg::pInstance->SetRenamingDone();
}

C4GUI::RenameEdit::RenameResult C4StartupScenSelDlg::ScenListItem::DoRenaming(RenameParams par, const char *szNewName)
{
	// check validity for new name
	if (!GetEntry()->RenameTo(szNewName)) return C4GUI::RenameEdit::RR_Invalid;
	// rename label
	pNameLabel->SetText(GetEntry()->GetName().getData());
	// main dlg update
	C4StartupScenSelDlg::pInstance->SetRenamingDone();
	C4StartupScenSelDlg::pInstance->ResortFolder();
	C4StartupScenSelDlg::pInstance->UpdateSelection();
	C4StartupScenSelDlg::pInstance->FocusScenList();
	// done; rename accepted and control deleted by ResortFolder
	return C4GUI::RenameEdit::RR_Deleted;
}

// ------------------------------------------------
// --- C4StartupScenSelDlg

C4StartupScenSelDlg::C4StartupScenSelDlg(bool fNetwork) : C4StartupDlg(LoadResStrNoAmp(fNetwork ? "IDS_DLG_NETSTART" : "IDS_DLG_STARTGAME")), pScenLoader(nullptr), pMapData(nullptr), pfctBackground(nullptr), fIsInitialLoading(false), fStartNetworkGame(fNetwork), pRenameEdit(nullptr)
{
	// ctor
	// assign singleton
	pInstance = this;

	// screen calculations
	UpdateSize();
	int32_t iButtonWidth,iCaptionFontHgt;
	int iButtonHeight = C4GUI_ButtonHgt;
	int iBookPageWidth;
	int iExtraHPadding = rcBounds.Wdt >= 700 ? rcBounds.Wdt/50 : 0;
	::GraphicsResource.CaptionFont.GetTextExtent("<< BACK", iButtonWidth, iCaptionFontHgt, true);
	iButtonWidth *= 3;
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,0, true);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(caMain.GetHeight()/8),rcBounds.Wdt/(rcBounds.Wdt >= 700 ? 128 : 256),0);
	C4Rect rcMap = caMain.GetCentered(caMain.GetWidth(), caMain.GetHeight());

	// tabular for different scenario selection designs
	pScenSelStyleTabular = new C4GUI::Tabular(rcMap, C4GUI::Tabular::tbNone);
	pScenSelStyleTabular->SetSheetMargin(0);
	pScenSelStyleTabular->SetGfx(&C4Startup::Get()->Graphics.fctDlgPaper, &C4Startup::Get()->Graphics.fctOptionsTabClip, &C4Startup::Get()->Graphics.fctOptionsIcons, &C4Startup::Get()->Graphics.BookSmallFont, false);
	AddElement(pScenSelStyleTabular);
	C4GUI::Tabular::Sheet *pSheetBook = pScenSelStyleTabular->AddSheet(nullptr);
	/* C4GUI::Tabular::Sheet *pSheetMap = */ pScenSelStyleTabular->AddSheet(nullptr);

	// scenario selection list
	C4GUI::ComponentAligner caBook(pSheetBook->GetClientRect(), caMain.GetWidth()/20, caMain.GetHeight()/20, true);
	C4GUI::ComponentAligner caBookLeft(caBook.GetFromLeft(iBookPageWidth=caBook.GetWidth()*4/9+4-iExtraHPadding*2), 0,5);

	CStdFont &rScenSelCaptionFont = C4Startup::Get()->Graphics.BookFontTitle;
	pScenSelCaption = new C4GUI::Label("", caBookLeft.GetFromTop(rScenSelCaptionFont.GetLineHeight()), ACenter, ClrScenarioItem, &rScenSelCaptionFont, false);
	pSheetBook->AddElement(pScenSelCaption);
	pScenSelCaption->SetToolTip(LoadResStr("IDS_DLGTIP_SELECTSCENARIO"));

	// search bar
	const char *labelText = LoadResStr("IDS_DLG_SEARCH");
	int32_t width = 100;
	int32_t height; // there's no point in specifying a default height - it's set by GetTextExtent, and we can't know how high the text is
	::GraphicsResource.TextFont.GetTextExtent(labelText, width, height, true);
	C4GUI::ComponentAligner caSearchBar(caBookLeft.GetFromBottom(height), 0, 0);
	auto *searchLabel = new C4GUI::WoodenLabel(labelText, caSearchBar.GetFromLeft(width + 10), C4GUI_Caption2FontClr, &::GraphicsResource.TextFont);
	searchLabel->SetToolTip(LoadResStr("IDS_DLGTIP_SEARCHLIST"));
	pSheetBook->AddElement(searchLabel);

	searchBar = new C4GUI::CallbackEdit<C4StartupScenSelDlg>(caSearchBar.GetAll(), this, &C4StartupScenSelDlg::OnSearchBarEnter);
	searchBar->SetToolTip(LoadResStr("IDS_DLGTIP_SEARCHLIST"));
	pSheetBook->AddElement(searchBar);

	// scenario selection list box
	pScenSelList = new C4GUI::ListBox(caBookLeft.GetAll());
	pScenSelList->SetToolTip(LoadResStr("IDS_DLGTIP_SELECTSCENARIO"));
	pScenSelList->SetDecoration(false, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	pSheetBook->AddElement(pScenSelList);
	pScenSelList->SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<C4StartupScenSelDlg>(this, &C4StartupScenSelDlg::OnSelChange));
	pScenSelList->SetSelectionDblClickFn(new C4GUI::CallbackHandler<C4StartupScenSelDlg>(this, &C4StartupScenSelDlg::OnSelDblClick));
	// scenario selection list progress labels
	pScenSelProgressLabel = new C4GUI::Label("", pScenSelList->GetBounds().GetMiddleX(), pScenSelList->GetBounds().GetMiddleY()-iCaptionFontHgt, ACenter, ClrScenarioItem, &(C4Startup::Get()->Graphics.BookFontCapt), false);
	pSheetBook->AddElement(pScenSelProgressLabel);
	pScenSelProgressInfoLabel = new C4GUI::Label("", pScenSelList->GetBounds().GetMiddleX(), pScenSelList->GetBounds().GetMiddleY(), ACenter, ClrScenarioItemXtra, &(C4Startup::Get()->Graphics.BookFontCapt), false);
	pSheetBook->AddElement(pScenSelProgressInfoLabel);

	// right side of book: Displaying current selection
	C4Rect bounds = caBook.GetFromRight(iBookPageWidth);
	const int32_t AvailWidth = bounds.Wdt;
	const int32_t AvailHeight = 2 * bounds.Hgt / 5;
	int32_t PictureWidth, PictureHeight;
	if(AvailWidth * C4StartupScenSel_TitlePictureHgt < AvailHeight * C4StartupScenSel_TitlePictureWdt)
	{
		PictureWidth = C4StartupScenSel_TitlePictureWdt * AvailWidth / C4StartupScenSel_TitlePictureWdt;
		PictureHeight = C4StartupScenSel_TitlePictureHgt * AvailWidth / C4StartupScenSel_TitlePictureWdt;
	}
	else
	{
		PictureWidth = C4StartupScenSel_TitlePictureWdt * AvailHeight / C4StartupScenSel_TitlePictureHgt;
		PictureHeight = C4StartupScenSel_TitlePictureHgt * AvailHeight / C4StartupScenSel_TitlePictureHgt;
	}
	pSelectionInfo = new C4GUI::TextWindow(bounds, PictureWidth+2*C4StartupScenSel_TitleOverlayMargin, PictureHeight+2*C4StartupScenSel_TitleOverlayMargin,
	                                       C4StartupScenSel_TitlePicturePadding, 100, 4096, nullptr, true, &C4Startup::Get()->Graphics.fctScenSelTitleOverlay, C4StartupScenSel_TitleOverlayMargin);
	pSelectionInfo->SetDecoration(false, false, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	pSheetBook->AddElement(pSelectionInfo);

	// bottom of right side of book: Custom options on selection
	// Arbitrary height and invisible by default. Height will be adjusted when options are created.
	pSelectionOptions = new C4GameOptionsList(C4Rect(bounds.x, bounds.y+bounds.Hgt-10, bounds.Wdt, 10), false, fNetwork ? C4GameOptionsList::GOLS_PreGameNetwork : C4GameOptionsList::GOLS_PreGameSingle);
	pSelectionOptions->SetDecoration(false, &C4Startup::Get()->Graphics.sfctBookScroll, true);
	pSelectionOptions->SetVisibility(false);
	pSheetBook->AddElement(pSelectionOptions);

	// back button
	C4GUI::CallbackButton<C4StartupScenSelDlg> *btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupScenSelDlg>(LoadResStr("IDS_BTN_BACK"), caButtonArea.GetFromLeft(iButtonWidth, iButtonHeight), &C4StartupScenSelDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
	AddElement(btn);
	// next button
	pOpenBtn = new C4GUI::CallbackButton<C4StartupScenSelDlg>(LoadResStr("IDS_BTN_OPEN"), caButtonArea.GetFromRight(iButtonWidth, iButtonHeight), &C4StartupScenSelDlg::OnNextBtn);
	pOpenBtn->SetToolTip(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));
	// game options boxes
	pGameOptionButtons = new C4GameOptionButtons(caButtonArea.GetAll(), fNetwork, true, false);
	AddElement(pGameOptionButtons);
	// next button adding
	AddElement(pOpenBtn);

	// dlg starts with focus on listbox
	SetFocus(pScenSelList, false);

	// key bindings
	pKeyBack = new C4KeyBinding(C4KeyCodeEx(K_LEFT), "StartupScenSelFolderUp", KEYSCOPE_Gui,
	                            new C4GUI::DlgKeyCB<C4StartupScenSelDlg>(*this, &C4StartupScenSelDlg::KeyBack), C4CustomKey::PRIO_CtrlOverride);
	pKeyRefresh = new C4KeyBinding(C4KeyCodeEx(K_F5), "StartupScenSelReload", KEYSCOPE_Gui,
	                               new C4GUI::DlgKeyCB<C4StartupScenSelDlg>(*this, &C4StartupScenSelDlg::KeyRefresh), C4CustomKey::PRIO_CtrlOverride);
	pKeyForward = new C4KeyBinding(C4KeyCodeEx(K_RIGHT), "StartupScenSelNext", KEYSCOPE_Gui,
	                               new C4GUI::DlgKeyCB<C4StartupScenSelDlg>(*this, &C4StartupScenSelDlg::KeyForward), C4CustomKey::PRIO_CtrlOverride);
	pKeyRename = new C4KeyBinding(C4KeyCodeEx(K_F2), "StartupScenSelRename", KEYSCOPE_Gui,
	                              new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeyRename), C4CustomKey::PRIO_CtrlOverride);
	pKeyDelete = new C4KeyBinding(C4KeyCodeEx(K_DELETE), "StartupScenSelDelete", KEYSCOPE_Gui,
	                              new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeyDelete), C4CustomKey::PRIO_CtrlOverride);
	pKeyCheat = new C4KeyBinding(C4KeyCodeEx(K_M, KEYS_Control), "StartupScenSelCheat", KEYSCOPE_Gui,
	                             new C4GUI::ControlKeyDlgCB<C4StartupScenSelDlg>(pScenSelList, *this, &C4StartupScenSelDlg::KeyCheat), C4CustomKey::PRIO_CtrlOverride);
}

C4StartupScenSelDlg::~C4StartupScenSelDlg()
{
	if (pScenLoader) delete pScenLoader;
	if (this == pInstance) pInstance = nullptr;
	delete pKeyCheat;
	delete pKeyDelete;
	delete pKeyRename;
	delete pKeyForward;
	delete pKeyRefresh;
	delete pKeyBack;
}

void C4StartupScenSelDlg::DrawElement(C4TargetFacet &cgo)
{
	// draw background
	if (pfctBackground)
		DrawBackground(cgo, *pfctBackground);
}

void C4StartupScenSelDlg::OnShown()
{
	C4StartupDlg::OnShown();
	// Collect achievements of all activated players
	UpdateAchievements();
	// init file list
	fIsInitialLoading = true;
	if (!pScenLoader) pScenLoader = new C4ScenarioListLoader(Achievements);
	pScenLoader->Load(StdStrBuf());
	UpdateList();
	UpdateSelection();
	fIsInitialLoading = false;
	// network activation by dialog type
	Game.NetworkActive = fStartNetworkGame;
}

void C4StartupScenSelDlg::OnClosed(bool fOK)
{
	AbortRenaming();
	// clear laoded scenarios
	if (pScenLoader)
	{
		delete pScenLoader;
		pScenLoader = nullptr;
		UpdateList(); // must clear scenario list, because it points to deleted stuff
		UpdateSelection(); // must clear picture facet of selection!
	}
	// dlg abort: return to main screen
	if (!fOK)
	{
		// clear settings: Password
		::Network.SetPassword(nullptr);
		C4Startup::Get()->SwitchDialog(C4Startup::SDID_Back);
	}
}

void C4StartupScenSelDlg::UpdateList()
{
	AbortRenaming();
	// default: Show book (also for loading screen)
	pMapData = nullptr;
	pScenSelStyleTabular->SelectSheet(ShowStyle_Book, false);
	// and delete any stuff from map selection
	C4GUI::Tabular::Sheet *pMapSheet = pScenSelStyleTabular->GetSheet(ShowStyle_Map);
	while (pMapSheet->GetFirst()) delete pMapSheet->GetFirst();
	pfctBackground = nullptr;
	// for now, all the list is loaded at once anyway
	// so just clear and add all loaded items
	// remember old selection
	C4ScenarioListLoader::Entry *pOldSelection = GetSelectedEntry();
	C4GUI::Element *pEl;
	while ((pEl = pScenSelList->GetFirst())) delete pEl;
	pScenSelCaption->SetText("");
	// scen loader still busy: Nothing to add
	if (!pScenLoader) return;
	if (pScenLoader->IsLoading())
	{
		StdStrBuf sProgressText;
		sProgressText.Format(LoadResStr("IDS_MSG_SCENARIODESC_LOADING"), (int32_t) pScenLoader->GetProgressPercent());
		pScenSelProgressLabel->SetText(sProgressText.getData());
		pScenSelProgressLabel->SetVisibility(true);
		pScenSelProgressInfoLabel->SetText(pScenLoader->GetProgressInfo());
		pScenSelProgressInfoLabel->SetVisibility(true);
		return;
	}
	pScenSelProgressLabel->SetVisibility(false);
	pScenSelProgressInfoLabel->SetVisibility(false);
	// is this a map folder? Then show the map instead
	C4ScenarioListLoader::Folder *pFolder = static_cast<C4ScenarioListLoader::Folder *>(pScenLoader->GetCurrFolder());
	if ((pMapData = pFolder->GetMapData()))
	{
		pMapData->ResetSelection();
		pMapData->CreateGUIElements(this, *pScenSelStyleTabular->GetSheet(ShowStyle_Map));
		pScenSelStyleTabular->SelectSheet(ShowStyle_Map, false);
	}
	else
	{
		// book style selection
		// add what has been loaded
		for (C4ScenarioListLoader::Entry *pEnt = pScenLoader->GetFirstEntry(); pEnt; pEnt = pEnt->GetNext())
		{
			if (pEnt->IsHidden()) continue; // no UI entry at all for hidden items
			if (!SLen(searchBar->GetText()) || SSearchNoCase(pEnt->GetName().getData(), searchBar->GetText()))
			{
				ScenListItem *pEntItem = new ScenListItem(pScenSelList, pEnt);
				if (pEnt == pOldSelection) pScenSelList->SelectEntry(pEntItem, false);
			}
			else if (pEnt == pOldSelection)
			{
				pOldSelection = nullptr;
			}
		}
		// set title of current folder
		// but not root
		if (pFolder && pFolder != pScenLoader->GetRootFolder())
			pScenSelCaption->SetText(pFolder->GetName().getData());
		else
		{
			// special root title
			pScenSelCaption->SetText(LoadResStr("IDS_DLG_SCENARIOS"));
		}
		// new list has been loaded: Select first entry if nothing else had been selected
		if (!pOldSelection) pScenSelList->SelectFirstEntry(false);
	}
}

void C4StartupScenSelDlg::ResortFolder()
{
	// if it's still loading, sorting will be done in the end anyway
	if (!pScenLoader || pScenLoader->IsLoading()) return;
	C4ScenarioListLoader::Folder *pFolder = pScenLoader->GetCurrFolder();
	if (!pFolder) return;
	pFolder->Resort();
	UpdateList();
}

void C4StartupScenSelDlg::UpdateSelection()
{
	AbortRenaming();
	if (!pScenLoader)
	{
		C4Facet fctNoPic;
		pSelectionInfo->SetPicture(fctNoPic);
		pSelectionInfo->ClearText(false);
		SetOpenButtonDefaultText();
		return;
	}
	// determine target text box
	C4GUI::TextWindow *pSelectionInfo = pMapData ? pMapData->GetSelectionInfoBox() : this->pSelectionInfo;
	// get selected entry
	C4ScenarioListLoader::Entry *pSel = GetSelectedEntry();
	if (!pSel)
	{
		// no selection: Display data of current parent folder
		pSel = pScenLoader->GetCurrFolder();
		// but not root
		if (pSel == pScenLoader->GetRootFolder()) pSel = nullptr;
	}
	// get title image and desc of selected entry
	C4Facet fctTitle; StdStrBuf sTitle, sDesc, sVersion, sAuthor;
	if (pSel)
	{
		pScenLoader->LoadExtended(pSel); // 2do: Multithreaded
		fctTitle = pSel->GetTitlePicture();
		sTitle.Ref(pSel->GetName());
		sDesc.Ref(pSel->GetDesc());
		sVersion.Ref(pSel->GetVersion());
		sAuthor.Ref(pSel->GetAuthor());
		// never show a pure title string: There must always be some text or an image
		if (!fctTitle.Surface && (!sDesc || !*sDesc.getData()))
			sTitle.Clear();
		// selection specific open/start button
		pOpenBtn->SetText(pSel->GetOpenText().getData());
		pOpenBtn->SetToolTip(pSel->GetOpenTooltip().getData());
	}
	else
		SetOpenButtonDefaultText();
	// set data in selection component
	pSelectionInfo->ClearText(false);
	pSelectionInfo->SetPicture(fctTitle);
	if (sTitle && (!sDesc || !*sDesc.getData())) pSelectionInfo->AddTextLine(sTitle.getData(), &C4Startup::Get()->Graphics.BookFontCapt, ClrScenarioItem, false, false);
	if (sDesc) pSelectionInfo->AddTextLine(sDesc.getData(), &C4Startup::Get()->Graphics.BookFont, ClrScenarioItem, false, false, &C4Startup::Get()->Graphics.BookFontCapt);
	if (sAuthor) pSelectionInfo->AddTextLine(FormatString(LoadResStr("IDS_CTL_AUTHOR"), sAuthor.getData()).getData(),
		    &C4Startup::Get()->Graphics.BookFont, ClrScenarioItemXtra, false, false);
	if (sVersion) pSelectionInfo->AddTextLine(FormatString(LoadResStr("IDS_DLG_VERSION"), sVersion.getData()).getData(),
		    &C4Startup::Get()->Graphics.BookFont, ClrScenarioItemXtra, false, false);
	// update custom scenario options panel
	if (pSel)
	{
		pSelectionOptions->SetParameters(pSel->GetParameterDefs(), pSel->GetParameters());
		pSelectionOptions->Update();
	}
	else
		pSelectionOptions->SetParameters(nullptr, nullptr);
	// update component heights
	C4Rect rcSelBounds = pSelectionInfo->GetBounds();
	int32_t ymax = pSelectionOptions->GetBounds().GetBottom();
	C4GUI::Element *pLastOption = pSelectionOptions->GetLast();
	if (pLastOption)
	{
		// custom options present: Info box reduced; options box at bottom
		// set options box max size to 1/3rd of selection info area
		int32_t options_hgt = std::min<int32_t>(pLastOption->GetBounds().GetBottom() + pSelectionOptions->GetMarginTop() + pSelectionOptions->GetMarginTop(), rcSelBounds.Hgt/3);
		rcSelBounds.Hgt = ymax - options_hgt - rcSelBounds.y;
		pSelectionInfo->SetBounds(rcSelBounds);
		rcSelBounds.y = ymax - options_hgt;
		rcSelBounds.Hgt = options_hgt;
		pSelectionOptions->SetBounds(rcSelBounds);
		pSelectionOptions->SetVisibility(true);
	}
	else
	{
		// custom options absent: Info takes full area
		pSelectionOptions->SetVisibility(false);
		rcSelBounds.Hgt = ymax - rcSelBounds.y;
		pSelectionInfo->SetBounds(rcSelBounds);
	}
	pSelectionInfo->UpdateHeight();
}

C4StartupScenSelDlg::ScenListItem *C4StartupScenSelDlg::GetSelectedItem()
{
	return static_cast<ScenListItem *>(pScenSelList->GetSelectedItem());
}

C4ScenarioListLoader::Entry *C4StartupScenSelDlg::GetSelectedEntry()
{
	// map layout: Get selection from map
	if (pMapData) return pMapData->GetSelectedEntry();
	// get selection in listbox
	ScenListItem *pSel = static_cast<ScenListItem *>(pScenSelList->GetSelectedItem());
	return pSel ? pSel->GetEntry() : nullptr;
}

bool C4StartupScenSelDlg::StartScenario(C4ScenarioListLoader::Scenario *pStartScen)
{
	assert(pStartScen);
	if (!pStartScen) return false;
	// get required object definitions
	if (pStartScen->GetC4S().Definitions.AllowUserChange)
	{
		// get definitions as user selects them
		StdStrBuf sDefinitions;
		if (!pStartScen->GetC4S().Definitions.GetModules(&sDefinitions)) sDefinitions.Copy("Objects.ocd");
		if (!C4DefinitionSelDlg::SelectDefinitions(GetScreen(), &sDefinitions))
			// user aborted during definition selection
			return false;
		SCopy(sDefinitions.getData(), ::Game.DefinitionFilenames, (sizeof Game.DefinitionFilenames)-1);
	}
	else
		// for no user change, just set default objects. Custom settings will override later anyway
		SCopy("Objects.ocd", ::Game.DefinitionFilenames);
	// set other default startup parameters
	::Game.fLobby = !!::Game.NetworkActive; // always lobby in network
	::Game.fObserve = false;
	C4ScenarioParameters *custom_params = pStartScen->GetParameters();
	if (custom_params) ::Game.StartupScenarioParameters = *custom_params; else ::Game.StartupScenarioParameters.Clear();
	// start with this set!
	::Application.OpenGame(pStartScen->GetEntryFilename().getData());
	return true;
}

bool C4StartupScenSelDlg::OpenFolder(C4ScenarioListLoader::Folder *pNewFolder)
{
	// open it through loader
	if (!pScenLoader) return false;
	searchBar->ClearText();
	bool fSuccess = pScenLoader->Load(pNewFolder, false);
	UpdateList();
	UpdateSelection();
	if (!pMapData) SetFocus(pScenSelList, false);
	return fSuccess;
}

bool C4StartupScenSelDlg::DoOK()
{
	AbortRenaming();
	// get selected entry
	C4ScenarioListLoader::Entry *pSel = GetSelectedEntry();
	if (!pSel) return false;
	// check if open is possible
	StdStrBuf sError;
	bool CanHide = false;
	if (!pSel->CanOpen(sError, CanHide))
	{
		GetScreen()->ShowMessage(sError.getData(), LoadResStr("IDS_MSG_CANNOTSTARTSCENARIO"), C4GUI::Ico_Error);
		return false;
	}
	// if CanOpen returned true but set an error message, that means it's a warning. Display it!
	if (sError.getLength())
	{
		if (!GetScreen()->ShowMessageModal(sError.getData(), LoadResStrNoAmp("IDS_DLG_STARTGAME"), C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Notify, CanHide ? &Config.Startup.HideMsgStartDedicated : nullptr))
			// user chose to not start it
			return false;
	}
	// start it!
	return pSel->Start();
}

bool C4StartupScenSelDlg::DoBack(bool fAllowClose)
{
	AbortRenaming();
	// if in a subfolder, try backtrace folders first
	if (pScenLoader && pScenLoader->FolderBack())
	{
		searchBar->ClearText();
		UpdateList();
		UpdateSelection();
		return true;
	}
	// while this isn't multithreaded, the dialog must not be aborted while initial load...
	if (pScenLoader && pScenLoader->IsLoading()) return false;
	// return to main screen
	if (fAllowClose)
	{
		Close(false);
		return true;
	}
	return false;
}

void C4StartupScenSelDlg::DoRefresh()
{
	if (pScenLoader && !pScenLoader->IsLoading())
	{
		pScenSelList->SelectNone(false);
		pScenLoader->ReloadCurrent();
		UpdateList();
		UpdateSelection();
	}
}

void C4StartupScenSelDlg::SetOpenButtonDefaultText()
{
	pOpenBtn->SetText(LoadResStr("IDS_BTN_OPEN"));
	pOpenBtn->SetToolTip(LoadResStr("IDS_DLGTIP_SCENSELNEXT"));
}

bool C4StartupScenSelDlg::KeyRename()
{
	// no rename in map mode
	if (pMapData) return false;
	// not if renaming already
	if (IsRenaming()) return false;
	// forward to selected scenario list item
	ScenListItem *pSel = GetSelectedItem();
	if (!pSel) return false;
	return pSel->KeyRename();
}

bool C4StartupScenSelDlg::KeyDelete()
{
	// do not delete from map folder
	if (pMapData) return false;
	// cancel renaming
	AbortRenaming();
	// delete selected item: Confirmation first
	ScenListItem *pSel = GetSelectedItem();
	if (!pSel) return false;
	C4ScenarioListLoader::Entry *pEnt = pSel->GetEntry();
	StdStrBuf sWarning;
	sWarning.Format(LoadResStr("IDS_MSG_PROMPTDELETE"), FormatString("%s %s", pEnt->GetTypeName().getData(), pEnt->GetName().getData()).getData(), pEnt->GetEntryFilename().getData());
	GetScreen()->ShowRemoveDlg(new C4GUI::ConfirmationDialog(sWarning.getData(), LoadResStr("IDS_MNU_DELETE"),
	                           new C4GUI::CallbackHandlerExPar<C4StartupScenSelDlg, ScenListItem *>(this, &C4StartupScenSelDlg::DeleteConfirm, pSel), C4GUI::MessageDialog::btnYesNo));
	return true;
}

void C4StartupScenSelDlg::DeleteConfirm(ScenListItem *pSel)
{
	// deletion confirmed. Do it.
	C4ScenarioListLoader::Entry *pEnt = pSel->GetEntry();
	if (!C4Group_DeleteItem(pEnt->GetEntryFilename().getData(), true))
	{
		StdStrBuf sMsg; sMsg.Format("%s", LoadResStr("IDS_FAIL_DELETE"));
		::pGUI->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_MNU_DELETE"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		return;
	}
	// remove from scenario list
	pScenSelList->SelectEntry(pSel->GetNext(), false);
	delete pEnt;
	delete pSel;
}

bool C4StartupScenSelDlg::KeyCheat()
{
	return ::pGUI->ShowRemoveDlg(new C4GUI::InputDialog(LoadResStr("IDS_TEXT_ENTERMISSIONPASSWORD"), LoadResStr("IDS_DLG_MISSIONACCESS"), C4GUI::Ico_Options,
	                             new C4GUI::InputCallback<C4StartupScenSelDlg>(this, &C4StartupScenSelDlg::KeyCheat2),
	                             false));
}

void C4StartupScenSelDlg::KeyCheat2(const StdStrBuf &rsCheatCode)
{
	// Special character "-": remove mission password(s)
	if (SEqual2(rsCheatCode.getData(), "-"))
	{
		const char *szPass = rsCheatCode.getPtr(1);
		if (szPass && *szPass)
		{
			SRemoveModules(Config.General.MissionAccess, szPass, false);
			UpdateList();
			UpdateSelection();
			return;
		}
	}

	// No special character: add mission password(s)
	const char *szPass = rsCheatCode.getPtr(0);
	if (szPass && *szPass)
	{
		SAddModules(Config.General.MissionAccess, szPass, false);
		UpdateList();
		UpdateSelection();
		return;
	}

}

void C4StartupScenSelDlg::FocusScenList()
{
	SetFocus(pScenSelList, false);
}

void C4StartupScenSelDlg::OnButtonScenario(C4GUI::Control *pEl)
{
	// map button was clicked: Update selected scenario
	if (!pMapData || !pEl) return;
	C4ScenarioListLoader::Entry *pSel = GetSelectedEntry(), *pSel2;
	pMapData->OnButtonScenario(pEl);
	pSel2 = GetSelectedEntry();
	if (pSel && pSel==pSel2)
	{
		// clicking on the selected scenario again starts it
		DoOK();
		return;
	}
	// the first click selects it
	SetFocus(pEl, false);
	UpdateSelection();
}

void C4StartupScenSelDlg::DeselectAll()
{
	// Deselect all so current folder info is displayed
	if (GetFocus()) C4GUI::GUISound("UI::Tick");
	SetFocus(nullptr, true);
	if (pMapData) pMapData->ResetSelection();
	UpdateSelection();
}

void C4StartupScenSelDlg::StartRenaming(C4GUI::RenameEdit *pNewRenameEdit)
{
	pRenameEdit = pNewRenameEdit;
}

void C4StartupScenSelDlg::AbortRenaming()
{
	if (pRenameEdit) pRenameEdit->Abort();
}

void C4StartupScenSelDlg::UpdateAchievements()
{
	// Extract all achievements from activated player files and merge them
	Achievements.Clear();
	char PlayerFilename[_MAX_FNAME_LEN];
	C4Group PlayerGrp;
	for (int i = 0; SCopySegment(Config.General.Participants, i, PlayerFilename, ';', _MAX_FNAME, true); i++)
	{
		const char *szPlayerFilename = Config.AtUserDataPath(PlayerFilename);
		if (!FileExists(szPlayerFilename)) continue;
		if (!PlayerGrp.Open(szPlayerFilename)) continue;
		C4PlayerInfoCore nfo;
		if (!nfo.Load(PlayerGrp)) continue;
		Achievements.Merge(nfo.Achievements);
	}
}

void C4StartupScenSelDlg::OnLeagueOptionChanged()
{
	if (pSelectionOptions) pSelectionOptions->Update();
}

// NICHT: 9, 7.2.2, 113-114, 8a

