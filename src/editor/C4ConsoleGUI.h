/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006, Armin Burgmeier
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#ifndef C4CONSOLEGUI_INC
#define C4CONSOLEGUI_INC

#include "game/C4Application.h"
#include "player/C4Player.h"
#include "control/C4GameControl.h"
#include "lib/StdBuf.h"

namespace OpenFileFlags
{
	const DWORD OFN_HIDEREADONLY = 1 << 0;
	const DWORD OFN_OVERWRITEPROMPT = 1 << 1;
	const DWORD OFN_FILEMUSTEXIST = 1 << 2;
	const DWORD OFN_ALLOWMULTISELECT = 1 << 3;

	const DWORD OFN_EXPLORER = 0; // ignored
}

// Separate class containing GUI code for C4Console while C4Console itself only contains functionality
class C4ConsoleGUI: public C4Window
{
public:

	template<class T> class InternalState
	{
	protected:
		typedef class InternalState<T> Super;
	private:
		T *owner;
	public:
		InternalState(T *owner): owner(owner) {}
		T *GetOwner() {return owner;}
	};
	
	enum InfoTextType
	{
		CONSOLE_Cursor,
		CONSOLE_FrameCounter,
		CONSOLE_TimeFPS
	};

	enum Stage
	{
		STAGE_Start,
		STAGE_Intermediate,
		STAGE_End,
	};

	enum Cursor
	{
		CURSOR_Normal,
		CURSOR_Wait
	};

	enum ClientOperation
	{
		CO_None,
		CO_Deactivate,
		CO_Activate,
		CO_Kick
	};

	class State;

private:
	State *state;
public:
	bool Editing;
	bool fGameOpen;

	C4ConsoleGUI();
	~C4ConsoleGUI();

#ifdef WITH_QT_EDITOR
	void Execute();
	void AddViewport(C4ViewportWindow *cvp);
	void RemoveViewport(C4ViewportWindow *cvp);
	void OnObjectSelectionChanged(class C4EditCursorSelection &selection); // selection changed (through other means than creator or object list view)
	bool CreateNewScenario(StdStrBuf *out_filename, bool *out_host_as_network);
	void OnStartGame();
	void ClearGamePointers();
	void EnsureDefinitionListInitialized();
	void CloseConsoleWindow();
	void ClearPointers(class C4Object *obj);
	void EditGraphControl(const class C4ControlEditGraph *control);

	// TODO some qt editor stuff is in state and needs to be public
	// Once other editors are removed, C4ConsoleGUI, C4ConsoleQt and C4ConsoleQtState should be reorganized
	State *GetState() const { return state; }

	friend class C4ConsoleQtMainWindow;
	friend class C4ToolsDlg;
#else
	void Execute() { }
	void AddViewport(C4ViewportWindow *cvp) { }
	void RemoveViewport(C4ViewportWindow *cvp) { }
	void OnObjectSelectionChanged(class C4EditCursorSelection &selection) { }
	bool CreateNewScenario(StdStrBuf *out_filename, bool *out_host_as_network) { return false; }
	void OnStartGame() { }
	void EnsureDefinitionListInitialized() { }
	void CloseConsoleWindow() {}
	void ClearPointers(class C4Object *obj) {}
	void EditGraphControl(const class C4ControlEditGraph *control) {}
#endif

	void SetCursor(Cursor cursor);
	void RecordingEnabled();
	void ShowAboutWithCopyright(StdStrBuf &copyright);
	bool UpdateModeCtrls(int iMode);
	void AddNetMenu();
	void ClearNetMenu();
	void AddNetMenuItemForPlayer(int32_t client_id, const char *text, C4ConsoleGUI::ClientOperation co);
	void ClearPlayerMenu();
	void SetInputFunctions(std::list<const char*> &functions);
	
	bool CreateConsoleWindow(C4AbstractApp *application);
	void DeleteConsoleWindow();
	void Out(const char* message);
	bool ClearLog();
	void DisplayInfoText(InfoTextType type, StdStrBuf& text);
	void SetCaptionToFileName(const char* file_name);
	bool FileSelect(StdStrBuf *sFilename, const char * szFilter, DWORD dwFlags, bool fSave);
	void AddMenuItemForPlayer(C4Player  *player, StdStrBuf& player_text);
	void AddKickPlayerMenuItem(C4Player *player, StdStrBuf& player_text, bool enabled);
	void ClearViewportMenu();
	bool Message(const char *message, bool query);

	void EnableControls(bool fEnable)
	{
		if (!Active) return;
		// disable Editing if no input allowed
		Editing &= !::Control.NoInput();
		DoEnableControls(fEnable);
	}
	void DoEnableControls(bool fEnable);

	bool UpdateHaltCtrls(bool fHalt)
	{
		if (!Active)
			return false;
		DoUpdateHaltCtrls(fHalt);
		return true;
	}
	bool DoUpdateHaltCtrls(bool fHalt);
	
	bool PropertyDlgOpen();
	void PropertyDlgClose();
	void PropertyDlgUpdate(class C4EditCursorSelection &rSelection, bool force_function_update);
	C4Object * PropertyDlgObject;
	
	bool ToolsDlgOpen(class C4ToolsDlg *dlg);
	void ToolsDlgClose();
	void ToolsDlgInitMaterialCtrls(class C4ToolsDlg *dlg);
	void ToolsDlgSelectTexture(C4ToolsDlg *dlg, const char *texture);
	void ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material);
	void ToolsDlgSelectBackTexture(C4ToolsDlg *dlg, const char *texture);
	void ToolsDlgSelectBackMaterial(C4ToolsDlg *dlg, const char *material);

#ifdef USE_WIN32_WINDOWS
	void Win32KeepDialogsFloating(HWND hwnd = 0);
	virtual bool Win32DialogMessageHandling(MSG *msg);
	void UpdateMenuText(HMENU hMenu);

	friend INT_PTR CALLBACK PropertyDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	friend INT_PTR CALLBACK ConsoleDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif
};

#endif
