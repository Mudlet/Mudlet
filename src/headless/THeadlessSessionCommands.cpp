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
