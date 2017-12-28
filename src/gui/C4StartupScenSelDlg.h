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

#ifndef INC_C4StartupScenSelDlg
#define INC_C4StartupScenSelDlg

#include "gui/C4Folder.h"
#include "gui/C4Startup.h"
#include "landscape/C4Scenario.h"
#include "player/C4ScenarioParameters.h"
#include "player/C4Achievement.h"

class C4StartupScenSelDlg;

const int32_t C4StartupScenSel_DefaultIcon_Scenario  = 14,
              C4StartupScenSel_DefaultIcon_Folder    =  0,
              C4StartupScenSel_DefaultIcon_SavegamesFolder =  29,
              C4StartupScenSel_DefaultIcon_WinFolder = 44,
              C4StartupScenSel_DefaultIcon_OldIconBG = 18,
              C4StartupScenSel_IconCount             = 45,
              C4StartupScenSel_TitlePictureWdt       = 640,
              C4StartupScenSel_TitlePictureHgt       = 480,
              C4StartupScenSel_TitlePicturePadding   = 10,
              C4StartupScenSel_TitleOverlayMargin    = 20, // number of pixels to each side of title overlay picture
              C4StartupScenSel_MaxAchievements       = 3; // maximum number of achievements shown next to entry

// a list of loaded scenarios
class C4ScenarioListLoader
{
public:
	class Folder;
	// either a scenario or scenario folder; manages singly linked tree
	class Entry
	{
	protected:
		class C4ScenarioListLoader *pLoader;
		Entry *pNext;
		class Folder *pParent;

		friend class Folder;

	protected:
		StdCopyStrBuf sName, sFilename, sDesc, sVersion, sAuthor;
		C4FacetSurface fctIcon, fctTitle;
		bool fBaseLoaded, fExLoaded;
		int iIconIndex;
		int iDifficulty;
		int iFolderIndex;

	public:
		Entry(class C4ScenarioListLoader *pLoader, class Folder *pParent);
		virtual ~Entry(); // dtor: unlink from tree

		bool Load(C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx); // load as child if pFromGrp, else directly from filename
		virtual bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) { return true; } // load custom data for entry type (e.g. scenario title fallback in Scenario.txt)
		virtual bool LoadCustomPre(C4Group &rGrp) { return true; } // preload stuff that's early in the group (Scenario.txt)
		virtual bool Start() = 0; // start/open entry
		virtual Folder *GetIsFolder() { return nullptr; } // return this if this is a folder

		const StdStrBuf &GetName() const { return sName; }
		const StdStrBuf &GetEntryFilename() const { return sFilename; }
		const StdStrBuf &GetVersion() const { return sVersion; }
		const StdStrBuf &GetAuthor() const { return sAuthor; }
		const C4Facet &GetIconFacet() const { return fctIcon; }
		const C4Facet &GetTitlePicture() const { return fctTitle; }
		const StdStrBuf &GetDesc() const { return sDesc; }
		int GetIconIndex() { return iIconIndex; }
		int GetDifficulty() { return iDifficulty; }
		int GetFolderIndex() { return iFolderIndex; }
		Entry *GetNext() const { return pNext; }
		class Folder *GetParent() const { return pParent; }
		virtual StdStrBuf GetTypeName() = 0;
		virtual bool GetAchievement(int32_t idx, C4Facet *out_facet, const char **out_description) { return false; } // return true and fill output parameters if player got the indexed achievement

		static Entry *CreateEntryForFile(const StdStrBuf &sFilename, C4ScenarioListLoader *pLoader, Folder *pParent); // create correct entry type based on file extension

		virtual bool CanOpen(StdStrBuf &sError, bool &CanHide) { return true; } // whether item can be started/opened (e.g. mission access, unregistered)
		virtual bool IsGrayed() { return false; } // additional condition for graying out - notice unreg folders are grayed but can still be opened
		virtual bool IsHidden() { return false; } // condition for hiding element completely
		virtual bool HasMissionAccess() const { return true; }
		virtual bool HasUnregisteredAccess() const { return false; }
		virtual StdStrBuf GetOpenText() = 0; // get open button text
		virtual StdStrBuf GetOpenTooltip() = 0;

