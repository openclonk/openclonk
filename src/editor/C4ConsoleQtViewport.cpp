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

/* Player and editor viewports in console */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "script/C4Value.h"
#include "game/C4Viewport.h"
#include "object/C4Object.h"
#include "gui/C4MouseControl.h"
#include "landscape/C4Landscape.h"
#include "object/C4GameObjects.h"
#include "player/C4PlayerList.h"
#ifndef USE_CONSOLE
#include <epoxy/gl.h>
#endif
// See C4ConsoleQt.cpp on the include order
#include "editor/C4ConsoleQtViewport.h"
#include "editor/C4ConsoleQtState.h"
#include "editor/C4Console.h"
#include "editor/C4ConsoleQtShapes.h"
#include "editor/C4ViewportWindow.h"
#include "editor/C4Console.h"

/* Console viewports */

C4ConsoleQtViewportView::C4ConsoleQtViewportView(class C4ConsoleQtViewportScrollArea *scrollarea)
	: QOpenGLWidget(scrollarea->dock), dock(scrollarea->dock), cvp(scrollarea->cvp)
{
	setAttribute(Qt::WA_ShowWithoutActivating, true);
	setFocusPolicy(Qt::WheelFocus);
	setMouseTracking(true);
	setContextMenuPolicy(Qt::CustomContextMenu);
	// Register for viewport
	C4ViewportWindow *window = dock->cvp;
	window->glwidget = this;
	// Enable context menu
	connect(this, &QWidget::customContextMenuRequested, this, &C4ConsoleQtViewportView::ShowContextMenu);
}

bool C4ConsoleQtViewportView::IsPlayViewport() const
{
	return (cvp && ::MouseControl.IsViewport(cvp)
		&& (::Console.EditCursor.GetMode() == C4CNS_ModePlay));
}

// On high-DPI screens, Qt's pixels are not equal to device pixels anymore. However, viewports
// still work with device pixels, so we have to adjust coordinates from Qt events.
qreal C4ConsoleQtViewportView::GetDevicePixelRatio()
{
	// Find the screen the viewport is on to get its pixel ratio.
	auto desktop = QApplication::desktop();
	auto screenNumber = desktop->screenNumber(this);
	// This can happen while moving to a different screen.
	if (screenNumber == -1)
		return 1;
	auto screen = QApplication::screens()[screenNumber];
	return screen->devicePixelRatio();
}

void C4ConsoleQtViewportView::AddSelectObjectContextEntry(C4Object *obj, QMenu *menu)
{
	// Add select object item for object and for all contents
	if (!obj || !obj->Status) return;
	int32_t object_number = obj->Number;
	QAction *select_object_action = new QAction(QString("%1 #%2 (%3/%4)").arg(obj->GetName()).arg(object_number).arg(obj->GetX()).arg(obj->GetY()), menu);
	connect(select_object_action, &QAction::triggered, menu, [object_number]() {
		bool add = !!(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier);
		C4Object *obj = ::Objects.SafeObjectPointer(object_number);
		if (obj) ::Console.EditCursor.DoContextObjsel(obj, !add);
	});
	menu->addAction(select_object_action);
	if (obj->Contents.ObjectCount())
	{
		QMenu *submenu = menu->addMenu(FormatString(LoadResStr("IDS_CNS_SELECTCONTENTS"), obj->GetName()).getData());
		for (C4Object *cobj : obj->Contents)
		{
			AddSelectObjectContextEntry(cobj, submenu);
		}
	}
}

