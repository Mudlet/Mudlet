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

#include "TNullAppFrontend.h"

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "TConsoleModel.h"
#include "TLabelModel.h"
#include "TLuaInterpreter.h"
#include "TNullConsoleFrontend.h"
#include "TWindowRegistry.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QRect>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <QStringList>
#include <QThread>
#include <QTimer>

using namespace std::chrono_literals;

namespace {
using Layout = TNullAppFrontend::Layout;
using Placement = TNullAppFrontend::Placement;

// Only where each user window and map widget is and whether it shows, in a file beside the GUI's windowLayout.dat; the
// magic tells a foreign file, which is damaged, from one of this format's later versions
const quint32 layoutFileMagic = 0x4d574c48; // "MWLH"
const quint32 layoutFileVersion = 1;

QString layoutFilePath()
{
    return MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
}

// Only '\' and '/' need escaping to keep keys unambiguous; dlgConnectionProfiles refuses both, but
// HostManager::addHost() takes them
QString placementKey(Host* pHost, const QString& name)
{
    QString profile = pHost->getName();
    profile.replace(QLatin1Char('\\'), qsl("\\\\")).replace(QLatin1Char('/'), qsl("\\/"));
    return qsl("%1/%2").arg(profile, name);
}

bool userWindowWithNoView(const Host* pHost, const QString& name)
{
    return !pHost->hasConsoleView() && pHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow;
}

TNullConsoleFrontend* nullConsoleOf(const Host* pHost)
{
    return pHost->hasConsoleView() ? nullptr : static_cast<TNullConsoleFrontend*>(pHost->consoleFrontend());
}

// Not windowVisible(), which answers hidden through the resize a show sends, where the GUI's !isHidden() is already true
bool userWindowShown(const Host* pHost, const QString& name)
{
    const TNullConsoleFrontend* pConsole = nullConsoleOf(pHost);
    return pConsole && pConsole->subConsoleShown(name);
}

Placement placementOf(Host* pHost, const QString& name)
{
    return {pHost->windowGeometry(name).value_or(QRect()), userWindowShown(pHost, name)};
}

// What a profile with no console view has of a layout now
void addPlacementsOf(Host* pHost, Layout& layout)
{
    for (const QString& name : pHost->windowRegistry().subConsoleNames()) {
        if (userWindowWithNoView(pHost, name)) {
            layout.userWindows.insert(placementKey(pHost, name), placementOf(pHost, name));
        }
    }
    if (const TNullConsoleFrontend* pConsole = nullConsoleOf(pHost)) {
        if (const auto placement = pConsole->mapWidgetPlacement()) {
            layout.mapWidgets.insert(pHost->getName(), *placement);
        }
    }
}

// Unreadable is one a save must not replace, as it may hold placements this cannot see, such as a later
// version's; a damaged one, short, corrupt, foreign or of a version before the first, has none this can read back
enum class LayoutFile { Read, Missing, Unreadable, Damaged };

const char* streamStatusName(const QDataStream::Status status)
{
    switch (status) {
    case QDataStream::Ok:
        return "ok";
    case QDataStream::ReadPastEnd:
        return "read past end";
    case QDataStream::ReadCorruptData:
        return "corrupt data";
    case QDataStream::WriteFailed:
        return "write failed";
    case QDataStream::SizeLimitExceeded:
        return "size limit exceeded";
    }
    return "unknown";
}

LayoutFile readLayout(Layout& layout, const bool saving)
{
    const char* unreadableOutcome =
            saving ? "- this save does not replace it: delete or rename it to save one" : "- none of it will be loaded, nor a layout saved over it, until it is deleted or renamed";
    QFile layoutFile(layoutFilePath());
    if (!layoutFile.exists()) {
        return LayoutFile::Missing;
    }
    if (!layoutFile.open(QIODevice::ReadOnly)) {
        qWarning() << "TNullAppFrontend: error opening window layout file:" << layoutFile.fileName() << layoutFile.errorString() << unreadableOutcome;
        return LayoutFile::Unreadable;
    }
    QDataStream ifs(&layoutFile);
    ifs.setVersion(QDataStream::Qt_5_12);
    quint32 magic = 0;
    quint32 version = 0;
    ifs >> magic >> version;
    const bool ours = magic == layoutFileMagic && ifs.status() == QDataStream::Ok;
    if (ours && version > layoutFileVersion) {
        qWarning() << "TNullAppFrontend: window layout file" << layoutFile.fileName() << "has version" << version << "not" << layoutFileVersion << unreadableOutcome;
        return LayoutFile::Unreadable;
    }
    if (ours && version == layoutFileVersion) {
        ifs >> layout.userWindows >> layout.mapWidgets;
    }
    if (!ours || version != layoutFileVersion || ifs.status() != QDataStream::Ok) {
        qWarning() << "TNullAppFrontend: window layout file" << layoutFile.fileName() << "is damaged or not one, magic" << magic << "version" << version << "read status"
                   << streamStatusName(ifs.status()) << (saving ? "- this save replaces it" : "- none of it will be loaded, and the next save replaces it");
        layout = Layout();
        return LayoutFile::Damaged;
    }
    return LayoutFile::Read;
}

// A guard on the model's notifier, not the model's address: a window a handler opens mid-load can reuse a freed
// model's address, but this guard clears when the model goes
QPointer<const QObject> windowIdentity(const Host* pHost, const QString& name)
{
    const TConsoleModel* pModel = pHost->windowRegistry().subConsoleModel(name);
    return pModel ? &pModel->mNotifier : nullptr;
}
} // namespace

