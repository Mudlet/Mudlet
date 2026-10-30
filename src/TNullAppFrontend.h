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
#include <QSet>
#include <QSize>

class QKeySequence;

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
    // Refuses what the main window refuses whatever its bars show, bar clashes with the main window's own shortcuts, and
    // keeps each command for a frontend to show and activate
    int addAddonCommand(const CommandRequest& request, Host* pHost, const QString& package, QString& error) override;
    bool removeAddonCommand(int commandId, Host* pHost) override;
    bool setAddonCommandEnabled(int commandId, bool enabled, Host* pHost) override;
    bool setAddonCommandChecked(int commandId, bool checked, Host* pHost) override;
    bool setAddonCommandIcon(int commandId, const QString& icon, Host* pHost) override;
    bool setAddonCommandTooltip(int commandId, const QString& tooltip, Host* pHost) override;
    bool setAddonCommandPinned(int commandId, bool pinned, Host* pHost) override;
    bool setAddonCommandPulse(int commandId, bool enabled, const QString& color1, const QString& color2, int interval, Host* pHost, QString& error) override;

    struct AddonCommand
    {
        QPointer<Host> pHost;
        // Empty for a command no package's code created
        QString package;
        CommandRequest request;
        QString icon;
        QString tooltip;
        bool enabled = true;
        bool checkable = false;
        bool checked = false;
        bool pinned = false;
        bool pulseEnabled = false;
        QString pulseColor1;
        QString pulseColor2;
        int pulseInterval = 0;
    };
    const QMap<int, AddonCommand>& addonCommands() const { return mAddonCommands; }
    // As a click on the main window's button or menu item: false for a disabled or unknown command
    bool activateAddonCommand(int commandId);

    // Why the main window refuses a request whatever it shows, or an empty string: a menu path or shortcut on a
    // command kept off the menu, a shortcut Qt cannot hold, or a pulse it cannot paint
    static QString addonRequestRefusal(const CommandRequest& request);
    static QString addonShortcutRefusal(const QString& shortcut);
    static QString addonPulseRefusal(bool onToolbar, bool enabled, const QString& color1, const QString& color2);
    // A key one of the profile's own key bindings holds, which the binding would stop answering to
    static QString addonKeyBindingRefusal(const QKeySequence& sequence, const Host* pHost);
    static QString addonShortcutTakenRefusal(const QKeySequence& sequence, const QString& holder);
    static QString addonShortcutTakenElsewhereRefusal(const QKeySequence& sequence);
    static QString addonSubmenuRefusal(const QString& pathPart);
    static QString addonLabelRefusal(const QString& name);

private:
    AddonCommand* ownedAddonCommand(int commandId, const Host* pHost);
    QString addonPlacementRefusal(const CommandRequest& request, const Host* pHost) const;
    // As the main window's, a command goes with its package, a profile reset and its profile
    void watchAddonCommandsOf(Host* pHost);
    void removeAddonCommandsOf(const Host* pHost, const QString& package = QString());

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
    QMap<int, AddonCommand> mAddonCommands;
    // One sequence for every profile's commands, as the main window's, so an id names one command or nothing
    int mNextAddonCommandId = 1;
    QSet<const Host*> mAddonCommandHosts;
};

#endif // MUDLET_TNULLAPPFRONTEND_H
