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

#ifndef DIRECTEDVECTOR_H
#define DIRECTEDVECTOR_H

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
#include "qquickdirection_p.h"
QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickTargetDirection : public QQuickDirection
{
    Q_OBJECT
    Q_PROPERTY(qreal targetX READ targetX WRITE setTargetX NOTIFY targetXChanged)
    Q_PROPERTY(qreal targetY READ targetY WRITE setTargetY NOTIFY targetYChanged)
    //If targetItem is set, X/Y are ignored. Aims at middle of item, use variation for variation
    Q_PROPERTY(QQuickItem* targetItem READ targetItem WRITE setTargetItem NOTIFY targetItemChanged)

    Q_PROPERTY(qreal targetVariation READ targetVariation WRITE setTargetVariation NOTIFY targetVariationChanged)

    //TODO: An enum would be better
    Q_PROPERTY(bool proportionalMagnitude READ proportionalMagnitude WRITE setProportionalMagnitude NOTIFY proprotionalMagnitudeChanged)
    Q_PROPERTY(qreal magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal magnitudeVariation READ magnitudeVariation WRITE setMagnitudeVariation NOTIFY magnitudeVariationChanged)

public:
    explicit QQuickTargetDirection(QObject *parent = 0);
    QPointF sample(const QPointF &from) override;

    qreal targetX() const
    {
        return m_targetX;
    }

    qreal targetY() const
    {
        return m_targetY;
    }

    qreal targetVariation() const
    {
        return m_targetVariation;
    }

    qreal magnitude() const
    {
        return m_magnitude;
    }

    bool proportionalMagnitude() const
    {
        return m_proportionalMagnitude;
    }

    qreal magnitudeVariation() const
    {
        return m_magnitudeVariation;
    }

    QQuickItem* targetItem() const
    {
        return m_targetItem;
    }

Q_SIGNALS:

    void targetXChanged(qreal arg);

    void targetYChanged(qreal arg);

    void targetVariationChanged(qreal arg);

    void magnitudeChanged(qreal arg);

    void proprotionalMagnitudeChanged(bool arg);

    void magnitudeVariationChanged(qreal arg);

    void targetItemChanged(QQuickItem* arg);

public Q_SLOTS:
    void setTargetX(qreal arg)
    {
        if (m_targetX != arg) {
            m_targetX = arg;
            Q_EMIT targetXChanged(arg);
        }
    }

    void setTargetY(qreal arg)
    {
        if (m_targetY != arg) {
            m_targetY = arg;
            Q_EMIT targetYChanged(arg);
        }
    }

    void setTargetVariation(qreal arg)
    {
        if (m_targetVariation != arg) {
            m_targetVariation = arg;
            Q_EMIT targetVariationChanged(arg);
        }
    }

    void setMagnitude(qreal arg)
    {
        if (m_magnitude != arg) {
            m_magnitude = arg;
            Q_EMIT magnitudeChanged(arg);
        }
    }

    void setProportionalMagnitude(bool arg)
    {
        if (m_proportionalMagnitude != arg) {
            m_proportionalMagnitude = arg;
            Q_EMIT proprotionalMagnitudeChanged(arg);
        }
    }

    void setMagnitudeVariation(qreal arg)
    {
        if (m_magnitudeVariation != arg) {
            m_magnitudeVariation = arg;
            Q_EMIT magnitudeVariationChanged(arg);
        }
    }

    void setTargetItem(QQuickItem* arg)
    {
        if (m_targetItem != arg) {
            m_targetItem = arg;
            Q_EMIT targetItemChanged(arg);
        }
    }

private:
    qreal m_targetX;
    qreal m_targetY;
    qreal m_targetVariation;
    bool m_proportionalMagnitude;
    qreal m_magnitude;
    qreal m_magnitudeVariation;
    QQuickItem *m_targetItem;
};

QT_END_NAMESPACE
#endif // DIRECTEDVECTOR_H