TAppFrontend* TAppFrontend::nullView()
{
    // Function-local, so it exists before any static initialiser can ask for it
    static TNullAppFrontend view;
    return &view;
}

std::optional<QSize> TNullAppFrontend::getImageSize(const QString& imageLocation)
{
    return TLabelModel::imageSize(imageLocation);
}

bool TNullAppFrontend::openProfile(const QString& profileName, const bool connect)
{
    HostManager* pool = HostManager::self();
    return pool && pool->openProfile(profileName, connect);
}

bool TNullAppFrontend::requestProfileClose(const QString& profileName)
{
    HostManager* pool = HostManager::self();
    return pool && pool->requestProfileClose(profileName);
}

namespace {
// Process-wide, as the timers below outlive any one view
bool sForceClosePending = false;
bool sQuitPending = false;

// quit() ends only a running exec() and does nothing from an event loop pump before one, which is
// where an embedder loading profiles meets this. A nested loop before exec() still loses it.
void quitOnceRunning()
{
    if (QThread::currentThread()->loopLevel() == 0) {
        QTimer::singleShot(50ms, quitOnceRunning);
        return;
    }
    sQuitPending = false;
    QCoreApplication::quit();
}

void closeAllAndQuit();

// Nothing would delete a profile taken out of the pool after the quit, and its script may open another
void closeAllOnceReleased()
{
    if (HostManager::releasingProfile()) {
        QTimer::singleShot(50ms, closeAllOnceReleased);
        return;
    }
    closeAllAndQuit();
}

void closeAllAndQuit()
{
    if (HostManager* pool = HostManager::self()) {
        // As the GUI's, until a load returns or a profile leaves a nested event loop or a save wait; a profile still
        // running a script or a map operation closes, and leaves the pool once that ends. A reset asked
        // for first that can run now ends first, as the GUI's alert runs the event loop once before its close.
        bool mustWait = pool->loadingProfile();
        for (const auto& pHost : pool->hostList()) {
            if (pHost->getLuaInterpreter()->pumpingEvents()) {
                qWarning() << "TNullAppFrontend::armForceClose() - a nested event loop is running, waiting for it to finish";
                mustWait = true;
                break;
            }
            if (pHost->waitingForProfileSave()) {
                qWarning() << "TNullAppFrontend::armForceClose() - profile" << pHost->getName() << "is waiting for its save to finish, waiting for it";
                mustWait = true;
                break;
            }
            mustWait |= pHost->profileResetInProgress() && !pHost->isClosingDown() && !pHost->getLuaInterpreter()->luaOnStack();
        }
        if (mustWait) {
            QTimer::singleShot(50ms, closeAllAndQuit);
            return;
        }
        pool->closeAllProfiles();
    }
    if (HostManager::releasingProfile()) {
        QTimer::singleShot(50ms, closeAllOnceReleased);
        return;
    }
    sForceClosePending = false;
    if (!sQuitPending) {
        sQuitPending = true;
        if (QThread::currentThread()->loopLevel() == 0) {
            qDebug() << "TNullAppFrontend::armForceClose() INFO - no event loop is running yet, so the quit waits for one";
        }
        quitOnceRunning();
    }
}
} // namespace

