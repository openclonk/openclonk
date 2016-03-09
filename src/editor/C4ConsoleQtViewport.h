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

#ifndef INC_C4ConsoleQtViewport
#define INC_C4ConsoleQtViewport
#ifdef WITH_QT_EDITOR

#include <C4Include.h> // needed for automoc
#include <C4ConsoleGUI.h> // for glew.h
#include <C4ConsoleQt.h>

class C4ConsoleQtViewportView : public QWidget
{
	Q_OBJECT

	class C4ConsoleQtViewportDockWidget *dock;
	class C4Viewport *cvp;

private:
	bool IsPlayViewport() const;

protected:
	void resizeEvent(QResizeEvent *resize_event) override;
	bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
	void mouseMoveEvent(QMouseEvent *eventMove) override;
	void mousePressEvent(QMouseEvent *eventPress) override;
	void mouseDoubleClickEvent(QMouseEvent *eventPress) override;
	void mouseReleaseEvent(QMouseEvent *releaseEvent) override;
	void wheelEvent(QWheelEvent *event) override;

public:
	C4ConsoleQtViewportView(class C4ConsoleQtViewportDockWidget *dock);
};

class C4ConsoleQtViewportDockWidget : public QDockWidget
{
	Q_OBJECT

	class C4ViewportWindow *cvp;
	C4ConsoleQtViewportView *view;
	QPalette pal_inactive, pal_active;

protected:
	void focusInEvent(QFocusEvent * event) override;
	void focusOutEvent(QFocusEvent * event) override;
	bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

public:
	C4ConsoleQtViewportDockWidget(class QMainWindow *parent, class C4ViewportWindow *window);
	virtual void closeEvent(QCloseEvent * event);
	void OnActiveChanged(bool active);
	class C4ViewportWindow *GetViewportWindow() const { return cvp; }

private slots :
	void DockLocationChanged(Qt::DockWidgetArea new_area);

	friend C4ConsoleQtViewportView;
};


#endif // WITH_QT_EDITOR
#endif // INC_C4ConsoleQtViewport
