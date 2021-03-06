/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2017, The OpenClonk Team and contributors
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

#ifndef INC_C4StartupModsDlg
#define INC_C4StartupModsDlg

#include "gui/C4Startup.h"
#include "network/C4InteractiveThread.h"
#include "platform/StdSync.h"

#include <functional>
#include <tuple>

class TiXmlElement;
class TiXmlNode;
class C4StartupModsDlg;
class C4StartupModsListEntry;
class C4HTTPClient;

// The data that can be parsed from downloaded/cached XML files.
struct ModXMLData
{
	struct FileInfo
	{
		std::string handle;
		size_t size;
		std::string name;
		std::string sha1;
	};
	std::vector<FileInfo> files;
	std::vector<std::string> dependencies;
	std::vector<std::string> tags;
	std::string title;
	std::string id;
	std::string description, longDescription;
	std::string slug;
	bool metadataMissing{ false };
	
	// Depending on the origin of the data, we might need an update query before doing anything.
	enum class Source
	{
		Unknown,
		Local,
		Overview,
		DetailView
	};
	Source source{ Source::Unknown };
	bool requiresUpdate() const { return source != Source::DetailView; }

	ModXMLData(const TiXmlElement *xml, Source source = Source::Unknown);
	~ModXMLData();

	// Used to write the element to a file in case the mod gets installed.
	TiXmlNode *originalXMLElement{ nullptr };
};

class C4StartupModsLocalModDiscovery : public StdThread
{
protected:
	void Execute() override;
public:
	C4StartupModsLocalModDiscovery(C4StartupModsDlg *parent) :
		discoveryFinishedEvent(true),
		parent(parent)
	{
		StdThread::Start();
	}

	struct ModsInfo
	{
		std::string id;
		std::string path;
		// Name parsed from the directory name.
		std::string name;
	};

	bool IsDiscoveryFinished() const { return discoveryFinished; }
	void WaitForDiscoveryFinished() { discoveryFinishedEvent.WaitFor(-1); }

	const bool IsModInstalled(const std::string &id)
	{
		CStdLock lock(&modInformationModification);
		assert(IsDiscoveryFinished());
		return modsInformation.count(id) != 0;
	}
	ModsInfo GetModInformation(const std::string &id)
	{
		CStdLock lock(&modInformationModification);
		assert(IsDiscoveryFinished());
		return modsInformation[id];
	}
	void RemoveMod(const std::string &id)
	{
		CStdLock lock(&modInformationModification);
		modsInformation.erase(id);
	}
	ModsInfo & AddMod(const std::string &id, const std::string &path, const std::string &name)
	{
		CStdLock lock(&modInformationModification);
		modsInformation[id] = ModsInfo{ id, path, name };
		return modsInformation[id];
	}

	// Careful, lock for yourself when iterating through this.
	const std::map<std::string, ModsInfo> & GetAllModInformation()
	{
		return modsInformation;
	}
	CStdLock Lock()
	{
		return std::move(CStdLock(&modInformationModification));
	}
private:
	C4StartupModsDlg *parent;
	void ExecuteDiscovery();
	bool discoveryFinished = false;
	CStdEvent discoveryFinishedEvent;
	CStdCSec modInformationModification;

	std::map<std::string, ModsInfo> modsInformation;
};

// This contains the downloading and installation logic.
class C4StartupModsDownloader : private C4InteractiveThread::Callback, protected CStdTimerProc
{
public:
	C4StartupModsDownloader(C4StartupModsDlg *parent, const C4StartupModsListEntry *entry);
	~C4StartupModsDownloader();

private:
	class ModInfo
	{
	public:
		struct FileInfo
		{
			std::string handle;
			std::string name;
			size_t size;
			std::string sha1;
		};

		std::string modID;
		std::string name;
		std::string slug;

		ModInfo() : localDiscoveryCheck(*this) {}
		ModInfo(const C4StartupModsListEntry *entry);
		// From minimal information, will require an update.
		ModInfo(std::string modID, std::string name);
		~ModInfo() { Clear(); }

