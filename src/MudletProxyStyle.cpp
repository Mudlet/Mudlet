/***************************************************************************
 *   Copyright (C) 2020 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "MudletProxyStyle.h"

#include <QStyleFactory>

#if defined(Q_OS_WIN)
#include <QCoreApplication>
#include <windows.h>
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>

#include "utils.h"
#endif

namespace {
#if defined(Q_OS_WIN)
bool screenReaderFlagSet()
{
    BOOL running = FALSE;
    return SystemParametersInfoW(SPI_GETSCREENREADER, 0, &running, 0) && running;
}
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
const QString csmA11yService = qsl("org.a11y.Bus");
const QString csmA11yPath = qsl("/org/a11y/bus");
const QString csmA11yStatusInterface = qsl("org.a11y.Status");
const QString csmPropertiesInterface = qsl("org.freedesktop.DBus.Properties");
const QString csmScreenReaderEnabled = qsl("ScreenReaderEnabled");
#endif
} // namespace

MudletProxyStyle::MudletProxyStyle()
{
    setObjectName(baseStyle()->objectName());
    watchScreenReaderStatus();
}

MudletProxyStyle::MudletProxyStyle(const QString& style)
: QProxyStyle(QStyleFactory::create(style))
{
    watchScreenReaderStatus();
}

void MudletProxyStyle::watchScreenReaderStatus()
{
#if defined(Q_OS_WIN)
    mScreenReaderRunning = screenReaderFlagSet();
    QCoreApplication::instance()->installNativeEventFilter(this);
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return;
    }

    bus.connect(csmA11yService, csmA11yPath, csmPropertiesInterface, qsl("PropertiesChanged"), this, SLOT(slot_a11yStatusChanged(QString, QVariantMap, QStringList)));

    // Asynchronous, so a slow or missing accessibility bus cannot stall startup
    QDBusMessage request = QDBusMessage::createMethodCall(csmA11yService, csmA11yPath, csmPropertiesInterface, qsl("Get"));
    request << csmA11yStatusInterface << csmScreenReaderEnabled;
    auto* watcher = new QDBusPendingCallWatcher(bus.asyncCall(request), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* call) {
        const QDBusPendingReply<QDBusVariant> reply = *call;
        if (!reply.isError()) {
            mScreenReaderRunning = reply.value().variant().toBool();
        }
        call->deleteLater();
    });
#endif
}

// Only installed on Windows, where screen readers announce themselves by
// setting SPI_SETSCREENREADER and broadcasting WM_SETTINGCHANGE
bool MudletProxyStyle::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(result)
#if defined(Q_OS_WIN)
    if (eventType == "windows_generic_MSG") {
        const auto* msg = static_cast<const MSG*>(message);
        if (msg->message == WM_SETTINGCHANGE && msg->wParam == SPI_SETSCREENREADER) {
            mScreenReaderRunning = screenReaderFlagSet();
        }
    }
#else
    Q_UNUSED(eventType)
    Q_UNUSED(message)
#endif
    return false;
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
void MudletProxyStyle::slot_a11yStatusChanged(const QString& interfaceName, const QVariantMap& changedProperties, const QStringList& invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties)
    if (interfaceName == csmA11yStatusInterface && changedProperties.contains(csmScreenReaderEnabled)) {
        mScreenReaderRunning = changedProperties.value(csmScreenReaderEnabled).toBool();
    }
}
#endif

int MudletProxyStyle::styleHint(StyleHint styleHint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (styleHint == QStyle::SH_MenuBar_AltKeyNavigation) {
        // A lone Alt tap focuses the menu bar only for screen reader users
        // (Mudlet/Mudlet#6145). Everyone else keeps focus on the command line
        // when an Alt-based keybinding is mistimed (Mudlet/Mudlet#4280)
        return mScreenReaderRunning ? 1 : 0;
    }
    if (styleHint == QStyle::SH_ItemView_ActivateItemOnSingleClick) {
        return 0;
    }

    return QProxyStyle::styleHint(styleHint, opt, widget, returnData);
}
