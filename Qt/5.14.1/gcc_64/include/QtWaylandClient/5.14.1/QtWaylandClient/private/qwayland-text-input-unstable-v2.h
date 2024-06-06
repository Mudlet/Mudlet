#ifndef QT_WAYLAND_TEXT_INPUT_UNSTABLE_V2
#define QT_WAYLAND_TEXT_INPUT_UNSTABLE_V2

#include <QtWaylandClient/private/wayland-text-input-unstable-v2-client-protocol.h>
#include <QByteArray>
#include <QString>

struct wl_registry;

QT_BEGIN_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

#if !defined(Q_WAYLAND_CLIENT_TEXT_INPUT_UNSTABLE_V2_EXPORT)
#  if defined(QT_SHARED)
#    define Q_WAYLAND_CLIENT_TEXT_INPUT_UNSTABLE_V2_EXPORT Q_DECL_EXPORT
#  else
#    define Q_WAYLAND_CLIENT_TEXT_INPUT_UNSTABLE_V2_EXPORT
#  endif
#endif

namespace QtWayland {
    class Q_WAYLAND_CLIENT_TEXT_INPUT_UNSTABLE_V2_EXPORT zwp_text_input_v2
    {
    public:
        zwp_text_input_v2(struct ::wl_registry *registry, int id, int version);
        zwp_text_input_v2(struct ::zwp_text_input_v2 *object);
        zwp_text_input_v2();

        virtual ~zwp_text_input_v2();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zwp_text_input_v2 *object);

        struct ::zwp_text_input_v2 *object() { return m_zwp_text_input_v2; }
        const struct ::zwp_text_input_v2 *object() const { return m_zwp_text_input_v2; }
        static zwp_text_input_v2 *fromObject(struct ::zwp_text_input_v2 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        enum content_hint {
            content_hint_none = 0x0, // no special behaviour
            content_hint_auto_completion = 0x1, // suggest word completions
            content_hint_auto_correction = 0x2, // suggest word corrections
            content_hint_auto_capitalization = 0x4, // switch to uppercase letters at the start of a sentence
            content_hint_lowercase = 0x8, // prefer lowercase letters
            content_hint_uppercase = 0x10, // prefer uppercase letters
            content_hint_titlecase = 0x20, // prefer casing for titles and headings (can be language dependent)
            content_hint_hidden_text = 0x40, // characters should be hidden
            content_hint_sensitive_data = 0x80, // typed text should not be stored
            content_hint_latin = 0x100, // just latin characters should be entered
            content_hint_multiline = 0x200, // the text input is multiline
        };

        enum content_purpose {
            content_purpose_normal = 0, // default input, allowing all characters
            content_purpose_alpha = 1, // allow only alphabetic characters
            content_purpose_digits = 2, // allow only digits
            content_purpose_number = 3, // input a number (including decimal separator and sign)
            content_purpose_phone = 4, // input a phone number
            content_purpose_url = 5, // input an URL
            content_purpose_email = 6, // input an email address
            content_purpose_name = 7, // input a name of a person
            content_purpose_password = 8, // input a password (combine with password or sensitive_data hint)
            content_purpose_date = 9, // input a date
            content_purpose_time = 10, // input a time
            content_purpose_datetime = 11, // input a date and time
            content_purpose_terminal = 12, // input for a terminal
        };

        enum update_state {
            update_state_change = 0, // updated state because it changed
            update_state_full = 1, // full state after enter or input_method_changed event
            update_state_reset = 2, // full state after reset
            update_state_enter = 3, // full state after switching focus to a different widget on client side
        };

        enum input_panel_visibility {
            input_panel_visibility_hidden = 0, // the input panel (virtual keyboard) is hidden
            input_panel_visibility_visible = 1, // the input panel (virtual keyboard) is visible
        };

        enum preedit_style {
            preedit_style_default = 0, // default style for composing text
            preedit_style_none = 1, // composing text should be shown the same as non-composing text
            preedit_style_active = 2, // composing text might be bold
            preedit_style_inactive = 3, // composing text might be cursive
            preedit_style_highlight = 4, // composing text might have a different background color
            preedit_style_underline = 5, // composing text might be underlined
            preedit_style_selection = 6, // composing text should be shown the same as selected text
            preedit_style_incorrect = 7, // composing text might be underlined with a red wavy line
        };

        enum text_direction {
            text_direction_auto = 0, // automatic text direction based on text and language
            text_direction_ltr = 1, // left-to-right
            text_direction_rtl = 2, // right-to-left
        };

        void destroy();
        void enable(struct ::wl_surface *surface);
        void disable(struct ::wl_surface *surface);
        void show_input_panel();
        void hide_input_panel();
        void set_surrounding_text(const QString &text, int32_t cursor, int32_t anchor);
        void set_content_type(uint32_t hint, uint32_t purpose);
        void set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height);
        void set_preferred_language(const QString &language);
        void update_state(uint32_t serial, uint32_t reason);

