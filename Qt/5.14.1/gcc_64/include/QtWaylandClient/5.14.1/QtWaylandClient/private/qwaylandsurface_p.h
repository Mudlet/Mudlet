/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the config.tests of the Qt Toolkit.
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

#ifndef QWAYLANDSURFACE_P_H
#define QWAYLANDSURFACE_P_H

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

#include <QtGui/QScreen>

#include <QtWaylandClient/private/qwayland-wayland.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandScreen;
class QWaylandWindow;
class QWaylandDisplay;

class QWaylandSurface : public QObject, public QtWayland::wl_surface
{
    Q_OBJECT
public:
    explicit QWaylandSurface(QWaylandDisplay *display);
    ~QWaylandSurface() override;
    QWaylandScreen *oldestEnteredScreen();
    QWaylandWindow *waylandWindow() const { return m_window; }

    static QWaylandSurface *fromWlSurface(::wl_surface *surface);

signals:
    void screensChanged();

private slots:
    void handleScreenRemoved(QScreen *qScreen);

protected:
    void surface_enter(struct ::wl_output *output) override;
    void surface_leave(struct ::wl_output *output) override;

    QVector<QWaylandScreen *> m_screens; //As seen by wl_surface.enter/leave events. Chronological order.
    QWaylandWindow *m_window = nullptr;

    friend class QWaylandWindow; // TODO: shouldn't need to be friends
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDSURFACE_P_H
