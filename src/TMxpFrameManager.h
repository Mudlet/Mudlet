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

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

class Host;
class TConsole;
class TDockWidget;

struct TMxpFrame {
    QString name;
    QString title;
    bool isInternal = true;
    QString align;          // "top", "bottom", "left", "right", "client"
    QString width;          // "40c", "25%", "350px"
    QString height;
    bool scrolling = true;
    QString dockFrame;      // For tab support - name of frame to dock into
    
    // UI elements - using QPointer for automatic null on deletion
    QPointer<QWidget> widget;           // The container widget (QFrame with title bar)
    QPointer<TConsole> console;         // The actual TConsole for text output
    QPointer<TDockWidget> dockWidget;   // Container for internal frames
    QPointer<QTabWidget> tabWidget;     // For tab-based frames
    
    // Hierarchy tracking
    TMxpFrame* parentFrame = nullptr;
    QList<TMxpFrame*> childFrames;
    
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
    
    // Output destination management
    void setDestination(const QString& frameName, bool eol, bool eof);
    void clearDestination();
    QString getCurrentDestination() const { return mCurrentDestination; }
    QWidget* getCurrentDestinationWidget() const;
    TConsole* getCurrentDestinationConsole() const;
    bool hasActiveDestination() const { return !mCurrentDestination.isEmpty(); }
    
    // Frame queries
    TMxpFrame* getFrame(const QString& name);
    const TMxpFrame* getFrame(const QString& name) const;
    QStringList getFrameNames() const;
    bool frameExists(const QString& name) const { return mFrames.contains(name); }
    int frameCount() const { return mFrames.size(); }
    
    // Configuration
    static constexpr int MAX_FRAMES = 20;
    
private:
    Host* mpHost;
    QMap<QString, TMxpFrame*> mFrames;
    QString mCurrentDestination;  // Current output target (empty = main console)
    QMargins mMxpBorders;         // MXP-specific borders, separate from Host::mBorders
    
    // Layout and sizing helpers
    void layoutInternalFrame(TMxpFrame* frame);
    void layoutExternalFrame(TMxpFrame* frame);
    void layoutTabFrame(TMxpFrame* frame);
    void layoutTabIntoExistingFrame(TMxpFrame* frame, TMxpFrame* targetFrame);
    QSize calculateFrameSize(const QString& spec, const QSize& containerSize, bool isHeight);
    Qt::DockWidgetArea alignmentToDockArea(const QString& align);
    
    // Validation
    bool validateFrameName(const QString& name) const;
    bool canCreateFrame() const;
    
    // Cleanup
    void removeFrameFromHierarchy(TMxpFrame* frame);
};

#endif // MUDLET_TMXPFRAMEMANAGER_H
