/***************************************************************************
 *   Copyright (C) 2022 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *   Copyright (C) 2023 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "Announcer.h"

#include <AppKit/AppKit.h>
#include <QDebug>

Announcer::Announcer(QWidget *parent)
: QWidget{parent}
{
    // Needed to prevent this (invisible) widget from being seen by itself in
    // the top left corner of the main application window where it masks part of
    // the main menu bar:
    setVisible(false);
}

void Announcer::announce(const QString& text, const QString& processing)
{
    Q_UNUSED(processing)
    NSDictionary *announcementInfo = @{
        NSAccessibilityAnnouncementKey : text.toNSString(),
        NSAccessibilityPriorityKey : @(NSAccessibilityPriorityHigh),
    };
    NSAccessibilityPostNotificationWithUserInfo([NSApp mainWindow], NSAccessibilityAnnouncementRequestedNotification, announcementInfo);
}
