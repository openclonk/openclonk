/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Editor windows using Qt*/

#include "C4Include.h"
#include "landscape/C4Texture.h"
#include "landscape/C4Landscape.h"
// Make sure to include landscape/* first. Otherwise, Qt will either include gltypes and that forbids including glew, or, if glew is inlcuded first, QT will undefine glew partially, and then it can't be included again.
#include "editor/C4ConsoleQtState.h"
#include "editor/C4ConsoleQtDefinitionListViewer.h"
#include "editor/C4ConsoleQtObjectListViewer.h"
#include "editor/C4ConsoleQtPropListViewer.h"
#include "editor/C4ConsoleQtShapes.h"
#include "editor/C4Console.h"
#include "editor/C4ConsoleGUI.h"
#include "script/C4AulExec.h"
#include "C4Version.h"

#include "editor/C4ConsoleQt.h"

void C4ConsoleGUI::OnStartGame()
{
	// Welcome screen made invisible on first game load
	state->HideWelcomeScreen();
}

void C4ConsoleGUI::Execute() { state->Execute(); }

void C4ConsoleGUI::SetCursor(C4ConsoleGUI::Cursor cursor)
{

}

void C4ConsoleGUI::RecordingEnabled()
{
	if (Active) state->SetRecording(true); // TODO this is never reset. Noone uses it anyway...
}

void C4ConsoleGUI::ShowAboutWithCopyright(StdStrBuf &copyright)
{
	QMessageBox::about(state->window.get(), QString(LoadResStr("IDS_MENU_ABOUT")), QString(copyright.getData()));
}

bool C4ConsoleGUI::UpdateModeCtrls(int iMode)
{
	if (!Active) return false;
	state->SetEditCursorMode(iMode);
	return true;
}

void C4ConsoleGUI::AddNetMenu()
{
	if (Active) state->SetNetEnabled(true);
}

void C4ConsoleGUI::ClearNetMenu()
{
	if (Active) state->ClearNetMenu();
}

void C4ConsoleGUI::AddNetMenuItemForPlayer(int32_t client_id, const char *text, C4ConsoleGUI::ClientOperation op)
{
	if (Active) state->AddNetMenuItem(client_id, text, op);
}

void C4ConsoleGUI::ClearPlayerMenu()
{
	if (Active) state->ClearPlayerMenu();
}

void C4ConsoleGUI::SetInputFunctions(std::list<const char*> &functions)
{
	if (Active) state->SetInputFunctions(functions);
}

bool C4ConsoleGUI::CreateConsoleWindow(C4AbstractApp *application)
{
	if (!state->CreateConsoleWindow(application)) return false;
	Active = true;
	EnableControls(fGameOpen);
	return true;
}

void C4ConsoleGUI::DeleteConsoleWindow()
{
	if (Active)
	{
		Active = false;
		state->DeleteConsoleWindow();
	}
}

void C4ConsoleGUI::Out(const char* message)
{
	// Log text: Add to log window
	if (state->window.get())
	{
		// Append text
		state->ui.logView->append(QString(message));
		// Scroll to end to display it
		QScrollBar *sb = state->ui.logView->verticalScrollBar();
		if (sb) sb->setValue(sb->maximum());
		state->Redraw();
	}
}

bool C4ConsoleGUI::ClearLog()
{
	// Empty log window
	if (!Active) return false;
	state->ui.logView->clear();
	return true;
}

void C4ConsoleGUI::DisplayInfoText(InfoTextType type, StdStrBuf& text)
{
	QLabel *target = nullptr;
	switch (type)
	{
	case CONSOLE_Cursor: target = state->status_cursor; break;
	case CONSOLE_FrameCounter: target = state->status_framecounter; break;
	case CONSOLE_TimeFPS: target = state->status_timefps; break;
	}
	if (!target) return;
	target->setText(text.getData());
}

void C4ConsoleGUI::SetCaptionToFileName(const char* file_name) { /* This is never even called? */ }

