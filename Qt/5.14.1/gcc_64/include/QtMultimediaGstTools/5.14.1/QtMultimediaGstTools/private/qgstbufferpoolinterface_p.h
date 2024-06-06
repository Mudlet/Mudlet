/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#ifndef QGSTBUFFERPOOLINTERFACE_P_H
#define QGSTBUFFERPOOLINTERFACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qgsttools_global_p.h>
#include <qabstractvideobuffer.h>
#include <qvideosurfaceformat.h>
#include <QtCore/qobject.h>
#include <QtCore/qplugin.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

const QLatin1String QGstBufferPoolPluginKey("bufferpool");

/*!
    Abstract interface for video buffers allocation.
*/
class Q_GSTTOOLS_EXPORT QGstBufferPoolInterface
{
public:
    virtual ~QGstBufferPoolInterface() {}

    virtual bool isFormatSupported(const QVideoSurfaceFormat &format) const = 0;
    virtual GstBuffer *takeBuffer(const QVideoSurfaceFormat &format, GstCaps *caps) = 0;
    virtual void clear() = 0;

    virtual QAbstractVideoBuffer::HandleType handleType() const = 0;

    /*!
      Build an QAbstractVideoBuffer instance from GstBuffer.
      Returns NULL if GstBuffer is not compatible with this buffer pool.

      This method is called from gstreamer video sink thread.
     */
    virtual QAbstractVideoBuffer *prepareVideoBuffer(GstBuffer *buffer, int bytesPerLine) = 0;
};

#define QGstBufferPoolInterface_iid "org.qt-project.qt.gstbufferpool/5.0"
Q_DECLARE_INTERFACE(QGstBufferPoolInterface, QGstBufferPoolInterface_iid)

class QGstBufferPoolPlugin : public QObject, public QGstBufferPoolInterface
{
    Q_OBJECT
    Q_INTERFACES(QGstBufferPoolInterface)
public:
    explicit QGstBufferPoolPlugin(QObject *parent = 0);
    virtual ~QGstBufferPoolPlugin() {}

    bool isFormatSupported(const QVideoSurfaceFormat &format) const override = 0;
    GstBuffer *takeBuffer(const QVideoSurfaceFormat &format, GstCaps *caps) override = 0;
    void clear() override = 0;

    QAbstractVideoBuffer::HandleType handleType() const override = 0;

    /*!
      Build an QAbstractVideoBuffer instance from compatible GstBuffer.
      Returns NULL if GstBuffer is not compatible with this buffer pool.

      This method is called from gstreamer video sink thread.
     */
    QAbstractVideoBuffer *prepareVideoBuffer(GstBuffer *buffer, int bytesPerLine) override = 0;
};

QT_END_NAMESPACE

#endif