void C4ConsoleQtViewportView::ShowContextMenu(const QPoint &pos)
{
	// Coordinates are in viewport (not adjusted by parent scroll window)
	if (!IsPlayViewport())
	{
		// Show context menu in editor viewport
		QMenu *menu = new QMenu(this);
		// Show current object(s) as unselectable item
		auto &ui = dock->main_window->GetConsoleState()->ui;
		menu->addSection(ui.selectionInfoLabel->text());
		// Object actions. Always shown but grayed out if no object is selected.
		bool has_object = false;
		int32_t contents_count = 0;
		for (const C4Value &sel : ::Console.EditCursor.GetSelection())
		{
			C4Object *obj = sel.getObj();
			if (obj)
			{
				has_object = true;
				contents_count += obj->Contents.ObjectCount();
			}
		}
		for (QAction *act : { ui.actionDeleteObject, ui.actionDuplicateObject, ui.actionEjectContents })
		{
			act->setEnabled(has_object);
			menu->addAction(act);
		}
		if (!contents_count)
		{
			ui.actionEjectContents->setEnabled(false);
		}
		ui.actionEjectContents->setText(QString("%1 (%2)").arg(LoadResStr("IDS_MNU_CONTENTS")).arg((int)contents_count));
		// Object selection section for overlapping objects
		auto pr = GetDevicePixelRatio();
		int32_t x = cvp->WindowToGameX(pr * pos.x()),
		        y = cvp->WindowToGameY(pr * pos.y());
		auto pFOl = new C4FindObjectAtPoint(x, y); // freed by ~C4FindObjectAnd
		auto pFOc = new C4FindObjectContainer(nullptr);  // freed by ~C4FindObjectAnd
		C4FindObject *pFO_conds[] = { pFOl , pFOc };
		C4FindObjectAnd pFO(2, pFO_conds, false);
		std::unique_ptr<C4ValueArray> atcursor(pFO.FindMany(::Objects, ::Objects.Sectors)); // needs freeing (single object ptr)
		int itemcount = atcursor->GetSize();
		if (itemcount)
		{
			menu->addSection(LoadResStr("IDS_CNS_SELECTNEARBYOBJECTS"));
			for (int32_t i = 0; i < itemcount; ++i)
			{
				AddSelectObjectContextEntry(atcursor->GetItem(i).getObj(), menu);
			}
		}
		menu->popup(mapToGlobal(pos));
	}
}

// Get Shift state as Win32 wParam
uint32_t GetShiftWParam(QKeyEvent * event = nullptr)
{
	auto modifiers = event ? event->modifiers() : QGuiApplication::keyboardModifiers();
	uint32_t result = 0;
	if (modifiers & Qt::ShiftModifier) result |= MK_SHIFT;
	if (modifiers & Qt::ControlModifier) result |= MK_CONTROL;
	if (modifiers & Qt::AltModifier) result |= MK_ALT;
	return result;
}

void C4ConsoleQtViewportView::mouseMoveEvent(QMouseEvent *eventMove)
{
	auto pr = GetDevicePixelRatio();
	if (IsPlayViewport())
	{
		bool is_in_drawrange = (Inside<int32_t>(eventMove->x() - cvp->DrawX, 0, cvp->ViewWdt - 1)
		                     && Inside<int32_t>(eventMove->y() - cvp->DrawY, 0, cvp->ViewHgt - 1));
		this->setCursor(is_in_drawrange ? Qt::BlankCursor : Qt::CrossCursor);
		C4GUI::MouseMove(C4MC_Button_None, eventMove->x() * pr, eventMove->y() * pr, GetShiftWParam(), cvp);
	}
	else
	{
		cvp->pWindow->EditCursorMove(eventMove->x() * pr, eventMove->y() * pr, GetShiftWParam());
		UpdateCursor();
	}
}

void C4ConsoleQtViewportView::UpdateCursor()
{
	Qt::CursorShape cursor;
	if (::Console.EditCursor.HasTransformCursor())
		cursor = Qt::SizeAllCursor;
	else if (::Console.EditCursor.GetShapes()->HasDragCursor())
		cursor = ::Console.EditCursor.GetShapes()->GetDragCursor();
	else
		cursor = Qt::CrossCursor;
	this->setCursor(cursor);
}

void C4ConsoleQtViewportView::mousePressEvent(QMouseEvent *eventPress)
{
	auto pr = GetDevicePixelRatio();
	if (IsPlayViewport())
	{
		int32_t btn = C4MC_Button_None;
		switch (eventPress->button())
		{
		case Qt::LeftButton: btn = C4MC_Button_LeftDown; break;
		case Qt::RightButton: btn = C4MC_Button_RightDown; break;
		case Qt::MiddleButton: btn = C4MC_Button_MiddleDown; break;
		case Qt::XButton1: btn = C4MC_Button_X1Down; break;
		case Qt::XButton2: btn = C4MC_Button_X2Down; break;
		}
		C4GUI::MouseMove(btn, eventPress->x() * pr, eventPress->y() * pr, GetShiftWParam(), cvp);
	}
	else
	{
		// movement update needed before, so target is always up-to-date
		cvp->pWindow->EditCursorMove(eventPress->x() * pr, eventPress->y() * pr, GetShiftWParam());
		switch (eventPress->button())
		{
		case Qt::LeftButton: ::Console.EditCursor.LeftButtonDown(GetShiftWParam()); break;
		case Qt::RightButton: ::Console.EditCursor.RightButtonDown(GetShiftWParam()); break;
		}
	}
}

