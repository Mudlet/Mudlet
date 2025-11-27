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
#include "TLabel.h"
#include "TMainConsole.h"

#include <QDebug>
#include <QFontMetrics>
#include <QFrame>
#include <QMainWindow>
#include <QVBoxLayout>

TMxpFrame::~TMxpFrame()
{
    // Clean up UI elements
    if (dockWidget) {
        delete dockWidget.data();
    } else if (widget) {
        delete widget.data();
    }
    
    // Note: We don't touch childFrames or parentFrame here.
    // When TMxpFrameManager::~TMxpFrameManager() calls qDeleteAll,
    // the iteration order is unpredictable, so child frames may already
    // be deleted. Accessing them would cause use-after-free.
    // The childFrames list is only valid during normal operation,
    // not during shutdown cleanup.
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
    if (mCurrentDestination.isEmpty()) {
        return qobject_cast<TConsole*>(mpHost->mpConsole);
    }
    
    const auto* frame = getFrame(mCurrentDestination);
    TConsole* console = frame ? frame->console.data() : nullptr;
    
    qDebug() << "TMxpFrameManager::getCurrentDestinationConsole() dest:" << mCurrentDestination
             << "frame:" << (frame ? frame->name : "null")
             << "console:" << (console ? "valid" : "null");
    
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
    
    TMainConsole* mainConsole = mpHost->mpConsole.data();
    
    // Check if DOCKFRAME is specified - if so, add as tab to existing frame
    if (!frame->dockFrame.isEmpty()) {
        TMxpFrame* targetFrame = getFrame(frame->dockFrame);
        if (targetFrame && targetFrame->tabWidget) {
            // Add this frame as a new tab in the target's TabWidget
            layoutTabIntoExistingFrame(frame, targetFrame);
            return;
        }
    }
    
    // Check if we're inside a DEST - if so, nest this frame inside the destination
    TMxpFrame* parentFrame = nullptr;
    if (!mCurrentDestination.isEmpty()) {
        parentFrame = getFrame(mCurrentDestination);
    }
    
    // Determine the container for this frame
    QSize containerSize;
    int containerX = 0;
    int containerY = 0;
    
    if (parentFrame && parentFrame->widget) {
        // Nested frame - position relative to parent
        containerSize = parentFrame->widget->size();
        containerX = parentFrame->widget->x();
        containerY = parentFrame->widget->y();
        
        // Track used space in parent for VBox-style stacking
        containerY += parentFrame->usedHeight;
        containerSize.setHeight(containerSize.height() - parentFrame->usedHeight);
        
        qDebug() << "  Nested frame in parent:" << parentFrame->name 
                 << "containerPos:" << containerX << "," << containerY
                 << "containerSize:" << containerSize
                 << "parent usedHeight:" << parentFrame->usedHeight;
    } else {
        // Top-level frame - use MXP-specific borders (not Host borders which are for Lua)
        containerSize = mainConsole->size();
        containerX = mMxpBorders.left();
        containerY = mMxpBorders.top();
        containerSize.setWidth(containerSize.width() - mMxpBorders.left() - mMxpBorders.right());
        containerSize.setHeight(containerSize.height() - mMxpBorders.top() - mMxpBorders.bottom());
        
        qDebug() << "  Top-level frame - windowSize:" << mainConsole->size()
                 << "mxpBorders:" << mMxpBorders
                 << "containerPos:" << containerX << "," << containerY
                 << "containerSize:" << containerSize;
    }
    
    // Calculate frame dimensions relative to container
    QSize widthSize = calculateFrameSize(frame->width, containerSize, false);
    QSize heightSize = calculateFrameSize(frame->height, containerSize, true);
    int frameWidth = widthSize.width();
    int frameHeight = heightSize.height();
    
    // For "100%" height in a nested context, use remaining space
    if (frame->height.trimmed() == qsl("100%") && parentFrame) {
        frameHeight = containerSize.height();  // Remaining height after usedHeight
    }
    
    qDebug() << "  layoutInternalFrame:" << frame->name 
             << "containerSize:" << containerSize
             << "spec:" << frame->width << "x" << frame->height
             << "calculated:" << frameWidth << "x" << frameHeight
             << "align:" << frame->align;
    
    // Ensure minimum size for visibility
    if (frameWidth < 50) frameWidth = 100;
    if (frameHeight < 20) frameHeight = 50;
    
    // Calculate position based on alignment
    int x = containerX;
    int y = containerY;
    QString align = frame->align.toLower();
    
    if (parentFrame) {
        // Nested frame - position within parent's bounds using VBox/HBox logic
        if (align == qsl("left") || align.isEmpty()) {
            // Default or left alignment in parent
            frameHeight = containerSize.height();
        } else if (align == qsl("right")) {
            x = containerX + containerSize.width() - frameWidth;
            frameHeight = containerSize.height();
        } else if (align == qsl("top")) {
            // VBox stacking - take full width, use calculated height
            frameWidth = containerSize.width();
            parentFrame->usedHeight += frameHeight;
        } else if (align == qsl("bottom")) {
            y = containerY + containerSize.height() - frameHeight;
            frameWidth = containerSize.width();
        }
    } else {
        // Top-level frame - position at window edges and update MXP borders
        QSize windowSize = mainConsole->size();
        
        if (align == qsl("left")) {
            // Left-aligned: position at actual left edge (after existing left MXP frames)
            x = mMxpBorders.left();
            y = 0;
            frameHeight = windowSize.height();
            // Update MXP left border
            mMxpBorders.setLeft(mMxpBorders.left() + frameWidth);
        } else if (align == qsl("right")) {
            // Right-aligned: position at right edge
            x = windowSize.width() - mMxpBorders.right() - frameWidth;
            y = 0;
            frameHeight = windowSize.height();
            mMxpBorders.setRight(mMxpBorders.right() + frameWidth);
        } else if (align == qsl("top")) {
            // Top-aligned: position at top edge
            x = mMxpBorders.left();
            y = mMxpBorders.top();
            frameWidth = windowSize.width() - mMxpBorders.left() - mMxpBorders.right();
            mMxpBorders.setTop(mMxpBorders.top() + frameHeight);
        } else if (align == qsl("bottom")) {
            // Bottom-aligned: position at bottom edge
            x = mMxpBorders.left();
            y = windowSize.height() - mMxpBorders.bottom() - frameHeight;
            frameWidth = windowSize.width() - mMxpBorders.left() - mMxpBorders.right();
            mMxpBorders.setBottom(mMxpBorders.bottom() + frameHeight);
        } else {
            // No alignment specified - treat as client/center area
            x = containerX;
            y = containerY;
        }
        
        qDebug() << "  Updated mMxpBorders:" << mMxpBorders;
        
        // Apply MXP borders to Host so main console resizes to accommodate frames
        qDebug() << "  Calling mpHost->setBorders(" << mMxpBorders << ")";
        mpHost->setBorders(mMxpBorders);
    }
    
    qDebug() << "  Frame position:" << x << "," << y << "size:" << frameWidth << "x" << frameHeight;
    
    // Create a container frame with TabWidget header
    const int tabBarHeight = 24;
    
    // Create the container widget for the frame
    auto* containerWidget = new QFrame(mainConsole->mpMainFrame);
    containerWidget->setObjectName(frame->name + qsl("_container"));
    containerWidget->setGeometry(x, y, frameWidth, frameHeight);
    containerWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    containerWidget->setLineWidth(1);
    containerWidget->setStyleSheet(
        qsl("QFrame { background-color: #1a1a1a; border: 1px solid #444444; }"));
    
    // Create a layout for the container
    auto* containerLayout = new QVBoxLayout(containerWidget);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    
    // Create TabWidget as the header - allows future tab additions
    auto* tabWidget = new QTabWidget(containerWidget);
    tabWidget->setObjectName(frame->name + qsl("_tabs"));
    tabWidget->setTabPosition(QTabWidget::North);
    tabWidget->setDocumentMode(true);  // Cleaner look
    tabWidget->setStyleSheet(qsl(
        "QTabWidget::pane { border: none; background-color: transparent; }"
        "QTabBar::tab { background-color: #2a2a2a; color: #cccccc; padding: 4px 12px; "
        "              border: 1px solid #444444; border-bottom: none; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #3a3a3a; color: #ffffff; }"
        "QTabBar::tab:hover { background-color: #333333; }"));
    
    // Create a page widget to hold the console
    auto* tabPage = new QWidget();
    auto* tabPageLayout = new QVBoxLayout(tabPage);
    tabPageLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create mini console inside the tab page
    auto* console = mainConsole->createMiniConsole(
        qsl("main"), 
        frame->name, 
        0, 0,  // Position managed by layout
        frameWidth, 
        frameHeight - tabBarHeight);
    
    if (!console) {
        qWarning() << "TMxpFrameManager::layoutInternalFrame: Failed to create console for" << frame->name;
        delete containerWidget;
        return;
    }
    
    // Add console to tab page layout
    tabPageLayout->addWidget(console);
    
    // Add the tab with the frame title
    tabWidget->addTab(tabPage, frame->title);
    
    // Add TabWidget to container
    containerLayout->addWidget(tabWidget);
    
    // Store the container, TabWidget, and console
    frame->widget = containerWidget;
    frame->tabWidget = tabWidget;
    frame->console = console;
    frame->dockWidget = nullptr;
    frame->usedHeight = 0;  // Initialize for potential child frames
    
    // Track parent-child relationship
    if (parentFrame) {
        frame->parentFrame = parentFrame;
        parentFrame->childFrames.append(frame);
    }
    
    // Configure scrolling
    if (!frame->scrolling) {
        console->setScrolling(false);
    }
    
    // Set console colors - slightly different background for visual distinction
    console->setFgColor(mainConsole->mFgColor);
    // Use a slightly lighter background for frames to distinguish from main console
    QColor frameBgColor = mainConsole->mBgColor;
    frameBgColor = frameBgColor.lighter(115);  // 15% lighter than main console
    console->setBgColor(frameBgColor);
    
    // Add a subtle border to visually separate frames
    console->setStyleSheet(qsl("QWidget { border: 1px solid #444444; }"));
    
    // Register console
    mainConsole->mSubConsoleMap.insert(frame->name, console);
    
    // Show the container widget and its contents
    containerWidget->show();
    containerWidget->raise();
    console->show();
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
    frame->console = console;
    
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
    frame->console = console;
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
            int result = chars * fm.lineSpacing();
            qDebug() << "    calculateFrameSize: spec=" << spec << "chars=" << chars << "lineSpacing=" << fm.lineSpacing() << "result=" << result;
            return QSize(0, result);
        } else {
            int result = chars * fm.averageCharWidth();
            qDebug() << "    calculateFrameSize: spec=" << spec << "chars=" << chars << "avgCharWidth=" << fm.averageCharWidth() << "result=" << result;
            return QSize(result, 0);
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
        
        qDebug() << "    calculateFrameSize: spec=" << spec << "percent=" << percent << "dimension=" << dimension << "result=" << size;
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

void TMxpFrameManager::layoutTabIntoExistingFrame(TMxpFrame* frame, TMxpFrame* targetFrame)
{
    if (!mpHost || !mpHost->mpConsole) {
        qWarning() << "TMxpFrameManager::layoutTabIntoExistingFrame: No console available";
        return;
    }
    
    if (!targetFrame->tabWidget) {
        qWarning() << "TMxpFrameManager::layoutTabIntoExistingFrame: Target frame has no TabWidget:" << targetFrame->name;
        return;
    }
    
    TMainConsole* mainConsole = mpHost->mpConsole.data();
    QTabWidget* tabWidget = targetFrame->tabWidget;
    
    qDebug() << "  Adding frame" << frame->name << "as tab in" << targetFrame->name;
    
    // Create a page widget to hold the console
    auto* tabPage = new QWidget();
    auto* tabPageLayout = new QVBoxLayout(tabPage);
    tabPageLayout->setContentsMargins(0, 0, 0, 0);
    
    // Get size from target frame's container
    QSize containerSize = targetFrame->widget ? targetFrame->widget->size() : QSize(200, 200);
    
    // Create mini console for this tab
    auto* console = mainConsole->createMiniConsole(
        qsl("main"), 
        frame->name, 
        0, 0,  // Position managed by layout
        containerSize.width(), 
        containerSize.height() - 30);  // Account for tab bar
    
    if (!console) {
        qWarning() << "TMxpFrameManager::layoutTabIntoExistingFrame: Failed to create console for" << frame->name;
        delete tabPage;
        return;
    }
    
    // Add console to tab page layout
    tabPageLayout->addWidget(console);
    
    // Add the new tab
    int tabIndex = tabWidget->addTab(tabPage, frame->title);
    
    // Store references
    frame->widget = tabPage;
    frame->tabWidget = tabWidget;  // Share the TabWidget reference
    frame->console = console;
    frame->parentFrame = targetFrame;
    targetFrame->childFrames.append(frame);
    
    // Configure scrolling
    if (!frame->scrolling) {
        console->setScrolling(false);
    }
    
    // Set console colors
    console->setFgColor(mainConsole->mFgColor);
    QColor frameBgColor = mainConsole->mBgColor;
    frameBgColor = frameBgColor.lighter(115);
    console->setBgColor(frameBgColor);
    
    // Register console
    mainConsole->mSubConsoleMap.insert(frame->name, console);
    
    // Optionally switch to the new tab
    tabWidget->setCurrentIndex(tabIndex);
    
    console->show();
    
    qDebug() << "  Tab added successfully, total tabs:" << tabWidget->count();
}
