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

#ifndef QWAYLANDDISPLAY_H
#define QWAYLANDDISPLAY_H

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

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QPointer>
#include <QtCore/QVector>

#include <QtCore/QWaitCondition>
#include <QtCore/QLoggingCategory>

#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwaylandshm_p.h>

#include <qpa/qplatforminputcontextfactory_p.h>

#if QT_CONFIG(xkbcommon)
#include <QtXkbCommonSupport/private/qxkbcommon_p.h>
#endif

struct wl_cursor_image;

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QSocketNotifier;
class QPlatformScreen;

namespace QtWayland {
    class qt_surface_extension;
    class zwp_text_input_manager_v2;
    class zxdg_output_manager_v1;
}

namespace QtWaylandClient {

Q_WAYLAND_CLIENT_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcQpaWayland);

class QWaylandInputDevice;
class QWaylandBuffer;
class QWaylandScreen;
class QWaylandClientBufferIntegration;
class QWaylandWindowManagerIntegration;
class QWaylandDataDeviceManager;
#if QT_CONFIG(wayland_client_primary_selection)
class QWaylandPrimarySelectionDeviceManagerV1;
#endif
class QWaylandTouchExtension;
class QWaylandQtKeyExtension;
class QWaylandWindow;
class QWaylandIntegration;
class QWaylandHardwareIntegration;
class QWaylandSurface;
class QWaylandShellIntegration;
class QWaylandCursor;
class QWaylandCursorTheme;

typedef void (*RegistryListener)(void *data,
                                 struct wl_registry *registry,
                                 uint32_t id,
                                 const QString &interface,
                                 uint32_t version);

class Q_WAYLAND_CLIENT_EXPORT QWaylandDisplay : public QObject, public QtWayland::wl_registry {
    Q_OBJECT

public:
    QWaylandDisplay(QWaylandIntegration *waylandIntegration);
    ~QWaylandDisplay(void) override;

#if QT_CONFIG(xkbcommon)
    struct xkb_context *xkbContext() const { return mXkbContext.get(); }
#endif

    QList<QWaylandScreen *> screens() const { return mScreens; }

    QWaylandScreen *screenForOutput(struct wl_output *output) const;
    void handleScreenInitialized(QWaylandScreen *screen);

    struct wl_surface *createSurface(void *handle);
    struct ::wl_region *createRegion(const QRegion &qregion);
    struct ::wl_subsurface *createSubSurface(QWaylandWindow *window, QWaylandWindow *parent);

    QWaylandShellIntegration *shellIntegration() const;
    QWaylandClientBufferIntegration *clientBufferIntegration() const;
    QWaylandWindowManagerIntegration *windowManagerIntegration() const;

#if QT_CONFIG(cursor)
    QWaylandCursor *waylandCursor();
    QWaylandCursorTheme *loadCursorTheme(const QString &name, int pixelSize);
#endif
    struct wl_display *wl_display() const { return mDisplay; }
    struct ::wl_registry *wl_registry() { return object(); }

    const struct wl_compositor *wl_compositor() const { return mCompositor.object(); }
    QtWayland::wl_compositor *compositor() { return &mCompositor; }
    int compositorVersion() const { return mCompositorVersion; }

    QList<QWaylandInputDevice *> inputDevices() const { return mInputDevices; }
    QWaylandInputDevice *defaultInputDevice() const;
    QWaylandInputDevice *currentInputDevice() const { return defaultInputDevice(); }
#if QT_CONFIG(wayland_datadevice)
    QWaylandDataDeviceManager *dndSelectionHandler() const { return mDndSelectionHandler.data(); }
#endif
#if QT_CONFIG(wayland_client_primary_selection)
    QWaylandPrimarySelectionDeviceManagerV1 *primarySelectionManager() const { return mPrimarySelectionManager.data(); }
#endif
    QtWayland::qt_surface_extension *windowExtension() const { return mWindowExtension.data(); }
    QWaylandTouchExtension *touchExtension() const { return mTouchExtension.data(); }
    QtWayland::zwp_text_input_manager_v2 *textInputManager() const { return mTextInputManager.data(); }
    QWaylandHardwareIntegration *hardwareIntegration() const { return mHardwareIntegration.data(); }
    QtWayland::zxdg_output_manager_v1 *xdgOutputManager() const { return mXdgOutputManager.data(); }

    bool usingInputContextFromCompositor() const { return mUsingInputContextFromCompositor; }

    struct RegistryGlobal {
        uint32_t id;
        QString interface;
        uint32_t version;
        struct ::wl_registry *registry = nullptr;
        RegistryGlobal(uint32_t id_, const QString &interface_, uint32_t version_, struct ::wl_registry *registry_)
            : id(id_), interface(interface_), version(version_), registry(registry_) { }
    };
    QList<RegistryGlobal> globals() const { return mGlobals; }
    bool hasRegistryGlobal(QStringView interfaceName) const;

