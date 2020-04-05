/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "Host.h"

#include "pre_guard.h"
#include <QApplication>
#include <QtEvents>
#include "post_guard.h"


TLabel::TLabel(Host* pH, QWidget* pW)
: QLabel(pW)
, mpHost(pH)
{
    setMouseTracking(true);
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
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mpHost && mClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mClickFunction, event);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void TLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mpHost && mDoubleClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mDoubleClickFunction, event);
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void TLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
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
    if (forwardEventToMapper(event)) {
        return;
    }
    if (mpHost && mMoveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mMoveFunction, event);
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void TLabel::wheelEvent(QWheelEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mpHost && mWheelFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mWheelFunction, event);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void TLabel::leaveEvent(QEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mpHost && mLeaveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mLeaveFunction, event);
        event->accept();
    } else {
        QWidget::leaveEvent(event);
    }
}

void TLabel::enterEvent(QEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mpHost && mEnterFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mEnterFunction, event);
        event->accept();
    } else {
        QWidget::enterEvent(event);
    }
}

bool TLabel::forwardEventToMapper(QEvent* event)
{
    // This function implements a workaround to the issue of the mapper not receiving
    //   mouse events while sharing space with labels, regardless of z-level. It works
    //   by checking, when a label receives a mouse event, if the top-most widget at
    //   the event's location is a child of the mapper object. If so, it redirects the
    //   event there manually.

    switch (event->type()) {
    case (QEvent::MouseButtonPress):
        [[fallthrough]];
    case (QEvent::MouseButtonDblClick):
        [[fallthrough]];
    case (QEvent::MouseButtonRelease):
        [[fallthrough]];
    case (QEvent::MouseMove): {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        QWidget* qw = qApp->widgetAt(mouseEvent->globalPos());

        if (qw && parentWidget()->findChild<QWidget*>(QStringLiteral("mapper")) && parentWidget()->findChild<QWidget*>(QStringLiteral("mapper"))->isAncestorOf(qw)) {
            QMouseEvent newEvent(mouseEvent->type(), qw->mapFromGlobal(mouseEvent->globalPos()), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
            qApp->sendEvent(qw, &newEvent);
            return true;
        }
        break;
    }
    case (QEvent::Enter):
        [[fallthrough]];
    case (QEvent::Leave): {
        QWidget* qw = qApp->widgetAt(QCursor::pos());

        if (qw && parentWidget()->findChild<QWidget*>(QStringLiteral("mapper")) && parentWidget()->findChild<QWidget*>(QStringLiteral("mapper"))->isAncestorOf(qw)) {
            QEvent newEvent(event->type());
            qApp->sendEvent(qw, &newEvent);
            return true;
        }
        break;
    }
    case (QEvent::Wheel): {
        auto wheelEvent = static_cast<QWheelEvent*>(event);
        QWidget* qw = qApp->widgetAt(wheelEvent->globalPos());

        if (qw && parentWidget()->findChild<QWidget*>(QStringLiteral("mapper")) && parentWidget()->findChild<QWidget*>(QStringLiteral("mapper"))->isAncestorOf(qw)) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
            // Have switched to the latest QWheelEvent as that handles both X
            // and Y wheels at the same time whereas previously we said the
            // event was a vertical one - even if it wasn't! Additionally we
            // pass on the source of the Qt event - and whether the delta values
            // are inverted:
            QWheelEvent newEvent(qw->mapFromGlobal(wheelEvent->globalPos()),
                                 wheelEvent->globalPos(),
                                 wheelEvent->pixelDelta(),
                                 wheelEvent->angleDelta(),
                                 wheelEvent->buttons(),
                                 wheelEvent->modifiers(),
                                 wheelEvent->phase(),
                                 wheelEvent->inverted(),
                                 wheelEvent->source());
#else
            // Unfortunately it was only introduced in Qt 5.12 and Qt didn't
            // document that initially... see
            // https://bugreports.qt.io/browse/QTBUG-80088 !
            // Anyhow QWheelEvent::delta() and QWheelEvent::orientation() are
            // Qt4 relics and have been declared obsolete for new code, but we
            // can still use them for older Qt versions:
            QWheelEvent newEvent(qw->mapFromGlobal(wheelEvent->globalPos()),
                                 wheelEvent->globalPos(),
                                 wheelEvent->pixelDelta(),
                                 wheelEvent->angleDelta(),
                                 wheelEvent->delta(),
                                 wheelEvent->orientation(),
                                 wheelEvent->buttons(),
                                 wheelEvent->modifiers(),
                                 wheelEvent->phase(),
                                 wheelEvent->source(),
                                 wheelEvent->inverted());
#endif

            qApp->sendEvent(qw, &newEvent);
            return true;
        }
        break;
    }
    }
    return false;
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
