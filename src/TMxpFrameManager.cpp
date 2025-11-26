/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TMxpFrameManager.h"
#include "Host.h"
#include "TConsole.h"
#include "TDockWidget.h"
#include "TMainConsole.h"

#include <QDebug>
#include <QFontMetrics>
#include <QMainWindow>

TMxpFrame::~TMxpFrame()
{
    // Clean up UI elements
    if (dockWidget) {
        delete dockWidget.data();
    } else if (widget) {
        delete widget.data();
    }
    
    // Don't delete child frames here - they're owned by TMxpFrameManager
    // Just clear parent pointers to avoid dangling references
    for (auto* child : childFrames) {
        if (child) {
            child->parentFrame = nullptr;
        }
    }

    childFrames.clear();
}

TMxpFrameManager::TMxpFrameManager(Host* host)
: mpHost(host)
{
}

TMxpFrameManager::~TMxpFrameManager()
{
    // Clean up all frames
    qDeleteAll(mFrames);
    mFrames.clear();
}

bool TMxpFrameManager::createFrame(const QString& name, const QMap<QString, QString>& attributes)
{
    qDebug() << "TMxpFrameManager::createFrame() called with name:" << name;
    qDebug() << "  Attributes:" << attributes;
    
    // Don't create frames if MXP is not enabled
    if (!mpHost->mMxpProcessor.isEnabled()) {
        qDebug() << "  REJECTED: MXP not enabled";
        return false;
    }
    
    if (!validateFrameName(name)) {
        qWarning() << "TMxpFrameManager::createFrame: Invalid frame name:" << name;
        return false;
    }
    
    if (frameExists(name)) {
        // Frame already exists - this is normal on screen refresh.
        // Return true to indicate the tag was handled (silently ignore duplicate).
        return true;
    }
    
    if (!canCreateFrame()) {
        qWarning() << "TMxpFrameManager::createFrame: Maximum frame limit reached";
        return false;
    }
    
    auto* frame = new TMxpFrame();
    frame->name = name;
    
    // Parse attributes
    frame->isInternal = attributes.contains(qsl("INTERNAL")) || !attributes.contains(qsl("EXTERNAL"));
    frame->align = attributes.value(qsl("ALIGN"), qsl("left")).toLower();
    frame->width = attributes.value(qsl("WIDTH"), qsl("25%"));
    frame->height = attributes.value(qsl("HEIGHT"), qsl("25%"));
    frame->title = attributes.value(qsl("TITLE"), name);
    frame->scrolling = attributes.value(qsl("SCROLLING"), qsl("YES")).toUpper() == qsl("YES");
    frame->dockFrame = attributes.value(qsl("DOCK"));
    
    qDebug() << "  Parsed frame attributes:";
    qDebug() << "    isInternal:" << frame->isInternal;
    qDebug() << "    align:" << frame->align;
    qDebug() << "    width:" << frame->width;
    qDebug() << "    height:" << frame->height;
    qDebug() << "    title:" << frame->title;
    qDebug() << "    scrolling:" << frame->scrolling;
    qDebug() << "    dockFrame:" << frame->dockFrame;
    
    // Note: We don't auto-assign dockFrame based on current DEST anymore.
    // MXP FRAME positioning is based on ALIGN attribute relative to main window,
    // not nested inside the current destination frame.
    
    // Check for action attribute
    QString action = attributes.value(qsl("ACTION"), qsl("open")).toLower();

    if (action == qsl("close")) {
        delete frame;
        return closeFrame(name);
    } else if (action == qsl("focus")) {
        delete frame;
        return focusFrame(name);
    }
    
    // Create the appropriate UI layout
    qDebug() << "  Creating UI layout - isInternal:" << frame->isInternal << "dockFrame:" << frame->dockFrame << "align:" << frame->align;
    if (frame->isInternal) {
        if (!frame->dockFrame.isEmpty() && frame->align == qsl("client")) {
            qDebug() << "  -> layoutTabFrame";
            layoutTabFrame(frame);
        } else {
            qDebug() << "  -> layoutInternalFrame";
            layoutInternalFrame(frame);
        }
    } else {
        qDebug() << "  -> layoutExternalFrame";
        layoutExternalFrame(frame);
    }
    
    // Store the frame
    mFrames[name] = frame;
    
    return true;
}

