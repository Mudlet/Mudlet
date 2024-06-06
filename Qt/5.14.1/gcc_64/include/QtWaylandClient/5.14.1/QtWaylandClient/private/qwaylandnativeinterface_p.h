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

#ifndef QWAYLANDNATIVEINTERFACE_H
#define QWAYLANDNATIVEINTERFACE_H

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

#include <QVariantMap>
#include <qpa/qplatformnativeinterface.h>

#include <QtWaylandClient/qtwaylandclientglobal.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandIntegration;
class QWaylandScreen;

class Q_WAYLAND_CLIENT_EXPORT QWaylandNativeInterface : public QPlatformNativeInterface
{
public:
    QWaylandNativeInterface(QWaylandIntegration *integration);
    void *nativeResourceForIntegration(const QByteArray &resource) override;
    void *nativeResourceForWindow(const QByteArray &resourceString,
                                  QWindow *window) override;
    void *nativeResourceForScreen(const QByteArray &resourceString,
                                  QScreen *screen) override;
#if QT_CONFIG(opengl)
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
#endif
    QVariantMap windowProperties(QPlatformWindow *window) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name) const override;
    QVariant windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const override;
    void setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value) override;

    void emitWindowPropertyChanged(QPlatformWindow *window, const QString &name);

    QFunctionPointer platformFunction(const QByteArray &resource) const override;

private:
    QWaylandIntegration *m_integration = nullptr;
    QHash<QPlatformWindow*, QVariantMap> m_windowProperties;

    static void setSync(QWindow *window);
    static void setDeSync(QWindow *window);
    static bool isSync(QWindow *window);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDNATIVEINTERFACE_H