bool C4ConsoleGUI::FileSelect(StdStrBuf *sFilename, const char * szFilter, DWORD dwFlags, bool fSave)
{
	// Prepare filters from double-zero-terminated list to ";;"-separated list in Qt format
	QString filter="", selected_filter, filename;
	QStringList filenames; bool has_multi = (dwFlags & OpenFileFlags::OFN_ALLOWMULTISELECT);
	if (szFilter)
	{
		while (*szFilter)
		{
			if (filter.length() > 0) filter.append(";;");
			filter.append(szFilter);
			szFilter += strlen(szFilter) + 1;
			if (*szFilter)
			{
				filter.append(" (");
				filter.append(szFilter);
				filter.append(")");
				szFilter += strlen(szFilter) + 1;
			}
			if (selected_filter.length() <= 0) selected_filter = filter;
		}
	}
#ifdef USE_WIN32_WINDOWS
	// cwd backup
	size_t l = GetCurrentDirectoryW(0, 0);
	std::unique_ptr<wchar_t []> wd(new wchar_t[l]);
	GetCurrentDirectoryW(l, wd.get());
#endif
	// Show dialogue
	if (fSave)
		filename = QFileDialog::getSaveFileName(state->window.get(), LoadResStr("IDS_DLG_SAVE"), QString(sFilename->getData()), filter, &selected_filter);
	else if (!has_multi)
		filename = QFileDialog::getOpenFileName(state->window.get(), LoadResStr("IDS_DLG_OPEN"), QString(), filter);
	else
		filenames = QFileDialog::getOpenFileNames(state->window.get(), LoadResStr("IDS_DLG_OPEN"), QString(), filter, &selected_filter);
#ifdef USE_WIN32_WINDOWS
	// Restore cwd; may have been changed in open/save dialogue
	SetCurrentDirectoryW(wd.get());
#endif
	// Process multi vs single file select
	if (has_multi)
	{
		// Multi-select: Return double-zero-terminated string list
		if (!filenames.length()) return false;
		for (auto fn : filenames)
		{
			sFilename->Append(fn.toUtf8());
			sFilename->AppendChar('\0');
		}
		return true;
	}
	// Cancelled?
	if (filename.length() <= 0) return false;
	// File selected!
	sFilename->Copy(filename.toUtf8());
	sFilename->AppendChar('\0');
	return true;
}

void C4ConsoleGUI::AddMenuItemForPlayer(C4Player  *player, StdStrBuf& player_text)
{
	// Add "new viewport for X" to window menu
	if (Active) state->AddPlayerViewportMenuItem(player->Number, player_text.getData());
}

void C4ConsoleGUI::AddKickPlayerMenuItem(C4Player *player, StdStrBuf& player_text, bool enabled)
{
	// Add "kick X" to player menu
	if (Active) state->AddKickPlayerMenuItem(player->Number, player_text.getData(), enabled);
}

void C4ConsoleGUI::ClearViewportMenu()
{
	// Remove all "new viewport for X" entries from window menu
	if (Active) state->ClearViewportMenu();
}

bool C4ConsoleGUI::Message(const char *message, bool query)
{
	// Show a message through Qt
	if (query)
	{
		auto result = QMessageBox::question(state->window.get(), C4ENGINECAPTION, message, QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel);
		return (result == QMessageBox::StandardButton::Ok);
	}
	else
	{
		QMessageBox::information(state->window.get(), C4ENGINECAPTION, message, QMessageBox::StandardButton::Ok);
		return true;
	}
}

void C4ConsoleGUI::DoEnableControls(bool fEnable)
{
	if (!Active) return;
	state->SetEnabled(fEnable);
	state->SetLandscapeMode(::Landscape.GetMode(), ::Game.C4S.Landscape.FlatChunkShapes); // initial setting
}

bool C4ConsoleGUI::DoUpdateHaltCtrls(bool fHalt)
{
	// Reflect halt state in play/pause buttons
	if (!Active) return false;
	state->ui.actionPlay->setChecked(!fHalt);
	state->ui.actionPause->setChecked(fHalt);
	return true;
}

bool C4ConsoleGUI::PropertyDlgOpen() { /* Always open */ return true; }
void C4ConsoleGUI::PropertyDlgClose() { /* Always open */ }

void C4ConsoleGUI::PropertyDlgUpdate(C4EditCursorSelection &rSelection, bool force_function_update)
{
	if (Active) state->PropertyDlgUpdate(rSelection, force_function_update);
}