bool TMxpFrameManager::closeFrame(const QString& name)
{
    auto* frame = getFrame(name);
    if (!frame) {
        return false;
    }
    
    // Clear destination if it was pointing to this frame (before deletion)
    if (mCurrentDestination == name) {
        clearDestination();
    }
    
    // Close all child frames first to avoid dangling pointers
    // Make a copy of childFrames list since closeFrame modifies it
    QList<TMxpFrame*> childrenCopy = frame->childFrames;

    for (auto* child : childrenCopy) {
        if (child) {
            closeFrame(child->name);
        }
    }
    
    // Unregister from main console maps before deletion
    if (mpHost && mpHost->mpConsole) {
        mpHost->mpConsole->mSubConsoleMap.remove(name);
        mpHost->mpConsole->mDockWidgetMap.remove(name);
    }
    
    // Remove from hierarchy
    removeFrameFromHierarchy(frame);
    
    // Remove from map and delete
    mFrames.remove(name);
    delete frame;
    
    return true;
}

bool TMxpFrameManager::focusFrame(const QString& name)
{
    auto* frame = getFrame(name);

    if (!frame) {
        return false;
    }
    
    if (frame->dockWidget) {
        frame->dockWidget->raise();
        frame->dockWidget->setFocus();
    } else if (frame->widget) {
        frame->widget->raise();
        frame->widget->setFocus();
    }
    
    return true;
}

void TMxpFrameManager::setDestination(const QString& frameName, bool eol, bool eof)
{
    qDebug() << "TMxpFrameManager::setDestination() called:" << frameName << "eol:" << eol << "eof:" << eof;
    
    // Don't set destination if MXP is not enabled
    if (!mpHost->mMxpProcessor.isEnabled()) {
        qDebug() << "  REJECTED: MXP not enabled";
        return;
    }
    
    if (frameName.isEmpty()) {
        qDebug() << "  Clearing destination (empty name)";
        clearDestination();
        return;
    }
    
    auto* frame = getFrame(frameName);

    if (!frame) {
        qWarning() << "TMxpFrameManager::setDestination: Frame not found:" << frameName;
        return;
    }
    
    qDebug() << "  Setting destination to:" << frameName;
    mCurrentDestination = frameName;
    
    // Handle content clearing
    auto* console = getCurrentDestinationConsole();

    if (console) {
        if (eof) {
            // Clear all content in the frame
            console->buffer.clear();
        } else if (eol) {
            // Clear only the current (last) line being built
            if (!console->buffer.buffer.empty()) {
                console->buffer.buffer.back().clear();
            }
        }
    }
}

void TMxpFrameManager::clearDestination()
{
    mCurrentDestination.clear();
}

QWidget* TMxpFrameManager::getCurrentDestinationWidget() const
{
    if (mCurrentDestination.isEmpty()) {
        return mpHost->mpConsole;
    }
    
    const auto* frame = getFrame(mCurrentDestination);
    return frame ? frame->widget.data() : nullptr;
}

TConsole* TMxpFrameManager::getCurrentDestinationConsole() const
{
    QWidget* widget = getCurrentDestinationWidget();
    TConsole* console = qobject_cast<TConsole*>(widget);
    
    if (!mCurrentDestination.isEmpty()) {
        qDebug() << "TMxpFrameManager::getCurrentDestinationConsole() dest:" << mCurrentDestination
                 << "widget:" << (widget ? widget->objectName() : "null")
                 << "console:" << (console ? "valid" : "null");
    }
    
    return console;
}