    /* wl_registry_add_listener does not add but rather sets a listener, so this function is used
     * to enable many listeners at once. */
    void addRegistryListener(RegistryListener listener, void *data);
    void removeListener(RegistryListener listener, void *data);

    QWaylandShm *shm() const { return mShm.data(); }

    static uint32_t currentTimeMillisec();

    void forceRoundTrip();

    bool supportsWindowDecoration() const;

    uint32_t lastInputSerial() const { return mLastInputSerial; }
    QWaylandInputDevice *lastInputDevice() const { return mLastInputDevice; }
    QWaylandWindow *lastInputWindow() const;
    void setLastInputDevice(QWaylandInputDevice *device, uint32_t serial, QWaylandWindow *window);

    bool isWindowActivated(const QWaylandWindow *window);
    void handleWindowActivated(QWaylandWindow *window);
    void handleWindowDeactivated(QWaylandWindow *window);
    void handleKeyboardFocusChanged(QWaylandInputDevice *inputDevice);
    void handleWindowDestroyed(QWaylandWindow *window);

    wl_event_queue *createEventQueue();
    void dispatchQueueWhile(wl_event_queue *queue, std::function<bool()> condition, int timeout = -1);

public slots:
    void blockingReadEvents();
    void flushRequests();

private:
    void waitForScreens();
    void checkError() const;

    void handleWaylandSync();
    void requestWaylandSync();

    struct Listener {
        Listener() = default;
        Listener(RegistryListener incomingListener,
                 void* incomingData)
        : listener(incomingListener), data(incomingData)
        {}
        RegistryListener listener = nullptr;
        void *data = nullptr;
    };

    struct wl_display *mDisplay = nullptr;
    QtWayland::wl_compositor mCompositor;
    QScopedPointer<QWaylandShm> mShm;
    QList<QWaylandScreen *> mWaitingScreens;
    QList<QWaylandScreen *> mScreens;
    QList<QWaylandInputDevice *> mInputDevices;
    QList<Listener> mRegistryListeners;
    QWaylandIntegration *mWaylandIntegration = nullptr;
#if QT_CONFIG(cursor)
    QMap<std::pair<QString, int>, QWaylandCursorTheme *> mCursorThemes; // theme name and size
    QScopedPointer<QWaylandCursor> mCursor;
#endif
#if QT_CONFIG(wayland_datadevice)
    QScopedPointer<QWaylandDataDeviceManager> mDndSelectionHandler;
#endif
    QScopedPointer<QtWayland::qt_surface_extension> mWindowExtension;
    QScopedPointer<QtWayland::wl_subcompositor> mSubCompositor;
    QScopedPointer<QWaylandTouchExtension> mTouchExtension;
    QScopedPointer<QWaylandQtKeyExtension> mQtKeyExtension;
    QScopedPointer<QWaylandWindowManagerIntegration> mWindowManagerIntegration;
#if QT_CONFIG(wayland_client_primary_selection)
    QScopedPointer<QWaylandPrimarySelectionDeviceManagerV1> mPrimarySelectionManager;
#endif
    QScopedPointer<QtWayland::zwp_text_input_manager_v2> mTextInputManager;
    QScopedPointer<QWaylandHardwareIntegration> mHardwareIntegration;
    QScopedPointer<QtWayland::zxdg_output_manager_v1> mXdgOutputManager;
    QSocketNotifier *mReadNotifier = nullptr;
    int mFd = -1;
    int mWritableNotificationFd = -1;
    QList<RegistryGlobal> mGlobals;
    int mCompositorVersion = -1;
    uint32_t mLastInputSerial = 0;
    QWaylandInputDevice *mLastInputDevice = nullptr;
    QPointer<QWaylandWindow> mLastInputWindow;
    QPointer<QWaylandWindow> mLastKeyboardFocus;
    QVector<QWaylandWindow *> mActiveWindows;
    struct wl_callback *mSyncCallback = nullptr;
    static const wl_callback_listener syncCallbackListener;

    bool mClientSideInputContextRequested = !QPlatformInputContextFactory::requested().isNull();
    bool mUsingInputContextFromCompositor = false;

    void registry_global(uint32_t id, const QString &interface, uint32_t version) override;
    void registry_global_remove(uint32_t id) override;

#if QT_CONFIG(xkbcommon)
    QXkbCommon::ScopedXKBContext mXkbContext;
#endif

    friend class QWaylandIntegration;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDDISPLAY_H
