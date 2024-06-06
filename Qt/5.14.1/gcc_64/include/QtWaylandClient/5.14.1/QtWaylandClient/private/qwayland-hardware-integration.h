#ifndef QT_WAYLAND_HARDWARE_INTEGRATION
#define QT_WAYLAND_HARDWARE_INTEGRATION

#include <QtWaylandClient/private/wayland-hardware-integration-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_HARDWARE_INTEGRATION_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_HARDWARE_INTEGRATION_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_HARDWARE_INTEGRATION_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_HARDWARE_INTEGRATION_EXPORT qt_hardware_integration
    {
    public:
        qt_hardware_integration(struct ::wl_registry *registry, int id, int version);
        qt_hardware_integration(struct ::qt_hardware_integration *object);
        qt_hardware_integration();

        virtual ~qt_hardware_integration();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::qt_hardware_integration *object);

        struct ::qt_hardware_integration *object() { return m_qt_hardware_integration; }
        const struct ::qt_hardware_integration *object() const { return m_qt_hardware_integration; }
        static qt_hardware_integration *fromObject(struct ::qt_hardware_integration *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

    protected:
        virtual void hardware_integration_client_backend(const QString &name);
        virtual void hardware_integration_server_backend(const QString &name);

    private:
        void init_listener();
        static const struct qt_hardware_integration_listener m_qt_hardware_integration_listener;
        static void handle_client_backend(
            void *data,
            struct ::qt_hardware_integration *object,
            const char *name);
        static void handle_server_backend(
            void *data,
            struct ::qt_hardware_integration *object,
            const char *name);
        struct ::qt_hardware_integration *m_qt_hardware_integration;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
