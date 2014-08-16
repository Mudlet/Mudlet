/****************************************************************************
 *  unitylauncher.cpp
 *
 *  Copyright (c) 2011 by Sidorov Aleksey <gorthauer87@ya.ru>
 *  Copyright (c) 2011 Vsevolod Velichko <torkvema@gmail.com>
 *
 ***************************************************************************
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
*****************************************************************************/

#include "qtdocktile.h"
#include "qtdocktile_p.h"
#include <QApplication>
#include <QDBusMessage>
#include <QDBusConnection>

static QString appDesktopUri()
{
    return QString("application://%1.desktop").arg(qAppName());
}

static QString appUri()
{
    return QString("/%1").arg(qAppName());
}

static void sendMessage(const QVariantMap &map)
{
    QDBusMessage message = QDBusMessage::createSignal(appUri(), "com.canonical.Unity.LauncherEntry", "Update");
    QVariantList args;
    args << appDesktopUri() << map;
    message.setArguments(args);
    if (!QDBusConnection::sessionBus().send(message))
        qWarning("QtDockTile: unable to send DBus message");
}

bool QtDockTile::isAvailable()
{
    return true; //TODO check com.canonical.Unity interface
}

void QtDockTilePrivate::setBadge(int badge)
{
    QVariantMap map;
    map.insert(QLatin1String("count"), badge);
    map.insert(QLatin1String("count-visible"), badge > 0);
    sendMessage(map);
}

void QtDockTilePrivate::setProgress(int progress)
{
    QVariantMap map;
    map.insert(QLatin1String("progress"), static_cast<double>(progress)/100.0);
    map.insert(QLatin1String("progress-visible"), progress > 0);
    sendMessage(map);
}
