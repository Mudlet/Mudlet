/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QJNI_P_H
#define QJNI_P_H

#include <jni.h>
#include <QtCore/private/qglobal_p.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

struct Q_CORE_EXPORT QJNILocalRefDeleter
{
    static void cleanup(jobject obj);
};

// To simplify this we only define it for jobjects.
typedef QScopedPointer<_jobject, QJNILocalRefDeleter> QJNIScopedLocalRef;

class Q_CORE_EXPORT QJNIEnvironmentPrivate
{
public:
    QJNIEnvironmentPrivate();
    ~QJNIEnvironmentPrivate();
    JNIEnv *operator->();
    operator JNIEnv*() const;
    static jclass findClass(const char *className, JNIEnv *env = 0);

private:
    friend class QAndroidJniEnvironment;
    Q_DISABLE_COPY_MOVE(QJNIEnvironmentPrivate)
    JNIEnv *jniEnv;
};

class Q_CORE_EXPORT QJNIObjectData
{
public:
    QJNIObjectData();
    ~QJNIObjectData();
    jobject m_jobject;
    jclass m_jclass;
    bool m_own_jclass;
    QByteArray m_className;
};

class Q_CORE_EXPORT QJNIObjectPrivate
{
public:
    QJNIObjectPrivate();
    explicit QJNIObjectPrivate(const char *className);
    QJNIObjectPrivate(const char *className, const char *sig, ...);
    explicit QJNIObjectPrivate(jclass clazz);
    QJNIObjectPrivate(jclass clazz, const char *sig, ...);
    // In most cases you should never call this function with a local ref. unless you intend
    // to manage the local ref. yourself.
    // NOTE: see fromLocalRef() for converting a local ref. to QJNIObjectPrivate.
    explicit QJNIObjectPrivate(jobject globalRef);

    template <typename T>
    T callMethod(const char *methodName,
                 const char *sig,
                 ...) const;
    template <typename T>
    T callMethod(const char *methodName) const;
    template <typename T>
    QJNIObjectPrivate callObjectMethod(const char *methodName) const;
    QJNIObjectPrivate callObjectMethod(const char *methodName,
                                       const char *sig,
                                       ...) const;
    template <typename T>
    static T callStaticMethod(const char *className,
                              const char *methodName,
                              const char *sig, ...);
    template <typename T>
    static T callStaticMethod(const char *className,
                              const char *methodName);
    template <typename T>
    static T callStaticMethod(jclass clazz,
                              const char *methodName,
                              const char *sig, ...);
    template <typename T>
    static T callStaticMethod(jclass clazz,
                              const char *methodName);
    static QJNIObjectPrivate callStaticObjectMethod(const char *className,
                                             const char *methodName,
                                             const char *sig, ...);

    static QJNIObjectPrivate callStaticObjectMethod(jclass clazz,
                                             const char *methodName,
                                             const char *sig, ...);

    template <typename T>
    T getField(const char *fieldName) const;
    template <typename T>
    static T getStaticField(const char *className, const char *fieldName);
    template <typename T>
    static T getStaticField(jclass clazz, const char *fieldName);

    QJNIObjectPrivate getObjectField(const char *fieldName, const char *sig) const;
    static QJNIObjectPrivate getStaticObjectField(const char *className,
                                                  const char *fieldName,
                                                  const char *sig);
    static QJNIObjectPrivate getStaticObjectField(jclass clazz,
                                                  const char *fieldName,
                                                  const char *sig);

    template <typename T>
    void setField(const char *fieldName, T value);
    template <typename T>
    void setField(const char *fieldName, const char *sig, T value);
    template <typename T>
    static void setStaticField(const char *className,
                               const char *fieldName,
                               T value);
    template <typename T>
    static void setStaticField(const char *className,
                               const char *fieldName,
                               const char *sig,
                               T value);
    template <typename T>
    static void setStaticField(jclass clazz,
                               const char *fieldName,
                               const char *sig,
                               T value);