void TNullAppFrontend::armForceClose()
{
    if (sForceClosePending) {
        return;
    }
    sForceClosePending = true;
    // Deferred, as closing deletes the Lua state the caller runs on
    QTimer::singleShot(0ms, closeAllAndQuit);
}

bool TNullAppFrontend::saveWindowLayoutForScript()
{
    HostManager* pManager = HostManager::self();
    if (!pManager || mpLayoutLoad) {
        qDebug() << "TNullAppFrontend::saveWindowLayoutForScript() - not saving:" << (pManager ? "a layout is loading" : "there is no HostManager");
        return false;
    }
    // Keeps what the file holds, as the GUI's saveState() writes each saved dock not open once a load has run; this
    // keeps them with no load too
    Layout layout;
    if (readLayout(layout, true) == LayoutFile::Unreadable) {
        return false;
    }
    // Newest last: what a closed profile had, then the deleted windows, as saveState() writes a deleted dock's
    // placeholder, then what is open
    layout.userWindows.insert(mClosedProfiles.userWindows);
    layout.mapWidgets.insert(mClosedProfiles.mapWidgets);
    layout.userWindows.insert(mDeleted.userWindows);
    layout.mapWidgets.insert(mDeleted.mapWidgets);
    for (const auto& pHost : pManager->hostList()) {
        addPlacementsOf(pHost.get(), layout);
    }

    QSaveFile layoutFile(layoutFilePath());
    if (!layoutFile.open(QIODevice::WriteOnly)) {
        qWarning() << "TNullAppFrontend::saveWindowLayoutForScript: error opening window layout file for writing:" << layoutFile.fileName() << layoutFile.errorString();
        return false;
    }
    QDataStream ofs(&layoutFile);
    ofs.setVersion(QDataStream::Qt_5_12);
    ofs << layoutFileMagic << layoutFileVersion << layout.userWindows << layout.mapWidgets;
    if (!layoutFile.commit()) {
        qWarning() << "TNullAppFrontend::saveWindowLayoutForScript: error saving window layout file:" << layoutFile.fileName() << layoutFile.errorString();
        return false;
    }
    mDeleted = Layout();
    mClosedProfiles = Layout();
    return true;
}

void TNullAppFrontend::userWindowRemoved(Host* pHost, const QString& name)
{
    if (!userWindowWithNoView(pHost, name)) {
        return;
    }
    const QString key = placementKey(pHost, name);
    Placement placement = placementOf(pHost, name);
    // One a handler deletes before the load reaches it takes the file's placement, as restoreState() gives its placeholder,
    // and for one a handler opened, only the geometry the load would have given it
    if (mpLayoutLoad) {
        const QPointer<const QObject> window = windowIdentity(pHost, name);
        const auto saved = mpLayoutLoad->placements.constFind(key);
        if (window && saved != mpLayoutLoad->placements.cend() && !mpLayoutLoad->placed.contains(window)) {
            placement.geometry = saved->geometry;
            if (mpLayoutLoad->present.contains(window)) {
                placement.shown = saved->shown;
            }
        }
    }
    mDeleted.userWindows.insert(key, placement);
}