bool C4ConsoleGUI::ToolsDlgOpen(class C4ToolsDlg *dlg) { /* Always open */ return true; }
void C4ConsoleGUI::ToolsDlgClose() { /* Always open */ }

void C4ConsoleGUI::ToolsDlgInitMaterialCtrls(class C4ToolsDlg *dlg)
{
	// All foreground materials
	assert(Active);
	if (!Active) return;
	if (state->ui.foregroundMatTexComboBox->count()) return; // already initialized
	state->ui.foregroundMatTexComboBox->clear();
	state->ui.foregroundMatTexComboBox->addItem(QString(C4TLS_MatSky));
	QStringList items;
	const C4TexMapEntry *entry; int32_t i = 0;
	while ((entry = ::TextureMap.GetEntry(i++)))
	{
		if (!entry->isNull())
		{
			const char *material_name = entry->GetMaterialName();
			if (strcmp(material_name, "Vehicle") && strcmp(material_name, "HalfVehicle"))
			{
				items.append(QString(FormatString("%s-%s", material_name, entry->GetTextureName()).getData()));
			}
		}
	}
	items.sort();
	for (QString &item : items) state->ui.foregroundMatTexComboBox->addItem(item);
	auto width = 130; /* The ToolBar randomly resizes the control */
	state->ui.foregroundMatTexComboBox->view()->setMinimumWidth(width);
	state->ui.foregroundMatTexComboBox->setFixedWidth(width);
	// Background materials: True background materials first; then the "funny" stuff
	state->ui.backgroundMatTexComboBox->addItem(QString(C4TLS_MatSky));
	items.clear();
	i = 0;
	while ((entry = ::TextureMap.GetEntry(i++)))
	{
		if (!entry->isNull())
		{
			const char *material_name = entry->GetMaterialName();
			C4Material *mat = entry->GetMaterial();
			if (strcmp(material_name, "Vehicle") && strcmp(material_name, "HalfVehicle") && mat->Density == C4M_Background)
			{
				items.append(QString(FormatString("%s-%s", material_name, entry->GetTextureName()).getData()));
			}
		}
	}
	items.sort();
	for (QString &item : items) state->ui.backgroundMatTexComboBox->addItem(item);
	state->ui.backgroundMatTexComboBox->addItem(QString("----------"));
	items.clear();
	i = 0;
	while ((entry = ::TextureMap.GetEntry(i++)))
	{
		if (!entry->isNull())
		{
			const char *material_name = entry->GetMaterialName();
			C4Material *mat = entry->GetMaterial();
			if (strcmp(material_name, "Vehicle") && strcmp(material_name, "HalfVehicle") && mat->Density != C4M_Background)
			{
				items.append(QString(FormatString("%s-%s", material_name, entry->GetTextureName()).getData()));
			}
		}
	}
	items.sort();
	for (QString &item : items) state->ui.backgroundMatTexComboBox->addItem(item);
	state->ui.backgroundMatTexComboBox->view()->setMinimumWidth(width);
	state->ui.backgroundMatTexComboBox->setFixedWidth(width);
	// Select current materials
	state->SetMaterial(dlg->Material);
	state->SetTexture(dlg->Texture);
	state->SetBackMaterial(dlg->BackMaterial);
	state->SetBackTexture(dlg->BackTexture);
	state->UpdateMatTex();
	state->UpdateBackMatTex();
}

void C4ConsoleGUI::ToolsDlgSelectTexture(C4ToolsDlg *dlg, const char *texture) { if (!Active) return; state->SetTexture(texture); }
void C4ConsoleGUI::ToolsDlgSelectMaterial(C4ToolsDlg *dlg, const char *material) { if (!Active) return; state->SetMaterial(material); }
void C4ConsoleGUI::ToolsDlgSelectBackTexture(C4ToolsDlg *dlg, const char *texture) { if (!Active) return; state->SetBackTexture(texture); }
void C4ConsoleGUI::ToolsDlgSelectBackMaterial(C4ToolsDlg *dlg, const char *material) { if (!Active) return; state->SetBackMaterial(material); }