    protected:
        virtual void zwp_text_input_v2_enter(uint32_t serial, struct ::wl_surface *surface);
        virtual void zwp_text_input_v2_leave(uint32_t serial, struct ::wl_surface *surface);
        virtual void zwp_text_input_v2_input_panel_state(uint32_t state, int32_t x, int32_t y, int32_t width, int32_t height);
        virtual void zwp_text_input_v2_preedit_string(const QString &text, const QString &commit);
        virtual void zwp_text_input_v2_preedit_styling(uint32_t index, uint32_t length, uint32_t style);
        virtual void zwp_text_input_v2_preedit_cursor(int32_t index);
        virtual void zwp_text_input_v2_commit_string(const QString &text);
        virtual void zwp_text_input_v2_cursor_position(int32_t index, int32_t anchor);
        virtual void zwp_text_input_v2_delete_surrounding_text(uint32_t before_length, uint32_t after_length);
        virtual void zwp_text_input_v2_modifiers_map(wl_array *map);
        virtual void zwp_text_input_v2_keysym(uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers);
        virtual void zwp_text_input_v2_language(const QString &language);
        virtual void zwp_text_input_v2_text_direction(uint32_t direction);
        virtual void zwp_text_input_v2_configure_surrounding_text(int32_t before_cursor, int32_t after_cursor);
        virtual void zwp_text_input_v2_input_method_changed(uint32_t serial, uint32_t flags);

    private:
        void init_listener();
        static const struct zwp_text_input_v2_listener m_zwp_text_input_v2_listener;
        static void handle_enter(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t serial,
            struct ::wl_surface *surface);
        static void handle_leave(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t serial,
            struct ::wl_surface *surface);
        static void handle_input_panel_state(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t state,
            int32_t x,
            int32_t y,
            int32_t width,
            int32_t height);
        static void handle_preedit_string(
            void *data,
            struct ::zwp_text_input_v2 *object,
            const char *text,
            const char *commit);
        static void handle_preedit_styling(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t index,
            uint32_t length,
            uint32_t style);
        static void handle_preedit_cursor(
            void *data,
            struct ::zwp_text_input_v2 *object,
            int32_t index);
        static void handle_commit_string(
            void *data,
            struct ::zwp_text_input_v2 *object,
            const char *text);
        static void handle_cursor_position(
            void *data,
            struct ::zwp_text_input_v2 *object,
            int32_t index,
            int32_t anchor);
        static void handle_delete_surrounding_text(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t before_length,
            uint32_t after_length);
        static void handle_modifiers_map(
            void *data,
            struct ::zwp_text_input_v2 *object,
            wl_array *map);
        static void handle_keysym(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t time,
            uint32_t sym,
            uint32_t state,
            uint32_t modifiers);
        static void handle_language(
            void *data,
            struct ::zwp_text_input_v2 *object,
            const char *language);
        static void handle_text_direction(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t direction);
        static void handle_configure_surrounding_text(
            void *data,
            struct ::zwp_text_input_v2 *object,
            int32_t before_cursor,
            int32_t after_cursor);
        static void handle_input_method_changed(
            void *data,
            struct ::zwp_text_input_v2 *object,
            uint32_t serial,
            uint32_t flags);
        struct ::zwp_text_input_v2 *m_zwp_text_input_v2;
    };

    class Q_WAYLAND_CLIENT_TEXT_INPUT_UNSTABLE_V2_EXPORT zwp_text_input_manager_v2
    {
    public:
        zwp_text_input_manager_v2(struct ::wl_registry *registry, int id, int version);
        zwp_text_input_manager_v2(struct ::zwp_text_input_manager_v2 *object);
        zwp_text_input_manager_v2();

        virtual ~zwp_text_input_manager_v2();

        void init(struct ::wl_registry *registry, int id, int version);
        void init(struct ::zwp_text_input_manager_v2 *object);

        struct ::zwp_text_input_manager_v2 *object() { return m_zwp_text_input_manager_v2; }
        const struct ::zwp_text_input_manager_v2 *object() const { return m_zwp_text_input_manager_v2; }
        static zwp_text_input_manager_v2 *fromObject(struct ::zwp_text_input_manager_v2 *object);

        bool isInitialized() const;

        static const struct ::wl_interface *interface();

        void destroy();
        struct ::zwp_text_input_v2 *get_text_input(struct ::wl_seat *seat);

    private:
        struct ::zwp_text_input_manager_v2 *m_zwp_text_input_manager_v2;
    };
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif
