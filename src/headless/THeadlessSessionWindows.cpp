/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

// The windows a profile's scripts make, for an app to draw as the GUI would

#include "THeadlessSession.h"

#include "Host.h"
#include "TNullConsoleFrontend.h"
#include "TWindowNotifier.h"

#include <QDebug>
#include <QHash>

#include <algorithm>
#include <functional>
#include <vector>

namespace {

using Record = TNullConsoleFrontend::WindowRecord;

THeadlessSession::ScriptWindow::Kind kindOf(const Record::Kind kind)
{
    using Kind = THeadlessSession::ScriptWindow::Kind;
    switch (kind) {
    case Record::Kind::miniConsole:
        return Kind::miniConsole;
    case Record::Kind::userWindow:
        return Kind::userWindow;
    case Record::Kind::label:
        return Kind::label;
    case Record::Kind::commandLine:
        return Kind::commandLine;
    case Record::Kind::textBox:
        return Kind::textBox;
    case Record::Kind::scrollBox:
        return Kind::scrollBox;
    case Record::Kind::mapper:
        return Kind::mapper;
    }
    Q_UNREACHABLE();
}

THeadlessSession::ScriptWindow scriptWindowOf(const Record& record, const int stackOrder)
{
    return {record.name, kindOf(record.kind), record.parent, record.geometry, record.visible, record.shown, stackOrder, record.background, record.font, record.inDock};
}

// Only these hold other windows, so a label that shares a scroll box's name is not taken for it
bool holdsWindows(const Record::Kind kind)
{
    return kind == Record::Kind::userWindow || kind == Record::Kind::scrollBox || kind == Record::Kind::miniConsole;
}

QHash<QString, QList<THeadlessSession::ScriptWindow>> byName(const QList<THeadlessSession::ScriptWindow>& windows)
{
    QHash<QString, QList<THeadlessSession::ScriptWindow>> named;
    for (const THeadlessSession::ScriptWindow& window : windows) {
        named[window.name].append(window);
    }
    return named;
}

QStringList changedNames(const QList<THeadlessSession::ScriptWindow>& before, const QList<THeadlessSession::ScriptWindow>& after)
{
    const auto namedBefore = byName(before);
    const auto namedAfter = byName(after);
    QStringList names;
    for (auto it = namedBefore.cbegin(); it != namedBefore.cend(); ++it) {
        if (namedAfter.value(it.key()) != it.value()) {
            names << it.key();
        }
    }
    for (auto it = namedAfter.cbegin(); it != namedAfter.cend(); ++it) {
        if (!namedBefore.contains(it.key())) {
            names << it.key();
        }
    }
    names.sort();
    return names;
}

} // namespace

QList<THeadlessSession::ScriptWindow> THeadlessSession::scriptWindows() const
{
    const auto* pView = mpHost ? dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()) : nullptr;
    if (!pView) {
        return {};
    }
    std::vector<Record> records = pView->windowRecords();
    std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.stackOrder < b.stackOrder;
    });
    std::vector<bool> placed(records.size(), false);
    QList<ScriptWindow> windows;
    // The docks share the main window's area's empty parent, but are not stacked with what is in it
    const std::function<void(const QString&, bool)> placeChildren = [&](const QString& parent, const bool docks) {
        int stackOrder = 0;
        for (size_t i = 0; i < records.size(); ++i) {
            if (placed[i] || records[i].parent != parent || records[i].inDock != docks) {
                continue;
            }
            placed[i] = true;
            const Record& record = records[i];
            windows.append(scriptWindowOf(record, stackOrder++));
            if (holdsWindows(record.kind)) {
                placeChildren(record.name, false);
            }
        }
    };
    placeChildren(QString(), false);
    placeChildren(QString(), true);
    // Only a window made in a user window or scroll box the null view does not hold, which a detached GUI view can
    // leave behind, has no parent here: listed last rather than lost, as Lua still finds it
    for (size_t i = 0; i < records.size(); ++i) {
        if (!placed[i]) {
            qWarning().nospace().noquote() << "THeadlessSession::scriptWindows() WARNING - the window \"" << records[i].name << "\" is in \"" << records[i].parent
                                           << "\", which is not among the windows, so it is listed last.";
            windows.append(scriptWindowOf(records[i], 0));
        }
    }
    return windows;
}

std::optional<THeadlessSession::ScriptWindow> THeadlessSession::scriptWindow(const QString& name, const std::optional<ScriptWindow::Kind> kind) const
{
    for (const ScriptWindow& window : scriptWindows()) {
        if (window.name == name && (!kind || window.kind == *kind)) {
            return window;
        }
    }
    return std::nullopt;
}

void THeadlessSession::connectWindowRelays(Host* pHost, const unsigned int run)
{
    mDeliveredWindows.reset();
    auto* pView = dynamic_cast<TNullConsoleFrontend*>(pHost->consoleFrontend());
    if (!pView) {
        return;
    }
    connect(
            &pView->windowNotifier(),
            &TWindowNotifier::windowsChanged,
            this,
            [this, run]() {
                if (run != mRun || !mpHost || !mDeliveredWindows) {
                    return;
                }
                QList<ScriptWindow> windows = scriptWindows();
                const QStringList names = changedNames(*mDeliveredWindows, windows);
                if (names.isEmpty()) {
                    return;
                }
                mDeliveredWindows = std::move(windows);
                const QString profile = mProfileName;
                emit scriptWindowsChanged(profile, names);
            },
            Qt::QueuedConnection);
}

void THeadlessSession::noteWindowsAtLoad()
{
    mDeliveredWindows = scriptWindows();
}
