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
#include "TMainConsole.h"
#include "TMxpFrameWidgets.h"

#include <QDebug>
#include <QFontMetrics>
#include <QTimer>
#include <optional>

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
    if (frame->parentFrame) {
        TMxpFrameWidgets* widgets = frameWidgets();
        if (widgets && widgets->removeFromParentTabs(name, frame->parentFrame->name)) {
            // Remove from hierarchy
            removeFrameFromHierarchy(frame);

            // Remove from frames map and delete
            mFrames.remove(name);
            mFrameOrder.removeOne(frame);
            // before the delete, as name may be the frame's own
            widgets->destroyFrame(name);
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

    removeFrameFromHierarchy(frame);

    mFrames.remove(name);
    mFrameOrder.removeOne(frame);
    if (auto* widgets = frameWidgets()) {
        widgets->destroyFrame(name);
    }
    delete frame;

    // Reposition what is left so it reclaims the space the frame gave up
    relayoutFrames();

    return true;
}

bool TMxpFrameManager::focusFrame(const QString& name)
{
    if (!frameExists(name)) {
        return false;
    }

    if (auto* widgets = frameWidgets()) {
        widgets->focusFrame(name);
    }

    return true;
}

bool TMxpFrameManager::showFrame(const QString& name)
{
    if (!frameExists(name)) {
        return false;
    }

    if (auto* widgets = frameWidgets()) {
        widgets->showFrame(name);
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
    qDebug() << "TMxpFrameManager::setDestination: Setting destination to" << frameName << "eol:" << eol << "eof:" << eof;
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

TPrintSink* TMxpFrameManager::currentDestinationSink() const
{
    if (mCurrentDestination.isEmpty()) {
        return nullptr;
    }

    if (!frameExists(mCurrentDestination)) {
        return nullptr;
    }

    const auto* widgets = frameWidgets();
    return widgets ? widgets->sink(mCurrentDestination) : nullptr;
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
    std::optional<QRect> parentArea;
    if (parentFrame) {
        if (const auto* widgets = frameWidgets()) {
            parentArea = widgets->placementArea(parentFrame->name);
        }
    }

    if (parentArea) {
        // Nested frame - position relative to parent
        containerSize = parentArea->size();
        containerX = parentArea->x();
        containerY = parentArea->y();

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
    auto* widgets = frameWidgets();
    if (!widgets) {
        qWarning() << "TMxpFrameManager::layoutInternalFrame: No console available";
        return;
    }

    // Note: DOCK tabbing is handled in createFrame() when ALIGN=CLIENT is set.
    // Per CMUD, ALIGN=CLIENT + DOCK creates tabbed frames.
    // This is a CMUD extension, not part of the official MXP 1.0 specification.

    // Check if we're inside a DEST - if so, nest this frame inside the destination
    TMxpFrame* parentFrame = nullptr;
    if (!mCurrentDestination.isEmpty()) {
        parentFrame = getFrame(mCurrentDestination);
    }

    const QRect geometry = calculateFrameGeometry(frame, parentFrame);
    const int frameHeight = geometry.height();

    // A nested frame never touches mMxpBorders, so this hands Host the margins
    // it already has and setBorders() drops it
    mpHost->setMxpBorders(mMxpBorders);

    const bool isCharacterHeight = frame->height.trimmed().endsWith('c', Qt::CaseInsensitive);
    const bool willHaveTitle = !frame->floating && frame->hasExplicitTitle;

    // FLOATING attribute, no explicit title, or very small height = borderless frame without header
    // Exception: character-based frames with explicit titles always show headers
    bool showHeader = !frame->floating && frame->hasExplicitTitle && (frameHeight >= 50 || (isCharacterHeight && willHaveTitle));

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameManager::layoutInternalFrame: Creating frame" << frame->name << "at" << geometry << "showHeader:" << showHeader;
#endif

    frame->usedHeight = 0;

    // Track parent-child relationship
    if (parentFrame) {
        frame->parentFrame = parentFrame;
        parentFrame->childFrames.append(frame);
    }

    widgets->createInternalFrame(frame->name, frame->title, geometry, showHeader, frame->scrolling);
}

void TMxpFrameManager::layoutExternalFrame(TMxpFrame* frame)
{
    auto* widgets = frameWidgets();
    if (!widgets) {
        qWarning() << "TMxpFrameManager::layoutExternalFrame: No console available";
        return;
    }

    // Calculate size
    QSize mainSize = widgets->mainConsoleSize();
    QSize widthSize = calculateFrameSize(frame->width, mainSize, false);
    QSize heightSize = calculateFrameSize(frame->height, mainSize, true);
    int frameWidth = widthSize.width();
    int frameHeight = heightSize.height();

    if (!widgets->createExternalFrame(frame->name, frame->title, QSize(frameWidth, frameHeight), frame->scrolling)) {
        qWarning() << "TMxpFrameManager::layoutExternalFrame: Failed to create console";
    }
}

void TMxpFrameManager::layoutTabFrame(TMxpFrame* frame)
{
    auto* widgets = frameWidgets();
    if (!widgets) {
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

    const std::optional<QSize> tabSize = widgets->tabAreaSize(parentFrame->name);
    if (!tabSize) {
        qWarning() << "TMxpFrameManager::layoutTabFrame: Failed to create tab widget";
        layoutInternalFrame(frame);
        return;
    }

    // Calculate size
    QSize frameSize = calculateFrameSize(frame->width, *tabSize, false) + calculateFrameSize(frame->height, *tabSize, true);

#ifdef DEBUG_MXP_PROCESSING
    qDebug() << "TMxpFrameManager::layoutTabFrame: Adding tab" << frame->name << "to parent" << frame->dockFrame << "size" << frameSize;
#endif

    frame->parentFrame = parentFrame;
    parentFrame->childFrames.append(frame);

    // If this is the first child tab, select it
    // (The parent frame's own tab at index 0 is typically unused for content)
    widgets->createTabFrame(frame->name, frame->title, parentFrame->name, frameSize, frame->scrolling, parentFrame->childFrames.size() == 1);
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

    auto* widgets = frameWidgets();
    if (!widgets) {
        mpHost->setMxpBorders(mMxpBorders);
        return;
    }

    // calculateFrameGeometry() also accumulates into a parent's usedHeight, so
    // without this nested frames would march further down on every pass
    for (auto* frame : std::as_const(mFrameOrder)) {
        frame->usedHeight = 0;
    }

    for (auto* frame : std::as_const(mFrameOrder)) {
        // Skip a frame whose layout never produced a widget, one docked as a tab
        // and one in a window of its own
        if (!widgets->placedOnMainWindow(frame->name)) {
            continue;
        }

        widgets->setGeometry(frame->name, calculateFrameGeometry(frame, frame->parentFrame));
    }

    mpHost->setMxpBorders(mMxpBorders);
}

TMxpFrameWidgets* TMxpFrameManager::frameWidgets() const
{
    if (!mpHost || !mpHost->mpConsole) {
        return nullptr;
    }
    return &mpHost->mpConsole->mxpFrameWidgets();
}
