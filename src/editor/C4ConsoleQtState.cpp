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
#include "editor/C4ConsoleQtState.h"
#include "editor/C4ConsoleQtPropListViewer.h"
#include "editor/C4ConsoleQtObjectListViewer.h"
#include "editor/C4ConsoleQtDefinitionListViewer.h"
#include "editor/C4ConsoleQtNewScenario.h"
#include "editor/C4ConsoleQtViewport.h"
#include "editor/C4ConsoleQtShapes.h"
#include "editor/C4Console.h"
#include "platform/StdRegistry.h"
#include "landscape/C4Landscape.h"
#include "player/C4PlayerList.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "game/C4Viewport.h"
#include "config/C4Config.h"

#ifdef USE_WIN32_WINDOWS
#include <shellapi.h>
#endif

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

C4ConsoleClientAction::C4ConsoleClientAction(int32_t client_id, const char *text, QObject *parent, C4ConsoleGUI::ClientOperation op)
	: QAction(text, parent), client_id(client_id), op(op)
{
	connect(this, SIGNAL(triggered()), this, SLOT(Execute()));
}

void C4ConsoleClientAction::Execute()
{
	if (!::Control.isCtrlHost()) return;
	switch (op)
	{
	case C4ConsoleGUI::CO_Deactivate:
		::Control.DoInput(CID_ClientUpdate, new C4ControlClientUpdate(client_id, CUT_Activate, false), CDT_Sync);
		break;
	case C4ConsoleGUI::CO_Activate:
		::Control.DoInput(CID_ClientUpdate, new C4ControlClientUpdate(client_id, CUT_Activate, true), CDT_Sync);
		break;
	case C4ConsoleGUI::CO_Kick:
		::Game.Clients.CtrlRemove(Game.Clients.getClientByID(client_id), LoadResStr("IDS_MSG_KICKBYMENU"));
		break;
	}	
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
	: QMainWindow(nullptr), state(state)
{
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

// Editor window state config file
struct EditorWindowState
{
	StdCopyBuf geometry, window_state;

	void CompileFunc(StdCompiler *comp)
	{
		comp->Value(geometry);
		comp->Value(window_state);
	}
};

void C4ConsoleQtMainWindow::closeEvent(QCloseEvent *event)
{
	// Store window settings
	EditorWindowState ws;
	QByteArray geometry = saveGeometry(), window_state = saveState();
	ws.geometry.Copy(geometry.constData(), geometry.size());
	ws.window_state.Copy(window_state.constData(), window_state.size());
	StdBuf ws_contents = DecompileToBuf<StdCompilerBinWrite>(ws);
	ws_contents.SaveToFile(Config.AtUserDataPath(C4CFN_EditorGeometry));
	// Perform close
	QMainWindow::closeEvent(event);
	::Console.Close();
}

void C4ConsoleQtMainWindow::LoadGeometry()
{
	// Restore window settings from file
	StdBuf ws_contents;
	if (ws_contents.LoadFromFile(Config.AtUserDataPath(C4CFN_EditorGeometry)))
	{
		try
		{
			EditorWindowState ws;
			CompileFromBuf<StdCompilerBinRead>(ws, ws_contents);
			QByteArray geometry(static_cast<const char *>(ws.geometry.getData()), ws.geometry.getSize()),
					   window_state(static_cast<const char *>(ws.window_state.getData()), ws.window_state.getSize());
			restoreGeometry(geometry);
			restoreState(window_state);
		}
		catch (StdCompiler::Exception *e)
		{
			Log("Editor: Could not restore window settings");
			delete e;
		}
	}
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
	else // can un-check by pressing again!
		::Console.DoPlay();
}

void C4ConsoleQtMainWindow::CursorGamePressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModePlay);
	}
	else
	{
		// cannot un-check by pressing again
		state->ui.actionCursorGame->setChecked(true);
	}
}

void C4ConsoleQtMainWindow::CursorSelectPressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModeEdit);
		// When the select cursor is activated, always show either the property dock
		state->ui.propertyDockWidget->raise();
	}
	else
	{
		// cannot un-check by pressing again
		state->ui.actionCursorSelect->setChecked(true);
	}
}

