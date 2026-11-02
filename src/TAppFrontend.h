#ifndef MUDLET_TAPPFRONTEND_H
#define MUDLET_TAPPFRONTEND_H

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

#include <QString>

#include <optional>

class Host;
class QObject;
class QSize;

// The application shell as core code sees it. The mudlet main window implements it;
// core code reaches it through instance() instead of naming the window class.
class TAppFrontend
{
public:
    // Never null: while there is no main window, before it is made and from the end of its
    // destructor, this is a null view whose answers are all "not there".
    static TAppFrontend* instance() { return smpInstance ? smpInstance : nullView(); }
    static bool hasView() { return smpInstance != nullptr; }
    // From viewDestroyed() until another view is set
    static bool viewGone() { return smViewGone; }
    static void setInstance(TAppFrontend* frontend)
    {
        smpInstance = frontend;
        smViewGone = smViewGone && !frontend;
    }
    // For the end of the view's destructor, unlike setInstance(nullptr), which a run with no view at all may also do
    static void viewDestroyed()
    {
        smpInstance = nullptr;
        smViewGone = true;
    }

    // The profile in the active tab, or nullptr if there is none or it has no console yet. With no main window, the open
    // profile setActiveProfileTab() last named.
    virtual Host* getActiveHost() = 0;
    virtual void announce(const QString& text, const QString& processing, bool isPlain) = 0;
    virtual void showOptionsDialog(const QString& tab, Host* pHost) = 0;
    virtual void showConnectionDialog() = 0;
    virtual void handleTelnetUri(const QString& uri) = 0;
    // A link from a game to a telnet:// or telnets:// address: asks the player first, always.
    virtual void connectFromGameLink(const QString& uri) = 0;
    // Only the profile in the active tab drives the menu's checkbox.
    virtual void setCompactInputLineChecked(Host* pHost, bool checked) = 0;
    // closeMudlet(): asks, from the event loop, for every profile to close and save and then the application to quit
    virtual void armForceClose() = 0;
    virtual void writeSettings() = 0;
    virtual bool openWebPage(const QString& url) = 0;
    // Without msecs, the frontend picks how long the notification stays up.
    // false when there is no window to show it from
    virtual bool showNotification(const QString& title, const QString& text, std::optional<int> msecs) = 0;
    virtual bool drawUpperLowerLevels() const = 0;
    virtual void setDrawUpperLowerLevels(bool draw) = 0;
    virtual void updateMapActionAvailability() = 0;
    virtual bool showTabConnectionIndicators() const = 0;
    virtual void setShowTabConnectionIndicators(bool show) = 0;
    virtual void alertUser(int milliseconds) = 0;
    virtual std::optional<QSize> getImageSize(const QString& imageLocation) = 0;
    // -1 for a profile with no tab
    virtual int profileTabIndex(const QString& profileName) const = 0;
    virtual void setActiveProfileTab(const QString& profileName) = 0;
    virtual void setAppStyleSheet(const QString& styleSheet) = 0;
    // false when there is no window to resize
    virtual bool resizeMainWindow(int width, int height) = 0;
    virtual bool loadWindowLayout() = 0;
    virtual bool layoutLoading() const = 0;
    // Saves even if quitting already saved the layout; if this save fails, quitting saves it again.
    virtual bool saveWindowLayoutForScript() = 0;
    // For a frontend keeping the layout of profiles with no console view: a script or a profile reset deleting a user
    // window, an embedded mapper replacing a map widget, a profile closing, and its pool ending.
    // Called while the window or map widget is still there.
    virtual void userWindowRemoved(Host*, const QString&) {}
    virtual void mapWidgetRemoved(Host*) {}
    // At most once per Host, after HostManager raises any sysExitEvent it still owes, while its windows are still there.
    virtual void profileClosing(Host*) {}
    // By self()'s pool as it ends, and by any pool ending while self() answers with none, once its Hosts are gone.
    virtual void profilesClosed() {}
    virtual bool quitting() const = 0;
    // Loads and starts the profile and, if connect, connects it; false if it could not be loaded.
    virtual bool openProfile(const QString& profileName, bool connect) = 0;
    // Asks the profile to close, as its tab's close button does; false if it is not open here.
    virtual bool requestProfileClose(const QString& profileName) = 0;
    // Re-lays out the active profile's console from a short timer, e.g. after a toolbar is shown or hidden.
    virtual void processEventLoopHack() = 0;
    // Puts up the profile's composer showing title and text; the window deletes itself once closed.
    virtual QObject* openComposer(Host* pHost, const QString& title, const QString& text) = 0;
    virtual void closeComposer(QObject* composer) = 0;
    // Each returns an empty string if the player cancels.
    virtual QString getOpenFileName(const QString& title, const QString& location) = 0;
    virtual QString getExistingDirectory(const QString& title, const QString& location) = 0;

    // Surfaces a command can be placed on. A client with different chrome maps
    // these onto whatever it has; one that has only a menu honours Menu alone.
    enum class CommandSurface { Menu, Toolbar, Both };

    struct CommandRequest
    {
        QString name;
        QString icon;
        QString tooltip;
        QString menuPath;
        QString shortcut;
        CommandSurface surfaces = CommandSurface::Both;
    };

    // package: whose code asked, empty for none. error: why a command could not
    // be placed, so the binding can say which.
    virtual int addAddonCommand(const CommandRequest& request, Host* pHost, const QString& package, QString& error) = 0;
    virtual bool removeAddonCommand(int commandId, Host* pHost) = 0;
    virtual bool setAddonCommandEnabled(int commandId, bool enabled, Host* pHost) = 0;
    virtual bool setAddonCommandChecked(int commandId, bool checked, Host* pHost) = 0;
    virtual bool setAddonCommandIcon(int commandId, const QString& icon, Host* pHost) = 0;
    virtual bool setAddonCommandTooltip(int commandId, const QString& tooltip, Host* pHost) = 0;
    virtual bool setAddonCommandPinned(int commandId, bool pinned, Host* pHost) = 0;
    virtual bool setAddonCommandPulse(int commandId, bool enabled, const QString& color1, const QString& color2, int interval, Host* pHost, QString& error) = 0;

protected:
    // The main window owns itself, so nothing deletes it through this interface.
    ~TAppFrontend() = default;

private:
    static TAppFrontend* nullView();

    inline static TAppFrontend* smpInstance = nullptr;
    inline static bool smViewGone = false;
};

#endif // MUDLET_TAPPFRONTEND_H