void TNullAppFrontend::mapWidgetRemoved(Host* pHost)
{
    if (const TNullConsoleFrontend* pConsole = nullConsoleOf(pHost)) {
        if (const auto placement = pConsole->mapWidgetPlacement()) {
            mDeleted.mapWidgets.insert(pHost->getName(), *placement);
        }
    }
}

// Each close writes, where the GUI's mHasSavedLayout lets none write once one has saved in the session, until a load
// with layout updates pending clears it, and so loses a later close's changes. One with a console view is left to its
// TMainConsole::closeEvent().
void TNullAppFrontend::profileClosing(Host* pHost)
{
    // Held until a save succeeds, and moved out of mDeleted, which a load drops: they are newer than the file's, which
    // would otherwise win when the profile reopens. So too for one a view took over, whose deleted windows predate it.
    const QString prefix = placementKey(pHost, QString());
    for (auto it = mDeleted.userWindows.begin(); it != mDeleted.userWindows.end();) {
        if (it.key().startsWith(prefix)) {
            mClosedProfiles.userWindows.insert(it.key(), it.value());
            it = mDeleted.userWindows.erase(it);
        } else {
            ++it;
        }
    }
    if (const auto mapWidget = mDeleted.mapWidgets.find(pHost->getName()); mapWidget != mDeleted.mapWidgets.end()) {
        mClosedProfiles.mapWidgets.insert(mapWidget.key(), mapWidget.value());
        mDeleted.mapWidgets.erase(mapWidget);
    }
    if (pHost->hasConsoleView()) {
        return;
    }
    addPlacementsOf(pHost, mClosedProfiles);
    saveWindowLayoutForScript();
}

void TNullAppFrontend::profilesClosed()
{
    QStringList held;
    for (const Layout* pLayout : {&mDeleted, &mClosedProfiles}) {
        held.append(pLayout->userWindows.keys());
        for (const QString& profile : pLayout->mapWidgets.keys()) {
            held.append(qsl("%1 map widget").arg(profile));
        }
    }
    if (held.isEmpty()) {
        return;
    }
    if (!saveWindowLayoutForScript()) {
        // A refusal for want of a HostManager is only a qDebug, so this says so
        qWarning() << "TNullAppFrontend::profilesClosed:" << (HostManager::self() ? "the window layout" : "with no HostManager, the window layout")
                   << "could not be saved, so where these were is lost:" << held;
        mDeleted = Layout();
        mClosedProfiles = Layout();
    }
}

