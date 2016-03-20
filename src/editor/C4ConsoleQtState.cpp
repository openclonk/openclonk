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

#include <C4Include.h>
#include <C4ConsoleQtState.h>
#include <C4ConsoleQtPropListViewer.h>
#include <C4ConsoleQtObjectListViewer.h>
#include <C4ConsoleQtDefinitionListViewer.h>
#include <C4ConsoleQtViewport.h>
#include <C4Console.h>
#include <StdRegistry.h>
#include <C4Landscape.h>
#include <C4PlayerList.h>
#include <C4Object.h>
#include <C4Viewport.h>

/* String translation */

QString C4ConsoleQtTranslator::translate(const char * context, const char * sourceText, const char * disambiguation, int n) const
{
	// Map to LoadResStr for all QStrings marked as res
	if (disambiguation && !strcmp(disambiguation, "res"))
		return QString(LoadResStr(sourceText));
	else
		return QString(sourceText);
}

C4ConsoleQtTranslator qt_translator;

/* Kick client action */

C4ConsoleClientAction::C4ConsoleClientAction(int32_t client_id, const char *text, QObject *parent)
	: QAction(text, parent), client_id(client_id)
{
	connect(this, SIGNAL(triggered()), this, SLOT(Execute()));
}

void C4ConsoleClientAction::Execute()
{
	if (!::Control.isCtrlHost()) return;
	::Game.Clients.CtrlRemove(Game.Clients.getClientByID(client_id), LoadResStr("IDS_MSG_KICKBYMENU"));
}


/* Remove player action */

C4ConsoleRemovePlayerAction::C4ConsoleRemovePlayerAction(int32_t player_num, const char *text, QObject *parent)
	: QAction(text, parent), player_num(player_num)
{
	connect(this, SIGNAL(triggered()), this, SLOT(Execute()));
}

void C4ConsoleRemovePlayerAction::Execute()
{
	C4Player *plr = ::Players.Get(player_num);
	if (!plr) return;
	::Control.Input.Add(CID_PlrAction, C4ControlPlayerAction::Eliminate(plr));
}


/* Add viewport for player action */

C4ConsoleOpenViewportAction::C4ConsoleOpenViewportAction(int32_t player_num, const char *text, QObject *parent)
	: QAction(text, parent), player_num(player_num)
{
	connect(this, SIGNAL(triggered()), this, SLOT(Execute()));
}

void C4ConsoleOpenViewportAction::Execute()
{
	::Viewports.CreateViewport(player_num);
}


/* Recursion check to avoid some crashing Qt re-entry */

class ExecRecursionCheck
{
	static int counter;
public:
	ExecRecursionCheck() { ++counter; }
	~ExecRecursionCheck() { --counter; }

	bool IsRecursion() const { return counter > 1; }
};

int ExecRecursionCheck::counter = 0;



/* Console main window */

C4ConsoleQtMainWindow::C4ConsoleQtMainWindow(C4AbstractApp *app, C4ConsoleGUIState *state)
	: QMainWindow(NULL), state(state)
{
#ifdef USE_WIN32_WINDOWS
	HWND hWindow = reinterpret_cast<HWND>(winId());
	// Set icon
	SendMessage(hWindow, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(app->GetInstance(), MAKEINTRESOURCE(IDI_00_C4X)));
	SendMessage(hWindow, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(app->GetInstance(), MAKEINTRESOURCE(IDI_00_C4X)));
#endif
}

void C4ConsoleQtMainWindow::keyPressEvent(QKeyEvent * event)
{
	if (HandleEditorKeyDown(event)) event->setAccepted(true);
	QMainWindow::keyPressEvent(event);
}

void C4ConsoleQtMainWindow::keyReleaseEvent(QKeyEvent * event)
{
	if (HandleEditorKeyUp(event)) event->setAccepted(true);
	QMainWindow::keyPressEvent(event);
}


