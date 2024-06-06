/****************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QPPSOBJECT_P_H
#define QPPSOBJECT_P_H

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

#include "qppsattribute_p.h"

#include <QMap>
#include <QObject>
#include <QVariantMap>

QT_BEGIN_NAMESPACE

class QPpsObjectPrivate;

class Q_CORE_EXPORT QPpsObject : public QObject
{
    Q_OBJECT

public:
    enum OpenMode {
        Publish = 1,
        Subscribe = 2,
        PublishSubscribe = Publish | Subscribe,
        Create = 4,
        DeleteContents = 8
    };
    Q_DECLARE_FLAGS(OpenModes, OpenMode)

    explicit QPpsObject(const QString &path, QObject *parent = 0);
    virtual ~QPpsObject();

    int error() const;
    QString errorString() const;

    bool isReadyReadEnabled() const;
    bool isBlocking() const;
    bool setBlocking(bool enable);
    bool isOpen() const;

    bool open(QPpsObject::OpenModes mode = QPpsObject::PublishSubscribe);
    bool close();
    bool remove();

    QByteArray read(bool *ok = 0);
    bool write(const QByteArray &byteArray);

    int writeMessage(const QString &msg, const QVariantMap &dat);
    int writeMessage(const QString &msg, const QString &id, const QVariantMap &dat);

    static QVariantMap decode(const QByteArray &rawData, bool *ok = 0);
    static QPpsAttributeMap decodeWithFlags(const QByteArray &rawData, bool *ok = 0);
    static QPpsAttributeMap decodeWithFlags(const QByteArray &rawData,
                                            QPpsAttribute *objectAttribute, bool *ok = 0);

    static QByteArray encode(const QVariantMap &ppsData, bool *ok = 0);
    static QByteArray encodeMessage(const QString &msg, const QVariantMap &dat, bool *ok = 0);
    static QByteArray encodeMessage(const QString &msg, const QString &id, const QVariantMap &dat,
                                    bool *ok = 0);

    static int sendMessage(const QString &path, const QString &message);
    static int sendMessage(const QString &path, const QVariantMap &message);
    static int sendMessage(const QString &path, const QString &msg, const QVariantMap &dat);
    static int sendMessage(const QString &path, const QByteArray &ppsData);

public Q_SLOTS:
    void setReadyReadEnabled(bool enable);

Q_SIGNALS:
    void readyRead();

private:
    Q_DECLARE_PRIVATE(QPpsObject)
    Q_DISABLE_COPY_MOVE(QPpsObject)
};

QT_END_NAMESPACE

#endif // QPPSOBJECT_P_H
