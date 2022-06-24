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

#ifndef ANNOUNCER_H
#define ANNOUNCER_H

#include <QWidget>
#include <QObject>
#include <QAccessible>
#include <QAccessibleInterface>
#include <QAccessibleWidget>

#if defined(Q_OS_LINUX)
class FakeNotification : public QWidget {
Q_OBJECT

public:
    Q_DISABLE_COPY(FakeNotification)
    FakeNotification(QWidget *parent);

    void setText(const QString &text);
    QString text();

private:
    QString mText;
};


// create a new class FakeStatusbar based on QWidget
class FakeStatusbar : public QWidget {
Q_OBJECT

public:
    Q_DISABLE_COPY(FakeStatusbar)
    FakeStatusbar(QWidget *parent);
};

class FakeAccessibleNotification: public QAccessibleWidget
{
public:
    explicit FakeAccessibleNotification(QWidget* w) : QAccessibleWidget(w, QAccessible::Role::Notification) { }

private:
    FakeNotification *notification() const;

protected:
    QString text(QAccessible::Text t) const override;

};

class FakeAccessibleStatusbar: public QAccessibleWidget
{
public:
    explicit FakeAccessibleStatusbar(QWidget* w) : QAccessibleWidget(w, QAccessible::Role::StatusBar) { }
};
#endif

class Announcer : public QWidget
{
Q_OBJECT
public:
    explicit Announcer(QWidget *parent = nullptr);
    void announce(QString text);

#if defined(Q_OS_LINUX)
    static QAccessibleInterface* accessibleFactory(const QString &classname, QObject *object)
    {
        QAccessibleInterface *interface = nullptr;

        if (classname == QLatin1String("FakeNotification") && object && object->isWidgetType()) {
            interface = new FakeAccessibleNotification(static_cast<QWidget *>(object));
        } else if (classname == QLatin1String("FakeStatusbar") && object && object->isWidgetType()) {
            interface = new FakeAccessibleStatusbar(static_cast<QWidget *>(object));
        }

        return interface;
    }
#endif

private:
#if defined(Q_OS_LINUX)
    FakeNotification* notification;
    FakeStatusbar* statusbar;
#endif
};
#endif // ANNOUNCER_H
