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

#include "ActionUnit.h"
#include "Host.h"
#include "TAction.h"
#include "TNullAppFrontend.h"

#include <QKeyEvent>
#include <QScopeGuard>

#include <algorithm>
#include <optional>

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

namespace {

THeadlessSession::Button buttonOf(const TAction& action)
{
    THeadlessSession::Button button;
    button.id = action.getID();
    button.name = action.getName();
    button.icon = action.getIcon();
    button.pushDown = action.isPushDownButton();
    button.checked = button.pushDown && action.mButtonState;
    button.menu = action.isFolder();
    return button;
}

// As TEasyButtonBar::fillMenu(): the folder's active children, each with the folder's icon
QList<THeadlessSession::Button> buttonBarMenu(const TAction& folder)
{
    QList<THeadlessSession::Button> entries;
    for (auto* pNode : *folder.mpMyChildrenList) {
        const auto& child = *static_cast<TAction*>(pNode);
        if (!child.isActive()) {
            continue;
        }
        THeadlessSession::Button entry = buttonOf(child);
        entry.icon = folder.getIcon();
        if (entry.menu) {
            entry.children = buttonBarMenu(child);
        }
        entries.append(entry);
    }
    return entries;
}

// As TToolBar::addActionToMenu(): an entry for the folder itself, whose menu holds every child, active or not
THeadlessSession::Button toolBarMenuEntry(const TAction& action)
{
    THeadlessSession::Button entry = buttonOf(action);
    if (entry.menu) {
        for (auto* pNode : *action.mpMyChildrenList) {
            entry.children.append(toolBarMenuEntry(*static_cast<TAction*>(pNode)));
        }
    }
    return entry;
}

// As TMainConsole::constructToolbar() fills a bar: nothing for an inactive one, nor for a button bar in a strip
// attachEasyButtonBar() has none for
std::optional<THeadlessSession::ButtonBar> barOf(const TAction& action, const bool floating)
{
    using Place = THeadlessSession::ButtonBar::Place;
    if (!action.isActive()) {
        return std::nullopt;
    }
    THeadlessSession::ButtonBar bar;
    if (floating) {
        bar.place = Place::floating;
    } else if (action.mLocation == 0) {
        bar.place = Place::top;
    } else if (action.mLocation == 2) {
        bar.place = Place::left;
    } else if (action.mLocation == 3) {
        bar.place = Place::right;
    } else {
        return std::nullopt;
    }
    bar.id = action.getID();
    bar.name = action.getName();
    bar.styleSheet = action.css;
    bar.orientation = action.mOrientation == 1 ? Qt::Vertical : Qt::Horizontal;
    bar.columns = std::max(1, action.getButtonColumns());
    bar.fillerOffset = action.getButtonFillerOffset();
    for (auto* pNode : *action.mpMyChildrenList) {
        const auto& child = *static_cast<TAction*>(pNode);
        if (!child.isActive()) {
            continue;
        }
        THeadlessSession::Button button = buttonOf(child);
        button.styleSheet = child.css;
        if (button.menu) {
            button.children = floating ? QList<THeadlessSession::Button>{toolBarMenuEntry(child)} : buttonBarMenu(child);
        }
        bar.buttons.append(button);
    }
    return bar;
}

bool listsButton(const QList<THeadlessSession::Button>& buttons, const int id)
{
    return std::any_of(buttons.cbegin(), buttons.cend(), [id](const auto& button) {
        return button.id == id || listsButton(button.children, id);
    });
}

} // namespace

QList<THeadlessSession::ButtonBar> THeadlessSession::buttonBars() const
{
    QList<ButtonBar> result;
    if (!mpHost) {
        return result;
    }
    auto add = [&result](const TAction& action, const bool floating) {
        if (auto bar = barOf(action, floating)) {
            result.append(*bar);
        }
    };
    // As TMainConsole::regenerateToolBars() and regenerateEasyButtonBars(): a package's root holds its bars, of which
    // only those that float are filled while the root floats, and only those that do not while it does not
    for (TAction* pRoot : mpHost->getActionUnit()->getActionRootNodeList()) {
        if (pRoot->mPackageName.isEmpty()) {
            add(*pRoot, pRoot->mLocation == 4);
            continue;
        }
        for (auto* pNode : *pRoot->mpMyChildrenList) {
            const auto& child = *static_cast<TAction*>(pNode);
            if (pRoot->mLocation == 4) {
                if (child.mLocation == 4) {
                    add(child, true);
                }
            } else if (child.mLocation != 4) {
                add(child, false);
            }
        }
    }
    return result;
}

bool THeadlessSession::pressButton(const int id)
{
    if (!usable()) {
        return false;
    }
    const QList<ButtonBar> bars = buttonBars();
    ActionUnit* pUnit = mpHost->getActionUnit();
    TAction* pAction = pUnit->getAction(id);
    if (!pAction || std::none_of(bars.cbegin(), bars.cend(), [id](const auto& bar) {
            return listsButton(bar.buttons, id);
        })) {
        //: Error from a frontend with no window that was asked to press a button its profile does not show, giving the button's id
        mErrorString = tr("The profile shows no button %1.").arg(id);
        return false;
    }
    // As the GUI's bars hold off ActionUnit::uninstall() while a button's script runs
    pUnit->beginProcessing();
    const auto processingGuard = qScopeGuard([pUnit] {
        pUnit->endProcessing();
        pUnit->doCleanup();
    });
    pAction->press(pAction->isPushDownButton() && !pAction->mButtonState);
    return true;
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
    if (mpHost) {
        connect(mpHost.data(), &Host::signal_buttonsChanged, this, [this, run]() {
            if (run != mRun || !mpHost || mButtonsQueuedRun == run) {
                return;
            }
            mButtonsQueuedRun = run;
            QMetaObject::invokeMethod(
                    this,
                    [this, run]() {
                        if (mButtonsQueuedRun == run) {
                            mButtonsQueuedRun = 0;
                        }
                        if (run == mRun) {
                            const QString profile = mProfileName;
                            emit buttonsChanged(profile);
                        }
                    },
                    Qt::QueuedConnection);
        });
    }
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
