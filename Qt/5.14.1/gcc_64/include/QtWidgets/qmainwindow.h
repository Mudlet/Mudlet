/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDYNAMICMAINWINDOW_H
#define QDYNAMICMAINWINDOW_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>
#if QT_CONFIG(tabwidget)
#include <QtWidgets/qtabwidget.h>
#endif

QT_REQUIRE_CONFIG(mainwindow);

QT_BEGIN_NAMESPACE

class QDockWidget;
class QMainWindowPrivate;
class QMenuBar;
class QStatusBar;
class QToolBar;
class QMenu;

class Q_WIDGETS_EXPORT QMainWindow : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)
    Q_PROPERTY(Qt::ToolButtonStyle toolButtonStyle READ toolButtonStyle WRITE setToolButtonStyle)
#if QT_CONFIG(dockwidget)
    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated)
#if QT_CONFIG(tabbar)
    Q_PROPERTY(bool documentMode READ documentMode WRITE setDocumentMode)
#endif // QT_CONFIG(tabbar)
#if QT_CONFIG(tabwidget)
    Q_PROPERTY(QTabWidget::TabShape tabShape READ tabShape WRITE setTabShape)
#endif // QT_CONFIG(tabwidget)
    Q_PROPERTY(bool dockNestingEnabled READ isDockNestingEnabled WRITE setDockNestingEnabled)
#endif // QT_CONFIG(dockwidget)
    Q_PROPERTY(DockOptions dockOptions READ dockOptions WRITE setDockOptions)
#if QT_CONFIG(toolbar)
    Q_PROPERTY(bool unifiedTitleAndToolBarOnMac READ unifiedTitleAndToolBarOnMac WRITE setUnifiedTitleAndToolBarOnMac)
#endif

public:
    enum DockOption {
        AnimatedDocks = 0x01,
        AllowNestedDocks = 0x02,
        AllowTabbedDocks = 0x04,
        ForceTabbedDocks = 0x08,  // implies AllowTabbedDocks, !AllowNestedDocks
        VerticalTabs = 0x10,      // implies AllowTabbedDocks
        GroupedDragging = 0x20    // implies AllowTabbedDocks
    };
    Q_ENUM(DockOption)
    Q_DECLARE_FLAGS(DockOptions, DockOption)
    Q_FLAG(DockOptions)

    explicit QMainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~QMainWindow();

    QSize iconSize() const;
    void setIconSize(const QSize &iconSize);

    Qt::ToolButtonStyle toolButtonStyle() const;
    void setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle);

#if QT_CONFIG(dockwidget)
    bool isAnimated() const;
    bool isDockNestingEnabled() const;
#endif

#if QT_CONFIG(tabbar)
    bool documentMode() const;
    void setDocumentMode(bool enabled);
#endif

#if QT_CONFIG(tabwidget)
    QTabWidget::TabShape tabShape() const;
    void setTabShape(QTabWidget::TabShape tabShape);
    QTabWidget::TabPosition tabPosition(Qt::DockWidgetArea area) const;
    void setTabPosition(Qt::DockWidgetAreas areas, QTabWidget::TabPosition tabPosition);
#endif // QT_CONFIG(tabwidget)

    void setDockOptions(DockOptions options);
    DockOptions dockOptions() const;

    bool isSeparator(const QPoint &pos) const;

#if QT_CONFIG(menubar)
    QMenuBar *menuBar() const;
    void setMenuBar(QMenuBar *menubar);

    QWidget  *menuWidget() const;
    void setMenuWidget(QWidget *menubar);
#endif

#if QT_CONFIG(statusbar)
    QStatusBar *statusBar() const;
    void setStatusBar(QStatusBar *statusbar);
#endif

    QWidget *centralWidget() const;
    void setCentralWidget(QWidget *widget);

    QWidget *takeCentralWidget();

#if QT_CONFIG(dockwidget)
    void setCorner(Qt::Corner corner, Qt::DockWidgetArea area);
    Qt::DockWidgetArea corner(Qt::Corner corner) const;
#endif

#if QT_CONFIG(toolbar)
    void addToolBarBreak(Qt::ToolBarArea area = Qt::TopToolBarArea);
    void insertToolBarBreak(QToolBar *before);

    void addToolBar(Qt::ToolBarArea area, QToolBar *toolbar);
    void addToolBar(QToolBar *toolbar);
    QToolBar *addToolBar(const QString &title);
    void insertToolBar(QToolBar *before, QToolBar *toolbar);
    void removeToolBar(QToolBar *toolbar);
    void removeToolBarBreak(QToolBar *before);

    bool unifiedTitleAndToolBarOnMac() const;

    Qt::ToolBarArea toolBarArea(
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        const
#endif
        QToolBar *toolbar) const;
    bool toolBarBreak(QToolBar *toolbar) const;
#endif
#if QT_CONFIG(dockwidget)
    void addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget);
    void addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget,
                       Qt::Orientation orientation);
    void splitDockWidget(QDockWidget *after, QDockWidget *dockwidget,
                         Qt::Orientation orientation);
#if QT_CONFIG(tabbar)
    void tabifyDockWidget(QDockWidget *first, QDockWidget *second);
    QList<QDockWidget*> tabifiedDockWidgets(QDockWidget *dockwidget) const;
#endif
    void removeDockWidget(QDockWidget *dockwidget);
    bool restoreDockWidget(QDockWidget *dockwidget);

    Qt::DockWidgetArea dockWidgetArea(QDockWidget *dockwidget) const;

    void resizeDocks(const QList<QDockWidget *> &docks,
                     const QList<int> &sizes, Qt::Orientation orientation);
#endif // QT_CONFIG(dockwidget)

    QByteArray saveState(int version = 0) const;
    bool restoreState(const QByteArray &state, int version = 0);

#if QT_CONFIG(menu)
    virtual QMenu *createPopupMenu();
#endif

public Q_SLOTS:
#if QT_CONFIG(dockwidget)
    void setAnimated(bool enabled);
    void setDockNestingEnabled(bool enabled);
#endif
#if QT_CONFIG(toolbar)
    void setUnifiedTitleAndToolBarOnMac(bool set);
#endif

Q_SIGNALS:
    void iconSizeChanged(const QSize &iconSize);
    void toolButtonStyleChanged(Qt::ToolButtonStyle toolButtonStyle);
#if QT_CONFIG(dockwidget)
    void tabifiedDockWidgetActivated(QDockWidget *dockWidget);
#endif

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif
    bool event(QEvent *event) override;

private:
    Q_DECLARE_PRIVATE(QMainWindow)
    Q_DISABLE_COPY(QMainWindow)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMainWindow::DockOptions)

QT_END_NAMESPACE

#endif // QDYNAMICMAINWINDOW_H
