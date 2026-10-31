#ifndef MUDLET_TNULLAPPFRONTEND_H
#define MUDLET_TNULLAPPFRONTEND_H

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

#include "TAppFrontend.h"
#include "TNullConsoleFrontend.h"
#include "utils.h"

#include <QList>
#include <QMap>
#include <QPointer>
#include <QSize>

// The application shell core code sees while there is no main window: before it is made, after it
// has gone, and in a run with no GUI. Most queries have no value and most actions fail or do nothing,
// and a file or folder dialog answers as if the player cancelled it. Profiles open and close through
// HostManager::self(), quitting ends the application's exec(), and the window layout of profiles with
// no main window is saved to and loaded from a file of its own.
class TNullAppFrontend : public TAppFrontend
{
public:
    using Placement = TNullConsoleFrontend::Placement;
    // User windows keyed by "profile/window", with the profile's '\' and '/' escaped by a '\', so no profile's key starts
    // another's; map widgets by profile
    struct Layout
    {
        QMap<QString, Placement> userWindows;
        QMap<QString, Placement> mapWidgets;
    };

    Host* getActiveHost() override { return nullptr; }
    void announce(const QString&, const QString&, bool) override {}
    void showOptionsDialog(const QString&, Host*) override {}
    void showConnectionDialog() override {}
    void handleTelnetUri(const QString&) override {}
    void setCompactInputLineChecked(Host*, bool) override {}
    void armForceClose() override;
    void writeSettings() override {}
    bool openWebPage(const QString&) override { return false; }
    void showNotification(const QString&, const QString&, std::optional<int>) override {}
    bool drawUpperLowerLevels() const override { return mDrawUpperLowerLevels; }
    void setDrawUpperLowerLevels(bool draw) override { mDrawUpperLowerLevels = draw; }
    void updateMapActionAvailability() override {}
    bool showTabConnectionIndicators() const override { return mShowTabConnectionIndicators; }
    void setShowTabConnectionIndicators(bool show) override { mShowTabConnectionIndicators = show; }
    void alertUser(int) override {}
    std::optional<QSize> getImageSize(const QString& imageLocation) override;
    int profileTabIndex(const QString&) const override { return -1; }
    void setActiveProfileTab(const QString&) override {}
    void setAppStyleSheet(const QString&) override {}
    void resizeMainWindow(int, int) override {}
    bool loadWindowLayout() override;
    bool layoutLoading() const override { return mpLayoutLoad != nullptr; }
    bool saveWindowLayoutForScript() override;
    void userWindowRemoved(Host*, const QString& name) override;
    void mapWidgetRemoved(Host*) override;
    void profileClosing(Host*) override;
    void profilesClosed() override;
    bool quitting() const override { return false; }
    bool openProfile(const QString& profileName, bool connect) override;
    bool requestProfileClose(const QString& profileName) override;
    void processEventLoopHack() override {}
    QObject* openComposer(Host*, const QString&, const QString&) override { return nullptr; }
    void closeComposer(QObject*) override {}
    QString getOpenFileName(const QString&, const QString&) override { return QString(); }
    QString getExistingDirectory(const QString&, const QString&) override { return QString(); }
    int addAddonCommand(const CommandRequest&, Host*, const QString&, QString& error) override
    {
        error = noViewError();
        return -1;
    }
    bool removeAddonCommand(int, Host*) override { return false; }
    bool setAddonCommandEnabled(int, bool, Host*) override { return false; }
    bool setAddonCommandChecked(int, bool, Host*) override { return false; }
    bool setAddonCommandIcon(int, const QString&, Host*) override { return false; }
    bool setAddonCommandTooltip(int, const QString&, Host*) override { return false; }
    bool setAddonCommandPinned(int, bool, Host*) override { return false; }
    bool setAddonCommandPulse(int, bool, const QString&, const QString&, int, Host*, QString& error) override
    {
        error = noViewError();
        return false;
    }

private:
    static QString noViewError() { return qsl("mudlet instance not available"); }

    // Held, unlike the rest, as getConfig() reads these back. Each starts at the main window's default
    // with no saved settings; nothing saves them.
    bool mDrawUpperLowerLevels = true;
    bool mShowTabConnectionIndicators = false;
    struct LayoutLoad
    {
        const QMap<QString, Placement>& placements;
        // The windows open as it began, which restoreState() shows or hides, and those it has placed
        const QList<QPointer<const QObject>>& present;
        const QList<QPointer<const QObject>>& placed;
    };
    // Set while a load runs, as mudlet::mIsLoadingLayout
    const LayoutLoad* mpLayoutLoad = nullptr;
    // Since the last successful save, or a load that read the file: where each user window and map widget deleted with
    // no console view was, of a profile still open, as QMainWindow keeps a deleted dock's placeholder until
    // restoreState() replaces it
    Layout mDeleted;
    // Since the last successful save: closed profiles' windows and map widgets, newer than the file, which a load places
    // over it once the profile is open again
    Layout mClosedProfiles;
};

#endif // MUDLET_TNULLAPPFRONTEND_H