void C4ConsoleQtMainWindow::closeEvent(QCloseEvent *event)
{
	QMainWindow::closeEvent(event);
	::Console.Close();
}

void C4ConsoleQtMainWindow::PlayPressed(bool down)
{
	if (down)
		::Console.DoPlay();
	else // cannot un-check by pressing again
		state->ui.actionPlay->setChecked(true);
}

void C4ConsoleQtMainWindow::PausePressed(bool down)
{
	if (down)
		::Console.DoHalt();
	else // cannot un-check by pressing again
		state->ui.actionPause->setChecked(true);
}

void C4ConsoleQtMainWindow::CursorGamePressed(bool down)
{
	if (down)
		::Console.EditCursor.SetMode(C4CNS_ModePlay);
	else // cannot un-check by pressing again
		state->ui.actionCursorGame->setChecked(true);
}

void C4ConsoleQtMainWindow::CursorSelectPressed(bool down)
{
	if (down)
		::Console.EditCursor.SetMode(C4CNS_ModeEdit);
	else // cannot un-check by pressing again
		state->ui.actionCursorSelect->setChecked(true);
}

void C4ConsoleQtMainWindow::CursorCreateObjPressed(bool down)
{
	if (down)
		::Console.EditCursor.SetMode(C4CNS_ModeCreateObject);
	else // cannot un-check by pressing again
		state->ui.actionCursorCreateObj->setChecked(true);
}

void C4ConsoleQtMainWindow::CursorDrawPenPressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModeDraw);
		::Console.ToolsDlg.SetTool(C4TLS_Brush, false);
	}
	else // cannot un-check by pressing again
		state->ui.actionCursorDrawPen->setChecked(true);
}

void C4ConsoleQtMainWindow::CursorDrawLinePressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModeDraw);
		::Console.ToolsDlg.SetTool(C4TLS_Line, false);
	}
	else // cannot un-check by pressing again
		state->ui.actionCursorDrawLine->setChecked(true);
}

void C4ConsoleQtMainWindow::CursorDrawRectPressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModeDraw);
		::Console.ToolsDlg.SetTool(C4TLS_Rect, false);
	}
	else // cannot un-check by pressing again
		state->ui.actionCursorDrawRect->setChecked(true);
}

void C4ConsoleQtMainWindow::CursorFillPressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModeDraw);
		::Console.ToolsDlg.SetTool(C4TLS_Fill, false);
	}
	else // cannot un-check by pressing again
		state->ui.actionCursorFill->setChecked(true);
}


void C4ConsoleQtMainWindow::CursorPickerPressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModeDraw);
		::Console.ToolsDlg.SetTool(C4TLS_Picker, false);
	}
	else // cannot un-check by pressing again
		state->ui.actionCursorPicker->setChecked(true);
}

void C4ConsoleQtMainWindow::DynamicLandscapePressed(bool down)
{
	if (down)
		::Console.ToolsDlg.SetLandscapeMode(C4LSC_Dynamic);
	else // cannot un-check by pressing again
		state->ui.actionDynamicLandscape->setChecked(true);
}

void C4ConsoleQtMainWindow::StaticLandscapePressed(bool down)
{
	if (down)
		::Console.ToolsDlg.SetLandscapeMode(C4LSC_Static);
	else // cannot un-check by pressing again
		state->ui.actionStaticLandscape->setChecked(true);
}

void C4ConsoleQtMainWindow::ExactLandscapePressed(bool down)
{
	if (down)
		::Console.ToolsDlg.SetLandscapeMode(C4LSC_Exact);
	else // cannot un-check by pressing again
		state->ui.actionExactLandscape->setChecked(true);
}

void C4ConsoleQtMainWindow::DrawSizeChanged(int newval)
{
	::Console.ToolsDlg.SetGrade(newval);
}