    template <typename T>
    static void setStaticField(jclass clazz,
                               const char *fieldName,
                               T value);

    static QJNIObjectPrivate fromString(const QString &string);
    QString toString() const;

    static bool isClassAvailable(const char *className);
    bool isValid() const;
    jobject object() const { return d->m_jobject; }

    template <typename T>
    inline QJNIObjectPrivate &operator=(T o)
    {
        jobject jobj = static_cast<jobject>(o);
        if (!isSameObject(jobj)) {
            d = QSharedPointer<QJNIObjectData>::create();
            if (jobj) {
                QJNIEnvironmentPrivate env;
                d->m_jobject = env->NewGlobalRef(jobj);
                jclass objectClass = env->GetObjectClass(jobj);
                d->m_jclass = static_cast<jclass>(env->NewGlobalRef(objectClass));
                env->DeleteLocalRef(objectClass);
            }
        }

        return *this;
    }

    // This function takes ownership of the jobject and releases the local ref. before returning.
    static QJNIObjectPrivate fromLocalRef(jobject lref);

private:
    friend class QAndroidJniObject;

    struct QVaListPrivate { operator va_list &() const { return m_args; } va_list &m_args; };

    QJNIObjectPrivate(const char *className, const char *sig, const QVaListPrivate &args);
    QJNIObjectPrivate(jclass clazz, const char *sig, const QVaListPrivate &args);

    template <typename T>
    T callMethodV(const char *methodName,
                   const char *sig,
                   va_list args) const;
    QJNIObjectPrivate callObjectMethodV(const char *methodName,
                                        const char *sig,
                                        va_list args) const;
    template <typename T>
    static T callStaticMethodV(const char *className,
                               const char *methodName,
                               const char *sig,
                               va_list args);
    template <typename T>
    static T callStaticMethodV(jclass clazz,
                               const char *methodName,
                               const char *sig,
                               va_list args);
    static QJNIObjectPrivate callStaticObjectMethodV(const char *className,
                                                     const char *methodName,
                                                     const char *sig,
                                                     va_list args);

    static QJNIObjectPrivate callStaticObjectMethodV(jclass clazz,
                                                     const char *methodName,
                                                     const char *sig,
                                                     va_list args);

    bool isSameObject(jobject obj) const;
    bool isSameObject(const QJNIObjectPrivate &other) const;

    friend bool operator==(const QJNIObjectPrivate &, const QJNIObjectPrivate &);
    friend bool operator!=(const QJNIObjectPrivate&, const QJNIObjectPrivate&);
    template <typename T> friend bool operator!=(const QJNIObjectPrivate&, T);
    template <typename T> friend bool operator==(const QJNIObjectPrivate&, T);
    template <typename T> friend bool operator!=(T, const QJNIObjectPrivate&);
    template <typename T> friend bool operator==(T, const QJNIObjectPrivate&);

    QSharedPointer<QJNIObjectData> d;
};

inline bool operator==(const QJNIObjectPrivate&obj1, const QJNIObjectPrivate&obj2)
{
    return obj1.isSameObject(obj2);
}

inline bool operator!=(const QJNIObjectPrivate&obj1, const QJNIObjectPrivate&obj2)
{
    return !obj1.isSameObject(obj2);
}

template <typename T>
inline bool operator==(const QJNIObjectPrivate &obj1, T obj2)
{
    return obj1.isSameObject(static_cast<jobject>(obj2));
}

template <typename T>
inline bool operator==(T obj1, const QJNIObjectPrivate &obj2)
{
    return obj2.isSameObject(static_cast<jobject>(obj1));
}

template <typename T>
inline bool operator!=(const QJNIObjectPrivate &obj1, T obj2)
{
    return !obj1.isSameObject(obj2);
}

template <typename T>
inline bool operator!=(T obj1, const QJNIObjectPrivate &obj2)
{
    return !obj2.isSameObject(obj1);
}

QT_END_NAMESPACE

#endif // QJNI_P_H
