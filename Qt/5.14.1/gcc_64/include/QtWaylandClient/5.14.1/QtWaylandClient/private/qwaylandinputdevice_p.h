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

#ifndef QWAYLANDINPUTDEVICE_H
#define QWAYLANDINPUTDEVICE_H

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

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include <QtCore/QScopedPointer>
#include <QSocketNotifier>
#include <QObject>
#include <QTimer>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtWaylandClient/private/qwayland-wayland.h>

#if QT_CONFIG(xkbcommon)
#include <QtXkbCommonSupport/private/qxkbcommon_p.h>
#endif

#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>

#if QT_CONFIG(cursor)
struct wl_cursor_image;
#endif

QT_BEGIN_NAMESPACE

namespace QtWayland {
class zwp_primary_selection_device_v1;
} //namespace QtWayland

namespace QtWaylandClient {

class QWaylandDataDevice;
class QWaylandDisplay;
#if QT_CONFIG(wayland_client_primary_selection)
class QWaylandPrimarySelectionDeviceV1;
#endif
class QWaylandTextInput;
#if QT_CONFIG(cursor)
class QWaylandCursorTheme;
class CursorSurface;
#endif

class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice
                            : public QObject
                            , public QtWayland::wl_seat
{
    Q_OBJECT
public:
    class Keyboard;
    class Pointer;
    class Touch;

    QWaylandInputDevice(QWaylandDisplay *display, int version, uint32_t id);
    ~QWaylandInputDevice() override;

    uint32_t capabilities() const { return mCaps; }

    struct ::wl_seat *wl_seat() { return QtWayland::wl_seat::object(); }

#if QT_CONFIG(cursor)
    void setCursor(const QCursor *cursor, const QSharedPointer<QWaylandBuffer> &cachedBuffer = {}, int fallbackOutputScale = 1);
#endif
    void handleEndDrag();

#if QT_CONFIG(wayland_datadevice)
    void setDataDevice(QWaylandDataDevice *device);
    QWaylandDataDevice *dataDevice() const;
#endif

#if QT_CONFIG(wayland_client_primary_selection)
    void setPrimarySelectionDevice(QWaylandPrimarySelectionDeviceV1 *primarySelectionDevice);
    QWaylandPrimarySelectionDeviceV1 *primarySelectionDevice() const;
#endif

    void setTextInput(QWaylandTextInput *textInput);
    QWaylandTextInput *textInput() const;

    void removeMouseButtonFromState(Qt::MouseButton button);

    QWaylandWindow *pointerFocus() const;
    QWaylandWindow *keyboardFocus() const;
    QWaylandWindow *touchFocus() const;

    QList<int> possibleKeys(const QKeyEvent *event) const;

    QPointF pointerSurfacePosition() const;

    Qt::KeyboardModifiers modifiers() const;

    uint32_t serial() const;

    virtual Keyboard *createKeyboard(QWaylandInputDevice *device);
    virtual Pointer *createPointer(QWaylandInputDevice *device);
    virtual Touch *createTouch(QWaylandInputDevice *device);

    Keyboard *keyboard() const;
    Pointer *pointer() const;
    Touch *touch() const;

private:
    QWaylandDisplay *mQDisplay = nullptr;
    struct wl_display *mDisplay = nullptr;

    int mVersion;
    uint32_t mCaps = 0;

#if QT_CONFIG(cursor)
    struct CursorState {
        QSharedPointer<QWaylandBuffer> bitmapBuffer; // not used with shape cursors
        int bitmapScale = 1;
        Qt::CursorShape shape = Qt::ArrowCursor;
        int fallbackOutputScale = 1;
        QPoint hotspot;
        QElapsedTimer animationTimer;
    } mCursor;
#endif

#if QT_CONFIG(wayland_datadevice)
    QWaylandDataDevice *mDataDevice = nullptr;
#endif

#if QT_CONFIG(wayland_client_primary_selection)
    QScopedPointer<QWaylandPrimarySelectionDeviceV1> mPrimarySelectionDevice;
#endif

    Keyboard *mKeyboard = nullptr;
    Pointer *mPointer = nullptr;
    Touch *mTouch = nullptr;

    QScopedPointer<QWaylandTextInput> mTextInput;

    uint32_t mTime = 0;
    uint32_t mSerial = 0;

    void seat_capabilities(uint32_t caps) override;
    void handleTouchPoint(int id, Qt::TouchPointState state, const QPointF &surfacePosition = QPoint());

    QTouchDevice *mTouchDevice = nullptr;

    friend class QWaylandTouchExtension;
    friend class QWaylandQtKeyExtension;
};

inline uint32_t QWaylandInputDevice::serial() const
{
    return mSerial;
}


class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice::Keyboard : public QObject, public QtWayland::wl_keyboard
{
    Q_OBJECT

public:
    Keyboard(QWaylandInputDevice *p);
    ~Keyboard() override;

