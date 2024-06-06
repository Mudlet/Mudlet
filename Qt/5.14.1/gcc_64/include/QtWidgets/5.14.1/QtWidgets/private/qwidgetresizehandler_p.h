/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QWIDGETRESIZEHANDLER_P_H
#define QWIDGETRESIZEHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtCore/qobject.h"
#include "QtCore/qpoint.h"

QT_REQUIRE_CONFIG(resizehandler);

QT_BEGIN_NAMESPACE

class QMouseEvent;
class QKeyEvent;

class Q_WIDGETS_EXPORT QWidgetResizeHandler : public QObject
{
    Q_OBJECT

public:
    enum Action {
        Move        = 0x01,
        Resize        = 0x02,
        Any        = Move|Resize
    };

    explicit QWidgetResizeHandler(QWidget *parent, QWidget *cw = nullptr);
    void setActive(bool b) { setActive(Any, b); }
    void setActive(Action ac, bool b);
    bool isActive() const { return isActive(Any); }
    bool isActive(Action ac) const;
    void setMovingEnabled(bool b) { movingEnabled = b; }
    bool isMovingEnabled() const { return movingEnabled; }

    bool isButtonDown() const { return buttonDown; }

    void setExtraHeight(int h) { extrahei = h; }
    void setSizeProtection(bool b) { sizeprotect = b; }

    void setFrameWidth(int w) { fw = w; }

    void doResize();
    void doMove();

Q_SIGNALS:
    void activate();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);

private:
    Q_DISABLE_COPY_MOVE(QWidgetResizeHandler)

    enum MousePosition {
        Nowhere,
        TopLeft, BottomRight, BottomLeft, TopRight,
        Top, Bottom, Left, Right,
        Center
    };

    QWidget *widget;
    QWidget *childWidget;
    QPoint moveOffset;
    QPoint invertedMoveOffset;
    MousePosition mode;
    int fw;
    int extrahei;
    int range;
    uint buttonDown            :1;
    uint moveResizeMode            :1;
    uint activeForResize    :1;
    uint sizeprotect            :1;
    uint movingEnabled                    :1;
    uint activeForMove            :1;

    void setMouseCursor(MousePosition m);
    bool isMove() const {
        return moveResizeMode && mode == Center;
    }
    bool isResize() const {
        return moveResizeMode && !isMove();
    }
};

QT_END_NAMESPACE

#endif // QWIDGETRESIZEHANDLER_P_H
