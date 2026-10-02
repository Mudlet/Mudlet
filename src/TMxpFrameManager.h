#ifndef MUDLET_TMXPFRAMEMANAGER_H
#define MUDLET_TMXPFRAMEMANAGER_H

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

#include "utils.h"

#include <QList>
#include <QMap>
#include <QMargins>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <optional>

class Host;
class TMxpFrameWidgets;
class TPrintSink;

/**
 * @brief Represents a single MXP frame for multi-window layouts.
 * 
 * Ownership Model:
 * - TMxpFrameManager owns all TMxpFrame instances via mFrames map (flat ownership)
 * - parentFrame/childFrames are NON-OWNING references for hierarchy tracking only
 * - TMxpFrameManager is responsible for deleting all frames; TMxpFrame destructor
 *   does NOT delete children (to avoid double-deletion)
 * - During destruction, frames remove themselves from parent's childFrames list
 *   and orphan their children (set child->parentFrame = nullptr)
 * - The frame's widgets are the view's, kept under the frame's name by
 *   TMxpFrameWidgets
 */
struct TMxpFrame {
    QString name;
    QString title;
    bool hasExplicitTitle = false; // True if TITLE attribute was explicitly provided
    bool isInternal = true;
    QString align;          // "top", "bottom", "left", "right", "client"
    QString width;          // "40c", "25%", "350px"
    QString height;
    QString left;           // Absolute X position (pixels or percentage)
    QString top;            // Absolute Y position (pixels or percentage)
    bool scrolling = true;
    bool floating = false;  // When true, frame has no title bar/header (borderless)
    QString dockFrame;      // For tab support - name of frame to dock into

    // How the view shows the frame, once it does
    enum class Shown {
        Not,
        // On the main window, or inside the Tab or Window frame named by
        // hostFrame; relayouts move it
        Placed,
        // A page of its parent's header, which places it
        Tab,
        // A window of its own, which the player places
        Window,
    };
    Shown shown = Shown::Not;
    QString hostFrame;
    // Where a Placed frame was last put, or the inside of a Window as the view
    // last reported it, which frames nested in it are placed against. A Tab's
    // inside is its header's tabArea.
    QRect geometry;
    // The space every page of this frame's tab header gets, as the view last
    // reported it; nothing for a frame without a header
    std::optional<QSize> tabArea;
    
    // Hierarchy tracking (non-owning references - see ownership model above)
    TMxpFrame* parentFrame = nullptr;
    QList<TMxpFrame*> childFrames;
    
    // Destruction state flag - set true when destructor begins to prevent
    // re-entrant access from children/parents during cleanup
    bool mBeingDestroyed = false;
    
    // VBox-style layout tracking
    int usedHeight = 0;     // Pixels used by child frames (for VBox stacking)
    int usedWidth = 0;      // Pixels used by child frames (for HBox stacking)
    
    ~TMxpFrame();
};

class TMxpFrameManager
{
public:
    explicit TMxpFrameManager(Host* host);
    ~TMxpFrameManager();
    
    // Frame lifecycle operations
    bool createFrame(const QString& name, const QMap<QString, QString>& attributes);
    bool closeFrame(const QString& name);
    bool focusFrame(const QString& name);
    bool showFrame(const QString& name);
    void resetAllFrames();
    
    // Output destination management
    void setDestination(const QString& frameName, bool eol, bool eof);
    void clearDestination();
    QString getCurrentDestination() const { return mCurrentDestination; }
    // Write-only so buffer translation never gets a view pointer back. Null when no destination
    // is set, its frame has gone, or it resolves to the main console (which would recurse).
    TPrintSink* currentDestinationSink() const;
    bool hasActiveDestination() const { return !mCurrentDestination.isEmpty(); }
    
    // Frame queries
    TMxpFrame* getFrame(const QString& name);
    const TMxpFrame* getFrame(const QString& name) const;
    QStringList getFrameNames() const;
    bool frameExists(const QString& name) const { return mFrames.contains(name); }
    int frameCount() const { return mFrames.size(); }

    // What the view measures of the main console after each resize, and while
    // a tab switch hides it, whenever the window it will come back to changes:
    // mainWindowSize is getMainWindowSize()'s, the space frames on the main
    // window are laid out in, and consoleSize the console's own, which EXTERNAL
    // frames are sized against. Frames already open stay where they are until
    // relayoutFrames().
    void setMainConsoleSize(const QSize& mainWindowSize, const QSize& consoleSize);
    // Reposition every frame on the main window against the last reported size
    // and the current borders
    void relayoutFrames();
    // What the view measures of an EXTERNAL frame's window whenever the player
    // or a script resizes it; the frames open inside it are placed again
    void setWindowSize(const QString& name, const QSize& size);
    // What the view measures of the pages of name's tab header whenever they
    // are resized. True when frames nested in its tabs have to be placed again,
    // which is left to the caller as this is reported from inside a relayout.
    bool setTabAreaSize(const QString& name, const QSize& size);

    // Configuration
    static constexpr int MAX_FRAMES = 20;
    
private:
    Host* mpHost;
    QMap<QString, TMxpFrame*> mFrames;
    // Frames in creation order: borders accumulate inwards, so the order frames
    // were opened in decides where each one sits
    QList<TMxpFrame*> mFrameOrder;
    QString mCurrentDestination;  // Current output target (empty = main console)
    QMargins mMxpBorders;         // MXP-specific borders, separate from Host::mBorders
    QSize mMainWindowSize;
    QSize mMainConsoleSize;

    // Layout and sizing helpers
    void layoutInternalFrame(TMxpFrame* frame);
    void layoutExternalFrame(TMxpFrame* frame);
    void layoutTabFrame(TMxpFrame* frame);
    QRect availableFrameArea() const;
    QRect calculateFrameGeometry(TMxpFrame* frame, TMxpFrame* parentFrame);
    // Nothing while the view shows nothing the frame could hold
    std::optional<QRect> nestingArea(const TMxpFrame& frame) const;
    QSize calculateFrameSize(const QString& spec, const QSize& containerSize, bool isHeight);
    // Null while the profile has no main console
    TMxpFrameWidgets* frameWidgets();
    const TMxpFrameWidgets* frameWidgets() const;

    // Validation
    bool validateFrameName(const QString& name) const;
    bool canCreateFrame() const;

    // Cleanup
    void removeFrameFromHierarchy(TMxpFrame* frame);
};

#endif // MUDLET_TMXPFRAMEMANAGER_H