		std::vector<FileInfo> files;
		std::vector<std::string> dependencies;
		// All filenames are held separately, too, because the 'files' list will be manipulated.
		std::set<std::string> requiredFilenames;

		void Clear();
		void FromXMLData(const ModXMLData &entry);
		void CheckProgress();
		void CancelRequest();

		std::tuple<size_t, size_t> GetProgress() const { return std::make_tuple(downloadedBytes, totalBytes); }
		void SetError(const std::string &e) { errorMessage += (errorMessage.empty() ? "" : " ") + e; }
		bool HasError() const { return !errorMessage.empty(); }
		bool WasSuccessful() const { return successful && !HasError(); }
		bool IsBusy() const { return postClient.get() != nullptr; }
		bool HasFilesRemaining() const { return !files.empty(); }
		bool RequiresMetadataUpdate() const { return hasOnlyIncompleteInformation; }
		std::string GetErrorMessage() const { if (errorMessage.empty()) return ""; return name + ": " + errorMessage; }
		std::string GetPath();

		struct LocalDiscoveryCheck : protected StdThread
		{
			std::string basePath;
			bool needsCheck{ true };
			bool installed{ false };
			bool atLeastOneFileExisted{ false };

			LocalDiscoveryCheck(ModInfo &mod) : mod(mod) { };
			void Start() { StdThread::Start(); }
		protected:
			ModInfo &mod;
			void Execute() override;
		} localDiscoveryCheck;
	private:
		bool successful{ false };
		// Whether the information might be outdated or incomplete and needs an update prior to hash-checking.
		bool hasOnlyIncompleteInformation{ true };
		size_t totalSuccesfullyDownloadedBytes{ 0 };
		size_t downloadedBytes{ 0 };
		size_t totalBytes{ 0 };
		std::unique_ptr<C4HTTPClient> postClient;
		std::string errorMessage;
		TiXmlNode *originalXMLNode{ nullptr };
	};
private:
	std::vector<std::unique_ptr<ModInfo>> items;

	C4StartupModsDlg * parent;

	C4GUI::ProgressDialog *progressDialog = nullptr;
	C4GUI::ProgressDialog * GetProgressDialog();
	// For sync the GUI thread (e.g. abort button) with the background thread.
	CStdCSec guiThreadResponse;
	void CancelRequest();
	void ExecuteCheckDownloadProgress();
	void ExecutePreRequestChecks();
	void ExecuteWaitForChecksums();
	void ExecuteRequestConfirmation();

	void ExecuteMetadataUpdate();
	std::unique_ptr<C4HTTPClient> postMetadataClient;
	int metadataQueriedForModIdx{ -1 };

	std::function<void(void)> progressCallback;

	// Called by CStdTimerProc.
	bool Execute(int, pollfd *) override
	{
		CStdLock lock(&guiThreadResponse);
		if (CheckAndReset() && progressCallback)
			progressCallback();
		return true;
	}

public:
	void AddModToQueue(std::string modID, std::string name);
	void RequestConfirmation();
	void OnConfirmInstallation(C4GUI::Element *element);
	// callback from C4Network2ReferenceClient
	virtual void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) {}
};

class C4StartupModsListEntry : public C4GUI::Window
{
public:
	C4StartupModsListEntry(C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBefore, class C4StartupModsDlg *pModsDlg);
	~C4StartupModsListEntry();


	enum { InfoLabelCount = 2, MaxInfoIconCount = 10 };

private:
	C4StartupModsDlg *pModsDlg;
	C4GUI::ListBox *pList;

	C4GUI::Icon *pIcon;       // scenario icon
	C4GUI::Label *pInfoLbl[InfoLabelCount]; // info labels for reference or query; left side
	C4GUI::Label *pInfoLabelsRight[InfoLabelCount]; // info labels on the right side
	C4GUI::Icon *pInfoIcons[MaxInfoIconCount]; // right-aligned status icons at topright position
	int32_t iInfoIconCount;
	bool isInfoEntry{ false };
	C4Rect rctIconSmall;    // bounds for small icon
	C4Rect rctIconLarge;    // bounds for large icon

