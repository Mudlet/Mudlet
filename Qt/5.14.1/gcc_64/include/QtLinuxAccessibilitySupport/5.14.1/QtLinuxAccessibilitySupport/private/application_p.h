/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef Q_SPI_APPLICATION_H
#define Q_SPI_APPLICATION_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtDBus/QDBusConnection>
#include <QtGui/QAccessibleInterface>

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

/*
 * Used for the root object.
 *
 * Uses the root object reference and reports its parent as the desktop object.
 */
class QSpiApplicationAdaptor :public QObject
{
    Q_OBJECT

public:
    QSpiApplicationAdaptor(const QDBusConnection &connection, QObject *parent);
    virtual ~QSpiApplicationAdaptor() {}
    void sendEvents(bool active);

Q_SIGNALS:
    void windowActivated(QObject* window, bool active);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void notifyKeyboardListenerCallback(const QDBusMessage& message);
    void notifyKeyboardListenerError(const QDBusError& error, const QDBusMessage& message);

private:
    static QKeyEvent* copyKeyEvent(QKeyEvent*);

    QQueue<QPair<QPointer<QObject>, QKeyEvent*> > keyEvents;
    QDBusConnection dbusConnection;
    bool inCapsLock;
};

QT_END_NAMESPACE

#endif