		virtual const char *GetDefaultExtension() { return nullptr; } // extension to be added when item is renamed
		virtual bool SetTitleInGroup(C4Group &rGrp, const char *szNewTitle);
		bool RenameTo(const char *szNewName); // change name+filename
		virtual bool IsScenario() { return false; }

		virtual C4ScenarioParameterDefs *GetParameterDefs() { return nullptr; }
		virtual C4ScenarioParameters *GetParameters() { return nullptr; }
	};

	// a loaded scenario to be started
	class Scenario : public Entry
	{
	private:
		C4Scenario C4S;
		C4ScenarioParameterDefs ParameterDefs;
		C4ScenarioParameters Parameters; // each entry caches its parameters set by the user
		C4FacetSurface fctAchievements[C4StartupScenSel_MaxAchievements];
		StdCopyStrBuf sAchievementDescriptions[C4StartupScenSel_MaxAchievements];
		int32_t nAchievements;
		bool fNoMissionAccess;
		int32_t iMinPlrCount;

	public:
		Scenario(class C4ScenarioListLoader *pLoader, class Folder *pParent) : Entry(pLoader, pParent), fNoMissionAccess(false), nAchievements(0), iMinPlrCount(0) {}
		~Scenario() override = default;

		bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) override; // do fallbacks for title and icon; check whether scenario is valid
		bool LoadCustomPre(C4Group &rGrp) override; // load scenario core
		bool Start() override; // launch scenario!

		bool CanOpen(StdStrBuf &sError, bool &CanHide) override; // check mission access, player count, etc.
		bool IsGrayed() override { return false; } // additional option for graying out
		bool IsHidden() override { return C4S.Head.Secret && !HasMissionAccess(); } // condition for hiding element completely
		bool HasMissionAccess() const override { return !fNoMissionAccess; };         // check mission access only
		StdStrBuf GetOpenText() override; // get open button text
		StdStrBuf GetOpenTooltip() override;
		const C4Scenario &GetC4S() const { return C4S; } // get scenario core
		bool GetAchievement(int32_t idx, C4Facet *out_facet, const char **out_description) override; // return true and fill output parameters if player got the indexed achievement

		StdStrBuf GetTypeName() override { return StdCopyStrBuf(LoadResStr("IDS_TYPE_SCENARIO"), true); }

		const char *GetDefaultExtension() override { return "ocs"; }

		C4ScenarioParameterDefs *GetParameterDefs() override { return &ParameterDefs; }
		C4ScenarioParameters *GetParameters() override { return &Parameters; }

		bool IsScenario() override { return true; }
	};

	// scenario folder
	class Folder : public Entry
	{
	protected:
		C4Folder C4F;
		bool fContentsLoaded; // if set, directory contents are already loaded
		Entry *pFirst; // tree structure
		class C4MapFolderData *pMapData; // if set, contains gfx and data for special map-style folders
		friend class Entry;

	public:
		Folder(class C4ScenarioListLoader *pLoader, Folder *pParent) : Entry(pLoader, pParent), fContentsLoaded(false), pFirst(nullptr), pMapData(nullptr) {}
		~Folder() override;

		bool LoadCustomPre(C4Group &rGrp) override; // load folder core

		bool LoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf *psFilename, bool fLoadEx, bool fReload); // load folder contents as child if pFromGrp, else directly from filename
		uint32_t GetEntryCount() const;

	protected:
		void ClearChildren();
		void Sort();
		virtual bool DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx) = 0; // load folder contents as child if pFromGrp, else directly from filename

	public:
		bool Start() override; // open as subfolder
		Folder *GetIsFolder() override { return this; } // this is a folder
		Entry *GetFirstEntry() const { return pFirst; }
		void Resort() { Sort(); }
		Entry *FindEntryByName(const char *szFilename) const; // find entry by filename comparison

		bool CanOpen(StdStrBuf &sError, bool &CanHide) override { return true; } // can always open folders
		bool IsGrayed() override; // unreg folders can be opened to view stuff but they are still grayed out for clarity
		StdStrBuf GetOpenText() override; // get open button text
		StdStrBuf GetOpenTooltip() override;
		C4MapFolderData *GetMapData() const { return pMapData; }

		virtual const C4ScenarioParameterDefs *GetAchievementDefs() const { return nullptr; }
		virtual const C4AchievementGraphics *GetAchievementGfx() const { return nullptr; }
	};

	// .ocf subfolder: Read through by group
	class SubFolder : public Folder
	{
	private:
		C4ScenarioParameterDefs AchievementDefs;
		C4AchievementGraphics AchievementGfx;

	public:
		SubFolder(class C4ScenarioListLoader *pLoader, Folder *pParent) : Folder(pLoader, pParent) {}
		~SubFolder() override = default;

		const char *GetDefaultExtension() override { return "ocf"; }

		StdStrBuf GetTypeName() override { return StdCopyStrBuf(LoadResStr("IDS_TYPE_FOLDER"), true); }

		const C4ScenarioParameterDefs *GetAchievementDefs() const override { return &AchievementDefs; }
		const C4AchievementGraphics *GetAchievementGfx() const override { return &AchievementGfx; }

	protected:
		bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) override; // load custom data for entry type - icon fallback to folder icon
		bool DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx) override; // load folder contents as child if pFromGrp, else directly from filename
	};

	// regular, open folder: Read through by directory iterator
	class RegularFolder : public Folder
	{
	public:
		RegularFolder(class C4ScenarioListLoader *pLoader, Folder *pParent) : Folder(pLoader, pParent) {}
		~RegularFolder() override;

		StdStrBuf GetTypeName() override { return StdCopyStrBuf(LoadResStr("IDS_TYPE_DIRECTORY"), true); }

		void Merge(const char *szPath);

	protected:
		bool LoadCustom(C4Group &rGrp, bool fNameLoaded, bool fIconLoaded) override; // load custom data for entry type - icon fallback to folder icon
		bool DoLoadContents(C4ScenarioListLoader *pLoader, C4Group *pFromGrp, const StdStrBuf &sFilename, bool fLoadEx) override; // load folder contents as child if pFromGrp, else directly from filename

		typedef std::list<std::string> NameList;
		NameList contents;
	};