void C4ConsoleQtViewportView::mouseDoubleClickEvent(QMouseEvent *eventPress)
{
	if (IsPlayViewport())
	{
		int32_t btn = C4MC_Button_None;
		switch (eventPress->button())
		{
		case Qt::LeftButton: btn = C4MC_Button_LeftDouble; break;
		case Qt::RightButton: btn = C4MC_Button_RightDouble; break;
		case Qt::MiddleButton: btn = C4MC_Button_MiddleDouble; break;
		case Qt::XButton1: btn = C4MC_Button_X1Double; break;
		case Qt::XButton2: btn = C4MC_Button_X2Double; break;
		}
		auto pr = GetDevicePixelRatio();
		C4GUI::MouseMove(btn, eventPress->x() * pr, eventPress->y() * pr, GetShiftWParam(), cvp);
	}
}

void C4ConsoleQtViewportView::mouseReleaseEvent(QMouseEvent *releaseEvent)
{
	if (IsPlayViewport())
	{
		int32_t btn = C4MC_Button_None;
		switch (releaseEvent->button())
		{
		case Qt::LeftButton: btn = C4MC_Button_LeftUp; break;
		case Qt::RightButton: btn = C4MC_Button_RightUp; break;
		case Qt::MiddleButton: btn = C4MC_Button_MiddleUp; break;
		case Qt::XButton1: btn = C4MC_Button_X1Up; break;
		case Qt::XButton2: btn = C4MC_Button_X2Up; break;
		}
		auto pr = GetDevicePixelRatio();
		C4GUI::MouseMove(btn, releaseEvent->x() * pr, releaseEvent->y() * pr, GetShiftWParam(), cvp);
	}
	else
	{
		switch (releaseEvent->button())
		{
		case Qt::LeftButton: ::Console.EditCursor.LeftButtonUp(GetShiftWParam()); break;
		case Qt::RightButton: ::Console.EditCursor.RightButtonUp(GetShiftWParam()); break;
		}
	}
}

void C4ConsoleQtViewportView::wheelEvent(QWheelEvent *event)
{
	if (IsPlayViewport())
	{
		int delta = event->delta() / 8;
		if (!delta) delta = event->delta(); // abs(delta)<8?
		uint32_t shift = (delta>0) ? (delta<<16) : uint32_t(delta<<16);
		shift += GetShiftWParam();
		auto pr = GetDevicePixelRatio();
		C4GUI::MouseMove(C4MC_Button_Wheel, event->x() * pr, event->y() * pr, shift, cvp);
	}
	else
	{
		auto delta = event->angleDelta();
		auto modifiers = QGuiApplication::keyboardModifiers();
		// Zoom with Ctrl + mouse wheel, just like for player viewports.
		if (modifiers & Qt::ControlModifier)
			cvp->ChangeZoom(pow(C4GFX_ZoomStep, (float) delta.y() / 120));
		else
		{
			// Viewport movement.
			float x = -ViewportScrollSpeed * delta.x() / 120, y = -ViewportScrollSpeed * delta.y() / 120;
			// Not everyone has a vertical scroll wheel...
			if (modifiers & Qt::ShiftModifier)
				std::swap(x, y);
			cvp->ScrollView(x, y);
		}
	}
	// Event has been handled - do not forward to scroll bars
	event->accept();
}

void C4ConsoleQtViewportView::focusInEvent(QFocusEvent * event)
{
	dock->OnActiveChanged(true);
	QWidget::focusInEvent(event);
}

void C4ConsoleQtViewportView::focusOutEvent(QFocusEvent * event)
{
	dock->OnActiveChanged(false);
	QWidget::focusOutEvent(event);
}



/* Keyboard scan code mapping from Qt to our keys */

/** Convert certain keys to (unix(?)) scancodes (those that differ from scancodes on Windows. Sometimes. Maybe.) */

