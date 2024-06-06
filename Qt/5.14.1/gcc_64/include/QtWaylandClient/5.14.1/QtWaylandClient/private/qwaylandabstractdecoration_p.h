/****************************************************************************
**
** Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWAYLANDABSTRACTDECORATION_H
#define QWAYLANDABSTRACTDECORATION_H

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

#include <QtCore/QMargins>
#include <QtCore/QPointF>
#include <QtGui/QGuiApplication>
#include <QtGui/QCursor>
#include <QtGui/QColor>
#include <QtGui/QStaticText>
#include <QtGui/QImage>
#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

class QWindow;
class QPaintDevice;
class QPainter;
class QEvent;

namespace QtWaylandClient {

class QWaylandScreen;
class QWaylandWindow;
class QWaylandInputDevice;
class QWaylandAbstractDecorationPrivate;

class Q_WAYLAND_CLIENT_EXPORT QWaylandAbstractDecoration : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandAbstractDecoration)
public:
    QWaylandAbstractDecoration();
    ~QWaylandAbstractDecoration() override;

    void setWaylandWindow(QWaylandWindow *window);
    QWaylandWindow *waylandWindow() const;

    void update();
    bool isDirty() const;

    virtual QMargins margins() const = 0;
    QWindow *window() const;
    const QImage &contentImage();

    virtual bool handleMouse(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global,Qt::MouseButtons b,Qt::KeyboardModifiers mods) = 0;
    virtual bool handleTouch(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global, Qt::TouchPointState state, Qt::KeyboardModifiers mods) = 0;

protected:
    virtual void paint(QPaintDevice *device) = 0;

    void setMouseButtons(Qt::MouseButtons mb);

    void startResize(QWaylandInputDevice *inputDevice, Qt::Edges edges, Qt::MouseButtons buttons);
    void startMove(QWaylandInputDevice *inputDevice, Qt::MouseButtons buttons);
    void showWindowMenu(QWaylandInputDevice *inputDevice);

    bool isLeftClicked(Qt::MouseButtons newMouseButtonState);
    bool isRightClicked(Qt::MouseButtons newMouseButtonState);
    bool isLeftReleased(Qt::MouseButtons newMouseButtonState);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDABSTRACTDECORATION_H