private:
	RegularFolder *pRootFolder;
	Folder *pCurrFolder; // scenario list in working directory
	int32_t iLoading, iProgress, iMaxProgress;
	StdCopyStrBuf current_load_info; // extra string for currently loaded item
	bool fAbortThis, fAbortPrevious; // activity state
	const C4ScenarioParameters &Achievements;

public:
	C4ScenarioListLoader(const C4ScenarioParameters &Achievements);
	~C4ScenarioListLoader();

private:
	// activity control (to be replaced by true multithreading)
	bool BeginActivity(bool fAbortPrevious);
	void EndActivity();

public:
	bool DoProcessCallback(int32_t iProgress, int32_t iMaxProgress, const char *current_load_info); // returns false if the activity was aborted

public:
	bool Load(const StdStrBuf &sRootFolder); // (unthreaded) loading of all entries in root folder
	bool Load(Folder *pSpecifiedFolder, bool fReload); // (unthreaded) loading of all entries in subfolder
	bool LoadExtended(Entry *pEntry); // (unthreaded) loading of desc and title image of specified entry
	bool FolderBack(); // go upwards by one folder
	bool ReloadCurrent(); // reload file list
	bool IsLoading() const { return !!iLoading; }
	Entry *GetFirstEntry() const { return pCurrFolder ? pCurrFolder->GetFirstEntry() : nullptr; }

	Folder *GetCurrFolder() const { return pCurrFolder; }
	Folder *GetRootFolder() const { return pRootFolder; }

	int32_t GetProgress() const { return iProgress; }
	int32_t GetMaxProgress() const { return iMaxProgress; }
	int32_t GetProgressPercent() const { return iProgress * 100 / std::max<int32_t>(iMaxProgress, 1); }
	const char *GetProgressInfo() const { return current_load_info.getData(); }

	const C4ScenarioParameters &GetAchievements() const { return Achievements; }
};


// -----------------------------------------------



// for map-style folders: Data for map display
class C4MapFolderData
{
private:
	struct Scenario
	{
		// compiled data
		StdStrBuf sFilename;
		StdStrBuf sBaseImage, sOverlayImage;

		// parameters for title as drawn on the map (if desired; otherwise sTitle empty)
		StdStrBuf sTitle;
		int32_t iTitleFontSize;
		uint32_t dwTitleInactClr, dwTitleActClr;
		int32_t iTitleOffX, iTitleOffY;
		uint8_t byTitleAlign;
		bool fTitleBookFont;
		bool fImgDump; // developer help: Dump background image part

