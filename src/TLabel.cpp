/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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
#include "TEvent.h"

#include "pre_guard.h"
#include <QtEvents>
#include "post_guard.h"

QMap<Qt::MouseButton, QString> TLabel::mMouseButtons = {
    {Qt::NoButton, QStringLiteral("NoButton")},
    {Qt::LeftButton, QStringLiteral("LeftButton")},
    {Qt::RightButton, QStringLiteral("RightButton")},
    {Qt::MidButton, QStringLiteral("MidButton")},
    {Qt::BackButton, QStringLiteral("BackButton")},
    {Qt::ForwardButton, QStringLiteral("ForwardButton")},
    {Qt::TaskButton, QStringLiteral("TaskButton")},
    {Qt::ExtraButton4, QStringLiteral("ExtraButton4")},
    {Qt::ExtraButton5, QStringLiteral("ExtraButton5")},
    {Qt::ExtraButton6, QStringLiteral("ExtraButton6")},
    {Qt::ExtraButton7, QStringLiteral("ExtraButton7")},
    {Qt::ExtraButton8, QStringLiteral("ExtraButton8")},
    {Qt::ExtraButton9, QStringLiteral("ExtraButton9")},
    {Qt::ExtraButton10, QStringLiteral("ExtraButton10")},
    {Qt::ExtraButton11, QStringLiteral("ExtraButton11")},
    {Qt::ExtraButton12, QStringLiteral("ExtraButton12")},
    {Qt::ExtraButton13, QStringLiteral("ExtraButton13")},
    {Qt::ExtraButton14, QStringLiteral("ExtraButton14")},
    {Qt::ExtraButton15, QStringLiteral("ExtraButton15")},
    {Qt::ExtraButton16, QStringLiteral("ExtraButton16")},
    {Qt::ExtraButton17, QStringLiteral("ExtraButton17")},
    {Qt::ExtraButton18, QStringLiteral("ExtraButton18")},
    {Qt::ExtraButton19, QStringLiteral("ExtraButton19")},
    {Qt::ExtraButton20, QStringLiteral("ExtraButton20")},
    {Qt::ExtraButton21, QStringLiteral("ExtraButton21")},
    {Qt::ExtraButton22, QStringLiteral("ExtraButton22")},
    {Qt::ExtraButton23, QStringLiteral("ExtraButton23")},
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

void TLabel::setRelease(Host* pHost, const QString& func, const TEvent& args)
{
    mpHost = pHost;
    mRelease = func;
    mReleaseParams = args;
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

void TLabel::mouseReleaseEvent(QMouseEvent* event)
{
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

void TLabel::leaveEvent(QEvent* event)
{
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
    if (mEnter != "") {
        if (mpHost) {
            mpHost->getLuaInterpreter()->callEventHandler(mEnter, mEnterParams);
        }
        event->accept();
        return;
    }
    QWidget::enterEvent(event);
}