void TMxpFrameManager::sendTextToDestination(const QString& text)
{
    if (text.isEmpty()) {
        return;
    }
    
    TConsole* console = getCurrentDestinationConsole();
    if (console && console != mpHost->mpConsole) {
        // Send to destination frame's console
        console->print(text);
    }
    // If no valid destination, text stays in main console (default behavior)
}

TMxpFrame* TMxpFrameManager::getFrame(const QString& name)
{
    return mFrames.value(name, nullptr);
}

const TMxpFrame* TMxpFrameManager::getFrame(const QString& name) const
{
    return mFrames.value(name, nullptr);
}

QStringList TMxpFrameManager::getFrameNames() const
{
    return mFrames.keys();
}

void TMxpFrameManager::layoutInternalFrame(TMxpFrame* frame)
{
    if (!mpHost || !mpHost->mpConsole) {
        qWarning() << "TMxpFrameManager::layoutInternalFrame: No console available";
        return;
    }
    
    // Check if frame should be nested in another frame's dock widget
    if (!frame->dockFrame.isEmpty()) {
        auto* parentFrame = getFrame(frame->dockFrame);

        if (parentFrame && parentFrame->dockWidget) {
            // Nest this frame inside parent's dock widget
            layoutNestedFrame(frame, parentFrame);
            return;
        }
    }
    
    // Get main window for docking
    auto* mainWindow = qobject_cast<QMainWindow*>(mpHost->mpConsole->window());

    if (!mainWindow) {
        qWarning() << "TMxpFrameManager::layoutInternalFrame: Main window not found";
        return;
    }
    
    // Create TDockWidget for MXP frame
    auto* dockWidget = new TDockWidget(mpHost, frame->name);
    dockWidget->setObjectName(qsl("mxpFrame_%1_%2").arg(mpHost->getName(), frame->name));
    dockWidget->setWindowTitle(frame->title);
    // MXP frames: allow closing but not moving/floating for cleaner layout
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    frame->dockWidget = dockWidget;
    
    // Register in main console's dock widget map
    mpHost->mpConsole->mDockWidgetMap.insert(frame->name, dockWidget);
    
    // Calculate size
    QSize mainSize = mainWindow->size();
    QSize widthSize = calculateFrameSize(frame->width, mainSize, false);
    QSize heightSize = calculateFrameSize(frame->height, mainSize, true);
    int frameWidth = widthSize.width();
    int frameHeight = heightSize.height();
    
    qDebug() << "  layoutInternalFrame:" << frame->name << "mainSize:" << mainSize 
             << "spec:" << frame->width << "x" << frame->height
             << "calculated:" << frameWidth << "x" << frameHeight;
    
    // Ensure minimum size for visibility
    if (frameWidth < 100) frameWidth = 200;
    if (frameHeight < 50) frameHeight = 150;
    
    // Create mini console for content
    auto* console = mpHost->mpConsole->createMiniConsole(
        qsl("main"), 
        frame->name, 
        0, 0, 
        frameWidth, 
        frameHeight);
    
    if (!console) {
        qWarning() << "TMxpFrameManager::layoutInternalFrame: Failed to create console";
        delete dockWidget;
        frame->dockWidget = nullptr;
        return;
    }
    
    frame->widget = console;
    
    // Configure scrolling
    if (!frame->scrolling) {
        console->setScrolling(false);
    }
    
    // Register console in main console's sub-console map
    mpHost->mpConsole->mSubConsoleMap.insert(frame->name, console);
    
    // Set console as dock widget content and link them
    dockWidget->setTConsole(console);
    
    // Apply profile stylesheet
    dockWidget->setStyleSheet(mpHost->mProfileStyleSheet);
    
    // Determine dock area and add to main window
    Qt::DockWidgetArea area = alignmentToDockArea(frame->align);
    
    // Check if there's already a dock widget in this area to tabify with
    QDockWidget* existingDock = nullptr;
    for (auto it = mFrames.begin(); it != mFrames.end(); ++it) {
        TMxpFrame* otherFrame = it.value();
        if (otherFrame != frame && otherFrame->dockWidget) {
            Qt::DockWidgetArea otherArea = mainWindow->dockWidgetArea(otherFrame->dockWidget);
            if (otherArea == area) {
                existingDock = otherFrame->dockWidget;
                break;
            }
        }
    }
    
    mainWindow->addDockWidget(area, dockWidget);
    
    // Tabify with existing dock widget in the same area for cleaner layout
    if (existingDock) {
        mainWindow->tabifyDockWidget(existingDock, dockWidget);
    }
    
    // Apply the requested size using resizeDocks
    // For left/right dock areas, use the width; for top/bottom, use the height
    if (area == Qt::LeftDockWidgetArea || area == Qt::RightDockWidgetArea) {
        if (frameWidth > 0) {
            mainWindow->resizeDocks({dockWidget}, {frameWidth}, Qt::Horizontal);
        }
    } else {
        if (frameHeight > 0) {
            mainWindow->resizeDocks({dockWidget}, {frameHeight}, Qt::Vertical);
        }
    }
    
    // Show and raise the dock widget (raise makes it the active tab if tabified)
    dockWidget->show();
    dockWidget->raise();
}