		C4Rect rcOverlayPos;
		FLOAT_RECT rcfOverlayPos;

		// set during initialization
		C4FacetSurface fctBase, fctOverlay;
		C4ScenarioListLoader::Entry *pScenEntry;
		C4GUI::Button *pBtn; // used to resolve button events to scenario

		void CompileFunc(StdCompiler *pComp);
	};

	struct AccessGfx
	{
		// compiled data
		StdStrBuf sPassword;
		StdStrBuf sOverlayImage;
		C4Rect rcOverlayPos;
		FLOAT_RECT rcfOverlayPos;

		// set during initialization
		C4FacetSurface fctOverlay;

		void CompileFunc(StdCompiler *pComp);
	};

	// non-interactive map item
	class MapPic : public C4GUI::Picture
	{
	private:
		FLOAT_RECT rcfBounds; // drawing bounds
	public:
		MapPic(const FLOAT_RECT &rcfBounds, const C4Facet &rfct); // ctor

		void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons - deselect everything if clicked

	protected:
		void DrawElement(C4TargetFacet &cgo) override; // draw the image
	};

private:
	C4FacetSurface fctBackgroundPicture; FLOAT_RECT rcfBG;
	bool fCoordinatesAdjusted{false};
	C4Rect rcScenInfoArea; // area in which scenario info is displayed
	class C4ScenarioListLoader::Folder *pScenarioFolder;
	class C4ScenarioListLoader::Entry *pSelectedEntry;
	C4GUI::TextWindow *pSelectionInfoBox;
	int32_t MinResX, MinResY; // minimum resolution for display of the map
	bool fUseFullscreenMap;
	Scenario **ppScenList{nullptr}; int32_t iScenCount{0};
	AccessGfx **ppAccessGfxList{nullptr}; int32_t iAccessGfxCount{0};
	class C4StartupScenSelDlg *pMainDlg{nullptr};

public:
	C4MapFolderData() = default;
	~C4MapFolderData() { Clear(); }

private:
	void ConvertFacet2ScreenCoord(const C4Rect &rc, FLOAT_RECT *pfrc, float fBGZoomX, float fBGZoomY, int iOffX, int iOffY);
	void ConvertFacet2ScreenCoord(int32_t *piValue, float fBGZoom, int iOff);
	void ConvertFacet2ScreenCoord(C4Rect &rcMapArea, bool fAspect); // adjust coordinates of loaded facets so they match given area

protected:
	void OnButtonScenario(C4GUI::Control *pEl);

	friend class C4StartupScenSelDlg;

public:
	void Clear();
	bool Load(C4Group &hGroup, C4ScenarioListLoader::Folder *pScenLoaderFolder);
	void CompileFunc(StdCompiler *pComp);
	void CreateGUIElements(C4StartupScenSelDlg *pMainDlg, C4GUI::Window &rContainer);
	void ResetSelection();

	C4GUI::TextWindow *GetSelectionInfoBox() const { return pSelectionInfoBox; }
	C4ScenarioListLoader::Entry *GetSelectedEntry() const { return pSelectedEntry; }
};




// -----------------------------------------------

// startup dialog: Scenario selection
class C4StartupScenSelDlg : public C4StartupDlg
{
public:
	// one item in the scenario list
	class ScenListItem : public C4GUI::Window
	{
	private:
		typedef C4GUI::Window BaseClass;
		// subcomponents
		C4GUI::Picture *pIcon;       // item icon
		C4GUI::Label *pNameLabel; // item caption
		C4GUI::Picture *ppAchievements[C4StartupScenSel_MaxAchievements]; // achievement icons
		C4ScenarioListLoader::Entry *pScenListEntry; // associated, loaded item info

	public:
		ScenListItem(C4GUI::ListBox *pForListBox, C4ScenarioListLoader::Entry *pForEntry, C4GUI::Element *pInsertBeforeElement=nullptr);

	protected:
		struct RenameParams { };
		void AbortRenaming(RenameParams par);
		C4GUI::RenameEdit::RenameResult DoRenaming(RenameParams par, const char *szNewName);
	public:
		bool KeyRename();

	protected:
		void UpdateOwnPos() override; // recalculate item positioning
		void Update() {}

