/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2017, The OpenClonk Team and contributors
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
 // Screen for mod handling

#include "C4Include.h"
#include "gui/C4StartupModsDlg.h"

#include "game/C4Application.h"
#include "gui/C4UpdateDlg.h"
#include "game/C4Game.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

#include <tinyxml.h>

#include <fstream>
#include <sstream>
#include <regex>
const std::string C4StartupModsDlg::baseServerURL = "frustrum.pictor.uberspace.de/larry/api/";

// Used to parse values of subelements from an XML element.
std::string getSafeStringValue(const TiXmlElement *xml, const char *childName, std::string fallback = "", bool isAttribute = false)
{
	if (xml == nullptr) return fallback;
	if (isAttribute)
	{
		const char * value = xml->Attribute(childName);
		if (value == nullptr)
			return fallback;
		return{ value };
	}
	const TiXmlElement *child = xml->FirstChildElement(childName);
	if (child == nullptr) return fallback;
	const char *nodeText = child->GetText();
	if (nodeText == nullptr) return fallback;
	const std::string value(nodeText);
	if (!value.empty()) return value;
	return fallback;
};

ModXMLData::ModXMLData(const TiXmlElement *xml, Source source)
{

	// Remember whether the loaded the element from a local file / overview, because we
	// then still have to do a query if we want to update.
	this->source = source;
	// Remember the XML element in case we need to pretty-print it later.
	originalXMLElement = xml->Clone();
	id = getSafeStringValue(xml, "id", "", true);
	title = getSafeStringValue(xml, "title", "");
	assert(IsValidUtf8(title.c_str()));
	slug = getSafeStringValue(xml, "slug", title);
	assert(IsValidUtf8(slug.c_str()));
	description = getSafeStringValue(xml, "description");
	assert(IsValidUtf8(description.c_str()));
	if (!description.empty())
	{
		if (description.size() > 150)
		{
			// Find a cutoff that is not inside a UTF-8 character.
			std::string::size_type characterCount{ 0 };
			for (const char *c = description.data(); *c != '\0';)
			{
				const uint8_t val = *c;
				if (val <= 127)
				{
					c += 1;
				}
				else
				{
					GetNextUTF8Character(&c);
				}
				characterCount += 1;

				if ((characterCount > 140 && *c == ' ') || (characterCount > 150))
				{
					uint32_t byteDifference = c - description.data();
					description.resize(byteDifference);
					description += "...";
					break;
				}
			}
		}
	}
	assert(IsValidUtf8(description.c_str()));
	// Additional meta-information.

	for (const TiXmlElement *node = xml->FirstChildElement("dependency"); node != nullptr; node = node->NextSiblingElement("dependency"))
	{
		const char *depID = node->GetText();
		if (depID != nullptr)
			dependencies.push_back(depID);
	}

	for (const TiXmlElement *filenode = xml->FirstChildElement("file"); filenode != nullptr; filenode = filenode->NextSiblingElement("file"))
	{
		// We guarantee that we do not modify the handle below, thus the const_cast is safe.
		const TiXmlHandle nodeHandle(const_cast<TiXmlNode*> (static_cast<const TiXmlNode*> (filenode)));

		const std::string handle = getSafeStringValue(filenode, "_id", "");
		const std::string name = getSafeStringValue(filenode, "filename", "");
		const std::string lengthString = getSafeStringValue(filenode, "length", "");

		std::string hashSHA1 = "";
		const auto hashNode = nodeHandle.FirstChild("metadata").FirstChild("hashes").Node();
		if (hashNode != nullptr)
		{
			hashSHA1 = getSafeStringValue(hashNode->ToElement(), "sha1", "");
		}

		if (handle.empty() || name.empty() || lengthString.empty()) continue;
		size_t length{ 0 };

		try
		{
			length = std::stoi(lengthString);
		}
		catch (...)
		{
			continue;
		}

		files.emplace_back(FileInfo{ handle, length, name, hashSHA1 });
	}
}

ModXMLData::~ModXMLData()
{
	delete originalXMLElement;
	originalXMLElement = nullptr;
}

// ----------- C4StartupNetListEntry -----------------------------------------------------------------------

C4StartupModsListEntry::C4StartupModsListEntry(C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBefore, C4StartupModsDlg *pModsDlg)
		: pModsDlg(pModsDlg), pList(pForListBox), fError(false), iInfoIconCount(0), iSortOrder(0), fIsSmall(false), fIsCollapsed(false), fIsEnabled(true), fIsImportant(false)
{
	// calc height
	int32_t iLineHgt = ::GraphicsResource.TextFont.GetLineHeight(), iHeight = iLineHgt * 2 + 4;
	// add icons - normal icons use small size, only animated netgetref uses full size
	rctIconLarge.Set(0, 0, iHeight, iHeight);
	int32_t iSmallIcon = iHeight * 2 / 3; rctIconSmall.Set((iHeight - iSmallIcon)/2, (iHeight - iSmallIcon)/2, iSmallIcon, iSmallIcon);
	pIcon = new C4GUI::Icon(rctIconSmall, C4GUI::Ico_None);
	AddElement(pIcon);
	SetBounds(pIcon->GetBounds());
	// add to listbox (will get resized horizontally and moved)
	pForListBox->InsertElement(this, pInsertBefore);
	// add status icons and text labels now that width is known
	CStdFont *pUseFont = &(::GraphicsResource.TextFont);
	int32_t iIconSize = pUseFont->GetLineHeight();
	C4Rect rcIconRect = GetContainedClientRect();
	int32_t iThisWdt = rcIconRect.Wdt;
	rcIconRect.x = iThisWdt - iIconSize * (iInfoIconCount + 1);
	rcIconRect.Wdt = rcIconRect.Hgt = iIconSize;
	for (int32_t iIcon = 0; iIcon<MaxInfoIconCount; ++iIcon)
	{
		AddElement(pInfoIcons[iIcon] = new C4GUI::Icon(rcIconRect, C4GUI::Ico_None));
		rcIconRect.x -= rcIconRect.Wdt;
	}
	C4Rect rcLabelBounds;
	rcLabelBounds.x = iHeight+3;
	rcLabelBounds.Hgt = iLineHgt;
	for (int i=0; i<InfoLabelCount; ++i)
	{
		const int alignments[] = { ALeft, ARight };
		for (int c = 0; c < 2; ++c)
		{
			const int alignment = alignments[c];
			rcLabelBounds.y = 1 + i*(iLineHgt + 2);
			rcLabelBounds.Wdt = iThisWdt - rcLabelBounds.x - 1;
			C4GUI::Label * const pLbl = new C4GUI::Label("", rcLabelBounds, alignment, C4GUI_CaptionFontClr);
			if (alignment == ALeft) pInfoLbl[i] = pLbl;
			else if (alignment == ARight) pInfoLabelsRight[i] = pLbl;
			else assert(false);
			AddElement(pLbl);
			// label will have collapsed due to no text: Repair it
			pLbl->SetAutosize(false);
			pLbl->SetBounds(rcLabelBounds);
		}
	}
	// update small state, which will resize this to a small entry
	UpdateSmallState();
	// Set*-function will fill icon and text and calculate actual size
}

C4StartupModsListEntry::~C4StartupModsListEntry()
{
	ClearRef();
}

void C4StartupModsListEntry::FromXML(const TiXmlElement *xml, ModXMLData::Source source, std::string fallbackID, std::string fallbackName)
{
	modXMLData = std::make_unique<ModXMLData>(xml, source);
	if (modXMLData->id.empty()) modXMLData->id = fallbackID;
	if (modXMLData->title.empty()) modXMLData->title = fallbackName;
	if (source != ModXMLData::Source::Local)
	{
		std::string updated = getSafeStringValue(xml, "updatedAt", "");
		std::string::size_type dateEnd;
		if (!updated.empty() && (dateEnd = updated.find('T')) != std::string::npos)
		{
			updated = updated.substr(0, dateEnd);
		}
		else
			updated = "/";
		sInfoTextRight[0].Format(LoadResStr("IDS_MODS_METAINFO"),
			updated.c_str(),
			getSafeStringValue(xml->FirstChildElement("voting"), "sum", "0").c_str());
	}
	const std::string author = getSafeStringValue(xml->FirstChildElement("author"), "username", "???");
	const std::string title = modXMLData->title.empty() ? "???" : modXMLData->title;
	sInfoText[0].Format(LoadResStr("IDS_MODS_TITLE"), title.c_str(), author.c_str());

	for (auto &file : modXMLData->files)
	{
		const auto &name = file.name;
		if (name.find(".ocd") != std::string::npos)
			defaultIcon = C4GUI::Icons::Ico_Definition;
		else if (name.find(".ocs") != std::string::npos)
			defaultIcon = C4GUI::Icons::Ico_Gfx;
		else if (name.find(".ocf") != std::string::npos)
			defaultIcon = C4GUI::Icons::Ico_Ex_GameList;
		else continue;
		break;
	}
}