bool TNullAppFrontend::loadWindowLayout()
{
    HostManager* pManager = HostManager::self();
    if (!pManager || mpLayoutLoad) {
        qDebug() << "TNullAppFrontend::loadWindowLayout() - not loading:" << (pManager ? "already loading" : "there is no HostManager");
        return false;
    }
    Layout layout;
    const LayoutFile file = readLayout(layout, false);
    // Held from open profiles' last close, which no save has written since: newer than the file
    Layout held;
    for (const auto& pHost : pManager->hostList()) {
        const QString prefix = placementKey(pHost.get(), QString());
        for (auto it = mClosedProfiles.userWindows.cbegin(); it != mClosedProfiles.userWindows.cend(); ++it) {
            if (it.key().startsWith(prefix)) {
                held.userWindows.insert(it.key(), it.value());
            }
        }
        if (const auto mapWidget = mClosedProfiles.mapWidgets.constFind(pHost->getName()); mapWidget != mClosedProfiles.mapWidgets.cend()) {
            held.mapWidgets.insert(mapWidget.key(), mapWidget.value());
        }
    }
    if (file != LayoutFile::Read && held.userWindows.isEmpty() && held.mapWidgets.isEmpty()) {
        return false;
    }
    layout.userWindows.insert(held.userWindows);
    layout.mapWidgets.insert(held.mapWidgets);
    // As restoreState() replaces the GUI's placeholders for deleted docks, even ones the file lacks, but only from a file
    // it could read
    if (file == LayoutFile::Read) {
        mDeleted = Layout();
    }
    const QMap<QString, Placement>& placements = layout.userWindows;

    QList<QPointer<const QObject>> present;
    for (const auto& pHost : pManager->hostList()) {
        for (const QString& name : pHost->windowRegistry().subConsoleNames()) {
            present.append(windowIdentity(pHost.get(), name));
        }
    }
    QList<QPointer<const QObject>> placed;
    // Resize handlers run mid-load, as in the GUI's restoreState(), and their load and saveWindowLayout() are refused as
    // there
    const LayoutLoad load{placements, present, placed};
    const QScopedValueRollback loading(mpLayoutLoad, &load);
    // Map widgets first: placing them runs no handler, so a handler run by placing a user window has the last word on them,
    // and none can remove one the load has yet to place
    for (const auto& pHost : pManager->hostList()) {
        TNullConsoleFrontend* pConsole = nullConsoleOf(pHost.get());
        if (!pConsole || !pConsole->mapWidgetPlacement()) {
            continue;
        }
        if (const auto mapWidget = layout.mapWidgets.constFind(pHost->getName()); mapWidget != layout.mapWidgets.cend()) {
            pConsole->restoreMapWidgetPlacement(*mapWidget);
        }
    }
    auto unplaced = [&placements, &placed](Host* pHost, const QString& name, const QPointer<const QObject>& window) {
        return window && !placed.contains(window) && placements.contains(placementKey(pHost, name)) && userWindowWithNoView(pHost, name);
    };
    // At most two passes, the first as restoreState() and the second as what restoreFloatingDockGeometries() places
    // of the windows handlers opened, so handlers opening windows at each other's resizes, or reopening their own,
    // cannot keep the load going
    bool placedMore = true;
    for (int pass = 0; placedMore && pass < 2; ++pass) {
        placedMore = false;
        for (const auto& pHost : pManager->hostList()) {
            for (const QString& name : pHost->windowRegistry().subConsoleNames()) {
                const QPointer<const QObject> window = windowIdentity(pHost.get(), name);
                if (!unplaced(pHost.get(), name, window)) {
                    continue;
                }
                placed.append(window);
                placedMore = true;
                const auto& [geometry, visible] = *placements.constFind(placementKey(pHost.get(), name));
                // One a handler opened is not a dock restoreState() began with, so it takes only its geometry
                const bool restoreShown = present.contains(window);
                // As QMainWindow::restoreState() hides each dock, places it and shows those it should: the resize reaches
                // scripts with the show, while the window still answers hidden, and none for one it leaves hidden
                if (restoreShown && userWindowShown(pHost.get(), name)) {
                    pHost->hideWindow(name);
                }
                pHost->moveWindow(name, geometry.x(), geometry.y());
                pHost->resizeWindow(name, geometry.width(), geometry.height());
                if (restoreShown && visible) {
                    pHost->showWindow(name);
                }
            }
        }
    }
    // The rest of restoreFloatingDockGeometries(): a window restoreState() placed takes its saved geometry again, so a
    // size or position a resize handler gave it gives way to the saved one. The GUI does so only for floating docks, and
    // a headless window has no docking
    for (const auto& pHost : pManager->hostList()) {
        for (const QString& name : pHost->windowRegistry().subConsoleNames()) {
            const QPointer<const QObject> window = windowIdentity(pHost.get(), name);
            if (!window || !present.contains(window) || !placed.contains(window) || !userWindowWithNoView(pHost.get(), name)) {
                continue;
            }
            const QRect& geometry = placements.constFind(placementKey(pHost.get(), name))->geometry;
            pHost->moveWindow(name, geometry.x(), geometry.y());
            pHost->resizeWindow(name, geometry.width(), geometry.height());
        }
    }
    // As the GUI's load has each profile's TMainConsole::reportDockGeometry() do once restoreState() succeeds
    for (const auto& pHost : pManager->hostList()) {
        if (TNullConsoleFrontend* pConsole = nullConsoleOf(pHost.get())) {
            pConsole->reportUserWindowVisibility();
        }
    }
    return true;
}