	public:
		void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override;
		C4ScenarioListLoader::Entry *GetEntry() const { return pScenListEntry; }
		ScenListItem *GetNext() { return static_cast<ScenListItem *>(BaseClass::GetNext()); }

		bool CheckNameHotkey(const char * c) override; // return whether this item can be selected by entering given char
	};

public:
	C4StartupScenSelDlg(bool fNetwork); // ctor
	~C4StartupScenSelDlg() override; // dtor

private:
	enum { ShowStyle_Book=0, ShowStyle_Map=1 };
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

	// book style scenario selection
	C4GUI::Label *pScenSelCaption;       // caption label atop scenario list; indicating current folder
	C4GUI::ListBox *pScenSelList;        // left page of book: Scenario selection
	C4GUI::Label *pScenSelProgressLabel; // progress label shown while scenario list is being generated
	C4GUI::Label *pScenSelProgressInfoLabel; // extra progress label showing currently processed item
	C4GUI::TextWindow *pSelectionInfo;   // used to display the description of the current selection
	class C4GameOptionsList *pSelectionOptions; // displays custom scenario options for selected item below description

	C4KeyBinding *pKeyRefresh, *pKeyBack, *pKeyForward, *pKeyRename, *pKeyDelete, *pKeyCheat;
	class C4GameOptionButtons *pGameOptionButtons;
	C4GUI::Button *pOpenBtn;
	C4GUI::Tabular *pScenSelStyleTabular;

	C4ScenarioListLoader *pScenLoader;

	// map style scenario selection
	C4MapFolderData *pMapData;
	C4Facet *pfctBackground;

	bool fIsInitialLoading;
	bool fStartNetworkGame;

	C4GUI::RenameEdit *pRenameEdit;

	// achievements of all activated players
	C4ScenarioParameters Achievements;

public:
	static C4StartupScenSelDlg *pInstance; // singleton

protected:
	int32_t GetMarginTop() override { return (rcBounds.Hgt/7); }
	bool HasBackground() override { return false; }
	void DrawElement(C4TargetFacet &cgo) override;

	bool OnEnter() override { DoOK(); return true; }
	bool OnEscape() override { DoBack(true); return true; }
	bool KeyBack() { return DoBack(true); }
	bool KeyRefresh() { DoRefresh(); return true; }
	bool KeyForward() { DoOK(); return true; }
	bool KeyRename();
	bool KeyDelete();
	bool KeyCheat();
	void KeyCheat2(const StdStrBuf &rsCheatCode);

	void DeleteConfirm(ScenListItem *pSel);

	void OnShown() override;             // callback when shown: Init file list
	void OnClosed(bool fOK) override;    // callback when dlg got closed: Return to main screen
	void OnBackBtn(C4GUI::Control *btn) { DoBack(true); }
	void OnNextBtn(C4GUI::Control *btn) { DoOK(); }
	void OnSelChange(class C4GUI::Element *pEl) { UpdateSelection(); }
	void OnSelDblClick(class C4GUI::Element *pEl) { DoOK(); }
	void OnButtonScenario(C4GUI::Control *pEl);

	void OnLeagueOptionChanged() override;

	friend class C4MapFolderData;

private:
	void UpdateList();
	void UpdateSelection();
	void ResortFolder();
	ScenListItem *GetSelectedItem();
	C4ScenarioListLoader::Entry *GetSelectedEntry();
	void SetOpenButtonDefaultText();
	void FocusScenList();
	void UpdateAchievements();

public:
	bool StartScenario(C4ScenarioListLoader::Scenario *pStartScen);
	bool OpenFolder(C4ScenarioListLoader::Folder *pNewFolder);
	void ProcessCallback() { UpdateList(); } // process callback by loader
	bool DoOK(); // open/start currently selected item
	bool DoBack(bool fAllowClose); // back folder, or abort dialog
	void DoRefresh(); // refresh file list
	void DeselectAll(); // reset focus and update selection info

	void StartRenaming(C4GUI::RenameEdit *pNewRenameEdit);
	void AbortRenaming();
	bool IsRenaming() const { return !!pRenameEdit; }
	void SetRenamingDone() { pRenameEdit=nullptr; }

	void SetBackground(C4Facet *pNewBG) { pfctBackground=pNewBG; }

	bool IsNetworkStart() const { return fStartNetworkGame; }

	friend class ScenListItem;
};


#endif // INC_C4StartupScenSelDlg