void C4StartupModsListEntry::MakeInfoEntry()
{
	isInfoEntry = true;

	const_cast<C4Facet &>(reinterpret_cast<const C4Facet &>(pIcon->GetFacet()))
		= (const C4Facet &)C4Startup::Get()->Graphics.fctNetGetRef;
	pIcon->SetAnimated(true, 1);
	pIcon->SetBounds(rctIconLarge);

	// set info
	sInfoText[0].Copy(LoadResStr("IDS_MODS_SEARCHING"));
	UpdateSmallState(); UpdateText();
}

void C4StartupModsListEntry::OnNoResultsFound()
{
	pIcon->SetAnimated(false, 1);
	sInfoText[0].Copy(LoadResStr("IDS_MODS_SEARCH_NORESULTS"));
	UpdateText();
}

void C4StartupModsListEntry::ShowPageInfo(int page, int totalPages, int totalResults)
{
	pIcon->SetAnimated(false, 1);
	sInfoText[0].Copy(LoadResStr("IDS_MODS_SEARCH_NEXTPAGE"));
	sInfoText[1].Format(LoadResStr("IDS_MODS_SEARCH_NEXTPAGE_DESC"), page, totalPages, totalResults);
	UpdateText();
}

void C4StartupModsListEntry::OnError(std::string message)
{
	pIcon->SetAnimated(false, 1);
	sInfoText[0].Copy(LoadResStr("IDS_MODS_SEARCH_ERROR"));
	sInfoText[1].Copy(message.c_str());
	UpdateText();
}

void C4StartupModsListEntry::DrawElement(C4TargetFacet &cgo)
{
	typedef C4GUI::Window ParentClass;
	// background if important and not selected
	if (fIsImportant && !IsSelectedChild(this))
	{
		int32_t x1 = cgo.X+cgo.TargetX+rcBounds.x;
		int32_t y1 = cgo.Y+cgo.TargetY+rcBounds.y;
		pDraw->DrawBoxDw(cgo.Surface, x1,y1, x1+rcBounds.Wdt, y1+rcBounds.Hgt, C4GUI_ImportantBGColor);
	}
	// inherited
	ParentClass::DrawElement(cgo);
}

void C4StartupModsListEntry::ClearRef()
{
	fError = false;
	sError.Clear();
	int32_t i;
	for (i = 0; i < InfoLabelCount; ++i)
	{
		sInfoText[i].Clear();
		sInfoTextRight[i].Clear();
	}
	InvalidateStatusIcons();
	fIsEnabled = true;
	fIsImportant = false;
}

bool C4StartupModsListEntry::Execute()
{
	return true;
}

C4GUI::Element* C4StartupModsListEntry::GetNextLower(int32_t sortOrder)
{
	// search list for the next element of a lower sort order
	for (C4GUI::Element *pElem = pList->GetFirst(); pElem; pElem = pElem->GetNext())
	{
		C4StartupModsListEntry *pEntry = static_cast<C4StartupModsListEntry *>(pElem);
		if (pEntry->iSortOrder < sortOrder)
			return pElem;
	}
	// none found: insert at start
	return nullptr;
}

void C4StartupModsListEntry::UpdateCollapsed(bool fToCollapseValue)
{
	// if collapsed state changed, update the text
	if (fIsCollapsed == fToCollapseValue) return;
	fIsCollapsed = fToCollapseValue;
	UpdateSmallState();
}

void C4StartupModsListEntry::UpdateSmallState()
{
	// small view: Always collapsed if there is no extended text
	bool fNewIsSmall = !sInfoText[2].getLength() || fIsCollapsed;
	if (fNewIsSmall == fIsSmall) return;
	fIsSmall = fNewIsSmall;
	for (int i = 2; i < InfoLabelCount; ++i)
	{
		pInfoLbl[i]->SetVisibility(!fIsSmall);
		pInfoLabelsRight[i]->SetVisibility(!fIsSmall);
	}
	UpdateEntrySize();
}

void C4StartupModsListEntry::UpdateEntrySize()
{
	if(fVisible) {
		// restack all labels by their size
		const int32_t iLblCnt = (fIsSmall ? 2 : InfoLabelCount);
		for (int c = 0; c < 2; ++c)
		{
			int iY = 1;
			C4GUI::Label **labelList = (c == 0) ? pInfoLbl : pInfoLabelsRight;
			for (int i = 0; i < iLblCnt; ++i)
			{
				C4Rect rcBounds = labelList[i]->GetBounds();
				rcBounds.y = iY;
				iY += rcBounds.Hgt + 2;
				pInfoLbl[i]->SetBounds(rcBounds);
			}
			// Resize this control according to the labels on the left.
			if (c == 0)
				GetBounds().Hgt = iY - 1;
		}
	} else GetBounds().Hgt = 0;
	UpdateSize();
}

void C4StartupModsListEntry::UpdateInstalledState(C4StartupModsLocalModDiscovery::ModsInfo *modInfo)
{
	if (isInfoEntry) return;

	isInstalled = modInfo != nullptr;

	std::string fullDescription = modXMLData->description;

	if (modInfo != nullptr)
	{
		pIcon->SetIcon(C4GUI::Icons::Ico_Save);

		fullDescription = std::string("<c 559955>") + LoadResStr("IDS_MODS_INSTALLED") + ".</c> " + modXMLData->description;
	}
	else
	{
		pIcon->SetIcon(defaultIcon);
	}

	sInfoText[1].Format("%s", fullDescription.c_str());
	UpdateText();
}

void C4StartupModsListEntry::UpdateText()
{
	bool fRestackElements=false;
	CStdFont *pUseFont = &(::GraphicsResource.TextFont);
	// adjust icons
	int32_t sx=iInfoIconCount*pUseFont->GetLineHeight();
	int32_t i;
	for (i=iInfoIconCount; i<MaxInfoIconCount; ++i)
	{
		pInfoIcons[i]->SetIcon(C4GUI::Ico_None);
		pInfoIcons[i]->SetToolTip(nullptr);
	}
	// text to labels
	for (int c = 0; c < 2; ++c)
	{
		C4GUI::Label **infoLabels = (c == 0) ? pInfoLbl : pInfoLabelsRight;
		StdStrBuf *infoTexts = (c == 0) ? sInfoText : sInfoTextRight;
		for (i = 0; i < InfoLabelCount; ++i)
		{
			C4GUI::Label *infoLabel = infoLabels[i];
			int iAvailableWdt = GetClientRect().Wdt - infoLabel->GetBounds().x - 1;
			if (!i) iAvailableWdt -= sx;
			StdStrBuf BrokenText;
			pUseFont->BreakMessage(infoTexts[i].getData(), iAvailableWdt, &BrokenText, true);
			int32_t iHgt, iWdt;
			if (pUseFont->GetTextExtent(BrokenText.getData(), iWdt, iHgt, true))
			{
				if ((infoLabel->GetBounds().Hgt != iHgt) || (infoLabel->GetBounds().Wdt != iAvailableWdt))
				{
					C4Rect rcBounds = infoLabel->GetBounds();
					rcBounds.Wdt = iAvailableWdt;
					rcBounds.Hgt = iHgt;
					infoLabel->SetBounds(rcBounds);
					fRestackElements = true;
				}
			}
			infoLabel->SetText(BrokenText.getData());
			infoLabel->SetColor(fIsEnabled ? C4GUI_MessageFontClr : C4GUI_InactMessageFontClr);
		}
	}
	if (fRestackElements) UpdateEntrySize();
}

void C4StartupModsListEntry::SetVisibility(bool fToValue) {
	bool fChange = fToValue != fVisible;
	C4GUI::Window::SetVisibility(fToValue);
	if(fChange) UpdateEntrySize();
}

void C4StartupModsListEntry::AddStatusIcon(C4GUI::Icons eIcon, const char *szToolTip)
{
	// safety
	if (iInfoIconCount==MaxInfoIconCount) return;
	// set icon to the left of the existing icons to the desired data
	pInfoIcons[iInfoIconCount]->SetIcon(eIcon);
	pInfoIcons[iInfoIconCount]->SetToolTip(szToolTip);
	++iInfoIconCount;
}

