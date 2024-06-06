#ifndef QT_WAYLAND_QT_KEY_UNSTABLE_V1
#define QT_WAYLAND_QT_KEY_UNSTABLE_V1

#include <QtWaylandClient/private/wayland-qt-key-unstable-v1-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_QT_KEY_UNSTABLE_V1_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_QT_KEY_UNSTABLE_V1_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_QT_KEY_UNSTABLE_V1_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_QT_KEY_UNSTABLE_V1_EXPORT zqt_key_v1
    {
    public:
        zqt_key_v1(struct ::wl_registry *registry, int id, int version);
        zqt_key_v1(struct ::zqt_key_v1 *object);
        zqt_key_v1();

        virtual ~zqt_key_v1();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zqt_key_v1 *object);

        struct ::zqt_key_v1 *object() { return m_zqt_key_v1; }
        const struct ::zqt_key_v1 *object() const { return m_zqt_key_v1; }
        static zqt_key_v1 *fromObject(struct ::zqt_key_v1 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

    protected:
        virtual void zqt_key_v1_key(struct ::wl_surface *surface, uint32_t time, uint32_t type, uint32_t key, uint32_t modifiers, uint32_t nativeScanCode, uint32_t nativeVirtualKey, uint32_t nativeModifiers, const QString &text, uint32_t autorepeat, uint32_t count);

    private:
        void init_listener();
        static const struct zqt_key_v1_listener m_zqt_key_v1_listener;
        static void handle_key(
            void *data,
            struct ::zqt_key_v1 *object,
            struct ::wl_surface *surface,
            uint32_t time,
            uint32_t type,
            uint32_t key,
            uint32_t modifiers,
            uint32_t nativeScanCode,
            uint32_t nativeVirtualKey,
            uint32_t nativeModifiers,
            const char *text,
            uint32_t autorepeat,
            uint32_t count);
        struct ::zqt_key_v1 *m_zqt_key_v1;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
