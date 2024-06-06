/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef QUICKTESTEVENT_P_H
#define QUICKTESTEVENT_P_H

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

#include <QtQuickTest/quicktestglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/QWindow>
#include <QtTest/qtesttouch.h>

QT_BEGIN_NAMESPACE

class QuickTestEvent;
class Q_QUICK_TEST_EXPORT QQuickTouchEventSequence : public QObject
{
    Q_OBJECT
public:
    explicit QQuickTouchEventSequence(QuickTestEvent *testEvent, QObject *item = nullptr);
public slots:
    QObject* press(int touchId, QObject *item, qreal x, qreal y);
    QObject* move(int touchId, QObject *item, qreal x, qreal y);
    QObject* release(int touchId, QObject *item, qreal x, qreal y);
    QObject* stationary(int touchId);
    QObject* commit();

private:
    QTest::QTouchEventSequence m_sequence;
    QuickTestEvent * const m_testEvent;
};

class Q_QUICK_TEST_EXPORT QuickTestEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int defaultMouseDelay READ defaultMouseDelay FINAL)
public:
    QuickTestEvent(QObject *parent = nullptr);
    ~QuickTestEvent() override;
    int defaultMouseDelay() const;

public Q_SLOTS:
    bool keyPress(int key, int modifiers, int delay);
    bool keyRelease(int key, int modifiers, int delay);
    bool keyClick(int key, int modifiers, int delay);

    bool keyPressChar(const QString &character, int modifiers, int delay);
    bool keyReleaseChar(const QString &character, int modifiers, int delay);
    bool keyClickChar(const QString &character, int modifiers, int delay);

    Q_REVISION(2) bool keySequence(const QVariant &keySequence);

    bool mousePress(QObject *item, qreal x, qreal y, int button,
                    int modifiers, int delay);
    bool mouseRelease(QObject *item, qreal x, qreal y, int button,
                      int modifiers, int delay);
    bool mouseClick(QObject *item, qreal x, qreal y, int button,
                    int modifiers, int delay);
    bool mouseDoubleClick(QObject *item, qreal x, qreal y, int button,
                          int modifiers, int delay);
    bool mouseDoubleClickSequence(QObject *item, qreal x, qreal y, int button,
                          int modifiers, int delay);
    bool mouseMove(QObject *item, qreal x, qreal y, int delay, int buttons);

#if QT_CONFIG(wheelevent)
    bool mouseWheel(QObject *item, qreal x, qreal y, int buttons,
               int modifiers, int xDelta, int yDelta, int delay);
#endif

    QQuickTouchEventSequence *touchEvent(QObject *item = nullptr);
private:
    QWindow *eventWindow(QObject *item = nullptr);
    QWindow *activeWindow();
    QTouchDevice *touchDevice();

    Qt::MouseButtons m_pressedButtons;

    friend class QQuickTouchEventSequence;
};

QT_END_NAMESPACE

#endif