void C4ConsoleQtMainWindow::CursorCreateObjPressed(bool down)
{
	if (down)
	{
		::Console.EditCursor.SetMode(C4CNS_ModeCreateObject);
		// When the creator cursor is activated, always show the defintiion list
		state->ui.creatorDockWidget->raise();
	}
	else
	{
		// cannot un-check by pressing again
		state->ui.actionCursorCreateObj->setChecked(true);
	}
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
		::Console.ToolsDlg.SetLandscapeMode(LandscapeMode::Dynamic, false);
	else // cannot un-check by pressing again
		state->ui.actionDynamicLandscape->setChecked(true);
}

void C4ConsoleQtMainWindow::StaticLandscapePressed(bool down)
{
	if (down)
		::Console.ToolsDlg.SetLandscapeMode(LandscapeMode::Static, false);
	else // cannot un-check by pressing again
		state->ui.actionStaticLandscape->setChecked(true);
}

void C4ConsoleQtMainWindow::StaticFlatLandscapePressed(bool down)
{
	if (down)
		::Console.ToolsDlg.SetLandscapeMode(LandscapeMode::Static, true);
	else // cannot un-check by pressing again
		state->ui.actionStaticFlatLandscape->setChecked(true);
}

void C4ConsoleQtMainWindow::ExactLandscapePressed(bool down)
{
	if (down)
		::Console.ToolsDlg.SetLandscapeMode(LandscapeMode::Exact, false);
	else // cannot un-check by pressing again
		state->ui.actionExactLandscape->setChecked(true);
}

void C4ConsoleQtMainWindow::DrawSizeChanged(int newval)
{
	::Console.ToolsDlg.SetGrade(newval);
}

// File menu
void C4ConsoleQtMainWindow::FileNew() { ::Console.FileNew(); }
void C4ConsoleQtMainWindow::FileOpen() { ::Console.FileOpen(nullptr, false); }
void C4ConsoleQtMainWindow::FileOpenInNetwork() { ::Console.FileOpen(nullptr, true); }
void C4ConsoleQtMainWindow::FileOpenWithPlayers() { Console.FileOpenWPlrs(); }
void C4ConsoleQtMainWindow::FileRecord() { ::Console.FileRecord(); }
void C4ConsoleQtMainWindow::FileSave() { ::Console.FileSave(); }
void C4ConsoleQtMainWindow::FileSaveAs() { ::Console.FileSaveAs(false); }
void C4ConsoleQtMainWindow::FileSaveGameAs() { ::Console.FileSaveAs(true); }
void C4ConsoleQtMainWindow::FileExportPacked() { ::Console.FileSaveAs(false, true); }
void C4ConsoleQtMainWindow::FileClose() { ::Console.FileClose(); }
void C4ConsoleQtMainWindow::FileQuit() { ::Console.FileQuit(); }

void C4ConsoleQtMainWindow::FileReInitScenario()
{
	::Control.DoInput(CID_ReInitScenario, new C4ControlReInitScenario(), CDT_Decide);
}

// Player menu
void C4ConsoleQtMainWindow::PlayerJoin() { ::Console.PlayerJoin(); }
// Window menu
void C4ConsoleQtMainWindow::ViewportNew() { ::Console.ViewportNew(); }
// Help menu
void C4ConsoleQtMainWindow::HelpAbout() { ::Console.HelpAbout(); }

void C4ConsoleQtMainWindow::HelpToggle(bool enabled)
{
	::Config.Developer.ShowHelp = enabled;
	::Console.EditCursor.InvalidateSelection();
	repaint();
}

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

void C4ConsoleQtMainWindow::AscendPropertyPath()
{
	state->property_model->AscendPath();
	::Console.EditCursor.InvalidateSelection();
}

void C4ConsoleQtMainWindow::AddArrayElement()
{
	if (state->property_model) state->property_model->AddArrayElement();
}

void C4ConsoleQtMainWindow::RemoveArrayElement()
{
	if (state->property_model) state->property_model->RemoveArrayElement();
}


