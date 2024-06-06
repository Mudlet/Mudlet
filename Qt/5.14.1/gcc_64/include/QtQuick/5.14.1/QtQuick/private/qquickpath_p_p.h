/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#ifndef QQUICKPATH_P_H
#define QQUICKPATH_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_path);

#include "qquickpath_p.h"

#include <qqml.h>
#include <QtCore/QStringList>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickPathPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPath)

public:
    static QQuickPathPrivate* get(QQuickPath *path) { return path->d_func(); }
    static const QQuickPathPrivate* get(const QQuickPath *path) { return path->d_func(); }

    QQuickPathPrivate() {}

    QPainterPath _path;
    QList<QQuickPathElement*> _pathElements;
    mutable QVector<QPointF> _pointCache;
    QList<QQuickPath::AttributePoint> _attributePoints;
    QStringList _attributes;
    QList<QQuickCurve*> _pathCurves;
    mutable QQuickCachedBezier prevBez;
    QQmlNullableValue<qreal> startX;
    QQmlNullableValue<qreal> startY;
    qreal pathLength = 0;
    QSizeF scale = QSizeF(1, 1);
    bool closed = false;
    bool componentComplete = true;
    bool isShapePath = false;
};

QT_END_NAMESPACE

#endif
