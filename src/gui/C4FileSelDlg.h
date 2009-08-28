/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008  Sven Eberhardt
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de
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
// file selection dialogs

#ifndef INC_C4FileSelDlg
#define INC_C4FileSelDlg

#include <C4Gui.h>

// callback handler for file selection dialog
class C4FileSel_BaseCB
	{
	public:
		C4FileSel_BaseCB() {}
		virtual ~C4FileSel_BaseCB() {}

	public:
		virtual void OnFileSelected(const char *szFilename) = 0;
	};

template <class CB> class C4FileSel_CBEx : public C4FileSel_BaseCB
	{
	public:
		typedef void (CB::*FileSelFunc)(const char *szFilename, int32_t idToken);

		private:
			CB *pCBClass;
			FileSelFunc SelFunc;
			int32_t idToken;
		public:
			virtual void OnFileSelected(const char *szFilename)
				{ if (pCBClass && SelFunc) (pCBClass->*SelFunc)(szFilename, idToken); }

		public:
			C4FileSel_CBEx(CB * pCBClass, FileSelFunc SelFunc, int32_t idToken) : pCBClass(pCBClass), SelFunc(SelFunc), idToken(idToken) { }
		};

// dialog to select one or more files
class C4FileSelDlg : public C4GUI::Dialog
	{
	public:
		class ListItem : public C4GUI::Control
			{
			protected:
				StdStrBuf sFilename; // full path to file

				virtual bool IsFocusOnClick() { return false; } // do not focus; keep focus on listbox

			public:
				ListItem(const char *szFilename);
				virtual ~ListItem();

				const char *GetFilename() { return sFilename.getData(); }

				// multisel-checkbox-options
				virtual bool IsChecked() const { return false; }
				virtual void SetChecked(bool fChecked) {}
				virtual bool IsGrayed() const { return false; }
				virtual bool UserToggleCheck() { return false; }
			};

		class DefaultListItem : public ListItem
			{
			private:
				typedef ListItem BaseClass;
				class C4GUI::Icon *pIco; class C4GUI::Label *pLbl;
				class C4GUI::CheckBox *pCheck;
				class C4KeyBinding *pKeyCheck; // space activates/deactivates selected file
				bool fGrayed;

			protected:
				void UpdateOwnPos();

			public:
				DefaultListItem(const char *szFilename, bool fTruncateExtension, bool fCheckbox, bool fGrayed, C4GUI::Icons eIcon);
				virtual ~DefaultListItem();

				virtual bool IsChecked() const;
				virtual void SetChecked(bool fChecked);
				virtual bool IsGrayed() const { return fGrayed; }
				virtual bool UserToggleCheck();
			};

	private:
		typedef C4GUI::Dialog BaseClass;

		C4KeyBinding *pKeyRefresh, *pKeyEnterOverride;

		C4GUI::ComboBox *pLocationComboBox;
		C4GUI::ListBox *pFileListBox;
		C4GUI::TextWindow *pSelectionInfoBox;
		C4GUI::Button *btnOK;

		StdCopyStrBuf sTitle; // dlg title

		StdCopyStrBuf sPath; // current path
		struct Location
			{
			StdCopyStrBuf sName;
			StdCopyStrBuf sPath;
			};
		Location *pLocations;
		int32_t iLocationCount;
		int32_t iCurrentLocationIndex;


		ListItem *pSelection;
		C4FileSel_BaseCB *pSelCallback;

		void UpdateFileList(); // rebuild file listbox from sPath

	protected:
		virtual void OnShown();
		virtual void UserClose(bool fOK); // allow OK only if something is sth is selected
		virtual void OnClosed(bool fOK); // callback when dlg got closed

		virtual const char *GetFileMask() const { return NULL; }
		virtual bool IsMultiSelection() const { return false; } // if true, files are checked/unchecked using checkboxes
		virtual bool IsItemGrayed(const char *szFilename) const { return false; }
		virtual void UpdateSelection();
		virtual bool HasNoneItem() const { return false; } // if true, an empty item can be selected
		virtual bool HasPreviewArea() const { return true; }
		virtual bool HasExtraOptions() const { return false; }
		virtual void AddExtraOptions(const C4Rect &rcOptionsRect) {}
		virtual C4GUI::Icons GetFileItemIcon() const { return C4GUI::Ico_None; }
		virtual int32_t GetFileSelColWidth() const { return 0; } // width of each file selection element; 0 for default all listbox

		void OnSelChange(class C4GUI::Element *pEl) { UpdateSelection(); }
		void OnSelDblClick(class C4GUI::Element *pEl);
		bool KeyRefresh() { UpdateFileList(); return true; }
		void OnLocationComboFill(C4GUI::ComboBox_FillCB *pFiller);
		bool OnLocationComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);

		void AddLocation(const char *szName, const char *szPath); // add location to be shown in dropdown list
		void AddCheckedLocation(const char *szName, const char *szPath); // add location to be shown in dropdown list, only if path exists and isn't added yet
		int32_t GetCurrentLocationIndex() const;
		void SetCurrentLocation(int32_t idx, bool fRefresh);

		virtual ListItem *CreateListItem(const char *szFilename);
		virtual void BeginFileListUpdate() {}
		virtual void EndFileListUpdate() {}

	public:
		C4FileSelDlg(const char *szRootPath, const char *szTitle, C4FileSel_BaseCB *pSelCallback, bool fInitElements=true);
		virtual ~C4FileSelDlg();
		void InitElements();

		void SetPath(const char *szNewPath, bool fRefresh=true);
		void SetSelection(const char *szNewSelection, bool fFilenameOnly);
		StdStrBuf GetSelection(const char *szFixedSelection, bool fFilenameOnly) const; // get single selected file for single selection dlg ';'-seperated list for multi selection dlg
	};

