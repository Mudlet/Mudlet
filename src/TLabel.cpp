/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2020, 2023 by Stephen Lyons - slysven@virginmedia.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "TLabel.h"
#include "TConsole.h"
#include "TDockWidget.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtEvents>
#include "post_guard.h"


TLabel::TLabel(Host* pH, const QString& name, QWidget* pW)
: QLabel(pW)
, mpHost(pH)
, mName(name)
{
    setMouseTracking(true);
    setObjectName(qsl("label_%1_%2").arg(pH->getName(), mName));
}

TLabel::~TLabel()
{
    if (mpMovie) {
        mpMovie->deleteLater();
        mpMovie = nullptr;
    }
}

void TLabel::setClick(const int func)
{
    releaseFunc(mClickFunction, func);
    mClickFunction = func;
}

void TLabel::setDoubleClick(const int func)
{
    releaseFunc(mDoubleClickFunction, func);
    mDoubleClickFunction = func;
}

void TLabel::setRelease(const int func)
{
    releaseFunc(mReleaseFunction, func);
    mReleaseFunction = func;
}

void TLabel::setMove(const int func)
{
    releaseFunc(mMoveFunction, func);
    mMoveFunction = func;
}

void TLabel::setWheel(const int func)
{
    releaseFunc(mWheelFunction, func);
    mWheelFunction = func;
}

void TLabel::setEnter(const int func)
{
    releaseFunc(mEnterFunction, func);
    mEnterFunction = func;
}

void TLabel::setLeave(const int func)
{
    releaseFunc(mLeaveFunction, func);
    mLeaveFunction = func;
}

void TLabel::mousePressEvent(QMouseEvent* event)
{

    if (mpHost && mClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mClickFunction, event);
        // The use of accept() here prevents the propagation of the event to
        // any parent, e.g. the containing TConsole
        event->accept();
        mudlet::self()->activateProfile(mpHost);
    } else {
        QWidget::mousePressEvent(event);
    }
}

void TLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (mpHost && mDoubleClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mDoubleClickFunction, event);
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void TLabel::mouseReleaseEvent(QMouseEvent* event)
{
    auto labelParent = qobject_cast<TConsole*>(parent());
    if (labelParent && labelParent->mpDockWidget && labelParent->mpDockWidget->isFloating()) {
        // move focus back to the active console / command line:
        mudlet::self()->activateProfile(mpHost);
    }

    if (mpHost && mReleaseFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mReleaseFunction, event);
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void TLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (mpHost && mMoveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mMoveFunction, event);
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void TLabel::wheelEvent(QWheelEvent* event)
{

    if (mpHost && mWheelFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mWheelFunction, event);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void TLabel::leaveEvent(QEvent* event)
{
    if (mpHost && mLeaveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mLeaveFunction, event);
        event->accept();
    } else {
        QWidget::leaveEvent(event);
    }
}

void TLabel::enterEvent(QEnterEventType* event)
{
    if (mpHost && mEnterFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mEnterFunction, event);
        event->accept();
    } else {
        QWidget::enterEvent(event);
    }
}

void TLabel::resizeEvent(QResizeEvent* event)
{
    emit resized();
    QWidget::resizeEvent(event);
}


// This function deferences previous functions in the Lua registry.
// This allows the functions to be safely overwritten.
void TLabel::releaseFunc(const int existingFunction, const int newFunction)
{
    if (newFunction != existingFunction) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(existingFunction);
    }
}

void TLabel::setClickThrough(bool clickthrough)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, clickthrough);
}