	StdStrBuf sInfoText[InfoLabelCount];
	StdStrBuf sInfoTextRight[InfoLabelCount];

	void AddStatusIcon(C4GUI::Icons eIcon, const char *szToolTip, bool insertLeft=true); // add a status icon with the specified tooltip

	void UpdateEntrySize();
	void UpdateText(); 
	
	C4GUI::Icons defaultIcon{ C4GUI::Icons::Ico_Resource };

	bool isInstalled;
	std::unique_ptr<ModXMLData> modXMLData;

protected:
	virtual int32_t GetListItemTopSpacing() { return 10; }
	virtual void DrawElement(C4TargetFacet &cgo);
public:
	void FromXML(const TiXmlElement *xml, ModXMLData::Source source, std::string fallbackID="", std::string fallbackName="");
	const ModXMLData &GetModXMLData() const { assert(modXMLData); return *modXMLData.get(); }
	void Clear();    // del any ref/refclient/error data

	bool Execute(); // update stuff - if false is returned, the item is to be removed
	void UpdateInstalledState(C4StartupModsLocalModDiscovery::ModsInfo *modInfo);
	void SetVisibility(bool fToValue);

	// There is a special entry that conveys status information.
	void MakeInfoEntry();
	bool IsInfoEntry() const { return isInfoEntry; }
	void OnNoResultsFound();
	void OnError(std::string message);
	void ShowPageInfo(int page, int totalPages, int totalResults);
	
	const TiXmlNode *GetXMLNode() const { return GetModXMLData().originalXMLElement; }
	std::string GetTitle() const { return GetModXMLData().title; }
	const std::vector<ModXMLData::FileInfo> & GetFileInfos() const { return GetModXMLData().files; }
	std::string GetID() const { return GetModXMLData().id; }
	bool IsInstalled() const { return isInstalled || IsLoadedFromLocal(); }
	bool IsLoadedFromLocal() const { return GetModXMLData().source == ModXMLData::Source::Local; }
};

// startup dialog: Network game selection
class C4StartupModsDlg : public C4StartupDlg, private C4InteractiveThread::Callback, private C4ApplicationSec1Timer
{
public:
	C4StartupModsDlg(); // ctor
	~C4StartupModsDlg(); // dtor

private:
	C4GUI::Tabular *pMainTabular;   // main tabular control: Contains game selection list and chat control
	C4GUI::ListBox *pGameSelList;        // game selection listbox
	C4KeyBinding *pKeyRefresh, *pKeyBack, *pKeyForward;
	
	C4GUI::Button *btnInstall, *btnRemove;
	C4GUI::IconButton* buttonShowInstalled{ nullptr };
	C4GUI::Edit *pSearchFieldEdt;
	struct _filters
	{
		C4GUI::CheckBox *showCompatible{ nullptr };
		C4GUI::CheckBox *showPlayable{ nullptr };
	} filters;
	// Whether the last query was successful. No re-fetching will be done.
	bool queryWasSuccessful = false;
	// The query will be retried on unsuccessful queries after QueryRetryTimeout seconds.
	time_t lastQueryEndTime = 0;
	static const time_t QueryRetryTimeout = 5;
	// Constructing this automatically checks for existing mods in a different thread.
	C4StartupModsLocalModDiscovery modsDiscovery;
	bool requiredSyncWithDiscovery{ false };

protected:
	virtual bool HasBackground() { return false; }
	virtual void DrawElement(C4TargetFacet &cgo);

	virtual C4GUI::Control *GetDefaultControl(); // get Auto-Focus control
	C4GUI::Control *GetDlgModeFocusControl(); // get control to be focused when main tabular sheet changes

	virtual bool OnEnter() { DoOK(); return true; }
	virtual bool OnEscape() { DoBack(); return true; }
	bool KeyBack() { return DoBack(); }
	bool KeyRefresh();
	bool KeyForward() { DoOK(); return true; }