    QWaylandWindow *focusWindow() const;

    void keyboard_keymap(uint32_t format,
                         int32_t fd,
                         uint32_t size) override;
    void keyboard_enter(uint32_t time,
                        struct wl_surface *surface,
                        struct wl_array *keys) override;
    void keyboard_leave(uint32_t time,
                        struct wl_surface *surface) override;
    void keyboard_key(uint32_t serial, uint32_t time,
                      uint32_t key, uint32_t state) override;
    void keyboard_modifiers(uint32_t serial,
                            uint32_t mods_depressed,
                            uint32_t mods_latched,
                            uint32_t mods_locked,
                            uint32_t group) override;
    void keyboard_repeat_info(int32_t rate, int32_t delay) override;

    QWaylandInputDevice *mParent = nullptr;
    ::wl_surface *mFocus = nullptr;

    uint32_t mNativeModifiers = 0;

    struct repeatKey {
        int key;
        uint32_t code;
        uint32_t time;
        QString text;
        Qt::KeyboardModifiers modifiers;
        uint32_t nativeVirtualKey;
        uint32_t nativeModifiers;
    } mRepeatKey;

    QTimer mRepeatTimer;
    int mRepeatRate = 25;
    int mRepeatDelay = 400;

    uint32_t mKeymapFormat = WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1;

    Qt::KeyboardModifiers modifiers() const;

    struct ::wl_keyboard *wl_keyboard() { return QtWayland::wl_keyboard::object(); }

private slots:
    void handleFocusDestroyed();
    void handleFocusLost();

private:
#if QT_CONFIG(xkbcommon)
    bool createDefaultKeymap();
#endif
    void handleKey(ulong timestamp, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                   quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                   const QString &text, bool autorepeat = false, ushort count = 1);

#if QT_CONFIG(xkbcommon)
    QXkbCommon::ScopedXKBKeymap mXkbKeymap;
    QXkbCommon::ScopedXKBState mXkbState;
#endif
    friend class QWaylandInputDevice;
};

class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice::Pointer : public QObject, public QtWayland::wl_pointer
{
    Q_OBJECT
public:
    explicit Pointer(QWaylandInputDevice *seat);
    ~Pointer() override;
    QWaylandWindow *focusWindow() const;
#if QT_CONFIG(cursor)
    QString cursorThemeName() const;
    int cursorSize() const; // in surface coordinates
    int idealCursorScale() const;
    void updateCursorTheme();
    void updateCursor();
    void cursorTimerCallback();
    void cursorFrameCallback();
    CursorSurface *getOrCreateCursorSurface();
#endif
    QWaylandInputDevice *seat() const { return mParent; }

    struct ::wl_pointer *wl_pointer() { return QtWayland::wl_pointer::object(); }

protected:
    void pointer_enter(uint32_t serial, struct wl_surface *surface,
                       wl_fixed_t sx, wl_fixed_t sy) override;
    void pointer_leave(uint32_t time, struct wl_surface *surface) override;
    void pointer_motion(uint32_t time,
                        wl_fixed_t sx, wl_fixed_t sy) override;
    void pointer_button(uint32_t serial, uint32_t time,
                        uint32_t button, uint32_t state) override;
    void pointer_axis(uint32_t time,
                      uint32_t axis,
                      wl_fixed_t value) override;
    void pointer_axis_source(uint32_t source) override;
    void pointer_axis_stop(uint32_t time, uint32_t axis) override;
    void pointer_axis_discrete(uint32_t axis, int32_t value) override;
    void pointer_frame() override;

private slots:
    void handleFocusDestroyed() { invalidateFocus(); }

private:
    void invalidateFocus();

public:
    void releaseButtons();

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandSurface> mFocus;
    uint32_t mEnterSerial = 0;
#if QT_CONFIG(cursor)
    struct {
        QWaylandCursorTheme *theme = nullptr;
        int themeBufferScale = 0;
        QScopedPointer<CursorSurface> surface;
        QTimer frameTimer;
        bool gotFrameCallback = false;
        bool gotTimerCallback = false;
    } mCursor;
#endif
    QPointF mSurfacePos;
    QPointF mGlobalPos;
    Qt::MouseButtons mButtons = Qt::NoButton;
#if QT_CONFIG(cursor)
    wl_buffer *mCursorBuffer = nullptr;
    Qt::CursorShape mCursorShape = Qt::BitmapCursor;
#endif