#ifdef USE_WIN32_WINDOWS
void C4ConsoleGUI::Win32KeepDialogsFloating(HWND hwnd) { /* Dialogues float nicely */ }
bool C4ConsoleGUI::Win32DialogMessageHandling(MSG *msg) { return false; /* message handling done through Qt (somehow?) */ }
void C4ConsoleGUI::UpdateMenuText(HMENU hMenu) { /* Translation done through QTranslator */ }
#endif

 void C4ConsoleGUI::AddViewport(C4ViewportWindow *cvp)
{
	// Add surrounding widget for viewport
	state->AddViewport(cvp);
}

 void C4ConsoleGUI::RemoveViewport(C4ViewportWindow *cvp)
 {
	 // Add surrounding widget for viewport
	 state->RemoveViewport(cvp);
 }

bool C4ConsoleGUI::CreateNewScenario(StdStrBuf *out_filename, bool *out_host_as_network)
{
#ifdef WITH_QT_EDITOR
	return state->CreateNewScenario(out_filename, out_host_as_network);
#else
	return false
#endif
}

 void C4ConsoleGUI::OnObjectSelectionChanged(class C4EditCursorSelection &selection)
 {
	// selection changed (through other means than creator or object list view)
	// reflect selection change in dialogues
	state->SetObjectSelection(selection);
 }

void C4ConsoleGUI::ClearGamePointers()
{
	state->ClearGamePointers();
}

void C4ConsoleGUI::EnsureDefinitionListInitialized()
{
	state->definition_list_model->EnsureInit();
}

void C4ConsoleGUI::CloseConsoleWindow()
{
	if (state && state->window) state->window->close();
}

void C4ConsoleGUI::ClearPointers(class C4Object *obj)
{
	if (state && state->object_list_model) state->object_list_model->Invalidate();
}

void C4ConsoleGUI::EditGraphControl(const class C4ControlEditGraph *control)
{
	if (state && state->property_model)
	{
		const char *path = control->GetPath();
		if (path && *path)
		{
			// Apply control to value: Resolve value
			C4Value graph_value = AulExec.DirectExec(::ScriptEngine.GetPropList(), path, "resolve graph edit", false, nullptr);
			// ...and apply changes (will check for value validity)
			C4ConsoleQtGraph::EditGraphValue(graph_value, control->GetAction(), control->GetIndex(), control->GetX(), control->GetY());
			// For remote clients, also update any edited shapes
			if (!control->LocalControl())
			{
				C4ConsoleQtShape *shape = state->property_model->GetShapeByPropertyPath(path);
				if (shape)
				{
					C4ConsoleQtGraph *shape_graph = shape->GetGraphShape();
					if (shape_graph)
					{
						shape_graph->EditGraph(false, control->GetAction(), control->GetIndex(), control->GetX(), control->GetY());
					}
				}
			}
		}
	}
}

void C4ToolsDlg::UpdateToolCtrls()
{
	// Set selected drawing tool
	if (::Console.Active) ::Console.state->SetDrawingTool(Tool);
}

void C4ToolsDlg::UpdateTextures() { /* Textures are done with materials */ }
void C4ToolsDlg::NeedPreviewUpdate() { /* No preview */}

void C4ToolsDlg::InitGradeCtrl()
{
	// Update current grade
	if (::Console.Active) ::Console.state->ui.drawSizeSlider->setValue(Grade);
}

bool C4ToolsDlg::PopMaterial()
{
	// Show material selection
	if (!::Console.Active) return false;
	::Console.state->ui.foregroundMatTexComboBox->setFocus();
	::Console.state->ui.foregroundMatTexComboBox->showPopup();
	return true;
}

bool C4ToolsDlg::PopTextures()
{
	// Show texture selection
	if (!::Console.Active) return false;
	::Console.state->ui.foregroundMatTexComboBox->setFocus();
	::Console.state->ui.foregroundMatTexComboBox->showPopup();
	return true;
}

void C4ToolsDlg::UpdateIFTControls() { /* not using IFT */ }

void C4ToolsDlg::UpdateLandscapeModeCtrls()
{
	// Update button down states for landscape mode
	if (::Console.Active) ::Console.state->SetLandscapeMode(::Landscape.GetMode(), ::Game.C4S.Landscape.FlatChunkShapes);
}


void C4ToolsDlg::EnableControls() { /* Handled internally by tool selection */ }

#include "editor/C4ConsoleGUICommon.h"
