/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
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

#include <QApplication>

#include "TLabel.h"

#include "Host.h"
#include "TEvent.h"

#include "pre_guard.h"
#include <QtEvents>
#include "post_guard.h"


const QMap<Qt::MouseButton, QString> TLabel::mMouseButtons = {
        {Qt::NoButton, QStringLiteral("NoButton")},           {Qt::LeftButton, QStringLiteral("LeftButton")},       {Qt::RightButton, QStringLiteral("RightButton")},
        {Qt::MidButton, QStringLiteral("MidButton")},         {Qt::BackButton, QStringLiteral("BackButton")},       {Qt::ForwardButton, QStringLiteral("ForwardButton")},
        {Qt::TaskButton, QStringLiteral("TaskButton")},       {Qt::ExtraButton4, QStringLiteral("ExtraButton4")},   {Qt::ExtraButton5, QStringLiteral("ExtraButton5")},
        {Qt::ExtraButton6, QStringLiteral("ExtraButton6")},   {Qt::ExtraButton7, QStringLiteral("ExtraButton7")},   {Qt::ExtraButton8, QStringLiteral("ExtraButton8")},
        {Qt::ExtraButton9, QStringLiteral("ExtraButton9")},   {Qt::ExtraButton10, QStringLiteral("ExtraButton10")}, {Qt::ExtraButton11, QStringLiteral("ExtraButton11")},
        {Qt::ExtraButton12, QStringLiteral("ExtraButton12")}, {Qt::ExtraButton13, QStringLiteral("ExtraButton13")}, {Qt::ExtraButton14, QStringLiteral("ExtraButton14")},
        {Qt::ExtraButton15, QStringLiteral("ExtraButton15")}, {Qt::ExtraButton16, QStringLiteral("ExtraButton16")}, {Qt::ExtraButton17, QStringLiteral("ExtraButton17")},
        {Qt::ExtraButton18, QStringLiteral("ExtraButton18")}, {Qt::ExtraButton19, QStringLiteral("ExtraButton19")}, {Qt::ExtraButton20, QStringLiteral("ExtraButton20")},
        {Qt::ExtraButton21, QStringLiteral("ExtraButton21")}, {Qt::ExtraButton22, QStringLiteral("ExtraButton22")}, {Qt::ExtraButton23, QStringLiteral("ExtraButton23")},
        {Qt::ExtraButton24, QStringLiteral("ExtraButton24")},

};



TLabel::TLabel(QWidget* pW) : QLabel(pW), mpHost(nullptr), mouseInside()
{
    setMouseTracking(true);
}

QString nothing = "";

void TLabel::setClick(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mClick = func;
    mClickParams = args;
}

void TLabel::setDoubleClick(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mDoubleClick = func;
    mDoubleClickParams = args;
}

void TLabel::setRelease(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mRelease = func;
    mReleaseParams = args;
}

void TLabel::setMove(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mMove = func;
    mMoveParams = args;
}

void TLabel::setWheel(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mWheel = func;
    mWheelParams = args;
}

void TLabel::setEnter(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mEnter = func;
    mEnterParams = args;
}

void TLabel::setLeave(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mLeave = func;
    mLeaveParams = args;
}

void TLabel::mousePressEvent(QMouseEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }
    if (mMouseButtons.contains(event->button())) {
        if (mpHost) {
            TEvent tmpClickParams = mClickParams;
            tmpClickParams.mArgumentList.append(mMouseButtons.value(event->button()));
            tmpClickParams.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mpHost->getLuaInterpreter()->callEventHandler(mClick, tmpClickParams);
        }
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void TLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }
    if (mMouseButtons.contains(event->button())) {
        if (mpHost) {
            TEvent tmpDoubleClickParams = mDoubleClickParams;
            tmpDoubleClickParams.mArgumentList.append(mMouseButtons.value(event->button()));
            tmpDoubleClickParams.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mpHost->getLuaInterpreter()->callEventHandler(mDoubleClick, tmpDoubleClickParams);
        }
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mMouseButtons.contains(event->button())) {
        if (mpHost) {
            TEvent tmpReleaseParams = mReleaseParams;
            tmpReleaseParams.mArgumentList.append(mMouseButtons.value(event->button()));
            tmpReleaseParams.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mpHost->getLuaInterpreter()->callEventHandler(mRelease, tmpReleaseParams);
        }
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void TLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }
    else if (mpHost) {
        TEvent tmpMoveParams = mMoveParams;
        for( auto button : mMouseButtons.keys())
        {
            if (button & event->buttons()) {
                tmpMoveParams.mArgumentList.append(mMouseButtons.value(button));
                tmpMoveParams.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            }

        }
        mpHost->getLuaInterpreter()->callEventHandler(mMove, tmpMoveParams);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);

}

void TLabel::wheelEvent(QWheelEvent* event)
{
    if (forwardEventToMapper(event))
        return;
    else if (mpHost) {
        TEvent tmpWheelParams = mWheelParams;
        tmpWheelParams.mArgumentList.append(QString::number(event->angleDelta().x()/8));
        tmpWheelParams.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        tmpWheelParams.mArgumentList.append(QString::number(event->angleDelta().y()/8));
        tmpWheelParams.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mpHost->getLuaInterpreter()->callEventHandler(mWheel, tmpWheelParams);
        event->accept();
        return;
    }
}

void TLabel::leaveEvent(QEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mLeave != "") {
        if (mpHost) {
            mpHost->getLuaInterpreter()->callEventHandler(mLeave, mLeaveParams);
        }
        event->accept();
        return;
    }
    QWidget::leaveEvent(event);
}

void TLabel::enterEvent(QEvent* event)
{
    if (forwardEventToMapper(event)) {
        return;
    }

    if (mEnter != "") {
        if (mpHost) {
            mpHost->getLuaInterpreter()->callEventHandler(mEnter, mEnterParams);
        }
        event->accept();
        return;
    }
    QWidget::enterEvent(event);
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
    case (QEvent::MouseButtonDblClick):
    case (QEvent::MouseButtonRelease):
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
            QWheelEvent newEvent(qw->mapFromGlobal(wheelEvent->globalPos()),
                                 wheelEvent->globalPos(),
                                 wheelEvent->pixelDelta(),
                                 wheelEvent->angleDelta(),
                                 wheelEvent->angleDelta().y() / 8,
                                 Qt::Vertical,
                                 wheelEvent->buttons(),
                                 wheelEvent->modifiers(),
                                 wheelEvent->phase());
            qApp->sendEvent(qw, &newEvent);
            return true;
        }
        break;
    }
    }
    return false;
}