// File menu
void C4ConsoleQtMainWindow::FileOpen() { ::Console.FileOpen(); }
void C4ConsoleQtMainWindow::FileOpenWithPlayers() { Console.FileOpenWPlrs(); }
void C4ConsoleQtMainWindow::FileRecord() { ::Console.FileRecord(); }
void C4ConsoleQtMainWindow::FileSave() { ::Console.FileSave(); }
void C4ConsoleQtMainWindow::FileSaveAs() { ::Console.FileSaveAs(false); }
void C4ConsoleQtMainWindow::FileSaveGameAs() { ::Console.FileSaveAs(true); }
void C4ConsoleQtMainWindow::FileClose() { ::Console.FileClose(); }
void C4ConsoleQtMainWindow::FileQuit() { ::Console.FileQuit(); }
// Player menu
void C4ConsoleQtMainWindow::PlayerJoin() { ::Console.PlayerJoin(); }
// Window menu
void C4ConsoleQtMainWindow::ViewportNew() { ::Console.ViewportNew(); }
// Help menu
void C4ConsoleQtMainWindow::HelpAbout() { ::Console.HelpAbout(); }

// Script enter
void C4ConsoleQtMainWindow::MainConsoleEditEnter()
{
	QLineEdit *main_console_edit = state->ui.consoleInputBox->lineEdit();
	::Console.RegisterRecentInput(main_console_edit->text().toUtf8(), C4Console::MRU_Scenario);
	::Console.In(main_console_edit->text().toUtf8());
}

void C4ConsoleQtMainWindow::PropertyConsoleEditEnter()
{
	QLineEdit *property_console_edit = state->ui.propertyInputBox->lineEdit();
	::Console.EditCursor.In(property_console_edit->text().toUtf8());
}

// View selection changes
void C4ConsoleQtMainWindow::OnCreatorSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	state->OnCreatorSelectionChanged(selected, deselected);
}

void C4ConsoleQtMainWindow::OnCreatorCurrentChanged(const QModelIndex & current, const QModelIndex & previous)
{
	state->OnCreatorCurrentChanged(current, previous);
}

void C4ConsoleQtMainWindow::OnObjectListSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	state->OnObjectListSelectionChanged(selected, deselected);
}

bool C4ConsoleQtMainWindow::HandleEditorKeyDown(QKeyEvent *event)
{
	switch (event->key())
	{
	case Qt::Key_Delete:
		::Console.EditCursor.Delete();
		return true;
	}
	uint32_t shift = 0;
	if (event->modifiers() & Qt::AltModifier) shift |= MK_ALT;
	if (event->modifiers() & Qt::ControlModifier) shift |= MK_CONTROL;
	if (event->modifiers() & Qt::ShiftModifier) shift |= MK_SHIFT;
	::Console.EditCursor.KeyDown(event->nativeScanCode(), shift);
	// key not handled (ignore shift handling done in EditCursor)
	return false;
}

bool C4ConsoleQtMainWindow::HandleEditorKeyUp(QKeyEvent *event)
{
	uint32_t shift = 0;
	if (event->modifiers() & Qt::AltModifier) shift |= MK_ALT;
	if (event->modifiers() & Qt::ControlModifier) shift |= MK_CONTROL;
	if (event->modifiers() & Qt::ShiftModifier) shift |= MK_SHIFT;
	::Console.EditCursor.KeyUp(event->nativeScanCode(), shift);
	// key not handled (ignore shift handling done in EditCursor)
	return false;
}

void SplitMaterialTexture(const QString &mat_tex, QString *mat, QString *tex)
{
	int sep = mat_tex.indexOf('-');
	if (sep < 0)
	{
		*mat = mat_tex;
		*tex = QString();
	}
	else
	{
		*mat = mat_tex.mid(0, sep);
		*tex = mat_tex.mid(sep + 1);
	}
}

void C4ConsoleQtMainWindow::ForegroundMaterialChanged(const QString &new_selection)
{
	QString mat, tex;
	SplitMaterialTexture(new_selection, &mat, &tex);
	if (mat.size() > 0) ::Console.ToolsDlg.SelectMaterial(mat.toUtf8(), true);
	if (tex.size() > 0) ::Console.ToolsDlg.SelectTexture(tex.toUtf8(), true);
}

