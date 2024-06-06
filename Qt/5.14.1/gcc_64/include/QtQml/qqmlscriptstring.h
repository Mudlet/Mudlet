/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLSCRIPTSTRING_H
#define QQMLSCRIPTSTRING_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QObject;
class QQmlContext;
class QQmlScriptStringPrivate;
class QQmlObjectCreator;
namespace QV4 {
    struct QObjectWrapper;
}
class Q_QML_EXPORT QQmlScriptString
{
public:
    QQmlScriptString();
    QQmlScriptString(const QQmlScriptString &);
    ~QQmlScriptString();

    QQmlScriptString &operator=(const QQmlScriptString &);

    bool operator==(const QQmlScriptString &) const;
    bool operator!=(const QQmlScriptString &) const;

    bool isEmpty() const;

    bool isUndefinedLiteral() const;
    bool isNullLiteral() const;
    QString stringLiteral() const;
    qreal numberLiteral(bool *ok) const;
    bool booleanLiteral(bool *ok) const;

private:
    QQmlScriptString(const QString &script, QQmlContext *context, QObject *scope);
    QSharedDataPointer<QQmlScriptStringPrivate> d;

    friend class QQmlObjectCreator;
    friend class QQmlScriptStringPrivate;
    friend class QQmlExpression;
    friend class QQmlBinding;
    friend struct QV4::QObjectWrapper;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlScriptString)

#endif // QQMLSCRIPTSTRING_H

