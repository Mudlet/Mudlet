#ifndef QT_WAYLAND_TOUCH_EXTENSION
#define QT_WAYLAND_TOUCH_EXTENSION

#include <QtWaylandClient/private/wayland-touch-extension-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_TOUCH_EXTENSION_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_TOUCH_EXTENSION_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_TOUCH_EXTENSION_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_TOUCH_EXTENSION_EXPORT qt_touch_extension
    {
    public:
        qt_touch_extension(struct ::wl_registry *registry, int id, int version);
        qt_touch_extension(struct ::qt_touch_extension *object);
        qt_touch_extension();

        virtual ~qt_touch_extension();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::qt_touch_extension *object);

        struct ::qt_touch_extension *object() { return m_qt_touch_extension; }
        const struct ::qt_touch_extension *object() const { return m_qt_touch_extension; }
        static qt_touch_extension *fromObject(struct ::qt_touch_extension *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        enum flags {
            flags_mouse_from_touch = 0x1,
        };

        void dummy();

    protected:
        virtual void touch_extension_touch(uint32_t time, uint32_t id, uint32_t state, int32_t x, int32_t y, int32_t normalized_x, int32_t normalized_y, int32_t width, int32_t height, uint32_t pressure, int32_t velocity_x, int32_t velocity_y, uint32_t flags, wl_array *rawdata);
        virtual void touch_extension_configure(uint32_t flags);

    private:
        void init_listener();
        static const struct qt_touch_extension_listener m_qt_touch_extension_listener;
        static void handle_touch(
            void *data,
            struct ::qt_touch_extension *object,
            uint32_t time,
            uint32_t id,
            uint32_t state,
            int32_t x,
            int32_t y,
            int32_t normalized_x,
            int32_t normalized_y,
            int32_t width,
            int32_t height,
            uint32_t pressure,
            int32_t velocity_x,
            int32_t velocity_y,
            uint32_t flags,
            wl_array *rawdata);
        static void handle_configure(
            void *data,
            struct ::qt_touch_extension *object,
            uint32_t flags);
        struct ::qt_touch_extension *m_qt_touch_extension;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