static C4KeyCode QtKeyToUnixScancode(const QKeyEvent &event)
{
	//LogF("VK: %x   SC: %x    key: %x", event.nativeVirtualKey(), event.nativeScanCode(), event.key());
	// Map some special keys
	switch (event.key())
	{
	case Qt::Key_Home:		return K_HOME;
	case Qt::Key_End:		return K_END;
	case Qt::Key_PageUp:	return K_PAGEUP;
	case Qt::Key_PageDown:	return K_PAGEDOWN;
	case Qt::Key_Up:		return K_UP;
	case Qt::Key_Down:		return K_DOWN;
	case Qt::Key_Left:		return K_LEFT;
	case Qt::Key_Right:		return K_RIGHT;
	case Qt::Key_Clear:		return K_CENTER;
	case Qt::Key_Insert:	return K_INSERT;
	case Qt::Key_Delete:	return K_DELETE;
	case Qt::Key_Menu:		return K_MENU;
	case Qt::Key_Pause:		return K_PAUSE;
	case Qt::Key_Print:		return K_PRINT;
	case Qt::Key_NumLock:	return K_NUM;
	case Qt::Key_ScrollLock:return K_SCROLL;
	default:
		// Some native Win32 key mappings...
#ifdef USE_WIN32_WINDOWS
		switch (event.nativeVirtualKey())
		{
		case VK_LWIN:		return K_WIN_L;
		case VK_RWIN:		return K_WIN_R;
		case VK_NUMPAD1:	return K_NUM1;
		case VK_NUMPAD2:	return K_NUM2;
		case VK_NUMPAD3:	return K_NUM3;
		case VK_NUMPAD4:	return K_NUM4;
		case VK_NUMPAD5:	return K_NUM5;
		case VK_NUMPAD6:	return K_NUM6;
		case VK_NUMPAD7:	return K_NUM7;
		case VK_NUMPAD8:	return K_NUM8;
		case VK_NUMPAD9:	return K_NUM9;
		case VK_NUMPAD0:	return K_NUM0;
		}
		switch (event.nativeScanCode())
		{
		case 285: return K_CONTROL_R;
		}
#endif
		// Otherwise rely on native scan code to be the same on all platforms
#if defined(USE_WIN32_WINDOWS) || defined(Q_OS_DARWIN)
		return event.nativeScanCode();
#else
		// Linux, FreeBSD, maybe others?
		return event.nativeScanCode() - 8;
#endif
	}
}

void C4ConsoleQtViewportView::keyPressEvent(QKeyEvent * event)
{
	// Convert key to our internal mapping
	C4KeyCode code = QtKeyToUnixScancode(*event);
	// Viewport-only handling
	bool handled = false;
	if (code == K_SCROLL)
	{
		cvp->TogglePlayerLock();
		handled = true;
	}
	// Handled if handled as player control or main editor
	if (!handled) handled = Game.DoKeyboardInput(code, KEYEV_Down, !!(event->modifiers() & Qt::AltModifier), !!(event->modifiers() & Qt::ControlModifier), !!(event->modifiers() & Qt::ShiftModifier), event->isAutoRepeat(), nullptr);
	if (!handled) handled = dock->main_window->HandleEditorKeyDown(event);
	// Modifiers may update the cursor state; refresh
	if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control || event->key() == Qt::Key_Alt || event->key() == Qt::Key_AltGr)
	{
		::Console.EditCursor.Move(GetShiftWParam(event));
		UpdateCursor();
	}
	event->setAccepted(handled);
}

void C4ConsoleQtViewportView::keyReleaseEvent(QKeyEvent * event)
{
	// Convert key to our internal mapping
	C4KeyCode code = QtKeyToUnixScancode(*event);
	// Handled if handled as player control
	bool handled = Game.DoKeyboardInput(code, KEYEV_Up, !!(event->modifiers() & Qt::AltModifier), !!(event->modifiers() & Qt::ControlModifier), !!(event->modifiers() & Qt::ShiftModifier), event->isAutoRepeat(), nullptr);
	if (!handled) handled = dock->main_window->HandleEditorKeyUp(event);
	// Modifiers may update the cursor state; refresh
	if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control || event->key() == Qt::Key_Alt || event->key() == Qt::Key_AltGr)
	{
		::Console.EditCursor.Move(GetShiftWParam(event));
		UpdateCursor();
	}
	event->setAccepted(handled);
}

void C4ConsoleQtViewportView::enterEvent(QEvent *)
{
	// TODO: This should better be managed by the viewport
	// looks weird when there's multiple viewports open
	// but for some reason, the EditCursor drawing stuff is not associated with the viewport (yet)
	::Console.EditCursor.SetMouseHover(true);
}

void C4ConsoleQtViewportView::leaveEvent(QEvent *)
{
	// TODO: This should better be managed by the viewport
	::Console.EditCursor.SetMouseHover(false);
}

void C4ConsoleQtViewportView::initializeGL()
{
	// nothing to do with epoxy
}

void C4ConsoleQtViewportView::resizeGL(int w, int h)
{
	auto pr = GetDevicePixelRatio();
	cvp->UpdateOutputSize(w * pr, h * pr);
}

void C4ConsoleQtViewportView::paintGL()
{
	cvp->ScrollBarsByViewPosition();
	cvp->Execute();
}


