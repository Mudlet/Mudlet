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

#ifndef QWAYLANDSCREEN_H
#define QWAYLANDSCREEN_H

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

#include <qpa/qplatformscreen.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qwayland-xdg-output-unstable-v1.h>


QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandCursor;

class Q_WAYLAND_CLIENT_EXPORT QWaylandScreen : public QPlatformScreen, QtWayland::wl_output, QtWayland::zxdg_output_v1
{
public:
    QWaylandScreen(QWaylandDisplay *waylandDisplay, int version, uint32_t id);
    ~QWaylandScreen() override;

    void maybeInitialize();

    void initXdgOutput(QtWayland::zxdg_output_manager_v1 *xdgOutputManager);

    QWaylandDisplay *display() const;

    QString manufacturer() const override;
    QString model() const override;

    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;

    QSizeF physicalSize() const override;

    QDpi logicalDpi() const override;
    QList<QPlatformScreen *> virtualSiblings() const override;

    void setOrientationUpdateMask(Qt::ScreenOrientations mask) override;

    Qt::ScreenOrientation orientation() const override;
    int scale() const;
    qreal devicePixelRatio() const override;
    qreal refreshRate() const override;

    QString name() const override { return mOutputName; }

#if QT_CONFIG(cursor)
    QPlatformCursor *cursor() const override;
#endif

    uint32_t outputId() const { return m_outputId; }
    ::wl_output *output() { return QtWayland::wl_output::object(); }

    static QWaylandScreen *waylandScreenFromWindow(QWindow *window);
    static QWaylandScreen *fromWlOutput(::wl_output *output);

private:
    void output_mode(uint32_t flags, int width, int height, int refresh) override;
    void output_geometry(int32_t x, int32_t y,
                         int32_t width, int32_t height,
                         int subpixel,
                         const QString &make,
                         const QString &model,
                         int32_t transform) override;
    void output_scale(int32_t factor) override;
    void output_done() override;
    void updateOutputProperties();

    // XdgOutput
    void zxdg_output_v1_logical_position(int32_t x, int32_t y) override;
    void zxdg_output_v1_logical_size(int32_t width, int32_t height) override;
    void zxdg_output_v1_done() override;
    void zxdg_output_v1_name(const QString &name) override;
    void updateXdgOutputProperties();

    int m_outputId;
    QWaylandDisplay *mWaylandDisplay = nullptr;
    QString mManufacturer;
    QString mModel;
    QRect mGeometry;
    QRect mXdgGeometry;
    int mScale = 1;
    int mDepth = 32;
    int mRefreshRate = 60000;
    int mTransform = -1;
    QImage::Format mFormat = QImage::Format_ARGB32_Premultiplied;
    QSize mPhysicalSize;
    QString mOutputName;
    Qt::ScreenOrientation m_orientation = Qt::PrimaryOrientation;
    bool mOutputDone = false;
    bool mXdgOutputDone = false;
    bool mInitialized = false;

#if QT_CONFIG(cursor)
    QScopedPointer<QWaylandCursor> mWaylandCursor;
#endif
};

}

QT_END_NAMESPACE

#endif // QWAYLANDSCREEN_H
