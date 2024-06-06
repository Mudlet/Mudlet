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


#ifndef QPPSATTRIBUTE_P_H
#define QPPSATTRIBUTE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QList>
#include <QMap>
#include <QSharedDataPointer>
#include <QVariant>

QT_BEGIN_NAMESPACE

class QPpsAttributePrivate;
class QPpsAttribute;

typedef QList<QPpsAttribute> QPpsAttributeList;
typedef QMap<QString, QPpsAttribute> QPpsAttributeMap;

class Q_CORE_EXPORT QPpsAttribute
{
public:

    enum Type {
        None   = 0,
        Number = 1,
        Bool   = 2,
        String = 3,
        Array  = 4,
        Object = 5
    };

    enum Flag {
        Incomplete = 0x01,
        Deleted    = 0x02,
        Created    = 0x04,
        Truncated  = 0x08,
        Purged     = 0x10
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QPpsAttribute();
    QPpsAttribute(const QPpsAttribute &other);
    ~QPpsAttribute();

    QPpsAttribute &operator=(const QPpsAttribute &other);
    bool operator==(const QPpsAttribute &other) const;
    bool operator!=(const QPpsAttribute &other) const;

    QPpsAttribute(QPpsAttribute &&other);
    QPpsAttribute &operator=(QPpsAttribute &&other);

    bool isValid() const;
    Type type() const;
    QPpsAttribute::Flags flags() const;

    bool isNumber() const;
    bool isBool() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    double toDouble() const;
    qlonglong toLongLong() const;
    int toInt() const;
    bool toBool() const;
    QString toString() const;
    QPpsAttributeList toList() const;
    QPpsAttributeMap toMap() const;
    QVariant toVariant() const;

private:
    QSharedDataPointer<QPpsAttributePrivate> d;
    friend class QPpsAttributePrivate;
};

inline bool QPpsAttribute::operator!=(const QPpsAttribute &other) const
{
    return !(*this == other);
}

Q_CORE_EXPORT QDebug operator<<(QDebug dbg, const QPpsAttribute &attribute);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QPpsAttributeList)
Q_DECLARE_METATYPE(QPpsAttributeMap)

#endif // QPPSATTRIBUTE_P_H