void TMxpFrameManager::layoutExternalFrame(TMxpFrame* frame)
{
    if (!mpHost || !mpHost->mpConsole) {
        qWarning() << "TMxpFrameManager::layoutExternalFrame: No console available";
        return;
    }
    
    // Calculate size
    QSize mainSize = mpHost->mpConsole->size();
    QSize widthSize = calculateFrameSize(frame->width, mainSize, false);
    QSize heightSize = calculateFrameSize(frame->height, mainSize, true);
    int frameWidth = widthSize.width();
    int frameHeight = heightSize.height();
    
    // Create standalone window with mini console
    auto* console = mpHost->mpConsole->createMiniConsole(
        qsl("main"),
        frame->name,
        0, 0,
        frameWidth,
        frameHeight);
    
    if (!console) {
        qWarning() << "TMxpFrameManager::layoutExternalFrame: Failed to create console";
        return;
    }
    
    frame->widget = console;
    
    // Configure scrolling
    if (!frame->scrolling) {
        console->setScrolling(false);
    }
    
    // Set window title and show as floating
    console->setWindowTitle(frame->title);
    console->setWindowFlags(Qt::Window);
    console->show();
}

void TMxpFrameManager::layoutTabFrame(TMxpFrame* frame)
{
    if (!mpHost || !mpHost->mpConsole) {
        qWarning() << "TMxpFrameManager::layoutTabFrame: No console available";
        return;
    }
    
    // Find parent frame for tab docking
    auto* parentFrame = getFrame(frame->dockFrame);

    if (!parentFrame) {
        qWarning() << "TMxpFrameManager::layoutTabFrame: Parent frame not found:" << frame->dockFrame;
        layoutInternalFrame(frame);
        return;
    }
    
    // Ensure parent has a tab widget
    if (!parentFrame->tabWidget && parentFrame->dockWidget) {
        // Create tab widget to replace single widget
        auto* tabWidget = new QTabWidget();
        
        // Move existing widget to first tab
        if (parentFrame->widget) {
            tabWidget->addTab(parentFrame->widget, parentFrame->title);
        }
        
        parentFrame->tabWidget = tabWidget;
        parentFrame->dockWidget->setWidget(tabWidget);
    }
    
    if (!parentFrame->tabWidget) {
        qWarning() << "TMxpFrameManager::layoutTabFrame: Failed to create tab widget";
        layoutInternalFrame(frame);
        return;
    }
    
    // Calculate size
    QSize tabSize = parentFrame->tabWidget->size();
    QSize frameSize = calculateFrameSize(frame->width, tabSize, false) + 
                      calculateFrameSize(frame->height, tabSize, true);
    
    // Create console for this tab
    auto* console = mpHost->mpConsole->createMiniConsole(
        qsl("main"),
        frame->name,
        0, 0,
        frameSize.width(),
        frameSize.height());
    
    if (!console) {
        qWarning() << "TMxpFrameManager::layoutTabFrame: Failed to create console";
        return;
    }
    
    frame->widget = console;
    frame->parentFrame = parentFrame;
    parentFrame->childFrames.append(frame);
    
    // Configure scrolling
    if (!frame->scrolling) {
        console->setScrolling(false);
    }
    
    // Add as new tab
    parentFrame->tabWidget->addTab(console, frame->title);
}

