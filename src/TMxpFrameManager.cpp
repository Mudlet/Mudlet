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

#include "TMxpFrameManager.h"
#include "Host.h"
#include "TConsole.h"
#include "TDockWidget.h"
#include "TLabel.h"
#include "TMainConsole.h"
#include "TTextEdit.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFontMetrics>
#include <QFrame>
#include <QMainWindow>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>

TMxpFrame::~TMxpFrame()
{
    mBeingDestroyed = true;

    if (parentFrame && !parentFrame->mBeingDestroyed) {
        parentFrame->childFrames.removeOne(this);
    }
    parentFrame = nullptr;

    // Orphan children - we do NOT delete them since TMxpFrameManager owns all frames
    for (TMxpFrame* child : std::as_const(childFrames)) {
        if (child && !child->mBeingDestroyed) {
            child->parentFrame = nullptr;
        }
    }
    childFrames.clear();

    if (dockWidget) {
        delete dockWidget.data();
    } else if (widget) {
        delete widget.data();
    }
}

TMxpFrameManager::TMxpFrameManager(Host* host)
: mpHost(host)
{
}

TMxpFrameManager::~TMxpFrameManager()
{
    // Clean up all frames properly (children before parents, clear destinations, etc.)
    resetAllFrames();
}

bool TMxpFrameManager::createFrame(const QString& name, const QMap<QString, QString>& attributes)
{
    if (!mpHost->mMxpProcessor.isEnabled()) {
        return false;
    }

    if (!validateFrameName(name)) {
        qWarning() << "TMxpFrameManager::createFrame: Invalid frame name:" << name;
        return false;
    }

    // Parse action first - avoids unnecessary TMxpFrame allocation for close/focus
    // Per Zugg/CMUD behavior: action determines what to do with the frame
    QString action = attributes.value(qsl("ACTION"), qsl("open")).toLower();

    if (action == qsl("close")) {
        return closeFrame(name);
    } else if (action == qsl("focus")) { // NOLINT(readability-else-after-return)
        return focusFrame(name);
    }

    // action="open" (default) - show existing frame or create new one
    if (frameExists(name)) {
        // Frame already exists - per CMUD 2.30 behavior, don't recreate or resize.
        // This respects any user changes to frame position/size.
        // Just ensure the frame is visible and return success.
        return showFrame(name);
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
    frame->left = attributes.value(qsl("LEFT"));
    frame->top = attributes.value(qsl("TOP"));
    frame->title = attributes.value(qsl("TITLE"), name);
    frame->hasExplicitTitle = attributes.contains(qsl("TITLE"));
    frame->scrolling = attributes.value(qsl("SCROLLING"), qsl("YES")).toUpper() == qsl("YES");
    frame->floating = attributes.contains(qsl("FLOATING"));
    frame->dockFrame = attributes.value(qsl("DOCK"));

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameManager::createFrame:" << name << "TITLE attr:" << attributes.value(qsl("TITLE")) << "title:" << frame->title << "floating:" << frame->floating;
#endif

    // relayoutFrames() works off mFrameOrder, so nothing may lay a frame out
    // before it is in there
    mFrames[name] = frame;
    mFrameOrder.append(frame);

    // Create the appropriate UI layout
    if (frame->isInternal) {
        if (!frame->dockFrame.isEmpty() && frame->align == qsl("client")) {
            layoutTabFrame(frame);
        } else {
            layoutInternalFrame(frame);
        }
    } else {
        layoutExternalFrame(frame);
    }

    return true;
}

bool TMxpFrameManager::closeFrame(const QString& name)
{
    auto* frame = getFrame(name);
    if (!frame) {
        // Frame doesn't exist - treat as success since it's already "closed"
        // This prevents literal MXP tags from appearing when closing non-existent frames
        return true;
    }

    if (mCurrentDestination == name) {
        clearDestination();
    }

    // Special handling for frames that are tabs in a parent frame
    if (frame->parentFrame && frame->parentFrame->tabWidget && frame->widget) {
        QTabWidget* parentTabWidget = frame->parentFrame->tabWidget;
        QWidget* tabWidget = frame->widget;

        // Find the tab index for this frame's widget
        int tabIndex = parentTabWidget->indexOf(tabWidget);
        if (tabIndex >= 0) {
            // Remove the tab from the parent's tab widget
            parentTabWidget->removeTab(tabIndex);

            // Clean up console registration
            if (mpHost && mpHost->mpConsole) {
                mpHost->mpConsole->deregisterSubConsole(name);
                mpHost->mpConsole->deregisterDockWidget(name);
            }

            // Remove from hierarchy
            removeFrameFromHierarchy(frame);

            // Remove from frames map and delete
            mFrames.remove(name);
            mFrameOrder.removeOne(frame);
            delete frame;

            // No need to recalculate borders for tab frames since they don't affect main window borders
            return true;
        }
    }

    // Close children first - make a copy since closeFrame modifies the list
    QList<TMxpFrame*> childrenCopy = frame->childFrames;

    for (auto* child : childrenCopy) {
        if (child) {
            closeFrame(child->name);
        }
    }

    if (mpHost && mpHost->mpConsole) {
        mpHost->mpConsole->deregisterSubConsole(name);
        mpHost->mpConsole->deregisterDockWidget(name);
    }

    removeFrameFromHierarchy(frame);

    mFrames.remove(name);
    mFrameOrder.removeOne(frame);
    delete frame;

    // Reposition what is left so it reclaims the space the frame gave up
    relayoutFrames();

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

bool TMxpFrameManager::showFrame(const QString& name)
{
    auto* frame = getFrame(name);

    if (!frame) {
        return false;
    }

    // Make the frame visible - per CMUD 2.30 behavior, action="open" on existing
    // frame should just show it without changing size/position
    if (frame->dockWidget) {
        frame->dockWidget->show();
        frame->dockWidget->raise();
    } else if (frame->widget) {
        frame->widget->show();
        frame->widget->raise();
    }

    return true;
}

void TMxpFrameManager::resetAllFrames()
{
    // Called on reconnect - MXP frames don't persist between sessions
    QStringList frameNames = mFrames.keys();

    for (const QString& name : std::as_const(frameNames)) {
        closeFrame(name);
    }

    mFrameOrder.clear();
    mMxpBorders = QMargins();

    if (mpHost) {
        mpHost->setMxpBorders(QMargins());
    }

    clearDestination();
}

void TMxpFrameManager::setDestination(const QString& frameName, bool eol, bool eof)
{
    if (!mpHost->mMxpProcessor.isEnabled()) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpFrameManager::setDestination: MXP not enabled, ignoring";
#endif
        return;
    }

    if (frameName.isEmpty()) {
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpFrameManager::setDestination: Empty frame name, clearing destination";
#endif
        clearDestination();
        return;
    }

    auto* frame = getFrame(frameName);

    if (!frame) {
        qWarning() << "TMxpFrameManager::setDestination: Frame not found:" << frameName;
#ifdef DEBUG_MXP_PROCESSING
        qDebug() << "TMxpFrameManager::setDestination: Available frames:" << getFrameNames();
#endif
        return;
    }

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameManager::setDestination: Setting destination to" << frameName << "frame->console:" << (frame->console ? "valid" : "null") << "eol:" << eol << "eof:" << eof;
#endif

    mCurrentDestination = frameName;

    auto* sink = currentDestinationSink();

    if (sink) {
        if (eof) {
            sink->discardAll();
        } else if (eol) {
            sink->discardLastLine();
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

TPrintSink* TMxpFrameManager::currentDestinationSink() const
{
    if (mCurrentDestination.isEmpty()) {
        return nullptr;
    }

    const auto* frame = getFrame(mCurrentDestination);
    if (!frame) {
        return nullptr;
    }

    TConsole* console = frame->console.data();
    if (!console || console == mpHost->mpConsole) {
        return nullptr;
    }

    return console;
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

QRect TMxpFrameManager::availableFrameArea() const
{
    if (!mpHost || mpHost->mpConsole.isNull()) {
        return {};
    }

    // getMainWindowSize() rather than mpMainFrame's own geometry, which
    // TConsole::resizeEvent() sets to the full console size until the layout
    // corrects it. It is also the size Lua scripts lay themselves out against,
    // so taking the user borders off it keeps frames out of the space a package
    // such as the base UI has reserved with setBorderRight() and friends.
    QRect area = QRect(QPoint(0, 0), mpHost->mpConsole->getMainWindowSize()).marginsRemoved(mpHost->userBorders());
    if (area.width() < 0) {
        area.setWidth(0);
    }
    if (area.height() < 0) {
        area.setHeight(0);
    }
    return area;
}

// Works out where a frame goes. An edge aligned top level frame consumes the
// space it takes from mMxpBorders and a nested one advances its parent's
// usedHeight; an absolutely positioned frame consumes neither. Callers are
// responsible for pushing the updated borders to the Host.
QRect TMxpFrameManager::calculateFrameGeometry(TMxpFrame* frame, TMxpFrame* parentFrame)
{
    // Determine the container for this frame
    QSize containerSize;
    int containerX = 0;
    int containerY = 0;
    QRect area;

    if (parentFrame && parentFrame->widget) {
        // Nested frame - position relative to parent
        containerSize = parentFrame->widget->size();
        containerX = parentFrame->widget->x();
        containerY = parentFrame->widget->y();

        // Track used space in parent for VBox-style stacking
        containerY += parentFrame->usedHeight;
        containerSize.setHeight(containerSize.height() - parentFrame->usedHeight);
    } else {
        // A parent with no widget of its own gives nothing to place against, so
        // such a frame is placed as a top level one for the rest of this
        // calculation - frame->parentFrame still records the hierarchy
        parentFrame = nullptr;
        // MXP borders stack inwards from the area the user borders leave.
        // userBorders() and not borders(), which already carries the MXP borders
        // this function is in the middle of recomputing.
        area = availableFrameArea();
        containerX = area.x() + mMxpBorders.left();
        containerY = area.y() + mMxpBorders.top();
        containerSize = QSize(area.width() - mMxpBorders.left() - mMxpBorders.right(), area.height() - mMxpBorders.top() - mMxpBorders.bottom());
    }

    // Calculate frame dimensions relative to container
    QSize widthSize = calculateFrameSize(frame->width, containerSize, false);
    QSize heightSize = calculateFrameSize(frame->height, containerSize, true);
    int frameWidth = widthSize.width();
    int frameHeight = heightSize.height();

    // For "100%" height in a nested context, use remaining space
    if (frame->height.trimmed() == qsl("100%") && parentFrame) {
        frameHeight = containerSize.height();
    }

    // Ensure minimum size for visibility
    if (frameWidth < 50) {
        frameWidth = 100;
    }

    // For character-based height specs, handle minimum size more carefully
    const bool isCharacterHeight = frame->height.trimmed().endsWith('c', Qt::CaseInsensitive);
    const bool isCharacterWidth = frame->width.trimmed().endsWith('c', Qt::CaseInsensitive);
    const bool willHaveTitle = !frame->floating && frame->hasExplicitTitle;

    // Apply minimum size for visibility to non-character-based frames
    if (frameHeight < 20 && !isCharacterHeight) {
        frameHeight = 50;
    }

    // For character-based frames, ensure adequate space regardless of title
    if (isCharacterHeight) {
        int minFrameSize;
        if (willHaveTitle) {
            // Has explicit title: minimum = header (24px) + content space (30px)
            minFrameSize = 24 + 30;
        } else {
            // No title: just ensure adequate content space (30px minimum)
            minFrameSize = 30;
        }

        if (frameHeight < minFrameSize) {
            frameHeight = minFrameSize;
        }

        // IMPORTANT: If this frame will have a tab header, add extra height
        // to the character-based calculation so the final console area (after
        // subtracting tab widget overhead) matches the requested character count
        if (willHaveTitle) {
            // Tab widget overhead includes: tab bar (24px) + content margins/padding (~6px)
            frameHeight += 30; // More accurate tab widget overhead
        }
    }

    // Add small padding compensation for character-based frames to ensure full character visibility
    // This accounts for any internal widget padding or text rendering margins
    if (isCharacterHeight || isCharacterWidth) {
        if (isCharacterWidth) {
            frameWidth += 4; // Add 4px horizontal padding for character visibility
        }
        if (isCharacterHeight) {
            frameHeight += 4; // Add 4px vertical padding for character visibility
        }
    }

    // Calculate position based on alignment
    int x = containerX;
    int y = containerY;
    const QString align = frame->align.toLower();

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

        return {x, y, frameWidth, frameHeight};
    }

    // Top-level frame - position at the edges of the available area and update MXP borders

    // Check for LEFT/TOP absolute positioning first - these take precedence over alignment
    const bool hasAbsolutePosition = !frame->left.isEmpty() || !frame->top.isEmpty();

    if (hasAbsolutePosition) {
        // Absolute positioning via LEFT/TOP attributes
        if (!frame->left.isEmpty()) {
            QSize leftSize = calculateFrameSize(frame->left, area.size(), false);
            if (leftSize.width() > 0) {
                x = area.x() + leftSize.width();
            }
        }
        if (!frame->top.isEmpty()) {
            QSize topSize = calculateFrameSize(frame->top, area.size(), true);
            if (topSize.height() > 0) {
                y = area.y() + topSize.height();
            }
        }
        // Absolute positioned frames don't modify MXP borders
    } else if (align == qsl("left")) {
        // Left-aligned: position at actual left edge (after existing left MXP frames)
        x = area.x() + mMxpBorders.left();
        y = area.y();
        frameHeight = area.height();
        // Update MXP left border
        mMxpBorders.setLeft(mMxpBorders.left() + frameWidth);
    } else if (align == qsl("right")) {
        // Right-aligned: position at right edge
        x = area.x() + area.width() - mMxpBorders.right() - frameWidth;
        y = area.y();
        frameHeight = area.height();
        mMxpBorders.setRight(mMxpBorders.right() + frameWidth);
    } else if (align == qsl("top")) {
        // Top-aligned: position at top edge
        x = area.x() + mMxpBorders.left();
        y = area.y() + mMxpBorders.top();
        frameWidth = area.width() - mMxpBorders.left() - mMxpBorders.right();
        mMxpBorders.setTop(mMxpBorders.top() + frameHeight);
    } else if (align == qsl("bottom")) {
        // Bottom-aligned: position at bottom edge
        x = area.x() + mMxpBorders.left();
        y = area.y() + area.height() - mMxpBorders.bottom() - frameHeight;
        frameWidth = area.width() - mMxpBorders.left() - mMxpBorders.right();
        mMxpBorders.setBottom(mMxpBorders.bottom() + frameHeight);
    }

    return {x, y, frameWidth, frameHeight};
}

void TMxpFrameManager::layoutInternalFrame(TMxpFrame* frame)
{
    if (!mpHost || !mpHost->mpConsole) {
        qWarning() << "TMxpFrameManager::layoutInternalFrame: No console available";
        return;
    }

    TMainConsole* mainConsole = mpHost->mpConsole.data();

    // Note: DOCK tabbing is handled in createFrame() when ALIGN=CLIENT is set.
    // Per CMUD, ALIGN=CLIENT + DOCK creates tabbed frames.
    // This is a CMUD extension, not part of the official MXP 1.0 specification.

    // Check if we're inside a DEST - if so, nest this frame inside the destination
    TMxpFrame* parentFrame = nullptr;
    if (!mCurrentDestination.isEmpty()) {
        parentFrame = getFrame(mCurrentDestination);
    }

    const QRect geometry = calculateFrameGeometry(frame, parentFrame);
    const int x = geometry.x();
    const int y = geometry.y();
    const int frameWidth = geometry.width();
    const int frameHeight = geometry.height();

    // A nested frame never touches mMxpBorders, so this hands Host the margins
    // it already has and setBorders() drops it
    mpHost->setMxpBorders(mMxpBorders);

    const bool isCharacterHeight = frame->height.trimmed().endsWith('c', Qt::CaseInsensitive);
    const bool willHaveTitle = !frame->floating && frame->hasExplicitTitle;

    // FLOATING attribute, no explicit title, or very small height = borderless frame without header
    // Exception: character-based frames with explicit titles always show headers
    bool showHeader = !frame->floating && frame->hasExplicitTitle && (frameHeight >= 50 || (isCharacterHeight && willHaveTitle));
    const int tabBarHeight = showHeader ? 30 : 0; // Tab widget overhead including margins

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameManager::layoutInternalFrame: Creating frame" << frame->name << "at" << x << y << "size" << frameWidth << "x" << frameHeight << "showHeader:" << showHeader;
#endif

    // Create the container widget for the frame - use WA_DontShowOnScreen to prevent any rendering
    auto* containerWidget = new QFrame(mainConsole->mpMainFrame);
    containerWidget->setAttribute(Qt::WA_DontShowOnScreen, true);
    containerWidget->setObjectName(frame->name + qsl("_container"));
    containerWidget->setGeometry(x, y, frameWidth, frameHeight);

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
        tabWidget->setObjectName(frame->name + qsl("_tabs"));
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

        // Create console directly with tabPage as parent to avoid flash on mpMainFrame
        // (createMiniConsole parents to mpMainFrame and calls show() before we can intervene)
        console = new TConsole(mpHost, frame->name, TConsole::SubConsole, tabPage);

        // Setup console properties (mirroring what createMiniConsole does)
        console->setObjectName(frame->name);
        const auto& hostCommandLine = mpHost->mpConsole->mpCommandLine;
        console->setFocusProxy(hostCommandLine);
        console->mUpperPane->setFocusProxy(hostCommandLine);
        console->mLowerPane->setFocusProxy(hostCommandLine);
        console->resize(frameWidth, frameHeight - tabBarHeight);
        console->mOldX = 0;
        console->mOldY = 0;
        console->setContentsMargins(0, 0, 0, 0);
        int fontSize = mpHost->getDisplayFont().pointSize();
        console->setFontSize(fontSize > 0 ? fontSize : 12);
        console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        // Don't show yet - wait until frame is fully set up
        console->hide();

        tabPageLayout->addWidget(console);
        int tabIndex = tabWidget->addTab(tabPage, frame->title);
        tabWidget->setCurrentIndex(tabIndex); // Make this tab active
        containerLayout->addWidget(tabWidget);
    } else {
        // Floating/borderless: console directly in container, no tab header
        // Create console directly with containerWidget as parent to avoid flash
        console = new TConsole(mpHost, frame->name, TConsole::SubConsole, containerWidget);

        // Setup console properties (mirroring what createMiniConsole does)
        console->setObjectName(frame->name);
        const auto& hostCommandLine = mpHost->mpConsole->mpCommandLine;
        console->setFocusProxy(hostCommandLine);
        console->mUpperPane->setFocusProxy(hostCommandLine);
        console->mLowerPane->setFocusProxy(hostCommandLine);
        console->resize(frameWidth, frameHeight);
        console->mOldX = 0;
        console->mOldY = 0;
        console->setContentsMargins(0, 0, 0, 0);
        int fontSize = mpHost->getDisplayFont().pointSize();
        console->setFontSize(fontSize > 0 ? fontSize : 12);
        console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        console->hide();

        containerLayout->addWidget(console);
    }

    // Store the container, TabWidget (may be null), and console
    frame->widget = containerWidget;
    frame->tabWidget = tabWidget;
    frame->console = console;
    frame->dockWidget = nullptr;
    frame->usedHeight = 0;

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
    frameBgColor = frameBgColor.lighter(115); // 15% lighter than main console
    console->setBgColor(frameBgColor);

    // Only add border for borderless/floating frames (no tab header)
    // Tabbed frames already have visual separation from the tab widget
    if (!showHeader) {
        console->setStyleSheet(qsl("QWidget { border: 1px solid #444444; }"));
    }

    // Register console
    mainConsole->registerSubConsole(frame->name, console);

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
    auto* console = mpHost->mpConsole->createMiniConsole(qsl("main"), frame->name, 0, 0, frameWidth, frameHeight);

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
    QSize frameSize = calculateFrameSize(frame->width, tabSize, false) + calculateFrameSize(frame->height, tabSize, true);

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameManager::layoutTabFrame: Adding tab" << frame->name << "to parent" << frame->dockFrame << "size" << frameSize;
#endif

    // Create a page widget to hold the console (avoids flash on mpMainFrame)
    auto* tabPage = new QWidget();
    tabPage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* tabPageLayout = new QVBoxLayout(tabPage);
    tabPageLayout->setContentsMargins(0, 0, 0, 0);

    // Create console directly with tabPage as parent
    auto* console = new TConsole(mpHost, frame->name, TConsole::SubConsole, tabPage);

    // Setup console properties
    TMainConsole* mainConsole = mpHost->mpConsole;
    console->setObjectName(frame->name);
    const auto& hostCommandLine = mainConsole->mpCommandLine;
    console->setFocusProxy(hostCommandLine);
    console->mUpperPane->setFocusProxy(hostCommandLine);
    console->mLowerPane->setFocusProxy(hostCommandLine);
    console->resize(frameSize.width(), frameSize.height());
    console->mOldX = 0;
    console->mOldY = 0;
    console->setContentsMargins(0, 0, 0, 0);
    int fontSize = mpHost->getDisplayFont().pointSize();
    console->setFontSize(fontSize > 0 ? fontSize : 12);
    console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    tabPageLayout->addWidget(console);

    frame->widget = tabPage;
    frame->console = console;
    frame->parentFrame = parentFrame;
    parentFrame->childFrames.append(frame);

    // Configure scrolling
    if (!frame->scrolling) {
        console->setScrolling(false);
    }

    // Register console in map
    mainConsole->registerSubConsole(frame->name, console);

    // Add as new tab - if this is the first child tab, select it
    // (The parent frame's own tab at index 0 is typically unused for content)
    int newTabIndex = parentFrame->tabWidget->addTab(tabPage, frame->title);
    if (parentFrame->childFrames.size() == 1) {
        // First child tab - select it instead of the parent's empty tab
        parentFrame->tabWidget->setCurrentIndex(newTabIndex);
    }
    console->show();
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
            // Use height() instead of lineSpacing() to match Host::calcFontSize()
            // and avoid extra line spacing that reduces actual character count
            int result = chars * fm.height();
            return QSize(0, result);
        }
        // Use horizontalAdvance('W') instead of averageCharWidth() for consistency
        // with Host::calcFontSize() which uses this for more accurate character width
        int result = chars * fm.horizontalAdvance(QChar('W'));
        return QSize(result, 0);
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
    } else if (lower == qsl("bottom")) { // NOLINT(readability-else-after-return)
        return Qt::BottomDockWidgetArea;
    } else if (lower == qsl("right")) { // NOLINT(readability-else-after-return)
        return Qt::RightDockWidgetArea;
    } else { // NOLINT(readability-else-after-return)
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
    if (!frame || frame->mBeingDestroyed) {
        return;
    }

    // Remove from parent's child list (if parent is valid)
    if (frame->parentFrame && !frame->parentFrame->mBeingDestroyed) {
        frame->parentFrame->childFrames.removeOne(frame);
    }
    frame->parentFrame = nullptr;

    // Orphan children (set their parentFrame to nullptr)
    for (TMxpFrame* child : std::as_const(frame->childFrames)) {
        if (child && !child->mBeingDestroyed) {
            child->parentFrame = nullptr;
        }
    }
    frame->childFrames.clear();
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

    auto* tabPage = new QWidget();
    auto* tabPageLayout = new QVBoxLayout(tabPage);
    tabPageLayout->setContentsMargins(0, 0, 0, 0);

    // Get size from target frame's container
    QSize containerSize = targetFrame->widget ? targetFrame->widget->size() : QSize(200, 200);

    // Create mini console for this tab
    auto* console = mainConsole->createMiniConsole(qsl("main"),
                                                   frame->name,
                                                   0,
                                                   0, // Position managed by layout
                                                   containerSize.width(),
                                                   containerSize.height() - 30); // Account for tab bar

    if (!console) {
        qWarning() << "TMxpFrameManager::layoutTabIntoExistingFrame: Failed to create console for" << frame->name;
        delete tabPage;
        return;
    }

    // Ensure console expands to fill available space in the tab
    console->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Add console to tab page layout
    tabPageLayout->addWidget(console);

    // Add the new tab
    int tabIndex = tabWidget->addTab(tabPage, frame->title);

    // Store references
    frame->widget = tabPage;
    frame->tabWidget = tabWidget; // Share the TabWidget reference
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
    mainConsole->registerSubConsole(frame->name, console);

    // Optionally switch to the new tab
    tabWidget->setCurrentIndex(tabIndex);

    console->show();
}

void TMxpFrameManager::scheduleRelayout()
{
    if (mRelayoutPending || mFrames.isEmpty()) {
        return;
    }

    // Deferred so that the layout of the widgets frames are placed against has
    // settled, and so that pushing new borders from here cannot re-enter the
    // resize handling that asked for the relayout. The push at the end of a
    // relayout does schedule one more pass, which then finds the same borders
    // and stops there because Host::setBorders() ignores an unchanged value.
    mRelayoutPending = true;
    QTimer::singleShot(0, mpHost, [this]() {
        mRelayoutPending = false;
        relayoutFrames();
    });
}

void TMxpFrameManager::relayoutFrames()
{
    if (!mpHost) {
        return;
    }

    // calculateFrameGeometry() accumulates into these, so they have to start
    // empty or every pass would count the same frames again
    mMxpBorders = QMargins();

    if (mpHost->mpConsole.isNull() || !mpHost->mpConsole->mpMainFrame) {
        mpHost->setMxpBorders(mMxpBorders);
        return;
    }

    const QWidget* mainFrame = mpHost->mpConsole->mpMainFrame;

    // calculateFrameGeometry() also accumulates into a parent's usedHeight, so
    // without this nested frames would march further down on every pass
    for (auto* frame : std::as_const(mFrameOrder)) {
        frame->usedHeight = 0;
    }

    for (auto* frame : std::as_const(mFrameOrder)) {
        // Skip a frame whose layout never produced a widget, one docked as a tab
        // (the QTabWidget places it), and one in a window of its own. An external
        // frame keeps mpMainFrame as its parent even after Qt::Window is set on
        // it, so isWindow() rather than the parent is what tells them apart.
        if (!frame->widget || frame->widget->isWindow() || frame->widget->parentWidget() != mainFrame) {
            continue;
        }

        frame->widget->setGeometry(calculateFrameGeometry(frame, frame->parentFrame));
    }

    mpHost->setMxpBorders(mMxpBorders);
}
