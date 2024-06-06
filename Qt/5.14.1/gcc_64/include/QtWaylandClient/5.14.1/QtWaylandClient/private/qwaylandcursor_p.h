/****************************************************************************
**
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

#ifndef QWAYLANDCURSOR_H
#define QWAYLANDCURSOR_H

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

#include <qpa/qplatformcursor.h>
#include <QtCore/QMap>
#include <QtWaylandClient/qtwaylandclientglobal.h>

#if QT_CONFIG(cursor)

struct wl_cursor;
struct wl_cursor_image;
struct wl_cursor_theme;

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandBuffer;
class QWaylandDisplay;
class QWaylandScreen;
class QWaylandShm;

class Q_WAYLAND_CLIENT_EXPORT QWaylandCursorTheme
{
public:
    static QWaylandCursorTheme *create(QWaylandShm *shm, int size, const QString &themeName);
    ~QWaylandCursorTheme();
    ::wl_cursor *cursor(Qt::CursorShape shape);

private:
    enum WaylandCursor {
        ArrowCursor = Qt::ArrowCursor,
        UpArrowCursor,
        CrossCursor,
        WaitCursor,
        IBeamCursor,
        SizeVerCursor,
        SizeHorCursor,
        SizeBDiagCursor,
        SizeFDiagCursor,
        SizeAllCursor,
        BlankCursor,
        SplitVCursor,
        SplitHCursor,
        PointingHandCursor,
        ForbiddenCursor,
        WhatsThisCursor,
        BusyCursor,
        OpenHandCursor,
        ClosedHandCursor,
        DragCopyCursor,
        DragMoveCursor,
        DragLinkCursor,
        // The following are used for cursors that don't have equivalents in Qt
        ResizeNorthCursor = Qt::CustomCursor + 1,
        ResizeSouthCursor,
        ResizeEastCursor,
        ResizeWestCursor,
        ResizeNorthWestCursor,
        ResizeSouthEastCursor,
        ResizeNorthEastCursor,
        ResizeSouthWestCursor
    };

    explicit QWaylandCursorTheme(struct ::wl_cursor_theme *theme) : m_theme(theme) {}
    struct ::wl_cursor *requestCursor(WaylandCursor shape);
    struct ::wl_cursor_theme *m_theme = nullptr;
    QMap<WaylandCursor, wl_cursor *> m_cursors;
};

class Q_WAYLAND_CLIENT_EXPORT QWaylandCursor : public QPlatformCursor
{
public:
    explicit QWaylandCursor(QWaylandDisplay *display);

    void changeCursor(QCursor *cursor, QWindow *window) override;
    void pointerEvent(const QMouseEvent &event) override;
    QPoint pos() const override;
    void setPos(const QPoint &pos) override;

    static QSharedPointer<QWaylandBuffer> cursorBitmapBuffer(QWaylandDisplay *display, const QCursor *cursor);

private:
    QWaylandDisplay *mDisplay = nullptr;
    QPoint mLastPos;
};

}

QT_END_NAMESPACE

#endif // cursor
#endif // QWAYLANDCURSOR_H
