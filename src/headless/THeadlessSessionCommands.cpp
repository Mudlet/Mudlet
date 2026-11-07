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

// The commands and buttons that scripts add to the GUI's main window and toolbars

#include "THeadlessSession.h"

#include "Host.h"
#include "TNullAppFrontend.h"

#include <QKeyEvent>

QList<THeadlessSession::AddonCommand> THeadlessSession::addonCommands() const
{
    QList<AddonCommand> result;
    const auto* pView = dynamic_cast<const TNullAppFrontend*>(TAppFrontend::instance());
    if (!mpHost || !pView) {
        return result;
    }
    const QMap<int, TNullAppFrontend::AddonCommand>& commands = pView->addonCommands();
    for (auto it = commands.cbegin(); it != commands.cend(); ++it) {
        const TNullAppFrontend::AddonCommand& command = it.value();
        if (command.pHost != mpHost) {
            continue;
        }
        AddonCommand entry;
        entry.id = it.key();
        entry.package = command.package;
        entry.name = command.request.name;
        entry.icon = command.icon;
        entry.tooltip = command.tooltip;
        entry.menuPath = command.request.menuPath;
        entry.shortcut = command.request.shortcut;
        entry.onMenu = command.request.surfaces != TAppFrontend::CommandSurface::Toolbar;
        entry.onToolbar = command.request.surfaces != TAppFrontend::CommandSurface::Menu;
        entry.enabled = command.enabled;
        entry.checkable = command.checkable;
        entry.checked = command.checked;
        entry.pinned = command.pinned;
        entry.pulse = command.pulseEnabled;
        entry.pulseColor1 = QColor::fromString(command.pulseColor1);
        entry.pulseColor2 = QColor::fromString(command.pulseColor2);
        entry.pulseIntervalMs = command.pulseInterval;
        result.append(entry);
    }
    return result;
}

bool THeadlessSession::activateAddonCommand(const int id)
{
    if (!usable()) {
        return false;
    }
    auto* pView = dynamic_cast<TNullAppFrontend*>(TAppFrontend::instance());
    const TNullAppFrontend::AddonCommand* pCommand = nullptr;
    if (pView) {
        const auto it = pView->addonCommands().constFind(id);
        if (it != pView->addonCommands().cend() && it->pHost == mpHost) {
            pCommand = &it.value();
        }
    }
    if (!pCommand) {
        //: Error from a frontend with no window that was asked to click a command a script added, giving its id
        mErrorString = tr("The profile has no command %1.").arg(id);
        return false;
    }
    if (!pCommand->enabled) {
        //: Error from a frontend with no window that was asked to click a command a script disabled, giving its name
        mErrorString = tr("The command \"%1\" is disabled.").arg(pCommand->request.name);
        return false;
    }
    return pView->activateAddonCommand(id);
}

int THeadlessSession::addonCommandOnKey(const QKeyEvent& event) const
{
    const auto* pView = dynamic_cast<const TNullAppFrontend*>(TAppFrontend::instance());
    if (!pView) {
        return 0;
    }
    const auto key = static_cast<Qt::Key>(event.key());
    // Qt's shortcut map tries a key on the keypad without its keypad flag once nothing takes it with it
    QList<QKeySequence> pressed{QKeySequence(QKeyCombination(event.modifiers(), key))};
    if (event.modifiers() & Qt::KeypadModifier) {
        pressed.append(QKeySequence(QKeyCombination(event.modifiers() & ~Qt::KeypadModifier, key)));
    }
    const QMap<int, TNullAppFrontend::AddonCommand>& commands = pView->addonCommands();
    for (const QKeySequence& sequence : std::as_const(pressed)) {
        for (auto it = commands.cbegin(); it != commands.cend(); ++it) {
            const TNullAppFrontend::AddonCommand& command = it.value();
            // Qt's shortcut map passes over a disabled action. Only the session's own, as addonCommands() lists,
            // where the GUI's menu also keeps another profile's pinned ones
            if (command.pHost != mpHost || !command.enabled || command.request.surfaces == TAppFrontend::CommandSurface::Toolbar) {
                continue;
            }
            if (!command.request.shortcut.isEmpty() && QKeySequence(command.request.shortcut) == sequence) {
                return it.key();
            }
        }
    }
    return 0;
}

void THeadlessSession::connectCommandRelays(const unsigned int run)
{
    auto* pView = dynamic_cast<TNullAppFrontend*>(TAppFrontend::instance());
    if (!pView) {
        return;
    }
    connect(&pView->mNotifier, &TNullAppFrontendNotifier::addonCommandsChanged, this, [this, run](const Host* pHost) {
        if (run != mRun || !mpHost || pHost != mpHost.data() || mAddonCommandsQueuedRun == run) {
            return;
        }
        mAddonCommandsQueuedRun = run;
        QMetaObject::invokeMethod(
                this,
                [this, run]() {
                    if (mAddonCommandsQueuedRun == run) {
                        mAddonCommandsQueuedRun = 0;
                    }
                    if (run == mRun) {
                        const QString profile = mProfileName;
                        emit addonCommandsChanged(profile);
                    }
                },
                Qt::QueuedConnection);
    });
}

void THeadlessSession::disconnectCommandRelays()
{
    if (auto* pView = dynamic_cast<TNullAppFrontend*>(TAppFrontend::instance())) {
        disconnect(&pView->mNotifier, nullptr, this, nullptr);
    }
}