void C4ConsoleQtMainWindow::BackgroundMaterialChanged(const QString &new_selection)
{
	QString mat, tex;
	SplitMaterialTexture(new_selection, &mat, &tex);
	if (mat.size() > 0) ::Console.ToolsDlg.SelectBackMaterial(mat.toUtf8(), true);
	if (tex.size() > 0) ::Console.ToolsDlg.SelectBackTexture(tex.toUtf8(), true);
}




/* Common C4ConsoleGUI interface */

C4ConsoleGUIState::C4ConsoleGUIState(C4ConsoleGUI *console) : viewport_area(NULL),
		enabled(false), recording(false), net_enabled(false), landscape_mode(C4LSC_Dynamic),
	editcursor_mode(C4CNS_ModePlay), drawing_tool(C4TLS_Brush), is_object_selection_updating(0)
{
}

C4ConsoleGUIState::~C4ConsoleGUIState()
{
}

void C4ConsoleGUIState::AddToolbarSpacer(int space)
{
	auto spacer = new QWidget();
	spacer->setFixedWidth(space);
	ui.toolBar->addWidget(spacer);
}
	
bool C4ConsoleGUIState::CreateConsoleWindow(C4AbstractApp *app)
{
	// No Qt main loop execution during console creation
	ExecRecursionCheck no_qt_recursion;

	// Initialize OpenGL.
	QSurfaceFormat format;
	format.setMajorVersion(/*REQUESTED_GL_CTX_MAJOR*/ 3);
	format.setMinorVersion(/*REQUESTED_GL_CTX_MINOR*/ 2);
	format.setRedBufferSize(8);
	format.setGreenBufferSize(8);
	format.setBlueBufferSize(8);
	format.setDepthBufferSize(8);
	format.setProfile(QSurfaceFormat::CoreProfile);
	if (Config.Graphics.DebugOpenGL)
		format.setOption(QSurfaceFormat::DebugContext);
	QSurfaceFormat::setDefaultFormat(format);
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);


	// Basic Qt+Main window setup from .ui file
	int fake_argc = 0;
	application.reset(new QApplication(fake_argc, NULL));
	application->installTranslator(&qt_translator);
	window.reset(new C4ConsoleQtMainWindow(app, this));
	ui.setupUi(window.get());

	// Setup some extra stuff that cannot be done easily in the designer
	// Divide status bar
	status_cursor = new QLabel("", window.get());
	status_framecounter = new QLabel("", window.get());
	status_timefps = new QLabel("", window.get());
	ui.statusbar->addPermanentWidget(status_cursor, 3);
	ui.statusbar->addPermanentWidget(status_framecounter, 1);
	ui.statusbar->addPermanentWidget(status_timefps, 1);
	// Move drawing tools into toolbar
	ui.toolBar->addWidget(ui.foregroundMatTexComboBox);
	ui.toolBar->addWidget(ui.backgroundMatTexComboBox);
	AddToolbarSpacer(5);
	ui.toolBar->addWidget(ui.drawSizeLabel);
	AddToolbarSpacer(5);
	ui.toolBar->addWidget(ui.drawSizeSlider);
	ui.drawSizeSlider->setMaximum(C4TLS_GradeMax);
	ui.drawSizeSlider->setMinimum(C4TLS_GradeMin);
	ui.drawSizeSlider->setValue(C4TLS_GradeDefault);
	// Console input box signal
	QLineEdit *main_console_edit = ui.consoleInputBox->lineEdit();
	main_console_edit->connect(main_console_edit, SIGNAL(returnPressed()), window.get(), SLOT(MainConsoleEditEnter()));
	QLineEdit *property_console_edit = ui.propertyInputBox->lineEdit();
	property_console_edit->connect(property_console_edit, SIGNAL(returnPressed()), window.get(), SLOT(PropertyConsoleEditEnter()));
	// Add window menu actions
	window_menu_separator = ui.menuWindows->addSeparator();
	ui.menuWindows->addAction(ui.creatorDockWidget->toggleViewAction());
	ui.menuWindows->addAction(ui.objectListDockWidget->toggleViewAction());
	ui.menuWindows->addAction(ui.propertyDockWidget->toggleViewAction());
	ui.menuWindows->addAction(ui.logDockWidget->toggleViewAction());
	// Viewport area setup
	viewport_area = new QMainWindow();
	viewport_area->setWindowFlags(Qt::Widget);
	window->setCentralWidget(viewport_area);
	window->setDockNestingEnabled(true);
	viewport_area->setDockNestingEnabled(true);
	QWidget *foo = new QWidget(viewport_area);
	viewport_area->setCentralWidget(foo);
	foo->hide();

	// View models
	property_model.reset(new C4ConsoleQtPropListModel());
	ui.propertyTable->setModel(property_model.get());
	object_list_model.reset(new C4ConsoleQtObjectListModel());
	ui.objectListView->setModel(object_list_model.get());
	window->connect(ui.objectListView->selectionModel(), &QItemSelectionModel::selectionChanged, window.get(), &C4ConsoleQtMainWindow::OnObjectListSelectionChanged);
	definition_list_model.reset(new C4ConsoleQtDefinitionListModel());
	ui.creatorTreeView->setModel(definition_list_model.get());
	window->connect(ui.creatorTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, window.get(), &C4ConsoleQtMainWindow::OnCreatorSelectionChanged);
	window->connect(ui.creatorTreeView->selectionModel(), &QItemSelectionModel::currentChanged, window.get(), &C4ConsoleQtMainWindow::OnCreatorCurrentChanged);
	
	window->showNormal();