QSize TMxpFrameManager::calculateFrameSize(const QString& spec, const QSize& containerSize, bool isHeight)
{
    if (spec.isEmpty()) {
        return QSize(0, 0);
    }
    
    QString trimmed = spec.trimmed();
    
    // Character-based size (e.g., "40c")
    if (trimmed.endsWith('c', Qt::CaseInsensitive)) {
        bool ok;
        int chars = trimmed.left(trimmed.length() - 1).toInt(&ok);
        if (!ok || chars <= 0) {
            return QSize(0, 0);
        }
        
        // Get font metrics from main console
        QFont font = mpHost->getDisplayFont();
        QFontMetrics fm(font);
        
        if (isHeight) {
            return QSize(0, chars * fm.lineSpacing());
        } else {
            return QSize(chars * fm.averageCharWidth(), 0);
        }
    }
    
    // Percentage-based size (e.g., "25%")
    if (trimmed.endsWith('%')) {
        bool ok;
        int percent = trimmed.left(trimmed.length() - 1).toInt(&ok);

        if (!ok || percent <= 0 || percent > 100) {
            return QSize(0, 0);
        }
        
        int dimension = isHeight ? containerSize.height() : containerSize.width();
        int size = (dimension * percent) / 100;
        
        return isHeight ? QSize(0, size) : QSize(size, 0);
    }
    
    // Pixel-based size (e.g., "350px" or just "350")
    QString pixelStr = trimmed;

    if (pixelStr.endsWith(qsl("px"), Qt::CaseInsensitive)) {
        pixelStr = pixelStr.left(pixelStr.length() - 2);
    }
    
    bool ok;
    int pixels = pixelStr.toInt(&ok);

    if (!ok || pixels <= 0) {
        return QSize(0, 0);
    }
    
    return isHeight ? QSize(0, pixels) : QSize(pixels, 0);
}

Qt::DockWidgetArea TMxpFrameManager::alignmentToDockArea(const QString& align)
{
    QString lower = align.toLower();

    if (lower == qsl("top")) {
        return Qt::TopDockWidgetArea;
    } else if (lower == qsl("bottom")) {
        return Qt::BottomDockWidgetArea;
    } else if (lower == qsl("right")) {
        return Qt::RightDockWidgetArea;
    } else {
        return Qt::LeftDockWidgetArea;
    }
}

bool TMxpFrameManager::validateFrameName(const QString& name) const
{
    if (name.isEmpty()) {
        return false;
    }
    
    // Basic validation - alphanumeric, underscore, hyphen
    static QRegularExpression validNamePattern(qsl("^[a-zA-Z0-9_-]+$"));
    return validNamePattern.match(name).hasMatch();
}

bool TMxpFrameManager::canCreateFrame() const
{
    return mFrames.size() < MAX_FRAMES;
}

void TMxpFrameManager::removeFrameFromHierarchy(TMxpFrame* frame)
{
    if (!frame) {
        return;
    }
    
    // Remove from parent's child list
    if (frame->parentFrame) {
        frame->parentFrame->childFrames.removeOne(frame);
    }
    
    // Handle orphaned children
    for (auto* child : frame->childFrames) {
        child->parentFrame = nullptr;
    }

    frame->childFrames.clear();
}

