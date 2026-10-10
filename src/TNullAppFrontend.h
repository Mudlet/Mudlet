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
#include "utils.h"

#include <QSize>

// The application shell core code sees while there is no main window: before it is made, after it
// has gone, and in a run with no GUI. Queries have no value, actions fail or do nothing, and a
// file or folder dialog answers as if the player cancelled it.
class TNullAppFrontend : public TAppFrontend
{
public:
    Host* getActiveHost() override { return nullptr; }
    void announce(const QString&, const QString&, bool) override {}
    void showOptionsDialog(const QString&, Host*) override {}
    void showConnectionDialog() override {}
    void handleTelnetUri(const QString&) override {}
    void setCompactInputLineChecked(Host*, bool) override {}
    void armForceClose() override {}
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
    void refreshTabBarsAfterStyleChange() override {}
    void resizeMainWindow(int, int) override {}
    bool loadWindowLayout() override { return false; }
    bool saveWindowLayoutForScript() override { return false; }
    bool quitting() const override { return false; }
    bool openProfile(const QString&, bool) override { return false; }
    bool requestProfileTabClose(const QString&) override { return false; }
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
};

#endif // MUDLET_TNULLAPPFRONTEND_H