#ifdef USE_WIN32_WINDOWS
	// Restore window position
	HWND hWindow = reinterpret_cast<HWND>(window->winId());
	RestoreWindowPosition(hWindow, "Main", Config.GetSubkeyPath("Console"));
#endif
	return true;
}

void C4ConsoleGUIState::Execute(bool redraw_only)
{
	// Avoid recursion in message processing; it's causing random crashes
	ExecRecursionCheck recursion_check;
	if (recursion_check.IsRecursion()) return;
	// Qt window message handling and object cleanup
	if (application)
	{
		if (redraw_only)
		{
			// process only non-critical events
			application->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
		else
		{
			// process everything
			application->processEvents();
			application->sendPostedEvents(0, QEvent::DeferredDelete);
		}
	}
}

// Set action pressed/checked and enabled states
void C4ConsoleGUIState::UpdateActionStates()
{
	// Enabled states
	bool has_draw_tools = enabled && landscape_mode != C4LSC_Dynamic;
	bool has_exact_draw_tools = enabled && landscape_mode == C4LSC_Exact;
	bool is_drawing = has_draw_tools && editcursor_mode == C4CNS_ModeDraw;
	ui.actionPlay->setEnabled(enabled);
	ui.actionPause->setEnabled(enabled);
	ui.actionCursorGame->setEnabled(enabled);
	ui.actionCursorSelect->setEnabled(enabled);
	ui.actionCursorCreateObj->setEnabled(enabled);
	ui.actionCursorDrawPen->setEnabled(has_draw_tools);
	ui.actionCursorDrawLine->setEnabled(has_draw_tools);
	ui.actionCursorDrawRect->setEnabled(has_draw_tools);
	ui.actionCursorPicker->setEnabled(has_draw_tools);
	ui.actionCursorFill->setEnabled(has_exact_draw_tools);
	ui.actionDynamicLandscape->setEnabled(enabled);
	ui.actionStaticLandscape->setEnabled(enabled);
	ui.actionExactLandscape->setEnabled(enabled);
	ui.foregroundMatTexComboBox->setEnabled(is_drawing);
	ui.backgroundMatTexComboBox->setEnabled(is_drawing);
	ui.drawSizeSlider->setEnabled(is_drawing);
	ui.actionFileClose->setEnabled(enabled);
	ui.actionFileRecord->setEnabled(enabled && !recording);
	ui.actionFileSaveGameAs->setEnabled(enabled);
	ui.actionFileSaveScenario->setEnabled(enabled);
	ui.actionFileSaveScenarioAs->setEnabled(enabled);
	ui.actionViewportNew->setEnabled(enabled);
	ui.actionPlayerJoin->setEnabled(enabled);
	ui.menuNet->setVisible(net_enabled);
	ui.menuNet->setHidden(!net_enabled);
	ui.menuNet->setEnabled(net_enabled);

	// Checked states
	ui.actionCursorGame->setChecked(editcursor_mode == C4CNS_ModePlay);
	ui.actionCursorSelect->setChecked(editcursor_mode == C4CNS_ModeEdit);
	ui.actionCursorCreateObj->setChecked(editcursor_mode == C4CNS_ModeCreateObject);
	ui.actionCursorDrawPen->setChecked((editcursor_mode == C4CNS_ModeDraw) && (drawing_tool == C4TLS_Brush));
	ui.actionCursorDrawLine->setChecked((editcursor_mode == C4CNS_ModeDraw) && (drawing_tool == C4TLS_Line));
	ui.actionCursorDrawRect->setChecked((editcursor_mode == C4CNS_ModeDraw) && (drawing_tool == C4TLS_Rect));
	ui.actionCursorFill->setChecked((editcursor_mode == C4CNS_ModeDraw) && (drawing_tool == C4TLS_Fill));
	ui.actionCursorPicker->setChecked((editcursor_mode == C4CNS_ModeDraw) && (drawing_tool == C4TLS_Picker));
	ui.actionDynamicLandscape->setChecked(landscape_mode == C4LSC_Dynamic);
	ui.actionStaticLandscape->setChecked(landscape_mode == C4LSC_Static);
	ui.actionExactLandscape->setChecked(landscape_mode == C4LSC_Exact);
	ui.actionFileRecord->setChecked(recording);
}

// Put function list into combo box selectable items
static void SetComboItems(QComboBox *box, std::list<const char*> &items)
{
	QString text = box->lineEdit()->text(); // remember and restore current text
	box->clear();
	for (std::list<const char*>::iterator it = items.begin(); it != items.end(); it++)
	{
		if (!*it)
			box->addItem("----------");
		else
			box->addItem(*it);
	}
	box->lineEdit()->setText(text);
}

void C4ConsoleGUIState::UpdateMatTex()
{
	// Update selection of mattex in combo box
	int new_index = 0;
	if (material != C4TLS_MatSky) new_index = ui.foregroundMatTexComboBox->findText(QString(FormatString("%s-%s", material.getData(), texture.getData()).getData()));
	if (new_index >= 0) ui.foregroundMatTexComboBox->setCurrentIndex(new_index);
}

void C4ConsoleGUIState::UpdateBackMatTex()
{
	// Update selection of mattex in combo box
	int new_index = 0;
	if (back_material != C4TLS_MatSky) new_index = ui.backgroundMatTexComboBox->findText(QString(FormatString("%s-%s", back_material.getData(), back_texture.getData()).getData()));
	if (new_index >= 0) ui.backgroundMatTexComboBox->setCurrentIndex(new_index);
}

void C4ConsoleGUIState::AddNetMenuItem(int32_t index, const char *text)
{
	auto *kick_action = new C4ConsoleClientAction(index, text, ui.menuNet);
	client_actions.emplace_back(kick_action);
	ui.menuNet->addAction(kick_action);
}

void C4ConsoleGUIState::ClearNetMenu()
{
	for (auto &action : client_actions) ui.menuNet->removeAction(action.get());
	client_actions.clear();
}

void C4ConsoleGUIState::AddKickPlayerMenuItem(int32_t plr, const char *text, bool item_enabled)
{
	auto *kick_action = new C4ConsoleRemovePlayerAction(plr, text, ui.menuPlayers);
	kick_action->setEnabled(item_enabled);
	player_actions.emplace_back(kick_action);
	ui.menuPlayers->addAction(kick_action);
}

void C4ConsoleGUIState::ClearPlayerMenu()
{
	for (auto &action : player_actions) ui.menuPlayers->removeAction(action.get());
	player_actions.clear();
}

void C4ConsoleGUIState::AddPlayerViewportMenuItem(int32_t plr, const char *text)
{
	auto *action = new C4ConsoleOpenViewportAction(plr, text, ui.menuWindows);
	viewport_actions.emplace_back(action);
	ui.menuWindows->insertAction(window_menu_separator, action);
}

void C4ConsoleGUIState::ClearViewportMenu()
{
	for (auto &action : viewport_actions) ui.menuWindows->removeAction(action.get());
	viewport_actions.clear();
}

void C4ConsoleGUIState::AddViewport(C4ViewportWindow *cvp)
{
	if (!viewport_area) return;
	C4ConsoleQtViewportDockWidget *new_viewport = new C4ConsoleQtViewportDockWidget(window.get(), viewport_area, cvp);
	viewport_area->addDockWidget(Qt::BottomDockWidgetArea, new_viewport);
	viewports.push_back(new_viewport);
	new_viewport->SetFocus();
}

void C4ConsoleGUIState::SetInputFunctions(std::list<const char*> &functions)
{
	SetComboItems(ui.consoleInputBox, functions);
}

void C4ConsoleGUIState::PropertyDlgUpdate(C4EditCursorSelection &rSelection, bool force_function_update)
{
	int sel_count = rSelection.size();
	if (sel_count != 1)
	{
		// Multi object selection: Hide property view; show info label
		ui.propertyTable->setVisible(false);
		ui.selectionInfoLabel->setText(rSelection.GetDataString().getData());
	}
	else
	{
		// Single object selection: Show property view + Object info in label
		property_model->SetPropList(rSelection.front().getPropList());
		ui.selectionInfoLabel->setText(rSelection.front().GetDataString(0).getData());
		ui.propertyTable->setVisible(true);
	}
	// Function update in script combo box
	if (force_function_update)
	{
		auto suggestions = ::Console.GetScriptSuggestions(::Console.PropertyDlgObject, C4Console::MRU_Object);
		SetComboItems(ui.propertyInputBox, suggestions);
	}
}

void C4ConsoleGUIState::ReInitDefinitions()
{
	if (definition_list_model) definition_list_model->ReInit();
}

void C4ConsoleGUIState::OnCreatorSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	if (is_object_selection_updating || !definition_list_model) return; // only process callbacks from users interacting with widget
	// Forward to EditCursor
	C4Def *def;
	for (const QModelIndex &item : deselected.indexes())
		if ((def = definition_list_model->GetDefByModelIndex(item)))
			::Console.EditCursor.RemoveFromSelection(def);
	for (const QModelIndex &item : selected.indexes())
		if ((def = definition_list_model->GetDefByModelIndex(item)))
			::Console.EditCursor.AddToSelection(def);
	::Console.EditCursor.OnSelectionChanged(true);
	// Switching to def selection mode: Remove any non-defs from selection
	if (!selected.empty())
	{
		C4EditCursorSelection sel_copy = ::Console.EditCursor.GetSelection();
		for (C4Value & v : sel_copy)
		{
			C4PropList *p = v.getPropList();
			if (!p) continue;
			if (p->GetObject() || !p->GetDef())
			{
				QModelIndex desel_index = object_list_model->GetModelIndexByItem(p);
				if (desel_index.isValid()) ui.objectListView->selectionModel()->select(desel_index, QItemSelectionModel::Deselect);
			}
		}
		// ...and switch to creator mode
		::Console.EditCursor.SetMode(C4CNS_ModeCreateObject);
	}
}

