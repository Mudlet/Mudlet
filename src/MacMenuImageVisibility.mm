/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "MacMenuImageVisibility.h"

#include <QMenu>
#include <QMenuBar>

#import <AppKit/AppKit.h>

// macOS 27 hides a menu item's image unless the item asks to show it, and Qt 6.11 never asks
static void setImageVisibility(NSMenu* menu, const bool visible) API_AVAILABLE(macos(27.0))
{
    for (NSMenuItem* item in menu.itemArray) {
#if defined(__MAC_27_0) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_27_0
        item.preferredImageVisibility = visible ? NSMenuItemImageVisibilityVisible : NSMenuItemImageVisibilityAutomatic;
#else
        // An older SDK does not declare it; Visible is 1 and Automatic 0
        [item setValue:@(visible ? 1 : 0) forKey:@"preferredImageVisibility"];
#endif
        if (item.submenu) {
            setImageVisibility(item.submenu, visible);
        }
    }
}

void setMacMenuImagesAlwaysVisible(QMenuBar* pMenuBar, const bool visible)
{
    if (!pMenuBar) {
        return;
    }
    if (@available(macOS 27.0, *)) {
        // Through each QMenu: the bar's own items have no submenus until the bar is first the main menu
        for (QAction* pAction : pMenuBar->actions()) {
            if (QMenu* pMenu = pAction->menu()) {
                if (NSMenu* menu = pMenu->toNSMenu()) {
                    setImageVisibility(menu, visible);
                }
            }
        }
        // Qt moves About, Preferences, Quit and the application-specific entries out of their QMenus into the
        // application menu, the main menu's first item
        if (NSMenu* applicationMenu = NSApp.mainMenu.itemArray.firstObject.submenu) {
            setImageVisibility(applicationMenu, visible);
        }
    }
}