	virtual void OnShown();             // callback when shown: Start searching for games
	virtual void OnClosed(bool fOK);    // callback when dlg got closed: Return to main screen
	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }
	void OnSearchOnlineBtn(C4GUI::Control *btn) { KeyRefresh(); }
	void OnInstallModBtn(C4GUI::Control *btn) { DoOK(); }
	void OnShowInstalledBtn(C4GUI::Control *btn);
	void OnUninstallModBtn(C4GUI::Control *btn) { CheckRemoveMod(); }
	void OnUpdateAllBtn(C4GUI::Control *btn) { CheckUpdateAll(); }
	void OnSelChange(class C4GUI::Element *pEl) { UpdateSelection(); }
	void OnSelDblClick(class C4GUI::Element *pEl) { DoOK(); }
	void OnSortComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnSortComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	
	C4GUI::Edit::InputResult OnSearchFieldEnter(C4GUI::Edit *edt, bool fPasting, bool fPastingMore)
	{ DoOK(); return C4GUI::Edit::IR_Abort; }

private:
	// Deletes lingering updates etc.
	void CancelRequest();

	static std::string GetBaseServerURL();
	static std::string GetOpenClonkVersionStringTag();
	void QueryModList(bool loadNextPage=false);
	void ClearList();
	void UpdateList(bool fGotReference = false, bool onlyWithLocalFiles = false);
	// When loading from local files, we must pass a fallback ID in case the XML is corrupted.
	// Otherwise, it wouldn't be possible to even delete installed mods without a valid XML file.
	struct TiXmlElementLoaderInfo
	{
		TiXmlElementLoaderInfo(const TiXmlElement* element, std::string id="", std::string name="") :
			element(element), id(id), name(name) {}
		const TiXmlElement* element;
		const TiXmlElement* operator->() const { return element; }
		std::string id, name;
	};
	void AddToList(std::vector<TiXmlElementLoaderInfo> elements, ModXMLData::Source source);
	void UpdateSelection();
	void CheckRemoveMod();
	void OnConfirmRemoveMod(C4GUI::Element *element);
	void CheckUpdateAll();
	//void AddReferenceQuery(const char *szAddress, C4StartupNetListEntry::QueryType eQueryType); // add a ref searcher entry and start searching

	// Set during update information retrieval.
	std::unique_ptr<C4HTTPClient> postClient;
	// Set during downloading of a mod.
	std::unique_ptr<C4StartupModsDownloader> downloader;

	// callback from C4Network2ReferenceClient
	virtual void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData);

	struct SortingOption
	{
		const char * key;
		const char * titleAsc, * titleDesc;
		SortingOption(const char * _key, const char * _titleAsc, const char * _titleDesc) :
			key(_key), titleAsc(_titleAsc), titleDesc(_titleDesc) {}
	};
	std::vector<SortingOption> sortingOptions;
	std::string sortKeySuffix = "";

	struct _PageInfo
	{
		const int maxResultsPerQuery{ 30 };
		int totalResults{ 0 };
		int currentlySkipped{ 0 };

		int getItemsToPage(int itemNumber) const
		{
			return itemNumber / maxResultsPerQuery + static_cast<int>(itemNumber % maxResultsPerQuery != 0);
		}
		int getCurrentPage() const
		{
			return getItemsToPage(currentlySkipped) + 1;
		}
		int getTotalPages() const
		{
			return getItemsToPage(totalResults);
		}
	} pageInfo;
public:
	// The "subscreen" is used by the clonk://installmod protocol and gives a mod ID to search.
	virtual bool SetSubscreen(const char *toScreen) override;
	bool DoOK(); // join currently selected item
	bool DoBack(); // abort dialog
	void DoRefresh(); // restart network search
	void QueueSyncWithDiscovery() { requiredSyncWithDiscovery = true; }
	//void OnReferenceEntryAdd(C4StartupNetListEntry *pEntry);

	void OnSec1Timer(); // idle proc: update list

	friend class C4StartupModsListEntry;
	friend class C4StartupModsDownloader;
};


#endif // INC_C4StartupModsDlg
