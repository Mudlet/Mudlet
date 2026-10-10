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

#include "TMxpFrameFrontend.h"

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
class TMxpFrameWidgets final : public TMxpFrameFrontend
{
public:
    explicit TMxpFrameWidgets(TMainConsole* pMainConsole);

    void createInternalFrame(const QString& name, const QString& hostName, const QString& title, const QRect& geometry, bool showHeader, bool scrolling) override;
    std::optional<QSize> createExternalFrame(const QString& name, const QString& title, const QSize& size, bool scrolling) override;
    void createTabFrame(const QString& name, const QString& title, const QString& parentName, const QSize& size, bool scrolling, bool select) override;
    bool removeFromParentTabs(const QString& name, const QString& parentName) override;
    void destroyFrame(const QString& name) override;
    void showFrame(const QString& name) override;
    void focusFrame(const QString& name) override;
    void setGeometry(const QString& name, const QRect& geometry) override;
    void reportSize() override;
    TPrintSink* sink(const QString& name) const override;
    bool hasFrameWidget(const QString& name) const override { return frameWidget(name) != nullptr; }

    // Tells the frame manager the main console's size on the next event loop
    // turn, and has it reposition the frames if relayout was set on any call
    // before then while frames were open
    void scheduleSizeReport(bool relayout);
    // Tells the frame manager the space the pages of headerName's tabs have,
    // and has it place what is nested in them again if that changed it
    void reportTabAreaSize(const QString& headerName, const QSize& size);

    // For callers that need the widgets themselves
    QWidget* frameWidget(const QString& name) const;
    TConsole* frameConsole(const QString& name) const;
    QTabWidget* frameTabs(const QString& name) const;

private:
    void destroyStaleFrame(const QString& name);

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
    bool mSizeReportPending = false;
    bool mRelayoutPending = false;
};

#endif // MUDLET_TMXPFRAMEWIDGETS_H
