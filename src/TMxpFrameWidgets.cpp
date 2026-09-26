/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "TMxpFrameWidgets.h"

#include "Host.h"
#include "TConsole.h"
#include "TMainConsole.h"

#include <QCoreApplication>
#include <QFrame>
#include <QSizePolicy>
#include <QTabWidget>
#include <QVBoxLayout>

TMxpFrameWidgets::TMxpFrameWidgets(TMainConsole* pMainConsole)
: mpMainConsole(pMainConsole)
{
}

void TMxpFrameWidgets::createInternalFrame(const QString& name, const QString& title, const QRect& geometry, bool showHeader, bool scrolling)
{
    const int frameWidth = geometry.width();
    const int frameHeight = geometry.height();
    const int tabBarHeight = showHeader ? 30 : 0; // Tab widget overhead including margins

    // Create the container widget for the frame - use WA_DontShowOnScreen to prevent any rendering
    auto* containerWidget = new QFrame(mpMainConsole->mpMainFrame);
    containerWidget->setAttribute(Qt::WA_DontShowOnScreen, true);
    containerWidget->setObjectName(name + qsl("_container"));
    containerWidget->setGeometry(geometry);

    if (showHeader) {
        containerWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
        containerWidget->setLineWidth(1);
        containerWidget->setStyleSheet(qsl("QFrame { background-color: #1a1a1a; border: 1px solid #444444; }"));
    } else {
        containerWidget->setFrameStyle(QFrame::NoFrame);
        containerWidget->setLineWidth(0);
        containerWidget->setStyleSheet(qsl("QFrame { background-color: transparent; border: none; }"));
    }

    // Create a layout for the container
    auto* containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    QTabWidget* tabWidget = nullptr;
    TConsole* console = nullptr;

    if (showHeader) {
        // Create TabWidget as the header - allows future tab additions
        tabWidget = new QTabWidget(containerWidget);
        tabWidget->setObjectName(name + qsl("_tabs"));
        tabWidget->setTabPosition(QTabWidget::North);
        tabWidget->setDocumentMode(true);
        tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tabWidget->setStyleSheet(qsl("QTabWidget::pane { border: none; background-color: transparent; }"
                                     "QTabBar::tab { background-color: #2a2a2a; color: #cccccc; padding: 4px 12px; "
                                     "              border: 1px solid #444444; border-bottom: none; margin-right: 2px; }"
                                     "QTabBar::tab:selected { background-color: #3a3a3a; color: #ffffff; }"
                                     "QTabBar::tab:hover { background-color: #333333; }"));

        // Create a page widget to hold the console
        auto* tabPage = new QWidget();
        tabPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        // Make tab page background transparent to avoid showing a box before console loads
        tabPage->setStyleSheet(qsl("background-color: transparent;"));
        auto* tabPageLayout = new QVBoxLayout(tabPage);
        tabPageLayout->setContentsMargins(0, 0, 0, 0);

        // Not createMiniConsole: it parents to mpMainFrame and calls show(), causing a flash
        console = mpMainConsole->createSubConsole(name, tabPage);
        console->resize(frameWidth, frameHeight - tabBarHeight);
        console->mOldX = 0;
        console->mOldY = 0;
        console->setContentsMargins(0, 0, 0, 0);
        int fontSize = mpMainConsole->mpHost->getDisplayFont().pointSize();
        console->setFontSize(fontSize > 0 ? fontSize : 12);
        console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        // Don't show yet - wait until frame is fully set up
        console->hide();

        tabPageLayout->addWidget(console);
        int tabIndex = tabWidget->addTab(tabPage, title);
        tabWidget->setCurrentIndex(tabIndex); // Make this tab active
        containerLayout->addWidget(tabWidget);
    } else {
        // Floating/borderless: console directly in container, no tab header
        console = mpMainConsole->createSubConsole(name, containerWidget);
        console->resize(frameWidth, frameHeight);
        console->mOldX = 0;
        console->mOldY = 0;
        console->setContentsMargins(0, 0, 0, 0);
        int fontSize = mpMainConsole->mpHost->getDisplayFont().pointSize();
        console->setFontSize(fontSize > 0 ? fontSize : 12);
        console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        console->hide();

        containerLayout->addWidget(console);
    }

    mFrames.insert(name, {containerWidget, console, tabWidget});

    // Configure scrolling
    if (!scrolling) {
        console->setScrolling(false);
    }

    // Set console colors - slightly different background for visual distinction
    console->setFgColor(mpMainConsole->mFgColor);
    // Use a slightly lighter background for frames to distinguish from main console
    QColor frameBgColor = mpMainConsole->mBgColor;
    frameBgColor = frameBgColor.lighter(115); // 15% lighter than main console
    console->setBgColor(frameBgColor);

    // Only add border for borderless/floating frames (no tab header)
    // Tabbed frames already have visual separation from the tab widget
    if (!showHeader) {
        console->setStyleSheet(qsl("QWidget { border: 1px solid #444444; }"));
    }

    mpMainConsole->registerSubConsole(name, console);

    // Force layout to calculate sizes
    containerWidget->layout()->activate();
    if (tabWidget) {
        tabWidget->adjustSize();
        // Make sure the current tab page fills the available space
        if (tabWidget->currentWidget()) {
            tabWidget->currentWidget()->resize(tabWidget->size());
        }
    }

    // Clear the WA_DontShowOnScreen attribute and show immediately
    containerWidget->setAttribute(Qt::WA_DontShowOnScreen, false);
    console->show();
    containerWidget->show();
    containerWidget->raise();

    // Force immediate repaint to prevent visual artifacts
    containerWidget->update();
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool TMxpFrameWidgets::createExternalFrame(const QString& name, const QString& title, const QSize& size, bool scrolling)
{
    // Create standalone window with mini console
    auto* console = mpMainConsole->createMiniConsole(qsl("main"), name, 0, 0, size.width(), size.height());

    if (!console) {
        return false;
    }

    mFrames.insert(name, {console, console, nullptr});

    // Configure scrolling
    if (!scrolling) {
        console->setScrolling(false);
    }

    // Set window title and show as floating
    console->setWindowTitle(title);
    console->setWindowFlags(Qt::Window);
    console->show();
    return true;
}

std::optional<QSize> TMxpFrameWidgets::tabAreaSize(const QString& parentName) const
{
    const QTabWidget* tabWidget = frameTabs(parentName);
    if (!tabWidget) {
        return std::nullopt;
    }
    return tabWidget->size();
}

void TMxpFrameWidgets::createTabFrame(const QString& name, const QString& title, const QString& parentName, const QSize& size, bool scrolling, bool select)
{
    QTabWidget* parentTabWidget = frameTabs(parentName);
    if (!parentTabWidget) {
        return;
    }

    // Create a page widget to hold the console (avoids flash on mpMainFrame)
    auto* tabPage = new QWidget();
    tabPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* tabPageLayout = new QVBoxLayout(tabPage);
    tabPageLayout->setContentsMargins(0, 0, 0, 0);

    auto* console = mpMainConsole->createSubConsole(name, tabPage);
    console->resize(size.width(), size.height());
    console->mOldX = 0;
    console->mOldY = 0;
    console->setContentsMargins(0, 0, 0, 0);
    int fontSize = mpMainConsole->mpHost->getDisplayFont().pointSize();
    console->setFontSize(fontSize > 0 ? fontSize : 12);
    console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    tabPageLayout->addWidget(console);

    mFrames.insert(name, {tabPage, console, nullptr});

    // Configure scrolling
    if (!scrolling) {
        console->setScrolling(false);
    }

    mpMainConsole->registerSubConsole(name, console);

    int newTabIndex = parentTabWidget->addTab(tabPage, title);
    if (select) {
        parentTabWidget->setCurrentIndex(newTabIndex);
    }
    console->show();
}

bool TMxpFrameWidgets::removeFromParentTabs(const QString& name, const QString& parentName)
{
    QTabWidget* parentTabWidget = frameTabs(parentName);
    QWidget* tabPage = frameWidget(name);
    if (!parentTabWidget || !tabPage) {
        return false;
    }

    const int tabIndex = parentTabWidget->indexOf(tabPage);
    if (tabIndex < 0) {
        return false;
    }

    parentTabWidget->removeTab(tabIndex);
    return true;
}

void TMxpFrameWidgets::destroyFrame(const QString& name)
{
    mpMainConsole->deregisterSubConsole(name);
    mpMainConsole->deregisterDockWidget(name);

    const Widgets widgets = mFrames.take(name);
    delete widgets.widget.data();
}

void TMxpFrameWidgets::showFrame(const QString& name)
{
    // Per CMUD 2.30 behavior, action="open" on an existing frame just shows it
    // without changing its size or position
    if (QWidget* widget = frameWidget(name)) {
        widget->show();
        widget->raise();
    }
}

void TMxpFrameWidgets::focusFrame(const QString& name)
{
    if (QWidget* widget = frameWidget(name)) {
        widget->raise();
        widget->setFocus();
    }
}

std::optional<QRect> TMxpFrameWidgets::placementArea(const QString& name) const
{
    const QWidget* widget = frameWidget(name);
    if (!widget) {
        return std::nullopt;
    }
    // pos() rather than geometry().topLeft(), which differ for a window
    return QRect(widget->pos(), widget->size());
}

bool TMxpFrameWidgets::placedOnMainWindow(const QString& name) const
{
    // An external frame keeps mpMainFrame as its parent even after Qt::Window
    // is set on it, so isWindow() rather than the parent is what tells them apart
    const QWidget* widget = frameWidget(name);
    return widget && !widget->isWindow() && widget->parentWidget() == mpMainConsole->mpMainFrame;
}

void TMxpFrameWidgets::setGeometry(const QString& name, const QRect& geometry)
{
    if (QWidget* widget = frameWidget(name)) {
        widget->setGeometry(geometry);
    }
}

QSize TMxpFrameWidgets::mainConsoleSize() const
{
    return mpMainConsole->size();
}

TPrintSink* TMxpFrameWidgets::sink(const QString& name) const
{
    TConsole* console = frameConsole(name);
    if (!console || console == mpMainConsole) {
        return nullptr;
    }
    return console;
}

QWidget* TMxpFrameWidgets::frameWidget(const QString& name) const
{
    return mFrames.value(name).widget.data();
}

TConsole* TMxpFrameWidgets::frameConsole(const QString& name) const
{
    return mFrames.value(name).console.data();
}

QTabWidget* TMxpFrameWidgets::frameTabs(const QString& name) const
{
    return mFrames.value(name).tabWidget.data();
}
