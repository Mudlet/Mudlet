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

#ifndef QSTYLEANIMATION_P_H
#define QSTYLEANIMATION_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qabstractanimation.h"
#include "qdatetime.h"
#include "qimage.h"

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qcommonstyle.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

class Q_WIDGETS_EXPORT QStyleAnimation : public QAbstractAnimation
{
    Q_OBJECT

public:
    QStyleAnimation(QObject *target);
    virtual ~QStyleAnimation();

    QObject *target() const;

    int duration() const override;
    void setDuration(int duration);

    int delay() const;
    void setDelay(int delay);

    QTime startTime() const;
    void setStartTime(const QTime &time);

    enum FrameRate {
        DefaultFps,
        SixtyFps,
        ThirtyFps,
        TwentyFps,
        FifteenFps
    };

    FrameRate frameRate() const;
    void setFrameRate(FrameRate fps);

    void updateTarget();

public Q_SLOTS:
    void start();

protected:
    virtual bool isUpdateNeeded() const;
    virtual void updateCurrentTime(int time) override;

private:
    int _delay;
    int _duration;
    QTime _startTime;
    FrameRate _fps;
    int _skip;
};

class Q_WIDGETS_EXPORT QProgressStyleAnimation : public QStyleAnimation
{
    Q_OBJECT

public:
    QProgressStyleAnimation(int speed, QObject *target);

    int animationStep() const;
    int progressStep(int width) const;

    int speed() const;
    void setSpeed(int speed);

protected:
    bool isUpdateNeeded() const override;

private:
    int _speed;
    mutable int _step;
};

class Q_WIDGETS_EXPORT QNumberStyleAnimation : public QStyleAnimation
{
    Q_OBJECT

public:
    QNumberStyleAnimation(QObject *target);

    qreal startValue() const;
    void setStartValue(qreal value);

    qreal endValue() const;
    void setEndValue(qreal value);

    qreal currentValue() const;

protected:
    bool isUpdateNeeded() const override;

private:
    qreal _start;
    qreal _end;
    mutable qreal _prev;
};

class Q_WIDGETS_EXPORT QBlendStyleAnimation : public QStyleAnimation
{
    Q_OBJECT

public:
    enum Type { Transition, Pulse };

    QBlendStyleAnimation(Type type, QObject *target);

    QImage startImage() const;
    void setStartImage(const QImage& image);

    QImage endImage() const;
    void setEndImage(const QImage& image);

    QImage currentImage() const;

protected:
    virtual void updateCurrentTime(int time) override;

private:
    Type _type;
    QImage _start;
    QImage _end;
    QImage _current;
};

class Q_WIDGETS_EXPORT QScrollbarStyleAnimation : public QNumberStyleAnimation
{
    Q_OBJECT

public:
    enum Mode { Activating, Deactivating };

    QScrollbarStyleAnimation(Mode mode, QObject *target);

    Mode mode() const;

    bool wasActive() const;
    void setActive(bool active);

private slots:
    void updateCurrentTime(int time) override;

private:
    Mode _mode;
    bool _active;
};

QT_END_NAMESPACE

#endif // QSTYLEANIMATION_P_H