void C4StartupModsListEntry::SetError(const char *szErrorText)
{
	// set error message
	fError = true;
	sInfoText[1].Copy(szErrorText);
	for (int i=2; i<InfoLabelCount; ++i) sInfoText[i].Clear();
	InvalidateStatusIcons();
	UpdateSmallState(); UpdateText();
	pIcon->SetIcon(C4GUI::Ico_Close);
	pIcon->SetAnimated(false, 0);
	pIcon->SetBounds(rctIconSmall);
}

void C4StartupModsLocalModDiscovery::Execute()
{
	assert(!discoveryFinished);
	discoveryFinishedEvent.Reset();

	ExecuteDiscovery();

	discoveryFinished = true;
	discoveryFinishedEvent.Set();

	parent->QueueSyncWithDiscovery();

	StdThread::SignalStop();
}

void C4StartupModsLocalModDiscovery::ExecuteDiscovery()
{
	// Check the mods directory for existing files.
	const std::string path = std::string(Config.General.UserDataPath) + "mods";
	for (DirectoryIterator iter(path.c_str()); *iter; ++iter)
	{
		const std::string filename(*iter);
		
		// No folder?
		if (!DirectoryExists(filename.c_str())) continue;

		const size_t lastSeparaterPosition = filename.find_last_of(DirectorySeparator);
		if (lastSeparaterPosition == std::string::npos) continue;
		const std::string leaf = filename.substr(lastSeparaterPosition + 1);
		// The leaf is prefixed with "<item ID>_" if it's a mod directory.
		const size_t idSeparatorPosition = leaf.find_first_of("_");
		if (idSeparatorPosition == std::string::npos) continue;
		const std::string id = leaf.substr(0, idSeparatorPosition);
		if (id.empty()) continue;
		const std::string name = leaf.substr(idSeparatorPosition + 1);
		AddMod(id, filename, name);
	}
}

bool C4StartupModsListEntry::KeywordMatch(const char *szMatch)
{
	return false;
}

C4StartupModsDownloader::C4StartupModsDownloader(C4StartupModsDlg *parent, const C4StartupModsListEntry *entry) : CStdTimerProc(30)
{
	this->parent = parent;

	if (entry != nullptr)
		items.emplace_back(std::move(std::make_unique<ModInfo>(entry)));

	// Register timer.
	Application.Add(this);
}

void C4StartupModsDownloader::AddModToQueue(std::string modID, std::string name)
{
	// Not if already contained.
	for (auto &mod : items)
		if (mod->modID == modID) return;
	items.emplace_back(std::move(std::make_unique<ModInfo>(modID, name)));
}

C4StartupModsDownloader::~C4StartupModsDownloader()
{
	CStdLock lock(&guiThreadResponse);
	Application.Remove(this);
	CancelRequest();
}

C4GUI::ProgressDialog * C4StartupModsDownloader::GetProgressDialog()
{
	if (!progressDialog)
	{
		progressDialog = new C4GUI::ProgressDialog("", LoadResStr("IDS_MODS_SEARCHING"), 100, 0, C4GUI::Icons::Ico_Save);
		parent->GetScreen()->ShowRemoveDlg(progressDialog);
		progressDialog->SetDelOnClose(false);
	}
	return progressDialog;
}

void C4StartupModsDownloader::CancelRequest()
{
	for (auto & mod : items)
		mod->CancelRequest();
	items.resize(0);

	if (postMetadataClient.get())
	{
		Application.InteractiveThread.RemoveProc(postMetadataClient.get());
		postMetadataClient.reset();
	}

	if (progressDialog)
		progressDialog->Close(true);
	delete progressDialog;
	progressDialog = nullptr;

	progressCallback = nullptr;
	metadataQueriedForModIdx = -1;
}

void C4StartupModsDownloader::OnConfirmInstallation(C4GUI::Element *element)
{
	CStdLock lock(&guiThreadResponse);

	assert(!items.empty());
	bool hasFile = false;
	for (auto & item : items)
		if (!item->files.empty()) hasFile = true;
	assert(hasFile);

	GetProgressDialog()->SetTitle(LoadResStr("IDS_MODS_INSTALLANDDOWNLOAD"));
	GetProgressDialog()->SetMessage("");
	GetProgressDialog()->SetProgress(0);
	GetProgressDialog()->SetVisibility(true);

	progressCallback = std::bind(&C4StartupModsDownloader::ExecuteCheckDownloadProgress, this);
}

C4StartupModsDownloader::ModInfo::ModInfo(const C4StartupModsListEntry *entry) : ModInfo()
{
	FromXMLData(entry->GetModXMLData());
}

C4StartupModsDownloader::ModInfo::ModInfo(std::string modID, std::string name) : ModInfo()
{
	Clear();
	this->modID = modID;
	this->name = name;
	this->hasOnlyIncompleteInformation = true;
}

void C4StartupModsDownloader::ModInfo::FromXMLData(const ModXMLData &xmlData)
{
	Clear();
	modID = xmlData.id;
	name = xmlData.title;
	slug = xmlData.slug;
	dependencies = xmlData.dependencies;
	originalXMLNode = xmlData.originalXMLElement->Clone();
	hasOnlyIncompleteInformation = xmlData.requiresUpdate();

	for (const auto & fileInfo : xmlData.files)
	{
		files.emplace_back(ModInfo::FileInfo{ fileInfo.handle, fileInfo.name, fileInfo.size, fileInfo.sha1 });
		requiredFilenames.insert(fileInfo.name);
	}
}

void C4StartupModsDownloader::ModInfo::Clear()
{
	CancelRequest();
	files.resize(0);
	requiredFilenames.clear();
	downloadedBytes = totalBytes = 0;
	delete originalXMLNode;
	originalXMLNode = nullptr;
}

void C4StartupModsDownloader::ModInfo::CancelRequest()
{
	if (!postClient.get()) return;
	Application.InteractiveThread.RemoveProc(postClient.get());
	postClient.reset();
}

std::string C4StartupModsDownloader::ModInfo::GetPath()
{
	return std::string(Config.General.UserDataPath) + "mods" + DirectorySeparator + \
		modID + "_" + slug;
}

void C4StartupModsDownloader::ModInfo::CheckProgress()
{
	// Determining success or starting a new download.
	if (HasError()) return;
	if (files.empty())
		successful = true;
	if (successful) return;

	if (postClient.get() == nullptr) // Start new file?
	{
		postClient = std::make_unique<C4Network2HTTPClient>();

		if (!postClient->Init() || !postClient->SetServer((C4StartupModsDlg::baseServerURL + "media/" + files.back().handle + "?download").c_str()))
		{
			assert(false);
			return;
		}
		postClient->SetExpectedResponseType(C4Network2HTTPClient::ResponseType::NoPreference);

		// Do the actual request.
		postClient->SetNotify(&Application.InteractiveThread);
		Application.InteractiveThread.AddProc(postClient.get());
		postClient->Query(nullptr, true); // Empty query for binary data.
	}

	// Update progress bar.
	downloadedBytes = postClient->getDownloadedSize();
	totalBytes = postClient->getTotalSize();

	if (!postClient->isBusy())
	{
		if (!postClient->isSuccess())
		{
			errorMessage = std::string(LoadResStr("IDS_MODS_NOINSTALL_CONNECTIONFAIL")) + postClient->GetError();
			CancelRequest();
			return;
		}
		else
		{
			const std::string path = GetPath();
			if (!CreatePath(path))
			{
				errorMessage = LoadResStr("IDS_MODS_NOINSTALL_CREATEDIR");
				CancelRequest();
				return;
			}

			std::ofstream os(path + DirectorySeparator + files.back().name, std::iostream::out | std::iostream::binary);
			if (!os.good())
			{
				errorMessage = LoadResStr("IDS_MODS_NOINSTALL_CREATEFILE");
				CancelRequest();
				return;
			}

			os.write(static_cast<const char*>(postClient->getResultBin().getData()), postClient->getDownloadedSize());
			os.close();

			CancelRequest();
			
			files.pop_back();
			if (files.empty())
			{
				// Write mod metadata to info file - to bad we don't use tinyxml with STL support.
				FILE *metadata = std::fopen((path + DirectorySeparator + "item.xml").c_str(), "w");
				if (metadata != nullptr)
				{
					originalXMLNode->Print(metadata, 0);
					std::fclose(metadata);
					successful = true;

					// Now clean up all files that don't belong to this mod.
					// This can be necessary if e.g. filenames change over updates.
					for (DirectoryIterator iter(path.c_str()); *iter; ++iter)
					{
						const std::string filename(*iter);
						// No folders.
						if (DirectoryExists(filename.c_str())) continue;
						// Safety: touch only a special set of file endings.
						const std::string::size_type typeIndex = filename.rfind(".");
						if (typeIndex == std::string::npos) continue;
						const std::string ending(filename.substr(typeIndex + 1));
						if (ending != "ocd" && ending != "ocf" && ending != "ocs") continue;
						// In the required files anyway?
						const std::string::size_type leafIndex = filename.rfind(DirectorySeparator);
						std::string leaf(filename);
						if (leafIndex != std::string::npos)
							leaf = filename.substr(leafIndex + 1);
						if (requiredFilenames.count(leaf) > 0) continue;
						EraseFile(filename.c_str());
					}
				}
			}
			return;
		}
	}
}