bool C4ConsoleQtMainWindow::HandleEditorKeyDown(QKeyEvent *event)
{
	switch (event->key())
	{
	case Qt::Key_Delete:
		::Console.EditCursor.Delete();
		return true;
	case Qt::Key_F2:
		::Console.EditCursor.Duplicate();
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

void C4ConsoleQtMainWindow::WelcomeLinkActivated(const QString &link)
{
	// Default links
	if (link == "new") FileNew();
	else if (link == "open") FileOpen();
	else if (link == "exploreuserpath")
	{
		bool success = false;
#ifdef USE_WIN32_WINDOWS
		StdStrBuf path(::Config.General.UserDataPath);
		intptr_t iError = (intptr_t) ::ShellExecute(nullptr, L"open", path.GetWideChar(), nullptr, path.GetWideChar(), SW_SHOW);
		if (iError > 32) success = true;
#else
		success = QDesktopServices::openUrl(QUrl::fromLocalFile(::Config.General.UserDataPath));
#endif
		if (!success)
			QMessageBox::critical(this, LoadResStr("IDS_MNU_EXPLOREUSERPATH"), LoadResStr("IDS_ERR_EXPLOREUSERPATH"));
	}
	// Open recent link
	else if (link.startsWith("open:"))
	{
		QString open_file = link.mid(5);
		::Console.FileOpen(open_file.toUtf8());
	}
}

void C4ConsoleQtMainWindow::SelectionDelete()
{
	::Console.EditCursor.Delete();
}

void C4ConsoleQtMainWindow::SelectionDuplicate()
{
	::Console.EditCursor.Duplicate();
}

void C4ConsoleQtMainWindow::SelectionEjectContents()
{
	::Console.EditCursor.GrabContents();
}

void C4ConsoleQtMainWindow::FocusGlobalScriptBox()
{
	state->ui.logDockWidget->show();
	state->ui.logDockWidget->raise();
	state->ui.consoleInputBox->setFocus();
}

void C4ConsoleQtMainWindow::FocusObjectScriptBox()
{
	state->ui.propertyDockWidget->show();
	state->ui.propertyDockWidget->raise();
	state->ui.propertyInputBox->setFocus();
}

void C4ConsoleQtMainWindow::OpenMaterialSelection()
{
	if (state->ui.foregroundMatTexComboBox->isEnabled())
	{
		state->ui.foregroundMatTexComboBox->setFocus();
		state->ui.foregroundMatTexComboBox->showPopup();
	}
}

void C4ConsoleQtMainWindow::FocusNextViewport()
{
	// Focus viewport after the one that has focus
	bool has_focus_vp = false;
	for (C4ConsoleQtViewportDockWidget *vp : state->viewports)
	{
		if (has_focus_vp)
		{
			vp->SetFocus();
			return;
		}
		else if (vp->HasFocus())
		{
			has_focus_vp = true;
		}
	}
	// No focus or last viewport was focused? Focus first.
	if (state->viewports.size())
	{
		state->viewports.front()->SetFocus();
	}
}

void C4ConsoleQtMainWindow::GradeUp()
{
	if (state->ui.drawSizeSlider->isEnabled())
	{
		state->ui.drawSizeSlider->setValue(state->ui.drawSizeSlider->value() + state->ui.drawSizeSlider->singleStep());
	}
}

void C4ConsoleQtMainWindow::GradeDown()
{
	if (state->ui.drawSizeSlider->isEnabled())
	{
		state->ui.drawSizeSlider->setValue(state->ui.drawSizeSlider->value() - state->ui.drawSizeSlider->singleStep());
	}
}


/* Common C4ConsoleGUI interface */

C4ConsoleGUIState::C4ConsoleGUIState(C4ConsoleGUI *console) : viewport_area(nullptr),
		enabled(false), recording(false), net_enabled(false), landscape_mode(LandscapeMode::Dynamic), flat_chunk_shapes(false),
	editcursor_mode(C4CNS_ModePlay), drawing_tool(C4TLS_Brush), is_object_selection_updating(0), disable_shortcut_filter(new C4DisableShortcutFilter(nullptr))
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
	format.setSwapInterval(0); // turn off vsync because otherwise each viewport causes an extra 1/(refesh rate) delay
	if (Config.Graphics.DebugOpenGL)
		format.setOption(QSurfaceFormat::DebugContext);
	QSurfaceFormat::setDefaultFormat(format);
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);


	// Basic Qt+Main window setup from .ui file
	// Note that QApplication needs at least one valid argument which must
	// stay valid over the lifetime of the application.
	static int fake_argc = 1;
	static const char *fake_argv[] = { "openclonk" };
	application.reset(new QApplication(fake_argc, const_cast<char **>(fake_argv)));
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
	ui.drawSizeSlider->setSingleStep(1);
	// Console input box signal
	QLineEdit *main_console_edit = ui.consoleInputBox->lineEdit();
	main_console_edit->completer()->setCaseSensitivity(Qt::CaseSensitivity::CaseSensitive);
	main_console_edit->connect(main_console_edit, SIGNAL(returnPressed()), window.get(), SLOT(MainConsoleEditEnter()));
	QLineEdit *property_console_edit = ui.propertyInputBox->lineEdit();
	property_console_edit->connect(property_console_edit, SIGNAL(returnPressed()), window.get(), SLOT(PropertyConsoleEditEnter()));
	property_console_edit->completer()->setCaseSensitivity(Qt::CaseSensitivity::CaseSensitive);
	// Add window menu actions
	window_menu_separator = ui.menuWindows->addSeparator();
	QAction *dock_action = ui.creatorDockWidget->toggleViewAction();
	dock_action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_1));
	ui.menuWindows->addAction(dock_action);
	dock_action = ui.objectListDockWidget->toggleViewAction();
	dock_action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_2));
	ui.menuWindows->addAction(dock_action);
	dock_action = ui.propertyDockWidget->toggleViewAction();
	dock_action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_3));
	ui.menuWindows->addAction(dock_action);
	dock_action = ui.logDockWidget->toggleViewAction();
	dock_action->setShortcut(QKeySequence(Qt::ALT | Qt::Key_4));
	ui.menuWindows->addAction(dock_action);
	// Viewport area setup
	viewport_area = new QMainWindow();
	viewport_area->setWindowFlags(Qt::Widget);
	window->setCentralWidget(viewport_area);
	window->setDockNestingEnabled(true);
	viewport_area->setDockNestingEnabled(true);
	QWidget *foo = new QWidget(viewport_area);
	viewport_area->setCentralWidget(foo);
	foo->hide();
	// Default action state
	ui.actionHelp->setChecked(::Config.Developer.ShowHelp);

	// Disable some shortcuts on actions that are handled internally
	// (none right now)

	// Property editor
	property_delegate_factory.reset(new C4PropertyDelegateFactory());
	ui.propertyTable->setItemDelegateForColumn(1, property_delegate_factory.get());
	ui.propertyEditAscendPathButton->setMaximumWidth(ui.propertyEditAscendPathButton->fontMetrics().boundingRect(ui.propertyEditAscendPathButton->text()).width() + 10);
	ui.propertyTable->setDropIndicatorShown(true);
	ui.propertyTable->setAcceptDrops(true);
	property_name_delegate.reset(new C4PropertyNameDelegate());
	ui.propertyTable->setItemDelegateForColumn(0, property_name_delegate.get());
	ui.propertyTable->setMouseTracking(true);

	// View models
	property_model.reset(new C4ConsoleQtPropListModel(property_delegate_factory.get()));
	property_delegate_factory->SetPropertyModel(property_model.get());
	property_name_delegate->SetPropertyModel(property_model.get());
	QItemSelectionModel *m = ui.propertyTable->selectionModel();
	ui.propertyTable->setModel(property_model.get());
	delete m;
	property_model->SetSelectionModel(ui.propertyTable->selectionModel());
	object_list_model.reset(new C4ConsoleQtObjectListModel());
	m = ui.objectListView->selectionModel();
	ui.objectListView->setModel(object_list_model.get());
	delete m;
	window->connect(ui.objectListView->selectionModel(), &QItemSelectionModel::selectionChanged, window.get(), &C4ConsoleQtMainWindow::OnObjectListSelectionChanged);
	definition_list_model.reset(new C4ConsoleQtDefinitionListModel());
	property_delegate_factory->SetDefinitionListModel(definition_list_model.get());
	m = ui.creatorTreeView->selectionModel();
	ui.creatorTreeView->setModel(definition_list_model.get());
	delete m;
	window->connect(ui.creatorTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, window.get(), &C4ConsoleQtMainWindow::OnCreatorSelectionChanged);
	window->connect(ui.creatorTreeView->selectionModel(), &QItemSelectionModel::currentChanged, window.get(), &C4ConsoleQtMainWindow::OnCreatorCurrentChanged);
	window->connect(ui.propertyTable->selectionModel(), &QItemSelectionModel::currentChanged, window.get(), [this]() {
		this->ui.arrayRemoveButton->setDisabled(this->property_model->IsTargetReadonly() || this->ui.propertyTable->selectionModel()->selectedRows().empty());
	});

	// Double-clicking an item in the object list focuses and raises the property window
	window->connect(ui.objectListView, &QTreeView::doubleClicked, window.get(), [this](const QModelIndex &index) {
		window->FocusObjectScriptBox();
	});

	// Initial layout is tabified (somehow I cannot do this in the designer)
	window->tabifyDockWidget(ui.objectListDockWidget, ui.propertyDockWidget);
	window->tabifyDockWidget(ui.objectListDockWidget, ui.creatorDockWidget);
	ui.propertyDockWidget->raise();

	// Welcome page
	InitWelcomeScreen();
	ShowWelcomeScreen();
	
	// Initial empty property page
	auto sel = C4EditCursorSelection();
	PropertyDlgUpdate(sel, true);

	// Restore layout & show!
	window->LoadGeometry();
	window->show();
	return true;
}

