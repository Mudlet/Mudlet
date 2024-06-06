#ifndef QT_WAYLAND_WP_PRIMARY_SELECTION_UNSTABLE_V1
#define QT_WAYLAND_WP_PRIMARY_SELECTION_UNSTABLE_V1

#include <QtWaylandClient/private/wayland-wp-primary-selection-unstable-v1-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_WP_PRIMARY_SELECTION_UNSTABLE_V1_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_WP_PRIMARY_SELECTION_UNSTABLE_V1_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_WP_PRIMARY_SELECTION_UNSTABLE_V1_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_WP_PRIMARY_SELECTION_UNSTABLE_V1_EXPORT zwp_primary_selection_device_manager_v1
    {
    public:
        zwp_primary_selection_device_manager_v1(struct ::wl_registry *registry, int id, int version);
        zwp_primary_selection_device_manager_v1(struct ::zwp_primary_selection_device_manager_v1 *object);
        zwp_primary_selection_device_manager_v1();

        virtual ~zwp_primary_selection_device_manager_v1();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zwp_primary_selection_device_manager_v1 *object);

        struct ::zwp_primary_selection_device_manager_v1 *object() { return m_zwp_primary_selection_device_manager_v1; }
        const struct ::zwp_primary_selection_device_manager_v1 *object() const { return m_zwp_primary_selection_device_manager_v1; }
        static zwp_primary_selection_device_manager_v1 *fromObject(struct ::zwp_primary_selection_device_manager_v1 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        struct ::zwp_primary_selection_source_v1 *create_source();
        struct ::zwp_primary_selection_device_v1 *get_device(struct ::wl_seat *seat);
        void destroy();

    private:
        struct ::zwp_primary_selection_device_manager_v1 *m_zwp_primary_selection_device_manager_v1;
    };

    class Q_WAYLAND_CLIENT_WP_PRIMARY_SELECTION_UNSTABLE_V1_EXPORT zwp_primary_selection_device_v1
    {
    public:
        zwp_primary_selection_device_v1(struct ::wl_registry *registry, int id, int version);
        zwp_primary_selection_device_v1(struct ::zwp_primary_selection_device_v1 *object);
        zwp_primary_selection_device_v1();

        virtual ~zwp_primary_selection_device_v1();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zwp_primary_selection_device_v1 *object);

        struct ::zwp_primary_selection_device_v1 *object() { return m_zwp_primary_selection_device_v1; }
        const struct ::zwp_primary_selection_device_v1 *object() const { return m_zwp_primary_selection_device_v1; }
        static zwp_primary_selection_device_v1 *fromObject(struct ::zwp_primary_selection_device_v1 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        void set_selection(struct ::zwp_primary_selection_source_v1 *source, uint32_t serial);
        void destroy();

    protected:
        virtual void zwp_primary_selection_device_v1_data_offer(struct ::zwp_primary_selection_offer_v1 *offer);
        virtual void zwp_primary_selection_device_v1_selection(struct ::zwp_primary_selection_offer_v1 *id);

    private:
        void init_listener();
        static const struct zwp_primary_selection_device_v1_listener m_zwp_primary_selection_device_v1_listener;
        static void handle_data_offer(
            void *data,
            struct ::zwp_primary_selection_device_v1 *object,
            struct ::zwp_primary_selection_offer_v1 *offer);
        static void handle_selection(
            void *data,
            struct ::zwp_primary_selection_device_v1 *object,
            struct ::zwp_primary_selection_offer_v1 *id);
        struct ::zwp_primary_selection_device_v1 *m_zwp_primary_selection_device_v1;
    };

    class Q_WAYLAND_CLIENT_WP_PRIMARY_SELECTION_UNSTABLE_V1_EXPORT zwp_primary_selection_offer_v1
    {
    public:
        zwp_primary_selection_offer_v1(struct ::wl_registry *registry, int id, int version);
        zwp_primary_selection_offer_v1(struct ::zwp_primary_selection_offer_v1 *object);
        zwp_primary_selection_offer_v1();

        virtual ~zwp_primary_selection_offer_v1();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zwp_primary_selection_offer_v1 *object);

        struct ::zwp_primary_selection_offer_v1 *object() { return m_zwp_primary_selection_offer_v1; }
        const struct ::zwp_primary_selection_offer_v1 *object() const { return m_zwp_primary_selection_offer_v1; }
        static zwp_primary_selection_offer_v1 *fromObject(struct ::zwp_primary_selection_offer_v1 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        void receive(const QString &mime_type, int32_t fd);
        void destroy();

    protected:
        virtual void zwp_primary_selection_offer_v1_offer(const QString &mime_type);

    private:
        void init_listener();
        static const struct zwp_primary_selection_offer_v1_listener m_zwp_primary_selection_offer_v1_listener;
        static void handle_offer(
            void *data,
            struct ::zwp_primary_selection_offer_v1 *object,
            const char *mime_type);
        struct ::zwp_primary_selection_offer_v1 *m_zwp_primary_selection_offer_v1;
    };

    class Q_WAYLAND_CLIENT_WP_PRIMARY_SELECTION_UNSTABLE_V1_EXPORT zwp_primary_selection_source_v1
    {
    public:
        zwp_primary_selection_source_v1(struct ::wl_registry *registry, int id, int version);
        zwp_primary_selection_source_v1(struct ::zwp_primary_selection_source_v1 *object);
        zwp_primary_selection_source_v1();

        virtual ~zwp_primary_selection_source_v1();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zwp_primary_selection_source_v1 *object);

        struct ::zwp_primary_selection_source_v1 *object() { return m_zwp_primary_selection_source_v1; }
        const struct ::zwp_primary_selection_source_v1 *object() const { return m_zwp_primary_selection_source_v1; }
        static zwp_primary_selection_source_v1 *fromObject(struct ::zwp_primary_selection_source_v1 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        void offer(const QString &mime_type);
        void destroy();

    protected:
        virtual void zwp_primary_selection_source_v1_send(const QString &mime_type, int32_t fd);
        virtual void zwp_primary_selection_source_v1_cancelled();

    private:
        void init_listener();
        static const struct zwp_primary_selection_source_v1_listener m_zwp_primary_selection_source_v1_listener;
        static void handle_send(
            void *data,
            struct ::zwp_primary_selection_source_v1 *object,
            const char *mime_type,
            int32_t fd);
        static void handle_cancelled(
            void *data,
            struct ::zwp_primary_selection_source_v1 *object);
        struct ::zwp_primary_selection_source_v1 *m_zwp_primary_selection_source_v1;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
