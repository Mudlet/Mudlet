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

#ifndef QWAYLANDDATASOURCE_H
#define QWAYLANDDATASOURCE_H

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

#include <QObject>

#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>

QT_REQUIRE_CONFIG(wayland_datadevice);

QT_BEGIN_NAMESPACE

class QMimeData;

namespace QtWaylandClient {

class QWaylandDataDeviceManager;
class QWaylandDisplay;

class Q_WAYLAND_CLIENT_EXPORT QWaylandDataSource : public QObject, public QtWayland::wl_data_source
{
    Q_OBJECT
public:
    QWaylandDataSource(QWaylandDataDeviceManager *dataDeviceManager, QMimeData *mimeData);
    ~QWaylandDataSource() override;

    QMimeData *mimeData() const;

Q_SIGNALS:
    void targetChanged(const QString &mime_type);
    void cancelled();

protected:
    void data_source_cancelled() override;
    void data_source_send(const QString &mime_type, int32_t fd) override;
    void data_source_target(const QString &mime_type) override;

private:
    QWaylandDisplay *m_display = nullptr;
    QMimeData *m_mime_data = nullptr;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDDATASOURCE_H
