#ifndef QT_WAYLAND_QT_WINDOWMANAGER
#define QT_WAYLAND_QT_WINDOWMANAGER

#include <QtWaylandClient/private/wayland-qt-windowmanager-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_QT_WINDOWMANAGER_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_QT_WINDOWMANAGER_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_QT_WINDOWMANAGER_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_QT_WINDOWMANAGER_EXPORT qt_windowmanager
    {
    public:
        qt_windowmanager(struct ::wl_registry *registry, int id, int version);
        qt_windowmanager(struct ::qt_windowmanager *object);
        qt_windowmanager();

        virtual ~qt_windowmanager();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::qt_windowmanager *object);

        struct ::qt_windowmanager *object() { return m_qt_windowmanager; }
        const struct ::qt_windowmanager *object() const { return m_qt_windowmanager; }
        static qt_windowmanager *fromObject(struct ::qt_windowmanager *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        void open_url(uint32_t remaining, const QString &url);

    protected:
        virtual void windowmanager_hints(int32_t show_is_fullscreen);
        virtual void windowmanager_quit();

    private:
        void init_listener();
        static const struct qt_windowmanager_listener m_qt_windowmanager_listener;
        static void handle_hints(
            void *data,
            struct ::qt_windowmanager *object,
            int32_t show_is_fullscreen);
        static void handle_quit(
            void *data,
            struct ::qt_windowmanager *object);
        struct ::qt_windowmanager *m_qt_windowmanager;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