void C4StartupModsDownloader::ExecuteCheckDownloadProgress()
{
	// Not even progressing yet?
	if (progressDialog == nullptr) return;

	if (progressDialog->IsAborted())
	{
		CancelRequest();
		return;
	}

	// Let mods check their progress.
	size_t downloadedBytes{ 0 }, totalBytes{ 0 };

	bool anyNotFinished = false;

	for (auto & mod : items)
	{
		mod->CheckProgress();
		size_t downloaded, total;
		std::tie(downloaded, total) = mod->GetProgress();
		
		downloadedBytes += downloaded;
		totalBytes += total;

		if (mod->IsBusy())
		{
			anyNotFinished = true;
		}
	}

	if (totalBytes)
		progressDialog->SetProgress(100 * downloadedBytes / totalBytes);

	// All done?
	if (!anyNotFinished)
	{
		// Report errors (all in one).
		std::string errorMessage;
		for (auto & mod : items)
		{
			if (mod->WasSuccessful())
			{
				parent->modsDiscovery.AddMod(mod->modID, mod->GetPath(), mod->name);
				parent->QueueSyncWithDiscovery();
			}
			const std::string modError = mod->GetErrorMessage();
			if (!modError.empty())
				errorMessage += "|" + modError;
		}

		CancelRequest();

		if (!errorMessage.empty())
		{
			::pGUI->ShowMessageModal(errorMessage.c_str(), LoadResStr("IDS_MODS_NOINSTALL"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		}
		return;
	}
}

void C4StartupModsDownloader::ExecuteMetadataUpdate()
{
	if (!progressDialog) return;

	if (progressDialog->IsAborted())
	{
		CancelRequest();
		return;
	}

	// Start querying new metadata?
	if (!postMetadataClient)
	{
		// Find first mod that requires an update.
		for (size_t i = static_cast<size_t>(metadataQueriedForModIdx + 1); i < items.size(); ++i)
		{
			auto &mod = items[i];
			if (!mod->RequiresMetadataUpdate() || mod->HasError()) continue;
			
			StdStrBuf progressMessage;
			progressMessage.Format(LoadResStr("IDS_MODS_INSTALL_UPDATEMETADATA_FOR"), mod->name.c_str());
			progressDialog->SetMessage(progressMessage.getData());

			postMetadataClient = std::make_unique<C4Network2HTTPClient>();

			if (!postMetadataClient->Init() || !postMetadataClient->SetServer((C4StartupModsDlg::baseServerURL + "uploads/" + mod->modID).c_str()))
			{
				assert(false);
				return;
			}
			postMetadataClient->SetExpectedResponseType(C4Network2HTTPClient::ResponseType::XML);
			// Do the actual request.
			Application.InteractiveThread.AddProc(postMetadataClient.get());
			postMetadataClient->Query(nullptr, false); // Empty query.
			
			metadataQueriedForModIdx = i;
			return;
		}
		// Nothing to be updated found? Great, give execution back.
		progressDialog->SetProgress(100);
		progressCallback = std::bind(&C4StartupModsDownloader::ExecutePreRequestChecks, this);
		return;
	}

	// We are already running a query!
	assert(metadataQueriedForModIdx >= 0);
	assert(metadataQueriedForModIdx < items.size());
	auto &mod = items[metadataQueriedForModIdx];
	// Check whether the data has arrived yet.
	if (!postMetadataClient->isBusy())
	{
		if (!postMetadataClient->isSuccess())
		{
			Log(postMetadataClient->GetError());
			// Destroy client and try next mod.
			Application.InteractiveThread.RemoveProc(postMetadataClient.get());
			postMetadataClient.reset();

			mod->SetError(LoadResStr("IDS_MODS_NOINSTALL_UPDATEMETADATAFAILED"));
			return;
		}

		TiXmlDocument xmlDocument;
		xmlDocument.Parse(postMetadataClient->getResultString());

		if (xmlDocument.Error())
		{
			Log(xmlDocument.ErrorDesc());
			CancelRequest();
			::pGUI->ShowMessageModal(LoadResStr("IDS_MODS_NOINSTALL_UPDATEMETADATAFAILED"), LoadResStr("IDS_MODS_NOINSTALL"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Resource);
			return;
		}
		const char * resourceElementName = "upload";
		const TiXmlElement *root = xmlDocument.RootElement();
		assert(strcmp(root->Value(), resourceElementName) == 0);

		// Re-use the parsing from the list entries.
		ModXMLData modXMLData(root, ModXMLData::Source::DetailView);

		// Find the mod matching the id from the metadata.
		size_t foundIdx = 0;
		for (;foundIdx < items.size(); ++foundIdx)
		{
			auto &mod = items[foundIdx];
			if (mod->modID != modXMLData.id) continue;
			mod->FromXMLData(modXMLData);
			break;
		}

		progressDialog->SetProgress(100 * foundIdx / items.size());

		// Somehow, the matching mod could not be found. That should not happen.
		if (foundIdx == items.size())
		{
			CancelRequest();
			::pGUI->ShowMessageModal(LoadResStr("IDS_MODS_NOINSTALL_UPDATEMETADATAFAILED"), LoadResStr("IDS_MODS_NOINSTALL"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Resource);
			assert(false);
			return;
		}
		else
		{
			// The mod might have some additional dependencies that need to be retrieved.
			const auto &mod{ items[foundIdx] };
			
			for (const std::string &dependency : mod->dependencies)
				AddModToQueue(dependency, dependency);
		}

		Application.InteractiveThread.RemoveProc(postMetadataClient.get());
		postMetadataClient.reset();
	}
}

void C4StartupModsDownloader::RequestConfirmation()
{
	progressCallback = std::bind(&C4StartupModsDownloader::ExecutePreRequestChecks, this);
}

void C4StartupModsDownloader::ExecutePreRequestChecks()
{
	// Disable callback during execution.
	progressCallback = nullptr;

	// In case some of the mods need an information update, do that first.
	for (auto &mod : items)
	{
		if (!mod->RequiresMetadataUpdate() || mod->HasError()) continue;
		progressCallback = std::bind(&C4StartupModsDownloader::ExecuteMetadataUpdate, this);
		GetProgressDialog()->SetTitle(LoadResStr("IDS_MODS_INSTALL_UPDATEMETADATA"));
		return;
	}
	// To be able to check against the installed mods, the discovery needs to be finished.
	parent->modsDiscovery.WaitForDiscoveryFinished();

	// Check all local files for compatibility.
	bool anyModNeedsCheck = false;
	for (auto &mod : items)
	{
		if (!mod->localDiscoveryCheck.needsCheck) continue;

		const bool modInstalled = mod->localDiscoveryCheck.installed = parent->modsDiscovery.IsModInstalled(mod->modID);
		mod->localDiscoveryCheck.basePath = modInstalled ? parent->modsDiscovery.GetModInformation(mod->modID).path : "";
		mod->localDiscoveryCheck.needsCheck = false;

		if (modInstalled)
		{

			for (auto file : mod->files)
			{
				if (file.sha1.empty()) continue;
				mod->localDiscoveryCheck.needsCheck = anyModNeedsCheck = true;
				break;
			}

			if (mod->localDiscoveryCheck.needsCheck)
			{
				mod->localDiscoveryCheck.Start();
			}
		}
	}

	if (anyModNeedsCheck)
	{
		progressCallback = std::bind(&C4StartupModsDownloader::ExecuteWaitForChecksums, this);
		GetProgressDialog()->SetTitle(LoadResStr("IDS_MODS_INSTALL_CHECKFILES"));
		GetProgressDialog()->SetMessage("");
		GetProgressDialog()->SetProgress(0);
	}
	else
	{
		// Just enter it directly.
		ExecuteRequestConfirmation();
	}
}

void C4StartupModsDownloader::ExecuteWaitForChecksums()
{
	for (auto &mod : items)
	{
		if (mod->localDiscoveryCheck.needsCheck)
			return;
	}
	progressCallback = std::bind(&C4StartupModsDownloader::ExecuteRequestConfirmation, this);
}

void C4StartupModsDownloader::ExecuteRequestConfirmation()
{
	progressCallback = nullptr;

	// Calculate total filesize to be downloaded.
	size_t totalSize{ 0 };
	bool atLeastOneFileExisted = false;
	std::string allMissingModNames;

	for (auto &mod : items)
	{
		if (mod->localDiscoveryCheck.atLeastOneFileExisted)
			atLeastOneFileExisted = true;
		if (!mod->files.empty())
		{
			allMissingModNames += (allMissingModNames.empty() ? "\"" : ", \"") + mod->name + "\"";
			for (auto file : mod->files)
			{
				totalSize += file.size;
			}
		}

	}

	// Hide progress bar, so it's not behind the modal dialogs.
	if (progressDialog)
		progressDialog->SetVisibility(false);

	if (totalSize == 0)
	{
		CancelRequest();
		if (atLeastOneFileExisted)
			::pGUI->ShowMessageModal(LoadResStr("IDS_MODS_NOINSTALL_ALREADYINSTALLED"), LoadResStr("IDS_MODS_NOINSTALL"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Resource);
		else
			::pGUI->ShowMessageModal(LoadResStr("IDS_MODS_NOINSTALL_NODATA"), LoadResStr("IDS_MODS_NOINSTALL"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		return;
	}

	std::string filesizeString;
	const size_t totalSizeMB = (totalSize / 1000 + 500) / 1000;
	if (totalSizeMB == 0)
	{
		filesizeString = "<1MB";
	}
	else
	{
		filesizeString = std::string("~") + std::to_string(totalSizeMB) + "MB";
	}

	StdStrBuf confirmationMessage;
	confirmationMessage.Format(LoadResStr("IDS_MODS_INSTALL_CONFIRM"), allMissingModNames.c_str(), filesizeString.c_str());
	auto *callbackHandler = new C4GUI::CallbackHandler<C4StartupModsDownloader>(this, &C4StartupModsDownloader::OnConfirmInstallation);
	auto *dialog = new C4GUI::ConfirmationDialog(confirmationMessage.getData(), LoadResStr("IDS_MODS_INSTALL_CONFIRM_TITLE"), callbackHandler, C4GUI::MessageDialog::btnYesNo, false, C4GUI::Icons::Ico_Save);
	parent->GetScreen()->ShowRemoveDlg(dialog);
}

void C4StartupModsDownloader::ModInfo::LocalDiscoveryCheck::Execute()
{
	assert(installed);

	for (auto fileIterator = mod.files.begin(); fileIterator != mod.files.end();)
	{
		auto &file = *fileIterator;
		bool fileExists = false;

		// Check if the file already exists.
		if (!file.sha1.empty())
		{
			const std::string &hashString = file.sha1;
			BYTE hash[SHA_DIGEST_LENGTH];
			const std::string filePath = basePath + DirectorySeparator + file.name;
			if (GetFileSHA1(filePath.c_str(), hash))
			{
				fileExists = true;

				// Match hashes (string against byte array).
				const size_t byteLen = 2;
				size_t index = 0;
				for (size_t offset = 0; offset < hashString.size(); offset += byteLen, index += 1)
				{
					// Oddly, the indices of the byte array to not correspond 1-to-1 to a standard sha1 string.
					const size_t hashIndex = (index / 4 * 4) + (3 - (index % 4));
					const BYTE &byte = hash[hashIndex];

					const std::string byteStr = hashString.substr(offset, byteLen);
					unsigned char byteStrValue = static_cast<unsigned char> (std::stoi(byteStr, nullptr, 16));

					if (byteStrValue != byte)
					{
						fileExists = false;
						break;
					}
				}
			}
		}

		if (fileExists)
		{
			fileIterator = mod.files.erase(fileIterator);
			atLeastOneFileExisted = true;
		}
		else
		{
			++fileIterator;
		}
	}

	// Fire-once check done.
	needsCheck = false;
	StdThread::SignalStop();
}


// ----------- C4StartupNetDlg ---------------------------------------------------------------------------------

C4StartupModsDlg::C4StartupModsDlg() : C4StartupDlg(LoadResStr("IDS_DLG_MODS")), pMasterserverClient(nullptr), fIsCollapsed(false), modsDiscovery(this)
{
	// ctor
	// key bindings
	C4CustomKey::CodeList keys;
	keys.push_back(C4KeyCodeEx(K_BACK)); keys.push_back(C4KeyCodeEx(K_LEFT));
	pKeyBack = new C4KeyBinding(keys, "StartupNetBack", KEYSCOPE_Gui,
	                            new C4GUI::DlgKeyCB<C4StartupModsDlg>(*this, &C4StartupModsDlg::KeyBack), C4CustomKey::PRIO_Dlg);
	pKeyRefresh = new C4KeyBinding(C4KeyCodeEx(K_F5), "StartupNetReload", KEYSCOPE_Gui,
	                               new C4GUI::DlgKeyCB<C4StartupModsDlg>(*this, &C4StartupModsDlg::KeyRefresh), C4CustomKey::PRIO_CtrlOverride);

	// screen calculations
	UpdateSize();
	int32_t iIconSize = C4GUI_IconExWdt;
	int32_t iButtonWidth,iCaptionFontHgt, iSideSize = std::max<int32_t>(GetBounds().Wdt/6, iIconSize);
	int32_t iButtonHeight = C4GUI_ButtonHgt, iButtonIndent = GetBounds().Wdt/40;
	::GraphicsResource.CaptionFont.GetTextExtent("<< BACK", iButtonWidth, iCaptionFontHgt, true);
	iButtonWidth *= 3;
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,0, true);
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromBottom(caMain.GetHeight()/7),0,0);
	int32_t iButtonAreaWdt = caButtonArea.GetWidth()*7/8;
	iButtonWidth = std::min<int32_t>(iButtonWidth, (iButtonAreaWdt - 8 * iButtonIndent)/4);
	iButtonIndent = (iButtonAreaWdt - 4 * iButtonWidth) / 8;
	C4GUI::ComponentAligner caButtons(caButtonArea.GetCentered(iButtonAreaWdt, iButtonHeight),iButtonIndent,0);
	C4GUI::ComponentAligner caLeftBtnArea(caMain.GetFromLeft(iSideSize), std::min<int32_t>(caMain.GetWidth()/20, (iSideSize-C4GUI_IconExWdt)/2), caMain.GetHeight()/40);
	C4GUI::ComponentAligner caConfigArea(caMain.GetFromRight(iSideSize), std::min<int32_t>(caMain.GetWidth()/20, (iSideSize-C4GUI_IconExWdt)/2), caMain.GetHeight()/40);

	// main area: Tabular to switch between game list and chat
	pMainTabular = new C4GUI::Tabular(caMain.GetAll(), C4GUI::Tabular::tbNone);
	pMainTabular->SetDrawDecoration(false);
	pMainTabular->SetSheetMargin(0);
	AddElement(pMainTabular);

	// main area: game selection sheet
	C4GUI::Tabular::Sheet *pSheetGameList = pMainTabular->AddSheet(nullptr);
	C4GUI::ComponentAligner caGameList(pSheetGameList->GetContainedClientRect(), 0,0, false);
	C4GUI::WoodenLabel *pGameListLbl; int32_t iCaptHgt = C4GUI::WoodenLabel::GetDefaultHeight(&::GraphicsResource.TextFont);
	pGameListLbl = new C4GUI::WoodenLabel(LoadResStr("IDS_MODS_MODSLIST"), caGameList.GetFromTop(iCaptHgt), C4GUI_Caption2FontClr, &::GraphicsResource.TextFont, ALeft);
	pSheetGameList->AddElement(pGameListLbl);

	// precalculate space needed for sorting labels
	int32_t maxSortLabelWidth = 0;
	
	sortingOptions =
	{
		{ "voting.sum", "IDS_MODS_SORT_RATING_DOWN", "IDS_MODS_SORT_RATING_UP" },
		{ "title", "IDS_MODS_SORT_NAME_UP", "IDS_MODS_SORT_NAME_DOWN" },
		{ "updatedAt", "IDS_MODS_SORT_DATE_DOWN", "IDS_MODS_SORT_DATE_UP" },
	};
	// Translate all labels.
	for (auto &option : sortingOptions)
	{
		int32_t iSortWdt = 100, iSortHgt;
		for (auto label : { &SortingOption::titleAsc, &SortingOption::titleDesc })
		{
			option.*label = LoadResStr(option.*label);
			// Get width of label and remember if it's the longest yet.
			::GraphicsResource.TextFont.GetTextExtent(option.*label, iSortWdt, iSortHgt, true);
			if (iSortWdt > maxSortLabelWidth)
				maxSortLabelWidth = iSortWdt;
		}
	}

	// search field
	C4GUI::WoodenLabel *pSearchLbl;
	const char *szSearchLblText = LoadResStr("IDS_NET_MSSEARCH"); // Text is the same as in the network view.
	int32_t iSearchWdt=100, iSearchHgt;
	::GraphicsResource.TextFont.GetTextExtent(szSearchLblText, iSearchWdt, iSearchHgt, true);
	C4GUI::ComponentAligner caSearch(caGameList.GetFromTop(iSearchHgt), 0,0);
	pSearchLbl = new C4GUI::WoodenLabel(szSearchLblText, caSearch.GetFromLeft(iSearchWdt+10), C4GUI_Caption2FontClr, &::GraphicsResource.TextFont);
	const char *szSearchTip = LoadResStr("IDS_MODS_SEARCH_DESC");
	pSearchLbl->SetToolTip(szSearchTip);
	pSheetGameList->AddElement(pSearchLbl);
	pSearchFieldEdt = new C4GUI::CallbackEdit<C4StartupModsDlg>(caSearch.GetFromLeft(caSearch.GetWidth() - maxSortLabelWidth - 40), this, &C4StartupModsDlg::OnSearchFieldEnter);
	pSearchFieldEdt->SetToolTip(szSearchTip);
	pSheetGameList->AddElement(pSearchFieldEdt);

	// Sorting options
	C4GUI::ComponentAligner caSorting(caSearch.GetAll(), 0, 0);
	auto pSortComboBox = new C4GUI::ComboBox(caSearch.GetAll());
	pSortComboBox->SetComboCB(new C4GUI::ComboBox_FillCallback<C4StartupModsDlg>(this, &C4StartupModsDlg::OnSortComboFill, &C4StartupModsDlg::OnSortComboSelChange));
	pSortComboBox->SetText(LoadResStr("IDS_MODS_SORT"));
	pSheetGameList->AddElement(pSortComboBox);

	pGameSelList = new C4GUI::ListBox(caGameList.GetFromTop(caGameList.GetHeight() - iCaptHgt));
	pGameSelList->SetDecoration(true, nullptr, true, true);
	pGameSelList->UpdateElementPositions();
	pGameSelList->SetSelectionDblClickFn(new C4GUI::CallbackHandler<C4StartupModsDlg>(this, &C4StartupModsDlg::OnSelDblClick));
	pGameSelList->SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<C4StartupModsDlg>(this, &C4StartupModsDlg::OnSelChange));
	pSheetGameList->AddElement(pGameSelList);

	// button area
	C4GUI::CallbackButton<C4StartupModsDlg> *btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupModsDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetFromLeft(iButtonWidth), &C4StartupModsDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
	AddElement(btnInstall = new C4GUI::CallbackButton<C4StartupModsDlg>(LoadResStr("IDS_MODS_INSTALL"), caButtons.GetFromLeft(iButtonWidth), &C4StartupModsDlg::OnInstallModBtn));
	btnInstall->SetToolTip(LoadResStr("IDS_MODS_INSTALL_DESC"));
	AddElement(btnRemove = new C4GUI::CallbackButton<C4StartupModsDlg>(LoadResStr("IDS_MODS_UNINSTALL"), caButtons.GetFromLeft(iButtonWidth), &C4StartupModsDlg::OnUninstallModBtn));
	btnRemove->SetToolTip(LoadResStr("IDS_MODS_UNINSTALL_DESC"));
	AddElement(btn = new C4GUI::CallbackButton<C4StartupModsDlg>(LoadResStr("IDS_MODS_UPDATEALL"), caButtons.GetFromLeft(iButtonWidth), &C4StartupModsDlg::OnUpdateAllBtn));
	btn->SetToolTip(LoadResStr("IDS_MODS_UPDATEALL_DESC"));

	// Left button area.
	auto buttonShowInstalled = new C4GUI::CallbackButton<C4StartupModsDlg, C4GUI::IconButton>(C4GUI::Ico_Save, caLeftBtnArea.GetFromTop(iIconSize, iIconSize), '\0', &C4StartupModsDlg::OnShowInstalledBtn);
	buttonShowInstalled->SetToolTip(LoadResStr("IDS_MODS_SHOWINSTALLED_DESC"));
	buttonShowInstalled->SetText(LoadResStr("IDS_MODS_SHOWINSTALLED"));
	AddElement(buttonShowInstalled);

	// Right button area.
	{
		auto filterLabel = new C4GUI::Label(LoadResStr("IDS_MODS_FILTER"), caConfigArea.GetFromTop(iSearchHgt), ALeft, C4GUI_Caption2FontClr, &::GraphicsResource.CaptionFont);
		AddElement(filterLabel);
	}
	{
		CStdFont *pUseFont = &(C4Startup::Get()->Graphics.BookFont);
		const char *szText = LoadResStr("IDS_MODS_FILTER_COMPATIBLE");
		int iWdt = 100, iHgt = 12;
		C4GUI::CheckBox::GetStandardCheckBoxSize(&iWdt, &iHgt, szText, pUseFont);
		filters.showCompatible = new C4GUI::CheckBox(caConfigArea.GetFromTop(iHgt, iWdt), szText, true);
		filters.showCompatible->SetToolTip(LoadResStr("IDS_MODS_FILTER_COMPATIBLE_DESC"));
		AddElement(filters.showCompatible);
	}
	// initial focus
	SetFocus(GetDlgModeFocusControl(), false);
	
	// register timer
	Application.Add(this);

	// register as receiver of reference notifies
	Application.InteractiveThread.SetCallback(Ev_HTTP_Response, this);

}

C4StartupModsDlg::~C4StartupModsDlg()
{
	CancelRequest();
	// disable notifies
	Application.InteractiveThread.ClearCallback(Ev_HTTP_Response, this);

	Application.Remove(this);
	if (pMasterserverClient) delete pMasterserverClient;
	// dtor
	delete pKeyBack;
	delete pKeyRefresh;
}

void C4StartupModsDlg::DrawElement(C4TargetFacet &cgo)
{
	// draw background
	typedef C4GUI::FullscreenDialog Base;
	Base::DrawElement(cgo);
}

void C4StartupModsDlg::OnShown()
{
	// callback when shown: Start searching for games
	C4StartupDlg::OnShown();
	QueryModList();
	OnSec1Timer();
}

void C4StartupModsDlg::OnClosed(bool fOK)
{
	// dlg abort: return to main screen
	CancelRequest();
	if (pMasterserverClient) { delete pMasterserverClient; pMasterserverClient=nullptr; }
	if (!fOK) DoBack();
}

C4GUI::Control *C4StartupModsDlg::GetDefaultControl()
{
	return nullptr;
}

C4GUI::Control *C4StartupModsDlg::GetDlgModeFocusControl()
{
	return pGameSelList;
}

void C4StartupModsDlg::QueryModList(bool loadNextPage)
{
	C4StartupModsListEntry *infoEntry{ nullptr };
	// New page requested? Leave the list as-is.
	if (loadNextPage && pGameSelList->GetLast() != nullptr)
	{
		infoEntry = static_cast<C4StartupModsListEntry*> (pGameSelList->GetLast());
	}
	else
	{
		// Clear the list and add an info entry.
		ClearList();
		infoEntry = new C4StartupModsListEntry(pGameSelList, nullptr, this);
	}
	infoEntry->MakeInfoEntry();

	// First, construct the 'where' part of the query.
	// Forward the filter-field to the server.
	std::string whereClauseContents = "";
	if (pSearchFieldEdt->GetText())
	{
		std::string searchText(pSearchFieldEdt->GetText());
		if (searchText.size() > 0)
		{
			// Sanity, escape quotes etc.
			searchText = std::regex_replace(searchText, std::regex("\""), "\\\"");
			searchText = std::regex_replace(searchText, std::regex("[ ]+"), "%20");
			whereClauseContents += "%22$text%22:{%22$search%22:%22" + searchText + "%22}";
		}
	}
	// Additional filter options set?
	if (filters.showCompatible->GetChecked())
	{
		const std::string versionTag = std::string("%22OpenClonk%20") + std::to_string(C4XVER1) + std::string(".0%22");
		whereClauseContents += std::string(whereClauseContents.empty() ? "" : ",") + "%22tags%22:" + versionTag;
	}
	// 'where' part is done; close it.
	std::string searchQueryPostfix("?");
	if (!whereClauseContents.empty())
		searchQueryPostfix += "where={" + whereClauseContents + "}&";

	// Forward the sorting criterion to the server.
	if (!sortKeySuffix.empty())
	{
		searchQueryPostfix += "sort=" + sortKeySuffix + "&";
	}

	// Request the correct page.
	const int requestedPage = loadNextPage ? pageInfo.currentPage + 1 : 1;
	searchQueryPostfix += "page=" + std::to_string(requestedPage) + "&";

	Log(searchQueryPostfix.c_str());
	// Initialize connection.
	// Abort possible running request.
	CancelRequest();
	queryWasSuccessful = false;
	postClient = std::make_unique<C4Network2HTTPClient>();
	
	if (!postClient->Init() || !postClient->SetServer((C4StartupModsDlg::baseServerURL + "uploads" + searchQueryPostfix).c_str()))
	{
		assert(false);
		return;
	}
	postClient->SetExpectedResponseType(C4Network2HTTPClient::ResponseType::XML);

	// Do the actual request.
	postClient->SetNotify(&Application.InteractiveThread);
	Application.InteractiveThread.AddProc(postClient.get());
	postClient->Query(nullptr, false); // Empty query.

	/*pMasterserverClient = new C4StartupModsListEntry(pGameSelList, nullptr, this);
	StdStrBuf strVersion; strVersion.Format("%d.%d", C4XVER1, C4XVER2);
	StdStrBuf strQuery; strQuery.format("%s?version=%s&platform=%s", Config.Network.GetLeagueServerAddress(), strVersion.getData(), C4_OS);
	pMasterserverClient->SetRefQuery(strQuery.getData(), C4StartupNetListEntry::NRQT_Masterserver);*/
}

void C4StartupModsDlg::CancelRequest()
{
	if (!postClient.get()) return;
	Application.InteractiveThread.RemoveProc(postClient.get());
	postClient.reset();
	lastQueryEndTime = time(nullptr);
}

void C4StartupModsDlg::ClearList()
{
	C4GUI::Element *pElem, *pNextElem = pGameSelList->GetFirst();
	while ((pElem = pNextElem))
	{
		pNextElem = pElem->GetNext();
		C4StartupModsListEntry *pEntry = static_cast<C4StartupModsListEntry *>(pElem);
		delete pEntry;
	}
}

void C4StartupModsDlg::AddToList(std::vector<TiXmlElementLoaderInfo> elements, ModXMLData::Source source)
{
	const bool modsDiscoveryFinished = modsDiscovery.IsDiscoveryFinished();
	for (const auto e : elements)
	{
		C4StartupModsListEntry *pEntry = new C4StartupModsListEntry(pGameSelList, nullptr, this);
		pEntry->FromXML(e.element, source, e.id, e.name);

		if (modsDiscoveryFinished && modsDiscovery.IsModInstalled(pEntry->GetID()))
		{
			C4StartupModsLocalModDiscovery::ModsInfo mod = modsDiscovery.GetModInformation(pEntry->GetID());
			pEntry->UpdateInstalledState(&mod);
		}
		else
		{
			pEntry->UpdateInstalledState(nullptr);
		}
	}
}

void C4StartupModsDlg::UpdateList(bool fGotReference, bool onlyWithLocalFiles)
{
	if (onlyWithLocalFiles)
	{
		if (postClient.get() != nullptr)
			CancelRequest();

		ClearList();
		modsDiscovery.WaitForDiscoveryFinished();

		// Load XML from mod files.
		auto lock = std::move(modsDiscovery.Lock());
		auto installedMods = modsDiscovery.GetAllModInformation();

		std::vector<TiXmlElementLoaderInfo> elements;
		for (const auto & modData: installedMods)
		{
			const auto &mod = modData.second;
			std::ifstream metadata(mod.path + DirectorySeparator + "item.xml", std::ifstream::in);
			if (!metadata.good()) continue;

			std::stringstream stream;
			stream << metadata.rdbuf();

			TiXmlDocument xmlDocument;
			xmlDocument.Parse(stream.str().c_str());
			if (xmlDocument.Error()) continue;

			const TiXmlElement *root = xmlDocument.RootElement();
			elements.emplace_back(static_cast<TiXmlElement*>(root->Clone()), mod.id, mod.name);
		}
		AddToList(elements, ModXMLData::Source::Local);
		// We took ownership, clear it.
		for (auto & e : elements)
			delete e.element;
		
		if (elements.empty())
		{
			C4StartupModsListEntry *infoEntry = new C4StartupModsListEntry(pGameSelList, nullptr, this);
			infoEntry->MakeInfoEntry();
			infoEntry->OnNoResultsFound();
		}
	}

	// Already running a query?
	if (postClient.get() != nullptr)
	{
		// Check whether the data has arrived yet.
		if (!postClient->isBusy())
		{
			// At this point we can assert that the info field is the last entry in the list.
			C4StartupModsListEntry *infoEntry = static_cast<C4StartupModsListEntry*> (pGameSelList->GetLast());
			assert(infoEntry != nullptr);

			if (!postClient->isSuccess())
			{
				Log(postClient->GetError());
				infoEntry->OnError(postClient->GetError());
				// Destroy client and try again later.
				CancelRequest();
				return;
			}
			queryWasSuccessful = true;

			TiXmlDocument xmlDocument;
			xmlDocument.Parse(postClient->getResultString());

			if (xmlDocument.Error())
			{
				Log(xmlDocument.ErrorDesc());
				CancelRequest();
				infoEntry->OnError(xmlDocument.ErrorDesc());
				return;
			}
			const char * rootElementName = "uploads";
			const char * resourceElementName = "upload";
			const TiXmlElement *root = xmlDocument.RootElement();
			assert(strcmp(root->Value(), rootElementName) == 0);

			// Parse pagination data.
			pageInfo.currentPage = pageInfo.totalPages = pageInfo.totalResults = 0;
			const TiXmlElement *meta = root->FirstChildElement("_meta");
			const TiXmlElement *pagination = meta ? meta->FirstChildElement("pagination") : nullptr;
			if (pagination != nullptr)
			{
				try
				{
					pageInfo.currentPage = std::stoi(getSafeStringValue(pagination, "page", "0"));
					pageInfo.totalResults = std::stoi(getSafeStringValue(pagination, "total", "0"));
					pageInfo.totalPages = std::stoi(getSafeStringValue(pagination, "pages", "0"));
				}
				catch (...) {}
			}
			std::vector<TiXmlElementLoaderInfo> elements;
			for (const TiXmlElement* e = root->FirstChildElement(resourceElementName); e != NULL; e = e->NextSiblingElement(resourceElementName))
				elements.push_back(e);
			AddToList(elements, ModXMLData::Source::Overview);

			// Nothing found? Notify!
			if (elements.empty())
				infoEntry->OnNoResultsFound();
			else if (pageInfo.currentPage < pageInfo.totalPages)
			{
				infoEntry->ShowPageInfo(pageInfo.currentPage, pageInfo.totalPages, pageInfo.totalResults);
				pGameSelList->RemoveElement(infoEntry);
				pGameSelList->AddElement(infoEntry);
			}
			else
				delete infoEntry;

			CancelRequest();
		}
	}
	else // Not running a query.
	{
		if (!queryWasSuccessful && lastQueryEndTime + QueryRetryTimeout < time(nullptr)) // Last query failed?
		{
			QueryModList();
			return;
		}
	}
	
	pGameSelList->FreezeScrolling();
	
	// Refresh the "installed" state of all entries after new discovery.
	bool fAnyRemoval{ false };

	if (requiredSyncWithDiscovery)
	{
		requiredSyncWithDiscovery = false;

		C4GUI::Element *pElem, *pNextElem = pGameSelList->GetFirst();
		while ((pElem = pNextElem))
		{
			pNextElem = pElem->GetNext(); // determine next exec element now - execution
			C4StartupModsListEntry *pEntry = static_cast<C4StartupModsListEntry *>(pElem);
			if (pEntry->IsInfoEntry()) continue;

			if (!modsDiscovery.IsModInstalled(pEntry->GetID()))
				pEntry->UpdateInstalledState(nullptr);
			else
			{
				C4StartupModsLocalModDiscovery::ModsInfo info = modsDiscovery.GetModInformation(pEntry->GetID());
				pEntry->UpdateInstalledState(&info);
			}
		}
	}

	// check whether view needs to be collapsed or uncollapsed
	if (fIsCollapsed && fAnyRemoval)
	{
		// try uncollapsing
		fIsCollapsed = false;
		UpdateCollapsed();
		// if scrolling is still necessary, the view will be collapsed again immediately
	}
	if (!fIsCollapsed && pGameSelList->IsScrollingNecessary())
	{
		fIsCollapsed = true;
		UpdateCollapsed();
	}

	// done; selection might have changed
	pGameSelList->UnFreezeScrolling();
	UpdateSelection(false);
}

void C4StartupModsDlg::UpdateCollapsed()
{
	// update collapsed state for all child entries
	for (C4GUI::Element *pElem = pGameSelList->GetFirst(); pElem; pElem = pElem->GetNext())
	{
		C4StartupModsListEntry *pEntry = static_cast<C4StartupModsListEntry *>(pElem);
		pEntry->UpdateCollapsed(fIsCollapsed && pElem != pGameSelList->GetSelectedItem());
	}
}

void C4StartupModsDlg::UpdateSelection(bool fUpdateCollapsed)
{
	// in collapsed view, updating the selection may uncollapse something
	if (fIsCollapsed && fUpdateCollapsed) UpdateCollapsed();

	C4StartupModsListEntry *selected = static_cast<C4StartupModsListEntry*>(pGameSelList->GetSelectedItem());
	btnInstall->SetEnabled(false);
	btnRemove->SetEnabled(false);

	if (selected != nullptr && !selected->IsInfoEntry())
	{
		btnInstall->SetEnabled(true);

		if (selected->IsInstalled())
		{
			btnRemove->SetEnabled(true);
			btnInstall->SetText(LoadResStr("IDS_MODS_UPDATE"));
			btnInstall->SetToolTip(LoadResStr("IDS_MODS_UPDATE_DESC"));
		}
		else
		{
			btnInstall->SetText(LoadResStr("IDS_MODS_INSTALL"));
			btnInstall->SetToolTip(LoadResStr("IDS_MODS_INSTALL_DESC"));
		}
	}
}

void C4StartupModsDlg::OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData)
{
	UpdateList(true);
}

void C4StartupModsDlg::CheckUpdateAll()
{
	// Wait for the discovery (in the same thread here).
	modsDiscovery.WaitForDiscoveryFinished();
	auto lock = std::move(modsDiscovery.Lock());

	const auto & allInstalledMods = modsDiscovery.GetAllModInformation();

	if (allInstalledMods.empty())
	{
		::pGUI->ShowMessageModal(LoadResStr("IDS_MODS_NOUPDATEALL_NOMODS"), LoadResStr("IDS_MODS_NOUPDATEALL"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		return;
	}

	if (downloader.get() != nullptr)
		downloader.reset();
	downloader = std::make_unique<C4StartupModsDownloader>(this, nullptr);

	for (const auto & pair : allInstalledMods)
	{
		const std::string &modID = pair.first;
		const std::string &name = pair.second.name;

		downloader->AddModToQueue(modID, name);
	}

	downloader->RequestConfirmation();
}

void C4StartupModsDlg::CheckRemoveMod()
{
	C4StartupModsListEntry *selected = static_cast<C4StartupModsListEntry*>(pGameSelList->GetSelectedItem());
	if (selected == nullptr) return;
	if (selected->IsInfoEntry()) return;

	StdStrBuf confirmationMessage;
	confirmationMessage.Format(LoadResStr("IDS_MODS_UNINSTALL_CONFIRM"), selected->GetTitle().c_str());
	auto *callbackHandler = new C4GUI::CallbackHandler<C4StartupModsDlg>(this, &C4StartupModsDlg::OnConfirmRemoveMod);
	auto *dialog = new C4GUI::ConfirmationDialog(confirmationMessage.getData(), LoadResStr("IDS_MODS_UNINSTALL"), callbackHandler, C4GUI::MessageDialog::btnYesNo, false, C4GUI::Icons::Ico_Confirm);
	GetScreen()->ShowRemoveDlg(dialog);
}

void C4StartupModsDlg::OnConfirmRemoveMod(C4GUI::Element *element)
{
	C4StartupModsListEntry *selected = static_cast<C4StartupModsListEntry*>(pGameSelList->GetSelectedItem());
	if (selected == nullptr) return;
	if (!selected->IsInstalled()) return;

	// Needs to have all infos for removing the mod.
	modsDiscovery.WaitForDiscoveryFinished();
	if (!modsDiscovery.IsModInstalled(selected->GetID())) return;
	const auto &mod = modsDiscovery.GetModInformation(selected->GetID());

	if (!EraseDirectory(mod.path.c_str()))
	{
		std::string errorMessage;
#ifdef _WIN32
		auto dw = GetLastError();
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer,
			0, NULL);

		errorMessage = std::string("||") + std::string(messageBuffer, size);
#endif
		::pGUI->ShowMessageModal((std::string(LoadResStr("IDS_MODS_NOUNINSTALL_REMOVEFAILED")) + errorMessage).c_str(), LoadResStr("IDS_MODS_NOUNINSTALL"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
	}
	else
	{
		modsDiscovery.RemoveMod(selected->GetID());
		QueueSyncWithDiscovery();
	}
}

bool C4StartupModsDlg::DoOK()
{
	if (GetFocus() == pSearchFieldEdt)
	{
		QueryModList();
		return true;
	}
	// get currently selected item
	C4GUI::Element *pSelection = pGameSelList->GetSelectedItem();
	StdCopyStrBuf strNoJoin(LoadResStr("IDS_MODS_NOINSTALL"));
	if (!pSelection)
	{
		// no ref selected: Oh noes!
		::pGUI->ShowMessageModal(
		  LoadResStr("IDS_MODS_NOINSTALL_NOMOD"),
		  strNoJoin.getData(),
		  C4GUI::MessageDialog::btnOK,
		  C4GUI::Ico_Error);
		return true;
	}
	else // Show confirmation dialogue.
	{
		auto *elem = static_cast<C4StartupModsListEntry*> (pSelection);
		if (elem->IsInfoEntry())
		{
			if (postClient.get() != nullptr) return false;
			// Next page?
			if (pageInfo.currentPage >= pageInfo.totalPages) return false;
			QueryModList(true);
			return true;
		}
		if (elem->GetID().empty()) return false;

		if (downloader.get() != nullptr)
			downloader.reset();
		downloader = std::make_unique<C4StartupModsDownloader>(this, elem);
		downloader->RequestConfirmation();
	}
	return true;
}

bool C4StartupModsDlg::DoBack()
{
	// abort dialog: Back to main
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Back);
	return true;
}

bool C4StartupModsDlg::KeyRefresh()
{
	if (!postClient.get())
		DoRefresh();
	return true;
}

void C4StartupModsDlg::DoRefresh()
{
	// restart masterserver query
	QueryModList();
	// done; update stuff
	UpdateList();
}

void C4StartupModsDlg::OnSec1Timer()
{
	// no updates if dialog is inactive (e.g., because a join password dlg is shown!)
	if (!IsActive(true))
		return;

	UpdateList(false);
}


void C4StartupModsDlg::OnSortComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	int32_t counter = 0;
	for (auto & option : sortingOptions)
	{
		// The labels were already translated earlier.
		pFiller->AddEntry(option.titleAsc, counter++);
		pFiller->AddEntry(option.titleDesc, counter++);
	}
}

bool C4StartupModsDlg::OnSortComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	const size_t selected = idNewSelection / 2;
	const bool descending = idNewSelection % 2 == 1;
	const std::string newSortKeySuffix = std::string(descending ? "-" : "") + sortingOptions[selected].key;
	if (newSortKeySuffix == sortKeySuffix) return true;
	sortKeySuffix = newSortKeySuffix;
	// Update label.
	const char *sortLabel = descending ? sortingOptions[selected].titleDesc : sortingOptions[selected].titleAsc;
	pForCombo->SetText(sortLabel);
	// Refresh view.
	QueryModList();
	return true;
}

bool C4StartupModsDlg::SetSubscreen(const char *toScreen)
{
	std::string modID(toScreen);
	if (modID.empty()) return false;

	if (downloader.get() != nullptr)
		downloader.reset();

	downloader = std::make_unique<C4StartupModsDownloader>(this, nullptr);

	std::stringstream ss(modID);
	while (std::getline(ss, modID, '-'))
		if (!modID.empty())
			downloader->AddModToQueue(modID, "???");

	downloader->RequestConfirmation();
	return true;
}
