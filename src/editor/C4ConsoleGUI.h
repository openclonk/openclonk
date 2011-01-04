/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2010  Mortimer
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

#ifndef C4CONSOLEGUI_INC
#define C4CONSOLEGUI_INC

#include "C4Application.h"
#include "C4Player.h"
#include "C4GameControl.h"
#include "StdBuf.h"

#ifdef WITH_DEVELOPER_MODE
#include <StdGtkWindow.h>
typedef CStdGtkWindow C4ConsoleBase;
#else
typedef CStdWindow C4ConsoleBase;
#endif

namespace OpenFileFlags
{
	const DWORD OFN_HIDEREADONLY = 1 << 0;
	const DWORD OFN_OVERWRITEPROMPT = 1 << 1;
	const DWORD OFN_FILEMUSTEXIST = 1 << 2;
	const DWORD OFN_ALLOWMULTISELECT = 1 << 3;

	const DWORD OFN_EXPLORER = 0; // ignored
}

// Separate class containing GUI code for C4Console while C4Console itself only contains functionality
class C4ConsoleGUI: public C4ConsoleBase
{
public:

	static const char *LIST_DIVIDER;

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
		CONSOLE_ScriptCounter,
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

	class State;

private:
	State *state;
public:
	bool Editing;
	bool fGameOpen;

	C4ConsoleGUI();
	~C4ConsoleGUI();

	void SetCursor(Cursor cursor);
	void RecordingEnabled();
	void ShowAboutWithCopyright(StdStrBuf &copyright);
	bool UpdateModeCtrls(int iMode);
	void UpdateNetMenu(Stage stage);
	void ClearNetMenu(Stage stage);
	void AddNetMenuItemForPlayer(int32_t index, StdStrBuf &text);
	void ClearInput();
	void ClearPlayerMenu();
	void SetInputFunctions(std::vector<char*> &functions);
	
	CStdWindow* CreateConsoleWindow(CStdApp *application);
	void Out(const char* message);
	bool ClearLog();
	void DisplayInfoText(InfoTextType type, StdStrBuf& text);
	void SetCaptionToFileName(const char* file_name);
	void SetCaption(const char *caption);
	bool FileSelect(char *sFilename, int iSize, const char * szFilter, DWORD dwFlags, bool fSave);
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
	
	bool PropertyDlgOpen(class C4PropertyDlg *dlg);
	void PropertyDlgUpdate(class C4PropertyDlg *dlg, StdStrBuf &text);
	void PropertyDlgSetFunctions(C4PropertyDlg *dlg, std::vector<char*> &functions);
	
	bool ToolsDlgOpen(class C4ToolsDlg *dlg);
	void ToolsDlgInitMaterialCtrls(class C4ToolsDlg *dlg);
	void ToolsDlgSetTexture(class C4ToolsDlg *dlg, const char *texture);
	void ToolsDlgSetMaterial(class C4ToolsDlg *dlg, const char *material);
	void ToolsDlgEnableControls(C4ToolsDlg *dlg);
	void ToolsDlgSelectTexture(C4ToolsDlg *dlg, const char *texture);
	void ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material);

#ifdef _WIN32
	void Win32KeepDialogsFloating(HWND hwnd = 0);
	virtual bool Win32DialogMessageHandling(MSG *msg);
#endif
#ifdef WITH_DEVELOPER_MODE
	virtual GtkWidget* InitGUI();
#endif
	
	void ClearDlg(void *dlg);
};

#endif