C4ConsoleQtViewportScrollArea::C4ConsoleQtViewportScrollArea(class C4ConsoleQtViewportDockWidget *dock)
	: QAbstractScrollArea(dock), dock(dock), cvp(dock->cvp->cvp), is_updating_scrollbars(0)
{
	cvp->scrollarea = this;
	// No scroll bars by default. Neutral viewports will toggle this.
	setScrollBarVisibility(false);
}

void C4ConsoleQtViewportScrollArea::scrollContentsBy(int dx, int dy)
{
	// Just use the absolute position in any case.
	if (!is_updating_scrollbars)
	{
		cvp->SetViewX(horizontalScrollBar()->value());
		cvp->SetViewY(verticalScrollBar()->value());
	}
}

bool C4ConsoleQtViewportScrollArea::viewportEvent(QEvent *e)
{
	// Pass everything to the viewport.
	return false;
}

void C4ConsoleQtViewportScrollArea::setupViewport(QWidget *viewport)
{
	// Don't steal focus from the viewport. This is necessary to make keyboard input work.
	viewport->setFocusProxy(nullptr);
	ScrollBarsByViewPosition();
}

void C4ConsoleQtViewportScrollArea::ScrollBarsByViewPosition()
{
	++is_updating_scrollbars; // Do not shift view just from updating scroll bars
	int x = viewport()->width() / cvp->GetZoom();
	horizontalScrollBar()->setRange(0, ::Landscape.GetWidth() - x);
	horizontalScrollBar()->setPageStep(x);
	horizontalScrollBar()->setValue(cvp->GetViewX());

	int y = viewport()->height() / cvp->GetZoom();
	verticalScrollBar()->setRange(0, ::Landscape.GetHeight() - y);
	verticalScrollBar()->setPageStep(y);
	verticalScrollBar()->setValue(cvp->GetViewY());
	--is_updating_scrollbars;
}

void C4ConsoleQtViewportScrollArea::setScrollBarVisibility(bool visible)
{
	if (visible)
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	}
	else
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}
}

C4ConsoleQtViewportDockWidget::C4ConsoleQtViewportDockWidget(C4ConsoleQtMainWindow *main_window, QMainWindow *parent, C4ViewportWindow *cvp)
	: QDockWidget("", parent), main_window(main_window), cvp(cvp)
{
	// Translated title or player name
	C4Player *plr = ::Players.Get(cvp->cvp->GetPlayer());
	setWindowTitle(plr ? plr->GetName() : LoadResStr("IDS_CNS_VIEWPORT"));
	// Actual view container, wrapped in scrolling area
	auto scrollarea = new C4ConsoleQtViewportScrollArea(this);
	view = new C4ConsoleQtViewportView(scrollarea);
	scrollarea->setViewport(view);
	setWidget(scrollarea);
	connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(TopLevelChanged(bool)));
	// Register viewport widget for periodic rendering updates.
	cvp->viewport_widget = view;
}

void C4ConsoleQtViewportDockWidget::mousePressEvent(QMouseEvent *eventPress)
{
	// Clicking the dock focuses the viewport
	view->setFocus();
	QDockWidget::mousePressEvent(eventPress);
}

void C4ConsoleQtViewportDockWidget::OnActiveChanged(bool active)
{
	// Title bar of the selected viewport should be drawn in highlight colors to show that keyboard input will now go to the viewport.
	// Unfortunately, color and font of the title is not taken from QDockWidget::title but directly from QDockWidget.
	// Provide them in both just in case Qt ever changes its definition.
	QColor bgclr = QApplication::palette(this).color(QPalette::Highlight);
	QColor fontclr = QApplication::palette(this).color(QPalette::HighlightedText);
	if (active)
		setStyleSheet(QString(
			"QDockWidget::title { text-align: left; background: %1; padding-left: 3px; color: %2; font-weight: bold; } QDockWidget { color: %2; font-weight: bold; }").arg(bgclr.name(), fontclr.name()));
	else
		setStyleSheet(QString());
}

void C4ConsoleQtViewportDockWidget::TopLevelChanged(bool is_floating)
{
	// Ensure focus after undock and after re-docking floating viewport window
	view->setFocus();
}

void C4ConsoleQtViewportDockWidget::closeEvent(QCloseEvent * event)
{
	QDockWidget::closeEvent(event);
	if (event->isAccepted())
	{
		if (cvp)
		{
			// This causes "this" to be deleted:
			cvp->Close();
		}
		else
		{
			deleteLater();
		}
	}
}

bool C4ConsoleQtViewportDockWidget::event(QEvent *e)
{
	// Focus on title bar click 
	if (e->type() == QEvent::NonClientAreaMouseButtonPress || e->type() == QEvent::MouseButtonPress) view->setFocus();
	return QDockWidget::event(e);
}
