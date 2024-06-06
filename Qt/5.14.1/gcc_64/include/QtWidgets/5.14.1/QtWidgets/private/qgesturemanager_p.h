/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QGESTUREMANAGER_P_H
#define QGESTUREMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qobject.h"
#include "qbasictimer.h"
#include "private/qwidget_p.h"
#include "qgesturerecognizer.h"

#ifndef QT_NO_GESTURES

#include <functional>

QT_BEGIN_NAMESPACE

class QBasicTimer;
class QGraphicsObject;
class QGestureManager : public QObject
{
    Q_OBJECT
public:
    QGestureManager(QObject *parent);
    ~QGestureManager();

    Qt::GestureType registerGestureRecognizer(QGestureRecognizer *recognizer);
    void unregisterGestureRecognizer(Qt::GestureType type);

    bool filterEvent(QWidget *receiver, QEvent *event);
    bool filterEvent(QObject *receiver, QEvent *event);
#if QT_CONFIG(graphicsview)
    bool filterEvent(QGraphicsObject *receiver, QEvent *event);
#endif // QT_CONFIG(graphicsview)

    enum InstanceCreation { ForceCreation, DontForceCreation };

    static QGestureManager *instance(InstanceCreation ic = ForceCreation); // declared in qapplication.cpp
    static bool gesturePending(QObject *o);

    void cleanupCachedGestures(QObject *target, Qt::GestureType type);

    void recycle(QGesture *gesture);

protected:
    bool filterEventThroughContexts(const QMultiMap<QObject *, Qt::GestureType> &contexts,
                                    QEvent *event);

private:
    QMultiMap<Qt::GestureType, QGestureRecognizer *> m_recognizers;

    QSet<QGesture *> m_activeGestures;
    QSet<QGesture *> m_maybeGestures;

    enum State {
        Gesture,
        NotGesture,
        MaybeGesture // this means timers are up and waiting for some
                     // more events, and input events are handled by
                     // gesture recognizer explicitly
    } state;

    struct ObjectGesture
    {
        QObject* object;
        Qt::GestureType gesture;

        ObjectGesture(QObject *o, const Qt::GestureType &g) : object(o), gesture(g) { }
        inline bool operator<(const ObjectGesture &rhs) const
        {
            if (std::less<QObject *>{}(object, rhs.object))
                return true;
            if (object == rhs.object)
                return gesture < rhs.gesture;
            return false;
        }
    };

    QMap<ObjectGesture, QList<QGesture *> > m_objectGestures;
    QHash<QGesture *, QGestureRecognizer *> m_gestureToRecognizer;
    QHash<QGesture *, QObject *> m_gestureOwners;

    QHash<QGesture *, QPointer<QWidget> > m_gestureTargets;

    int m_lastCustomGestureId;

    QHash<QGestureRecognizer *, QSet<QGesture *> > m_obsoleteGestures;
    QHash<QGesture *, QGestureRecognizer *> m_deletedRecognizers;
    QSet<QGesture *> m_gesturesToDelete;
    void cleanupGesturesForRemovedRecognizer(QGesture *gesture);

    QGesture *getState(QObject *widget, QGestureRecognizer *recognizer,
                       Qt::GestureType gesture);
    void deliverEvents(const QSet<QGesture *> &gestures,
                       QSet<QGesture *> *undeliveredGestures);
    void getGestureTargets(const QSet<QGesture*> &gestures,
                           QHash<QWidget *, QList<QGesture *> > *conflicts,
                           QHash<QWidget *, QList<QGesture *> > *normal);

    void cancelGesturesForChildren(QGesture *originatingGesture);
};

QT_END_NAMESPACE

#endif // QT_NO_GESTURES

#endif // QGESTUREMANAGER_P_H
