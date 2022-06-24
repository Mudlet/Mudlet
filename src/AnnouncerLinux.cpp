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
#include <QAccessibleWidget>

FakeNotification::FakeNotification(QWidget *parent) : QWidget(parent) {
    setObjectName("FakeNotification");
    setAccessibleName("FakeNotification");
    setAccessibleDescription("FakeNotification");
}

void FakeNotification::setText(const QString &text) {
    this->mText = text;
}

QString FakeNotification::text() {
    return mText;
}

FakeNotification* FakeAccessibleNotification::notification() const
{
    return static_cast<FakeNotification*>(object());
}

QString FakeAccessibleNotification::text(QAccessible::Text t) const {
    return notification()->text();
}

FakeStatusbar::FakeStatusbar(QWidget *parent) : QWidget(parent) {
    setObjectName("FakeStatusbar");
    setAccessibleName("FakeStatusbar");
    setAccessibleDescription("FakeStatusbar");
}

Announcer::Announcer(QWidget *parent): QWidget{parent}
{
#if defined(Q_OS_LINUX)
    statusbar = new FakeStatusbar(this);
    notification = new FakeNotification(statusbar);
#endif
}

void Announcer::announce(QString text)
{
    qDebug() << "announcing" << text;
    notification->setText(text);

    QAccessibleEvent event(notification, QAccessible::ObjectShow);
    QAccessible::updateAccessibility(&event);

    // https://github.com/mozilla/gecko-dev/blob/master/accessible/atk/AccessibleWrap.cpp#L1119
    //case nsIAccessibleEvent::EVENT_ALERT:
    //    // A hack using state change showing events as alert events.
    //    atk_object_notify_state_change(atkObj, ATK_STATE_SHOWING, true);
    //    break;


//    "announcement",                     // EVENT_ANNOUNCEMENT
//            "live region added",                // EVENT_LIVE_REGION_ADDED
//            "live region removed",              // EVENT_LIVE_REGION_REMOVED

    // https://website-archive.mozilla.org/www.mozilla.org/access/access/unix/new-atk.html

    // android: https://developer.android.com/reference/android/view/accessibility/AccessibilityEvent#TYPE_ANNOUNCEMENT

}
