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

#ifndef QPLATFORMINTEGRATION_WAYLAND_H
#define QPLATFORMINTEGRATION_WAYLAND_H

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

#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <qpa/qplatformintegration.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandBuffer;
class QWaylandDisplay;
class QWaylandClientBufferIntegration;
class QWaylandServerBufferIntegration;
class QWaylandShellIntegration;
class QWaylandInputDeviceIntegration;
class QWaylandInputDevice;

class Q_WAYLAND_CLIENT_EXPORT QWaylandIntegration : public QPlatformIntegration
{
public:
    QWaylandIntegration();
    ~QWaylandIntegration() override;

    bool hasFailed() { return mFailed; }

    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
#if QT_CONFIG(opengl)
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
#endif
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QAbstractEventDispatcher *createEventDispatcher() const override;
    void initialize() override;

    QPlatformFontDatabase *fontDatabase() const override;

    QPlatformNativeInterface *nativeInterface() const override;
#if QT_CONFIG(clipboard)
    QPlatformClipboard *clipboard() const override;
#endif
#if QT_CONFIG(draganddrop)
    QPlatformDrag *drag() const override;
#endif
    QPlatformInputContext *inputContext() const override;

    QVariant styleHint(StyleHint hint) const override;

#if QT_CONFIG(accessibility)
    QPlatformAccessibility *accessibility() const override;
#endif

    QPlatformServices *services() const override;

    QWaylandDisplay *display() const;

    QList<int> possibleKeys(const QKeyEvent *event) const override;

    QStringList themeNames() const override;

    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    QWaylandInputDevice *createInputDevice(QWaylandDisplay *display, int version, uint32_t id);

    virtual QWaylandClientBufferIntegration *clientBufferIntegration() const;
    virtual QWaylandServerBufferIntegration *serverBufferIntegration() const;
    virtual QWaylandShellIntegration *shellIntegration() const;

    void reconfigureInputContext();

private:
    // NOTE: mDisplay *must* be destructed after mDrag and mClientBufferIntegration
    // and mShellIntegration.
    // Do not move this definition into the private section at the bottom.
    QScopedPointer<QWaylandDisplay> mDisplay;

protected:
    QScopedPointer<QWaylandClientBufferIntegration> mClientBufferIntegration;
    QScopedPointer<QWaylandServerBufferIntegration> mServerBufferIntegration;
    QScopedPointer<QWaylandShellIntegration> mShellIntegration;
    QScopedPointer<QWaylandInputDeviceIntegration> mInputDeviceIntegration;

private:
    void initializeClientBufferIntegration();
    void initializeServerBufferIntegration();
    void initializeShellIntegration();
    void initializeInputDeviceIntegration();
    QWaylandShellIntegration *createShellIntegration(const QString& interfaceName);

    QScopedPointer<QPlatformFontDatabase> mFontDb;
#if QT_CONFIG(clipboard)
    QScopedPointer<QPlatformClipboard> mClipboard;
#endif
#if QT_CONFIG(draganddrop)
    QScopedPointer<QPlatformDrag> mDrag;
#endif
    QScopedPointer<QPlatformNativeInterface> mNativeInterface;
    QScopedPointer<QPlatformInputContext> mInputContext;
#if QT_CONFIG(accessibility)
    mutable QScopedPointer<QPlatformAccessibility> mAccessibility;
#endif
    bool mFailed = false;
    QMutex mClientBufferInitLock;
    bool mClientBufferIntegrationInitialized = false;
    bool mServerBufferIntegrationInitialized = false;
    bool mShellIntegrationInitialized = false;

    friend class QWaylandDisplay;
};

}

QT_END_NAMESPACE

#endif
