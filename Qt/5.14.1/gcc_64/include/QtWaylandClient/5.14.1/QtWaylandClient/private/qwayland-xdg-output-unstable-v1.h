#ifndef QT_WAYLAND_XDG_OUTPUT_UNSTABLE_V1
#define QT_WAYLAND_XDG_OUTPUT_UNSTABLE_V1

#include <QtWaylandClient/private/wayland-xdg-output-unstable-v1-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_XDG_OUTPUT_UNSTABLE_V1_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_XDG_OUTPUT_UNSTABLE_V1_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_XDG_OUTPUT_UNSTABLE_V1_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_XDG_OUTPUT_UNSTABLE_V1_EXPORT zxdg_output_manager_v1
    {
    public:
        zxdg_output_manager_v1(struct ::wl_registry *registry, int id, int version);
        zxdg_output_manager_v1(struct ::zxdg_output_manager_v1 *object);
        zxdg_output_manager_v1();

        virtual ~zxdg_output_manager_v1();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zxdg_output_manager_v1 *object);

        struct ::zxdg_output_manager_v1 *object() { return m_zxdg_output_manager_v1; }
        const struct ::zxdg_output_manager_v1 *object() const { return m_zxdg_output_manager_v1; }
        static zxdg_output_manager_v1 *fromObject(struct ::zxdg_output_manager_v1 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        void destroy();
        struct ::zxdg_output_v1 *get_xdg_output(struct ::wl_output *output);

    private:
        struct ::zxdg_output_manager_v1 *m_zxdg_output_manager_v1;
    };

    class Q_WAYLAND_CLIENT_XDG_OUTPUT_UNSTABLE_V1_EXPORT zxdg_output_v1
    {
    public:
        zxdg_output_v1(struct ::wl_registry *registry, int id, int version);
        zxdg_output_v1(struct ::zxdg_output_v1 *object);
        zxdg_output_v1();

        virtual ~zxdg_output_v1();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zxdg_output_v1 *object);

        struct ::zxdg_output_v1 *object() { return m_zxdg_output_v1; }
        const struct ::zxdg_output_v1 *object() const { return m_zxdg_output_v1; }
        static zxdg_output_v1 *fromObject(struct ::zxdg_output_v1 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        void destroy();

    protected:
        virtual void zxdg_output_v1_logical_position(int32_t x, int32_t y);
        virtual void zxdg_output_v1_logical_size(int32_t width, int32_t height);
        virtual void zxdg_output_v1_done();
        virtual void zxdg_output_v1_name(const QString &name);
        virtual void zxdg_output_v1_description(const QString &description);

    private:
        void init_listener();
        static const struct zxdg_output_v1_listener m_zxdg_output_v1_listener;
        static void handle_logical_position(
            void *data,
            struct ::zxdg_output_v1 *object,
            int32_t x,
            int32_t y);
        static void handle_logical_size(
            void *data,
            struct ::zxdg_output_v1 *object,
            int32_t width,
            int32_t height);
        static void handle_done(
            void *data,
            struct ::zxdg_output_v1 *object);
        static void handle_name(
            void *data,
            struct ::zxdg_output_v1 *object,
            const char *name);
        static void handle_description(
            void *data,
            struct ::zxdg_output_v1 *object,
            const char *description);
        struct ::zxdg_output_v1 *m_zxdg_output_v1;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