// dialog to select a player file
class C4PlayerSelDlg : public C4FileSelDlg
	{
	protected:
		virtual const char *GetFileMask() const { return C4CFN_PlayerFiles; }
		virtual C4GUI::Icons GetFileItemIcon() const { return C4GUI::Ico_Player; }

	public:
		C4PlayerSelDlg(C4FileSel_BaseCB *pSelCallback);
	};

// dialog to select definition files
class C4DefinitionSelDlg : public C4FileSelDlg
	{
	private:
		StdStrBuf sFixedSelection; // initial selection which cannot be deselected

	protected:
		virtual void OnShown();
		virtual const char *GetFileMask() const { return C4CFN_DefFiles; }
		virtual bool IsMultiSelection() const { return true; }
		virtual bool IsItemGrayed(const char *szFilename) const;
		virtual C4GUI::Icons GetFileItemIcon() const { return C4GUI::Ico_Definition; }

	public:
		C4DefinitionSelDlg(C4FileSel_BaseCB *pSelCallback, const char *szFixedSelection);

		static bool SelectDefinitions(C4GUI::Screen *pOnScreen, StdStrBuf *pSelection);
	};

// dialog to select portrait files
class C4PortraitSelDlg : public C4FileSelDlg
	{
	public:
		enum { ImagePreviewSize = 100 };

	private:
		class ListItem : public C4FileSelDlg::ListItem
			{
			private:
				bool fError; // loading error
				bool fLoaded; // image loaded but not yet scaled
				C4FacetSurface fctImage; // portrait, if loaded
				C4FacetSurface fctLoadedImage; // image as loaded by background thread. Must be scaled by main thread
				StdCopyStrBuf sFilenameLabelText;

			protected:
				void DrawElement(C4TargetFacet &cgo);

			public:
				ListItem(const char *szFilename);

				void Load();
			};

		// portrait loader thread
		class LoaderThread : public StdThread
			{
			private:
				std::list<ListItem *> LoadItems; // items to be loaded by this thread

			public:
				LoaderThread() {}
				virtual ~LoaderThread() { Stop(); }

				void ClearLoadItems(); // stops thread
				void AddLoadItem(ListItem *pItem); // not to be called when thread is running!

			public:
				virtual void Execute();
			};

	private:
		C4GUI::CheckBox *pCheckSetPicture, *pCheckSetBigIcon;
		bool fDefSetPicture, fDefSetBigIcon;

		LoaderThread ImageLoader;

	protected:
		void OnClosed(bool fOK);
		virtual const char *GetFileMask() const { return C4CFN_ImageFiles; }
		virtual bool HasNoneItem() const { return true; } // if true, a special <none> item can be selected
		virtual bool HasPreviewArea() const { return false; } // no preview area. Preview images directly
		virtual bool HasExtraOptions() const { return true; }
		virtual void AddExtraOptions(const C4Rect &rcOptionsRect);
		virtual int32_t GetFileSelColWidth() const { return ImagePreviewSize; } // width of each file selection element

		virtual C4FileSelDlg::ListItem *CreateListItem(const char *szFilename);
		virtual void BeginFileListUpdate();
		virtual void EndFileListUpdate();

		virtual void OnIdle();

	public:
		C4PortraitSelDlg(C4FileSel_BaseCB *pSelCallback, bool fSetPicture, bool fSetBigIcon);

		bool IsSetPicture() const { return pCheckSetPicture ? pCheckSetPicture->GetChecked() : fDefSetPicture; }
		bool IsSetBigIcon() const { return pCheckSetBigIcon ? pCheckSetBigIcon->GetChecked() : fDefSetBigIcon; }

		static bool SelectPortrait(C4GUI::Screen *pOnScreen, StdStrBuf *pSelection, bool *pfSetPicture, bool *pfSetBigIcon);
	};

#endif // INC_C4FileSelDlg
