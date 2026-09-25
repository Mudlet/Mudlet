#ifndef MUDLET_TMXPFRAMEWIDGETS_H
#define MUDLET_TMXPFRAMEWIDGETS_H

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

#include <QMap>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <optional>

class QTabWidget;
class QWidget;
class TConsole;
class TMainConsole;
class TPrintSink;

// The widgets of a profile's MXP frames, kept by frame name. TMxpFrameManager
// decides where each frame goes and how frames nest; this builds, shows and
// removes what the player sees of them.
class TMxpFrameWidgets
{
public:
    explicit TMxpFrameWidgets(TMainConsole* pMainConsole);

    // A frame on the main window, with its title on a tab header when showHeader
    void createInternalFrame(const QString& name, const QString& title, const QRect& geometry, bool showHeader, bool scrolling);
    // A frame in a window of its own; false when no console could be made for it
    bool createExternalFrame(const QString& name, const QString& title, const QSize& size, bool scrolling);
    // The space a tab added to parentName's header has, or nothing when that
    // frame has no header to add one to
    std::optional<QSize> tabAreaSize(const QString& parentName) const;
    void createTabFrame(const QString& name, const QString& title, const QString& parentName, const QSize& size, bool scrolling, bool select);
    // False, leaving everything alone, when name is not a tab in parentName's header
    bool removeFromParentTabs(const QString& name, const QString& parentName);
    // Also deregisters the name's sub-console and dock whether or not this
    // built them, as the name is the frame's to give up either way
    void destroyFrame(const QString& name);
    void showFrame(const QString& name);
    void focusFrame(const QString& name);

    // What frames nested inside this one are placed against
    std::optional<QRect> placementArea(const QString& name) const;
    // Only these are placed by a relayout: a tab is placed by its header, an
    // external frame by the player
    bool placedOnMainWindow(const QString& name) const;
    void setGeometry(const QString& name, const QRect& geometry);
    QSize mainConsoleSize() const;
    // Null for a frame without a console, and for one that would print back
    // into the main console
    TPrintSink* sink(const QString& name) const;

    // For callers that need the widgets themselves
    QWidget* frameWidget(const QString& name) const;
    TConsole* frameConsole(const QString& name) const;
    QTabWidget* frameTabs(const QString& name) const;

private:
    struct Widgets
    {
        // The container: a QFrame, a tab page, or the console itself for an
        // external frame
        QPointer<QWidget> widget;
        QPointer<TConsole> console;
        // The header of a titled frame, which tab frames are added to
        QPointer<QTabWidget> tabWidget;
    };

    TMainConsole* mpMainConsole;
    QMap<QString, Widgets> mFrames;
};

#endif // MUDLET_TMXPFRAMEWIDGETS_H