void C4ConsoleGUIState::OnObjectListSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	if (is_object_selection_updating) return; // only process callbacks from users interacting with widget
	// Forward to EditCursor
	C4PropList *p;
	for (const QModelIndex &item : deselected.indexes())
		if ((p = static_cast<C4PropList *>(item.internalPointer())))
			::Console.EditCursor.RemoveFromSelection(p);
	for (const QModelIndex &item : selected.indexes())
		if ((p = static_cast<C4PropList *>(item.internalPointer())))
			::Console.EditCursor.AddToSelection(p);
	::Console.EditCursor.OnSelectionChanged(true);
	// Switching to object/effect selection mode: Remove any non-objects/effects from selection
	if (!selected.empty())
	{
		C4EditCursorSelection sel_copy = ::Console.EditCursor.GetSelection();
		for (C4Value & v : sel_copy)
		{
			C4PropList *p = v.getPropList();
			if (!p) continue;
			C4Def *def = p->GetDef();
			if (def && !p->GetObject() && !p->GetEffect())
			{
				QModelIndex desel_index = definition_list_model->GetModelIndexByItem(def);
				if (desel_index.isValid()) ui.creatorTreeView->selectionModel()->select(desel_index, QItemSelectionModel::Deselect);
			}
		}
		// ...and switch to editing mode
		::Console.EditCursor.SetMode(C4CNS_ModeEdit);
	}
}

void C4ConsoleGUIState::SetObjectSelection(class C4EditCursorSelection &rSelection)
{
	if (!window.get()) return;
	// Callback from EditCursor when selection was changed e.g. from viewport
	// Reflect selection change in object and definition view
	C4Def *creator_def = ::Console.EditCursor.GetCreatorDef();
	++is_object_selection_updating;
	ui.objectListView->selectionModel()->clearSelection();
	ui.creatorTreeView->selectionModel()->clearSelection();
	QModelIndex last_idx_obj, last_idx_def, creator_idx;
	for (C4Value &v : rSelection)
	{
		C4PropList *p = v.getPropList();
		if (!p) continue;
		C4Def *def = p->GetDef();
		if (def && !p->GetObject())
		{
			
			QModelIndex idx = definition_list_model->GetModelIndexByItem(def);
			if (idx.isValid())
			{
				ui.creatorTreeView->selectionModel()->select(idx, QItemSelectionModel::Select);
				last_idx_def = idx;
				if (def == creator_def) creator_idx = idx;
			}
		}
		else
		{
			QModelIndex idx = object_list_model->GetModelIndexByItem(v.getPropList());
			if (idx.isValid())
			{
				ui.objectListView->selectionModel()->select(idx, QItemSelectionModel::Select);
				last_idx_obj = idx;
			}
		}
	}
	if (last_idx_obj.isValid()) ui.objectListView->scrollTo(last_idx_obj);
	if (last_idx_def.isValid()) ui.creatorTreeView->scrollTo(last_idx_def);
	else if (::Console.EditCursor.GetMode() == C4CNS_ModeCreateObject)
	{
		// Switch away from creator tool if user selected a non-definition
		::Console.EditCursor.SetMode(C4CNS_ModeEdit);
	}
	// Sync creator selection
	if (creator_idx.isValid()) ui.creatorTreeView->selectionModel()->select(creator_idx, QItemSelectionModel::Current);
	--is_object_selection_updating;
}

void C4ConsoleGUIState::OnCreatorCurrentChanged(const QModelIndex & current, const QModelIndex & previous)
{
	// A new definition was selected from the creator definition view
	// Reflect in selection and auto-switch to creation mode if necessery
	if (!definition_list_model) return;
	C4Def *new_def = definition_list_model->GetDefByModelIndex(current);
	//if (new_def) ::Console.EditCursor.SetMode(C4CNS_ModeCreateObject); - done by selection change
	::Console.EditCursor.SetCreatorDef(new_def); // set or clear def in EditCursor
}
