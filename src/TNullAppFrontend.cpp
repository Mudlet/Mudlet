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
#include "TLabelModel.h"
#include "TWindowRegistry.h"

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QRect>
#include <QSaveFile>
#include <QSet>

#include <utility>

namespace {
// A profile with no main window has no dock layout to save, only where each user window is and whether it
// shows, so that goes to a file of its own beside the GUI's windowLayout.dat rather than into it.
const quint32 layoutFileVersion = 1;

QString layoutFilePath()
{
    return MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
}

QString placementKey(Host* pHost, const QString& name)
{
    return qsl("%1/%2").arg(pHost->getName(), name);
}

bool userWindowWithNoView(const Host* pHost, const QString& name)
{
    return !pHost->hasConsoleView() && pHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow;
}

using Placements = QMap<QString, std::pair<QRect, bool>>;

// False with no file, and for one that cannot be read
bool readPlacements(Placements& placements)
{
    QFile layoutFile(layoutFilePath());
    if (!layoutFile.exists()) {
        return false;
    }
    if (!layoutFile.open(QIODevice::ReadOnly)) {
        qWarning() << "TNullAppFrontend: error opening window layout file:" << layoutFile.fileName() << layoutFile.errorString();
        return false;
    }
    QDataStream ifs(&layoutFile);
    ifs.setVersion(QDataStream::Qt_5_12);
    quint32 version = 0;
    ifs >> version;
    if (version != layoutFileVersion) {
        qWarning() << "TNullAppFrontend: window layout file" << layoutFile.fileName() << "has version" << version << "not" << layoutFileVersion;
        return false;
    }
    ifs >> placements;
    if (ifs.status() != QDataStream::Ok) {
        qWarning() << "TNullAppFrontend: window layout file" << layoutFile.fileName() << "is damaged, read status" << ifs.status();
        placements.clear();
        return false;
    }
    return true;
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

bool TNullAppFrontend::saveWindowLayoutForScript()
{
    HostManager* pManager = HostManager::self();
    if (!pManager || mLoadingLayout) {
        qDebug() << "TNullAppFrontend::saveWindowLayoutForScript() - not saving:" << (pManager ? "a layout is loading" : "there is no HostManager");
        return false;
    }
    // As QMainWindow::saveState() keeps a dock it no longer has, a profile not loaded keeps its entries
    Placements placements;
    readPlacements(placements);
    for (const auto& pHost : pManager->hostList()) {
        if (pHost->hasConsoleView()) {
            continue;
        }
        const QString prefix = placementKey(pHost.get(), QString());
        placements.removeIf([&prefix](const auto& placement) {
            return placement.key().startsWith(prefix);
        });
        for (const QString& name : pHost->windowRegistry().subConsoleNames()) {
            if (userWindowWithNoView(pHost.get(), name)) {
                placements.insert(placementKey(pHost.get(), name), {pHost->windowGeometry(name).value_or(QRect()), pHost->windowVisible(name).value_or(false)});
            }
        }
    }

    QSaveFile layoutFile(layoutFilePath());
    if (!layoutFile.open(QIODevice::WriteOnly)) {
        qWarning() << "TNullAppFrontend::saveWindowLayoutForScript: error opening window layout file for writing:" << layoutFile.fileName() << layoutFile.errorString();
        return false;
    }
    QDataStream ofs(&layoutFile);
    ofs.setVersion(QDataStream::Qt_5_12);
    ofs << layoutFileVersion << placements;
    if (!layoutFile.commit()) {
        qWarning() << "TNullAppFrontend::saveWindowLayoutForScript: error saving window layout file:" << layoutFile.fileName() << layoutFile.errorString();
        return false;
    }
    return true;
}

bool TNullAppFrontend::loadWindowLayout()
{
    HostManager* pManager = HostManager::self();
    if (!pManager || mLoadingLayout) {
        qDebug() << "TNullAppFrontend::loadWindowLayout() - not loading:" << (pManager ? "already loading" : "there is no HostManager");
        return false;
    }
    Placements placements;
    if (!readPlacements(placements)) {
        return false;
    }

    // Resizing a shown window raises sysUserWindowResizeEvent at once, where the GUI's dock events wait for
    // QMainWindow::restoreState() to finish, so a handler's loadWindowLayout() or saveWindowLayout() is refused
    // rather than placing or saving windows this load has yet to reach. A window a handler opens with the
    // layout is placed by walking again, as its own load was refused: by its model, as one reopened under a
    // placed name is a new window.
    mLoadingLayout = true;
    QSet<const TConsoleModel*> placed;
    for (bool placedMore = true; placedMore;) {
        placedMore = false;
        for (const auto& pHost : pManager->hostList()) {
            for (const QString& name : pHost->windowRegistry().subConsoleNames()) {
                const TConsoleModel* pModel = pHost->windowRegistry().subConsoleModel(name);
                const auto it = placements.constFind(placementKey(pHost.get(), name));
                if (it == placements.constEnd() || !pModel || placed.contains(pModel) || !userWindowWithNoView(pHost.get(), name)) {
                    continue;
                }
                placed.insert(pModel);
                placedMore = true;
                const auto& [geometry, visible] = it.value();
                // Hidden first: QMainWindow::restoreState() sizes a dock it hides without any event reaching scripts
                if (!visible && pHost->windowVisible(name).value_or(false)) {
                    pHost->hideWindow(name);
                }
                pHost->moveWindow(name, geometry.x(), geometry.y());
                pHost->resizeWindow(name, geometry.width(), geometry.height());
                if (visible && !pHost->windowVisible(name).value_or(false)) {
                    pHost->showWindow(name);
                }
            }
        }
    }
    mLoadingLayout = false;
    return true;
}
