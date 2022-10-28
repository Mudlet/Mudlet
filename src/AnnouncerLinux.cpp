/***************************************************************************
*   Copyright (C) 2022 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QDebug>
#include <QAccessible>

InvisibleNotification::InvisibleNotification(QWidget *parent) : QWidget(parent) {
    setObjectName("InvisibleNotification");
    setAccessibleName("InvisibleNotification");
    setAccessibleDescription("An invisible widget used as a workaround to announce text to the screen reader");
}

void InvisibleNotification::setText(const QString &text) {
    this->mText = text;
}

QString InvisibleNotification::text() {
    return mText;
}

InvisibleNotification* InvisibleAccessibleNotification::notification() const
{
    return static_cast<InvisibleNotification*>(object());
}

QString InvisibleAccessibleNotification::text(QAccessible::Text t) const {
    Q_UNUSED(t)

    // return the notifications contents regardless of the request as part of the workaround
    return notification()->text();
}

InvisibleStatusbar::InvisibleStatusbar(QWidget *parent) : QWidget(parent) {
    setObjectName("InvisibleStatusbar");
    setAccessibleName("InvisibleStatusbar");
    setAccessibleDescription("An invisible widget used as part as a workaround to announce text to the screen reader");
}

Announcer::Announcer(QWidget *parent): QWidget{parent}, statusbar(new InvisibleStatusbar(this))
{
    notification = new InvisibleNotification(statusbar);
}

void Announcer::announce(const QString& text, const QString& processing)
{
    Q_UNUSED(processing);
    notification->setText(text);

    QAccessibleEvent event(notification, QAccessible::ObjectShow);
    QAccessible::updateAccessibility(&event);
}
