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

#ifndef QQUICKSPRITESEQUENCE_P_P_H
#define QQUICKSPRITESEQUENCE_P_P_H

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

QT_REQUIRE_CONFIG(quick_sprite);

#include "qquickitem_p.h"
#include "qquicksprite_p.h"
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE

class QQuickSpriteSequence;

class QQuickSpriteSequencePrivate :public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickSpriteSequence)
public:
    QQuickSpriteSequencePrivate()
        : m_spriteEngine(nullptr)
        , m_curFrame(0)
        , m_pleaseReset(false)
        , m_running(true)
        , m_interpolate(true)
        , m_curStateIdx(0)
    {

    }
    QList<QQuickSprite*> m_sprites;
    QQuickSpriteEngine* m_spriteEngine;
    QElapsedTimer m_timestamp;
    int m_curFrame;
    bool m_pleaseReset;
    bool m_running;
    bool m_interpolate;
    QString m_goalState;
    QString m_curState;
    int m_curStateIdx;
    QSize m_sheetSize;
};

QT_END_NAMESPACE

#endif // QQUICKSPRITESEQUENCE_P_P_H
