/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QNETCONMONITOR_P_H
#define QNETCONMONITOR_P_H

#include <private/qtnetworkglobal_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qhostaddress.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QNetworkConnectionMonitorPrivate;
class Q_AUTOTEST_EXPORT QNetworkConnectionMonitor : public QObject
{
    Q_OBJECT

public:
    QNetworkConnectionMonitor();
    QNetworkConnectionMonitor(const QHostAddress &local, const QHostAddress &remote = {});
    ~QNetworkConnectionMonitor();

    bool setTargets(const QHostAddress &local, const QHostAddress &remote);
    bool isReachable();

    // Important: on Darwin you should not call isReachable() after
    // startMonitoring(), you have to listen to reachabilityChanged()
    // signal instead.
    bool startMonitoring();
    bool isMonitoring() const;
    void stopMonitoring();

Q_SIGNALS:
    // Important: connect to this using QueuedConnection. On Darwin
    // callback is coming on a special dispatch queue.
    void reachabilityChanged(bool isOnline);

private:
    Q_DECLARE_PRIVATE(QNetworkConnectionMonitor)
    Q_DISABLE_COPY_MOVE(QNetworkConnectionMonitor)
};

class QNetworkStatusMonitorPrivate;
class Q_AUTOTEST_EXPORT QNetworkStatusMonitor : public QObject
{
    Q_OBJECT

public:
    QNetworkStatusMonitor();
    ~QNetworkStatusMonitor();

    bool isNetworkAccessible();

    bool start();
    void stop();
    bool isMonitoring() const;

    static bool isEnabled();

Q_SIGNALS:
    // Unlike QNetworkConnectionMonitor, this can be connected to directly.
    void onlineStateChanged(bool isOnline);

private slots:
    void reachabilityChanged(bool isOnline);

private:
    Q_DECLARE_PRIVATE(QNetworkStatusMonitor)
    Q_DISABLE_COPY_MOVE(QNetworkStatusMonitor)
};

Q_DECLARE_LOGGING_CATEGORY(lcNetMon)

QT_END_NAMESPACE

#endif // QNETCONMONITOR_P_H