    struct FrameData {
        QWaylandPointerEvent *event = nullptr;

        QPointF delta;
        QPoint discreteDelta;
        axis_source axisSource = axis_source_wheel;

        void resetScrollData();
        bool hasPixelDelta() const;
        QPoint pixelDeltaAndError(QPointF *accumulatedError) const;
        QPoint pixelDelta() const { return hasPixelDelta() ? delta.toPoint() : QPoint(); }
        QPoint angleDelta() const;
        Qt::MouseEventSource wheelEventSource() const;
    } mFrameData;

    bool mScrollBeginSent = false;
    QPointF mScrollDeltaRemainder;

    void setFrameEvent(QWaylandPointerEvent *event);
    void flushScrollEvent();
    void flushFrameEvent();
private: //TODO: should other methods be private as well?
    bool isDefinitelyTerminated(axis_source source) const;
};

class Q_WAYLAND_CLIENT_EXPORT QWaylandInputDevice::Touch : public QtWayland::wl_touch
{
public:
    Touch(QWaylandInputDevice *p);
    ~Touch() override;

    void touch_down(uint32_t serial,
                    uint32_t time,
                    struct wl_surface *surface,
                    int32_t id,
                    wl_fixed_t x,
                    wl_fixed_t y) override;
    void touch_up(uint32_t serial,
                  uint32_t time,
                  int32_t id) override;
    void touch_motion(uint32_t time,
                      int32_t id,
                      wl_fixed_t x,
                      wl_fixed_t y) override;
    void touch_frame() override;
    void touch_cancel() override;

    bool allTouchPointsReleased();
    void releasePoints();

    struct ::wl_touch *wl_touch() { return QtWayland::wl_touch::object(); }

    QWaylandInputDevice *mParent = nullptr;
    QPointer<QWaylandWindow> mFocus;
    QList<QWindowSystemInterface::TouchPoint> mPendingTouchPoints;
};

class QWaylandPointerEvent
{
    Q_GADGET
public:
    enum Type {
        Enter,
        Leave,
        Motion,
        Press,
        Release,
        Wheel
    };
    Q_ENUM(Type)

    inline QWaylandPointerEvent(Type type, Qt::ScrollPhase phase, QWaylandWindow *surface,
                                ulong timestamp, const QPointF &localPos, const QPointF &globalPos,
                                Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
        : type(type)
        , phase(phase)
        , timestamp(timestamp)
        , local(localPos)
        , global(globalPos)
        , buttons(buttons)
        , modifiers(modifiers)
        , surface(surface)
    {}
    inline QWaylandPointerEvent(Type type, Qt::ScrollPhase phase, QWaylandWindow *surface,
                                ulong timestamp, const QPointF &local, const QPointF &global,
                                const QPoint &pixelDelta, const QPoint &angleDelta,
                                Qt::MouseEventSource source,
                                Qt::KeyboardModifiers modifiers)
        : type(type)
        , phase(phase)
        , timestamp(timestamp)
        , local(local)
        , global(global)
        , modifiers(modifiers)
        , pixelDelta(pixelDelta)
        , angleDelta(angleDelta)
        , source(source)
        , surface(surface)
    {}

    Type type;
    Qt::ScrollPhase phase = Qt::NoScrollPhase;
    ulong timestamp = 0;
    QPointF local;
    QPointF global;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
    QPoint pixelDelta;
    QPoint angleDelta;
    Qt::MouseEventSource source = Qt::MouseEventNotSynthesized;
    QPointer<QWaylandWindow> surface;
};

}

QT_END_NAMESPACE

#endif