void TMxpFrameManager::layoutNestedFrame(TMxpFrame* frame, TMxpFrame* parentFrame)
{
    if (!mpHost || !mpHost->mpConsole || !parentFrame || !parentFrame->dockWidget) {
        qWarning() << "TMxpFrameManager::layoutNestedFrame: Invalid parent frame or dock widget";
        return;
    }
    
    // Get main window for docking
    auto* mainWindow = qobject_cast<QMainWindow*>(mpHost->mpConsole->window());
    if (!mainWindow) {
        qWarning() << "TMxpFrameManager::layoutNestedFrame: Main window not found";
        return;
    }
    
    // Calculate size based on parent's content size
    QSize parentSize = parentFrame->dockWidget->size();
    QSize widthSize = calculateFrameSize(frame->width, parentSize, false);
    QSize heightSize = calculateFrameSize(frame->height, parentSize, true);
    int frameWidth = widthSize.width();
    int frameHeight = heightSize.height();
    
    // Create mini console for content
    auto* console = mpHost->mpConsole->createMiniConsole(
        qsl("main"), 
        frame->name, 
        0, 0, 
        frameWidth, 
        frameHeight);
    
    if (!console) {
        qWarning() << "TMxpFrameManager::layoutNestedFrame: Failed to create console";
        return;
    }
    
    frame->widget = console;
    
    // Configure scrolling
    if (!frame->scrolling) {
        console->setScrolling(false);
    }
    
    // Register console in main console's sub-console map
    mpHost->mpConsole->mSubConsoleMap.insert(frame->name, console);
    
    // Set up parent-child relationship
    frame->parentFrame = parentFrame;
    parentFrame->childFrames.append(frame);
    
    // Create a new TDockWidget for this nested frame
    auto* dockWidget = new TDockWidget(mpHost, frame->name);
    dockWidget->setObjectName(qsl("mxpFrame_%1_%2").arg(mpHost->getName(), frame->name));
    dockWidget->setWindowTitle(frame->title);
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    frame->dockWidget = dockWidget;
    
    // Register in main console's dock widget map
    mpHost->mpConsole->mDockWidgetMap.insert(frame->name, dockWidget);
    
    // Set console as dock widget content
    dockWidget->setTConsole(console);
    
    // Apply profile stylesheet
    dockWidget->setStyleSheet(mpHost->mProfileStyleSheet);
    
    // Determine orientation and order for splitDockWidget
    // splitDockWidget(first, second, orientation) places first before second
    // For top/left alignment: new frame should appear before parent
    // For bottom/right alignment: new frame should appear after parent
    Qt::Orientation orientation;
    bool newFrameFirst;
    int sizeToApply;
    
    if (frame->align == qsl("top")) {
        orientation = Qt::Vertical;
        newFrameFirst = true;
        sizeToApply = frameHeight;
    } else if (frame->align == qsl("bottom")) {
        orientation = Qt::Vertical;
        newFrameFirst = false;
        sizeToApply = frameHeight;
    } else if (frame->align == qsl("left")) {
        orientation = Qt::Horizontal;
        newFrameFirst = true;
        sizeToApply = frameWidth;
    } else {
        // Default to right
        orientation = Qt::Horizontal;
        newFrameFirst = false;
        sizeToApply = frameWidth;
    }
    
    // Split the parent dock widget to make room for this nested frame
    if (newFrameFirst) {
        mainWindow->splitDockWidget(dockWidget, parentFrame->dockWidget, orientation);
    } else {
        mainWindow->splitDockWidget(parentFrame->dockWidget, dockWidget, orientation);
    }
    
    // Apply the requested size using resizeDocks
    if (sizeToApply > 0) {
        mainWindow->resizeDocks({dockWidget}, {sizeToApply}, orientation);
    }
    
    dockWidget->show();
}