void C4ConsoleGUIState::DeleteConsoleWindow()
{
	// Reset to a state before CreateConsoleWindow was called
	action_object = C4VNull;
	is_object_selection_updating = false;

	editcursor_mode = C4CNS_ModePlay;
	drawing_tool = C4TLS_Brush;
	landscape_mode = LandscapeMode::Dynamic;
	net_enabled = false;
	recording = false;
	enabled = false;
	
	window_menu_separator = nullptr;
	status_cursor = status_framecounter = status_timefps = nullptr;

	while (!viewports.empty())
	{
		auto vp = viewports.front();
		viewports.erase(viewports.begin());

		viewport_area->removeDockWidget(vp);
		delete vp;
	}

	client_actions.clear();
	player_actions.clear();
	viewport_actions.clear();
	viewport_area = nullptr;

	disable_shortcut_filter.reset(nullptr);
	definition_list_model.reset(nullptr);
	object_list_model.reset(nullptr);
	property_name_delegate.reset(nullptr);
	property_delegate_factory.reset(nullptr);
	property_model.reset(nullptr);
	window.reset(nullptr);
	application.reset(nullptr);
}

void C4ConsoleGUIState::Execute(bool redraw_only)
{
	// Nothing to do - Qt's event loop is handling everything.
}

// Set action pressed/checked and enabled states
void C4ConsoleGUIState::UpdateActionStates()
{
	// Enabled states
	bool has_draw_tools = enabled && landscape_mode != LandscapeMode::Dynamic;
	bool has_exact_draw_tools = enabled && landscape_mode == LandscapeMode::Exact;
	bool is_drawing = has_draw_tools && editcursor_mode == C4CNS_ModeDraw;
	bool is_lobby = ::Network.isLobbyActive();
	ui.actionFileNew->setEnabled(!enabled);
	ui.actionFileReInitScenario->setEnabled(enabled);
	ui.actionPlay->setEnabled(enabled || is_lobby);
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
	ui.actionStaticFlatLandscape->setEnabled(enabled);
	ui.actionExactLandscape->setEnabled(enabled);
	ui.foregroundMatTexComboBox->setEnabled(is_drawing);
	ui.backgroundMatTexComboBox->setEnabled(is_drawing);
	ui.drawSizeSlider->setEnabled(is_drawing);
	ui.actionFileClose->setEnabled(enabled);
	ui.actionFileRecord->setEnabled(enabled && !recording);
	ui.actionFileSaveGameAs->setEnabled(enabled);
	ui.actionFileSaveScenario->setEnabled(enabled);
	ui.actionFileSaveScenarioAs->setEnabled(enabled);
	ui.actionFileExportScenarioPacked->setEnabled(enabled);
	ui.actionViewportNew->setEnabled(enabled);
	ui.actionPlayerJoin->setEnabled(enabled);
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
	ui.actionDynamicLandscape->setChecked(landscape_mode == LandscapeMode::Dynamic);
	ui.actionStaticLandscape->setChecked(landscape_mode == LandscapeMode::Static && !flat_chunk_shapes);
	ui.actionStaticFlatLandscape->setChecked(landscape_mode == LandscapeMode::Static && flat_chunk_shapes);
	ui.actionExactLandscape->setChecked(landscape_mode == LandscapeMode::Exact);
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

void C4ConsoleGUIState::AddNetMenuItem(int32_t index, const char *text, C4ConsoleGUI::ClientOperation op)
{
	auto *kick_action = new C4ConsoleClientAction(index, text, ui.menuNet, op);
	if (op == C4ConsoleGUI::CO_None) kick_action->setDisabled(true);
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

void C4ConsoleGUIState::RemoveViewport(C4ViewportWindow *cvp)
{
	if (!viewport_area) return;

	for (auto iter = viewports.begin(); iter != viewports.end(); )
	{
		auto vp = *iter;
		if (vp->GetViewportWindow() == cvp)
		{
			viewport_area->removeDockWidget(vp);
			iter = viewports.erase(iter);

			// cannot use deleteLater here because Qt will then
			// still select/deselect the viewport's GL context
			// behind the scenes, leaving us with an unselected
			// GL context.
			// Documented at http://doc.qt.io/qt-5/qopenglwidget.html
			// Instead, delete the viewport widget directly.
			delete vp;
		}
		else
		{
			++iter;
		}
	}
}

void C4ConsoleGUIState::SetInputFunctions(std::list<const char*> &functions)
{
	SetComboItems(ui.consoleInputBox, functions);
}

void C4ConsoleGUIState::PropertyDlgUpdate(C4EditCursorSelection &rSelection, bool force_function_update)
{
	int sel_count = rSelection.size();
	bool is_array = false;
	if (sel_count != 1)
	{
		// Multi object selection: Hide property view; show info label
		property_model->SetBasePropList(nullptr);
		ui.propertyTable->setEnabled(false);
		ui.selectionInfoLabel->setText(rSelection.GetDataString().getData());
		ui.propertyEditAscendPathButton->hide();
		UpdateActionObject(nullptr);
		ui.selectionHelpLabel->hide();
	}
	else
	{
		// Single object selection: Show property view + Object info in label
		C4PropList *prev_list = property_model->GetBasePropList(), *new_list = rSelection.front().getPropList();
		if (prev_list != new_list)
		{
			property_model->SetBasePropList(new_list);
			ui.propertyTable->setFirstColumnSpanned(0, QModelIndex(), true);
			ui.propertyTable->setFirstColumnSpanned(1, QModelIndex(), true);
			ui.propertyTable->expand(property_model->index(0, 0, QModelIndex()));
			UpdateActionObject(new_list->GetObject());
		}
		else if (::Console.EditCursor.IsSelectionInvalidated())
		{
			property_model->UpdateValue(false);
		}
		ui.selectionInfoLabel->setText(property_model->GetTargetPathText());
		QString help_text = property_model->GetTargetPathHelp();
		if (!help_text.isEmpty() && ::Config.Developer.ShowHelp)
		{
			const char *help_label = property_model->GetTargetPathName();
			if (!help_label) help_label = LoadResStr("IDS_CNS_DESCRIPTION");
			ui.selectionHelpLabel->setText(QString("%1: %2").arg(help_label).arg(help_text));
			ui.selectionHelpLabel->show();
		}
		else
		{
			ui.selectionHelpLabel->hide();
		}
		ui.propertyEditAscendPathButton->setVisible(property_model->GetTargetPathStackSize() >= 1);
		is_array = property_model->IsArray();
		if (is_array)
		{
			bool is_readonly = property_model->IsTargetReadonly();
			ui.arrayAddButton->setDisabled(is_readonly);
			ui.arrayRemoveButton->setDisabled(is_readonly || ui.propertyTable->selectionModel()->selectedRows().empty());
		}
		ui.propertyTable->setEnabled(true);
		::Console.EditCursor.ValidateSelection();
	}
	ui.arrayAddButton->setVisible(is_array);
	ui.arrayRemoveButton->setVisible(is_array);
	// Function update in script combo box
	if (force_function_update)
	{
		auto suggestions = ::Console.GetScriptSuggestions(rSelection.GetObject(), C4Console::MRU_Object);
		SetComboItems(ui.propertyInputBox, suggestions);
	}
}

void C4ConsoleGUIState::ReInitDefinitions()
{
	if (definition_list_model) definition_list_model->ReInit();
	// This also affects the object list
	if (object_list_model) object_list_model->Invalidate();
}

void C4ConsoleGUIState::OnCreatorSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	if (is_object_selection_updating || !definition_list_model) return; // only process callbacks from users interacting with widget
	// Forward to EditCursor
	C4Def *def;
	auto deselected_indexes = deselected.indexes();
	for (const QModelIndex &item : deselected_indexes)
		if ((def = definition_list_model->GetDefByModelIndex(item)))
			::Console.EditCursor.RemoveFromSelection(def);
	auto selected_indexes = selected.indexes();
	for (const QModelIndex &item : selected_indexes)
		if ((def = definition_list_model->GetDefByModelIndex(item)))
			::Console.EditCursor.AddToSelection(def);
	::Console.EditCursor.OnSelectionChanged(true);
	// Switching to def selection mode: Remove any non-defs from selection
	if (!selected.empty())
	{
		ui.objectListView->selectionModel()->clearSelection();
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
		if ((p = object_list_model->GetItemByModelIndex(item)))
			::Console.EditCursor.RemoveFromSelection(p);
	for (const QModelIndex &item : selected.indexes())
		if ((p = object_list_model->GetItemByModelIndex(item)))
			::Console.EditCursor.AddToSelection(p);
	::Console.EditCursor.OnSelectionChanged(true);
	// Switching to object/effect selection mode: Remove any non-objects/effects from selection
	if (!selected.empty()) 
	{
		ui.creatorTreeView->selectionModel()->clearSelection();
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

bool C4ConsoleGUIState::CreateNewScenario(StdStrBuf *out_filename, bool *out_host_as_network)
{
	// Show dialogue
	std::unique_ptr<C4ConsoleQtNewScenarioDlg> dlg(new C4ConsoleQtNewScenarioDlg(window.get()));
	if (!dlg->exec()) return false;
	// Dlg said OK! Scenario created
	out_filename->Copy(dlg->GetFilename());
	*out_host_as_network = dlg->IsHostAsNetwork();
	return true;
}

void C4ConsoleGUIState::InitWelcomeScreen()
{
	// Init links
	ui.welcomeNewLabel->setText(QString("<a href=\"new\">%1</a>").arg(ui.welcomeNewLabel->text()));
	ui.welcomeOpenLabel->setText(QString("<a href=\"open\">%1</a>").arg(ui.welcomeOpenLabel->text()));
	ui.welcomeExploreUserPathLabel->setText(QString("<a href=\"exploreuserpath\">%1</a>").arg(ui.welcomeExploreUserPathLabel->text()));
	// Recently opened scenarios
	bool any_file = false;
	int recent_idx = ui.welcomeScrollLayout->indexOf(ui.welcomeRecentLabel);
	for (int32_t i = 0; i < CFG_MaxEditorMRU; ++i)
	{
		const char *filename = ::Config.Developer.RecentlyEditedSzenarios[i];
		if (*filename && ::ItemExists(filename))
		{
			StdStrBuf basename(GetFilename(filename), true);
			if (basename == C4CFN_ScenarioCore)
			{
				// If a Scenario.txt was opened, use the enclosing .ocs name
				basename.Copy(filename, strlen(filename) - basename.getLength());
				int32_t len = basename.getLength();
				while (len && (basename.getData()[len - 1] == DirectorySeparator || basename.getData()[len - 1] == AltDirectorySeparator))
					basename.SetLength(--len);
				StdStrBuf base_folder_name(GetFilename(basename.getData()), true);
				basename.Take(base_folder_name);
			}
			RemoveExtension(&basename);
			QLabel *link = new QLabel(ui.welcomeScrollAreaWidgetContents);
			ui.welcomeScrollLayout->insertWidget(++recent_idx, link);
			link->setIndent(ui.welcomeOpenLabel->indent());
			link->setTextInteractionFlags(ui.welcomeOpenLabel->textInteractionFlags());
			link->setText(QString("<a href=\"open:%1\">%2</a>").arg(filename).arg(basename.getData())); // let's hope file names never contain "
			any_file = true;
			window->connect(link, SIGNAL(linkActivated(QString)), window.get(), SLOT(WelcomeLinkActivated(QString)));
		}
	}
	if (!any_file) ui.welcomeRecentLabel->hide();
}

void C4ConsoleGUIState::ShowWelcomeScreen()
{
	viewport_area->addDockWidget(Qt::BottomDockWidgetArea, ui.welcomeDockWidget);
}

void C4ConsoleGUIState::HideWelcomeScreen()
{
	ui.welcomeDockWidget->close();
}

void C4ConsoleGUIState::ClearGamePointers()
{
	if (property_delegate_factory) property_delegate_factory->ClearDelegates();
}

void C4ConsoleGUIState::UpdateActionObject(C4Object *new_action_object)
{
	// No change? Do not recreate buttons then because it may interfere with their usage
	C4Object *prev_object = action_object.getObj();
	if (new_action_object && prev_object == new_action_object) return;
	action_object = C4VObj(new_action_object);
	// Clear old action buttons
	int32_t i = ui.objectActionPanel->count();
	while (i--)
	{
		ui.objectActionPanel->itemAt(i)->widget()->deleteLater();
	}
	// Create new buttons
	// Actions are defined as properties in a local proplist called EditorActions
	if (!new_action_object) return;
	C4PropList *editor_actions_list = new_action_object->GetPropertyPropList(P_EditorActions);
	if (!editor_actions_list) return;
	auto new_properties = editor_actions_list->GetSortedProperties(nullptr);
	int row = 0, column = 0;
	for (C4String *action_def_id : new_properties)
	{
		// Get action definition proplist
		C4Value action_def_val;
		if (!editor_actions_list->GetPropertyByS(action_def_id, &action_def_val))
		{
			// property disappeared (cannot happen)
			continue;
		}
		C4PropList *action_def = action_def_val.getPropList();
		if (!action_def)
		{
			// property is of wrong type (can happen; scripter error)
			continue;
		}
		// Get action name
		C4String *action_name = action_def->GetPropertyStr(P_Name);
		if (!action_name)
		{
			// Fallback to identifier for unnamed actions
			action_name = action_def_id;
		}
		// Get action help
		QString action_help;
		C4String *action_help_s = action_def->GetPropertyStr(P_EditorHelp);
		if (action_help_s)
		{
			action_help = QString(action_help_s->GetCStr()).replace('|', '\n');
		}
		// Script command to execute
		C4RefCntPointer<C4String> script_command = action_def->GetPropertyStr(P_Command);
		int32_t object_number = new_action_object->Number;
		// Create action button
		QPushButton *btn = new QPushButton(action_name->GetCStr(), window.get());
		if (!action_help.isEmpty()) btn->setToolTip(action_help);
		if (script_command)
		{
			bool select_returned_object = action_def->GetPropertyBool(P_Select);
			btn->connect(btn, &QPushButton::pressed, btn, [script_command, object_number, select_returned_object]()
			{
				// Action execution. Replace %player% by first local player.
				StdStrBuf script_command_cpy(script_command->GetData(), true);
				C4Player *local_player = ::Players.GetLocalByIndex(0);
				int32_t local_player_number = local_player ? local_player->Number : NO_OWNER;
				script_command_cpy.Replace("%player%", FormatString("%d", (int)local_player_number).getData());
				::Console.EditCursor.EMControl(CID_Script, new C4ControlScript(script_command_cpy.getData(), object_number, false, select_returned_object));
			});
		}
		ui.objectActionPanel->addWidget(btn, row, column);
		if (++column >= 3)
		{
			column = 0;
			++row;
		}
	}
}
