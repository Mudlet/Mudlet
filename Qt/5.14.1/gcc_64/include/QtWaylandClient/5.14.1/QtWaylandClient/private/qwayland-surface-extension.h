#ifndef QT_WAYLAND_SURFACE_EXTENSION
#define QT_WAYLAND_SURFACE_EXTENSION

#include <QtWaylandClient/private/wayland-surface-extension-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_SURFACE_EXTENSION_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_SURFACE_EXTENSION_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_SURFACE_EXTENSION_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_SURFACE_EXTENSION_EXPORT qt_surface_extension
    {
    public:
        qt_surface_extension(struct ::wl_registry *registry, int id, int version);
        qt_surface_extension(struct ::qt_surface_extension *object);
        qt_surface_extension();

        virtual ~qt_surface_extension();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::qt_surface_extension *object);

        struct ::qt_surface_extension *object() { return m_qt_surface_extension; }
        const struct ::qt_surface_extension *object() const { return m_qt_surface_extension; }
        static qt_surface_extension *fromObject(struct ::qt_surface_extension *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        struct ::qt_extended_surface *get_extended_surface(struct ::wl_surface *surface);

    private:
        struct ::qt_surface_extension *m_qt_surface_extension;
    };

    class Q_WAYLAND_CLIENT_SURFACE_EXTENSION_EXPORT qt_extended_surface
    {
    public:
        qt_extended_surface(struct ::wl_registry *registry, int id, int version);
        qt_extended_surface(struct ::qt_extended_surface *object);
        qt_extended_surface();

        virtual ~qt_extended_surface();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::qt_extended_surface *object);

        struct ::qt_extended_surface *object() { return m_qt_extended_surface; }
        const struct ::qt_extended_surface *object() const { return m_qt_extended_surface; }
        static qt_extended_surface *fromObject(struct ::qt_extended_surface *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        enum orientation {
            orientation_PrimaryOrientation = 0,
            orientation_PortraitOrientation = 1,
            orientation_LandscapeOrientation = 2,
            orientation_InvertedPortraitOrientation = 4,
            orientation_InvertedLandscapeOrientation = 8,
        };

        enum windowflag {
            windowflag_OverridesSystemGestures = 1,
            windowflag_StaysOnTop = 2,
            windowflag_BypassWindowManager = 4,
        };

        void update_generic_property(const QString &name, const QByteArray &value);
        void set_content_orientation_mask(int32_t orientation);
        void set_window_flags(int32_t flags);
        void raise();
        void lower();

    protected:
        virtual void extended_surface_onscreen_visibility(int32_t visible);
        virtual void extended_surface_set_generic_property(const QString &name, wl_array *value);
        virtual void extended_surface_close();

    private:
        void init_listener();
        static const struct qt_extended_surface_listener m_qt_extended_surface_listener;
        static void handle_onscreen_visibility(
            void *data,
            struct ::qt_extended_surface *object,
            int32_t visible);
        static void handle_set_generic_property(
            void *data,
            struct ::qt_extended_surface *object,
            const char *name,
            wl_array *value);
        static void handle_close(
            void *data,
            struct ::qt_extended_surface *object);
        struct ::qt_extended_surface *m_qt_extended_surface;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
