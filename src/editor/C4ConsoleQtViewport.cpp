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

#include <C4Include.h>
#include <C4Value.h>
#include <C4ConsoleQtViewport.h>
#include <C4Viewport.h>
#include <C4ViewportWindow.h>
#include <C4Console.h>
#include <C4MouseControl.h>

/* Console viewports */

C4ConsoleQtViewportView::C4ConsoleQtViewportView(class C4ConsoleQtViewportDockWidget *dock)
	: QWidget(dock), dock(dock), cvp(dock->cvp ? dock->cvp->cvp : NULL)
{
	setAutoFillBackground(false);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
	setAttribute(Qt::WA_ShowWithoutActivating, true);
	setWindowFlags(Qt::FramelessWindowHint);
	setFocusPolicy(Qt::TabFocus);
	setMouseTracking(true);
	// Register for viewport
#ifdef USE_WIN32_WINDOWS
	dock->cvp->hWindow = reinterpret_cast<HWND>(this->winId());
#else
	TODO
#endif
}

bool C4ConsoleQtViewportView::IsPlayViewport() const
{
	return (cvp && ::MouseControl.IsViewport(cvp)
		&& (::Console.EditCursor.GetMode() == C4CNS_ModePlay));
}

void C4ConsoleQtViewportView::resizeEvent(QResizeEvent *resize_event)
{
	QWidget::resizeEvent(resize_event);
	if (cvp) dock->cvp->cvp->UpdateOutputSize();
}

bool C4ConsoleQtViewportView::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	// Handle native Windows messages
#ifdef USE_WIN32_WINDOWS
	MSG *msg = static_cast<MSG*>(message);
	switch (msg->message)
	{
	//----------------------------------------------------------------------------------------------------------------------------------
	case WM_HSCROLL:
		switch (LOWORD(msg->wParam))
		{
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION: cvp->SetViewX(float(HIWORD(msg->wParam)) / cvp->GetZoom()); break;
		case SB_LINELEFT: cvp->ScrollView(-ViewportScrollSpeed, 0.0f); break;
		case SB_LINERIGHT: cvp->ScrollView(+ViewportScrollSpeed, 0.0f); break;
		case SB_PAGELEFT: cvp->ScrollView(-cvp->ViewWdt / cvp->GetZoom(), 0.0f); break;
		case SB_PAGERIGHT: cvp->ScrollView(+cvp->ViewWdt / cvp->GetZoom(), 0.0f); break;
		}
		cvp->Execute();
		cvp->ScrollBarsByViewPosition();
		return true;
	//----------------------------------------------------------------------------------------------------------------------------------
	case WM_VSCROLL:
		switch (LOWORD(msg->wParam))
		{
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION: cvp->SetViewY(float(HIWORD(msg->wParam)) / cvp->GetZoom()); break;
		case SB_LINEUP: cvp->ScrollView(0.0f, -ViewportScrollSpeed); break;
		case SB_LINEDOWN: cvp->ScrollView(0.0f, +ViewportScrollSpeed); break;
		case SB_PAGEUP: cvp->ScrollView(0.0f, -cvp->ViewWdt / cvp->GetZoom()); break;
		case SB_PAGEDOWN: cvp->ScrollView(0.0f, +cvp->ViewWdt / cvp->GetZoom()); break;
		}
		cvp->Execute();
		cvp->ScrollBarsByViewPosition();
		return true;
		//----------------------------------------------------------------------------------------------------------------------------------
	}
#endif
	return false;
}

// Get Shift state as Win32 wParam
uint32_t GetShiftWParam()
{
#ifdef USE_WIN32_WINDOWS
	uint32_t result = 0;
	if (GetKeyState(VK_CONTROL) < 0) result |= MK_CONTROL;
	if (GetKeyState(VK_SHIFT) < 0) result |= MK_SHIFT;
	if (GetKeyState(VK_MENU) < 0) result |= MK_ALT;
	return result;
#else
	TODO Get shift state
#endif
}

