
#ifndef QKEYCHAIN_EXPORT_H
#define QKEYCHAIN_EXPORT_H

#ifdef QT5KEYCHAIN_STATIC_DEFINE
#  define QKEYCHAIN_EXPORT
#  define QT5KEYCHAIN_NO_EXPORT
#else
#  ifndef QKEYCHAIN_EXPORT
#    ifdef qt5keychain_EXPORTS
        /* We are building this library */
#      define QKEYCHAIN_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define QKEYCHAIN_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef QT5KEYCHAIN_NO_EXPORT
#    define QT5KEYCHAIN_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef QT5KEYCHAIN_DEPRECATED
#  define QT5KEYCHAIN_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef QT5KEYCHAIN_DEPRECATED_EXPORT
#  define QT5KEYCHAIN_DEPRECATED_EXPORT QKEYCHAIN_EXPORT QT5KEYCHAIN_DEPRECATED
#endif

#ifndef QT5KEYCHAIN_DEPRECATED_NO_EXPORT
#  define QT5KEYCHAIN_DEPRECATED_NO_EXPORT QT5KEYCHAIN_NO_EXPORT QT5KEYCHAIN_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef QT5KEYCHAIN_NO_DEPRECATED
#    define QT5KEYCHAIN_NO_DEPRECATED
#  endif
#endif

#endif /* QKEYCHAIN_EXPORT_H */
