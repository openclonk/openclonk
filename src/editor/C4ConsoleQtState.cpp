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
#include <C4Console.h>
#include <StdRegistry.h>
#include <C4ViewportWindow.h>
#include <C4Landscape.h>
#include <C4PlayerList.h>
#include <C4Viewport.h>
#include <C4Object.h>

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


/* Console viewports */

C4ConsoleViewportWidget::C4ConsoleViewportWidget(QMainWindow *parent, C4ViewportWindow *cvp)
	: QDockWidget("", parent), cvp(cvp)
{
	// Translated title
	setWindowTitle(LoadResStr("IDS_CNS_VIEWPORT"));
#ifdef USE_WIN32_WINDOWS
	// hack
	window = QWindow::fromWinId(reinterpret_cast<WId>(cvp->hWindow));
	QWidget *window_container = QWidget::createWindowContainer(window, parent, Qt::Widget);
	window_container->setFocusPolicy(Qt::TabFocus);
	setWidget(window_container);
#else
	TODO
#endif
	connect(this, SIGNAL(dockLocationChanged(bool)), this, SLOT(DockLocationChanged(bool)));
	OnActiveChanged(true);
}

void C4ConsoleViewportWidget::OnActiveChanged(bool active)
{
	// set color schemes for inactive / active viewport headers
	QColor bgclr = QApplication::palette(this).color(QPalette::Highlight);
	QColor fontclr = QApplication::palette(this).color(QPalette::HighlightedText);
	if (active)
		setStyleSheet(QString(
			"QDockWidget::title { background: %1; padding: 5px; } QDockWidget { color: %2; font-weight: bold; }")
			.arg(bgclr.name(), fontclr.name()));
	else
		setStyleSheet("");
}

void C4ConsoleViewportWidget::focusInEvent(QFocusEvent * event)
{
	OnActiveChanged(true);
	QDockWidget::focusInEvent(event);
}

void C4ConsoleViewportWidget::focusOutEvent(QFocusEvent * event)
{
	OnActiveChanged(false);
	QDockWidget::focusOutEvent(event);
}

void C4ConsoleViewportWidget::DockLocationChanged(Qt::DockWidgetArea new_area)
{
	// Re-docked: 
}

void C4ConsoleViewportWidget::closeEvent(QCloseEvent * event)
{
	QDockWidget::closeEvent(event);
	if (event->isAccepted())
	{
		if (cvp) cvp->Close();
		cvp = NULL;
		deleteLater();
	}
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



/* Common C4ConsoleGUI interface */

C4ConsoleGUIState::C4ConsoleGUIState(C4ConsoleGUI *console) : viewport_area(NULL),
		enabled(false), recording(false), net_enabled(false), landscape_mode(C4LSC_Dynamic),
	editcursor_mode(C4CNS_ModePlay), drawing_tool(C4TLS_Brush)
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

	// View models
	property_model.reset(new C4ConsoleQtPropListModel());
	ui.propertyTable->setModel(property_model.get());
	object_list_model.reset(new C4ConsoleQtObjectListModel(ui.objectListView));
	
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
	C4ConsoleViewportWidget *new_viewport = new C4ConsoleViewportWidget(viewport_area, cvp);
	viewport_area->addDockWidget(Qt::BottomDockWidgetArea, new_viewport);
	viewports.push_back(new_viewport);
	new_viewport->setFocus();
}

void C4ConsoleGUIState::OnViewportActiveChanged(C4ViewportWindow *cvp, bool is_active)
{
	// Viewport active state changed: Forward to appropriate viewport window
	for (auto vp : viewports)
		if (vp->GetViewportWindow() == cvp)
		{
			vp->OnActiveChanged(is_active);
		}
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
		SetComboItems(ui.propertyInputBox, ::Console.GetScriptSuggestions(::Console.PropertyDlgObject, C4Console::MRU_Object));
}
