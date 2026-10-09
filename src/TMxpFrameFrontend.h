#ifndef MUDLET_TMXPFRAMEFRONTEND_H
#define MUDLET_TMXPFRAMEFRONTEND_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QSize>

#include <optional>

class QRect;
class QString;
class TPrintSink;

// What TMxpFrameManager asks of the widgets of a profile's MXP frames, kept by frame name. The
// main console's TMxpFrameWidgets implements it; core code reaches it through
// TConsoleFrontend::mxpFrames().
class TMxpFrameFrontend
{
public:
    // A frame on the main window, or inside hostName's frame when that is set,
    // with its title on a tab header when showHeader
    virtual void createInternalFrame(const QString& name, const QString& hostName, const QString& title, const QRect& geometry, bool showHeader, bool scrolling) = 0;
    // A frame in a window of its own, whose resizes are reported to the frame
    // manager from then on. The size the window is shown at, or nothing when no
    // console could be made for it.
    virtual std::optional<QSize> createExternalFrame(const QString& name, const QString& title, const QSize& size, bool scrolling) = 0;
    virtual void createTabFrame(const QString& name, const QString& title, const QString& parentName, const QSize& size, bool scrolling, bool select) = 0;
    // False, leaving everything alone, when name is not a tab in parentName's header
    virtual bool removeFromParentTabs(const QString& name, const QString& parentName) = 0;
    // Also deregisters the name's sub-console and dock whether or not this
    // built them, as the name is the frame's to give up either way
    virtual void destroyFrame(const QString& name) = 0;
    virtual void showFrame(const QString& name) = 0;
    virtual void focusFrame(const QString& name) = 0;
    virtual void setGeometry(const QString& name, const QRect& geometry) = 0;
    // Tells the frame manager the main console's size now
    virtual void reportSize() = 0;
    // Null for a frame without a console, and for one that would print back
    // into the main console
    virtual TPrintSink* sink(const QString& name) const = 0;
    // False once the frame's widget has gone, which deleteMiniConsole() can do without the frame closing
    virtual bool hasFrameWidget(const QString& name) const = 0;

protected:
    // Owned and deleted by the main console as the concrete type.
    ~TMxpFrameFrontend() = default;
};

#endif // MUDLET_TMXPFRAMEFRONTEND_H