void C4ConsoleQtViewportView::mouseMoveEvent(QMouseEvent *eventMove)
{
	if (IsPlayViewport())
	{
		bool is_in_drawrange = (Inside<int32_t>(eventMove->x() - cvp->DrawX, 0, cvp->ViewWdt - 1)
		                     && Inside<int32_t>(eventMove->y() - cvp->DrawY, 0, cvp->ViewHgt - 1));
		this->setCursor(is_in_drawrange ? Qt::BlankCursor : Qt::CrossCursor);
		C4GUI::MouseMove(C4MC_Button_None, eventMove->x(), eventMove->y(), GetShiftWParam(), cvp);
	}
	else
	{
		cvp->pWindow->EditCursorMove(eventMove->x(), eventMove->y(), GetShiftWParam());
	}

}

void C4ConsoleQtViewportView::mousePressEvent(QMouseEvent *eventPress)
{
	if (IsPlayViewport())
	{
		int32_t btn = C4MC_Button_None;
		switch (eventPress->button())
		{
		case Qt::LeftButton: btn = C4MC_Button_LeftDown; break;
		case Qt::RightButton: btn = C4MC_Button_RightDown; break;
		}
		C4GUI::MouseMove(btn, eventPress->x(), eventPress->y(), GetShiftWParam(), cvp);
	}
	else
	{
		// movement update needed before, so target is always up-to-date
		cvp->pWindow->EditCursorMove(eventPress->x(), eventPress->y(), GetShiftWParam());
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
		}
		C4GUI::MouseMove(btn, eventPress->x(), eventPress->y(), GetShiftWParam(), cvp);
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
		}
		C4GUI::MouseMove(btn, releaseEvent->x(), releaseEvent->y(), GetShiftWParam(), cvp);
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
		C4GUI::MouseMove(C4MC_Button_Wheel, event->x(), event->y(), shift, cvp);
	}
	else
	{
		// TODO zoom?
	}
}

C4ConsoleQtViewportDockWidget::C4ConsoleQtViewportDockWidget(QMainWindow *parent, C4ViewportWindow *cvp)
	: QDockWidget("", parent), cvp(cvp)
{
	// Translated title
	setWindowTitle(LoadResStr("IDS_CNS_VIEWPORT"));
	// Actual view container
	view = new C4ConsoleQtViewportView(this);
	setWidget(view);
	connect(this, SIGNAL(dockLocationChanged(bool)), this, SLOT(DockLocationChanged(bool)));
	OnActiveChanged(true);
}

void C4ConsoleQtViewportDockWidget::OnActiveChanged(bool active)
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

void C4ConsoleQtViewportDockWidget::focusInEvent(QFocusEvent * event)
{
	OnActiveChanged(true);
	QDockWidget::focusInEvent(event);
}

void C4ConsoleQtViewportDockWidget::focusOutEvent(QFocusEvent * event)
{
	OnActiveChanged(false);
	QDockWidget::focusOutEvent(event);
}

void C4ConsoleQtViewportDockWidget::DockLocationChanged(Qt::DockWidgetArea new_area)
{
	// Re-docked: 
}

void C4ConsoleQtViewportDockWidget::closeEvent(QCloseEvent * event)
{
	QDockWidget::closeEvent(event);
	if (event->isAccepted())
	{
		if (cvp) cvp->Close();
		cvp = NULL;
		deleteLater();
	}
}

#ifdef USE_WIN32_WINDOWS
bool ConsoleHandleWin32KeyboardMessage(MSG *msg); // in C4WindowWin32.cpp
#endif


bool C4ConsoleQtViewportDockWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	// Handle keyboard messages on detached viewport windows
#ifdef USE_WIN32_WINDOWS
	MSG *msg = static_cast<MSG*>(message);
	if (ConsoleHandleWin32KeyboardMessage(msg)) return true;
#else
TODO: Should implement this through native Qt keyboard signals
#endif
	// Handle by Qt
	return false;
}
