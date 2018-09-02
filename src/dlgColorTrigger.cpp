/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "dlgColorTrigger.h"


#include "Host.h"
#include "TTextEdit.h"
#include "TTrigger.h"
#include "mudlet.h"


dlgColorTrigger::dlgColorTrigger(QWidget* pF, TTrigger* pT, int mode) : QDialog(pF), mpTrigger(pT), mMode(mode)
{
    // init generated dialog
    setupUi(this);
    connect(closeButton, &QAbstractButton::pressed, this, &dlgColorTrigger::slot_save_and_exit);
    connect(pushButton_black, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorBlack);
    QPalette palette;
    QString styleSheet;
    QColor color;
    color = mpTrigger->mpHost->mBlack;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_black->setStyleSheet(styleSheet);

    connect(pushButton_Lblack, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightBlack);
    color = mpTrigger->mpHost->mLightBlack;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lblack->setStyleSheet(styleSheet);

    connect(pushButton_green, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorGreen);
    color = mpTrigger->mpHost->mGreen;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_green->setStyleSheet(styleSheet);

    connect(pushButton_Lgreen, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightGreen);
    color = mpTrigger->mpHost->mLightGreen;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lgreen->setStyleSheet(styleSheet);

    connect(pushButton_red, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorRed);
    color = mpTrigger->mpHost->mRed;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_red->setStyleSheet(styleSheet);

    connect(pushButton_Lred, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightRed);
    color = mpTrigger->mpHost->mLightRed;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lred->setStyleSheet(styleSheet);

    connect(pushButton_blue, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorBlue);
    color = mpTrigger->mpHost->mBlue;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_blue->setStyleSheet(styleSheet);

    connect(pushButton_Lblue, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightBlue);
    color = mpTrigger->mpHost->mLightBlue;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lblue->setStyleSheet(styleSheet);

    connect(pushButton_yellow, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorYellow);
    color = mpTrigger->mpHost->mYellow;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_yellow->setStyleSheet(styleSheet);

    connect(pushButton_Lyellow, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightYellow);
    color = mpTrigger->mpHost->mLightYellow;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lyellow->setStyleSheet(styleSheet);

    connect(pushButton_cyan, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorCyan);
    color = mpTrigger->mpHost->mCyan;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_cyan->setStyleSheet(styleSheet);

    connect(pushButton_Lcyan, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightCyan);
    color = mpTrigger->mpHost->mLightCyan;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lcyan->setStyleSheet(styleSheet);

    connect(pushButton_magenta, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorMagenta);
    color = mpTrigger->mpHost->mMagenta;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_magenta->setStyleSheet(styleSheet);

    connect(pushButton_Lmagenta, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightMagenta);
    color = mpTrigger->mpHost->mLightMagenta;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lmagenta->setStyleSheet(styleSheet);

    connect(pushButton_white, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorWhite);
    color = mpTrigger->mpHost->mWhite;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_white->setStyleSheet(styleSheet);

    connect(pushButton_Lwhite, &QAbstractButton::clicked, this, &dlgColorTrigger::setColorLightWhite);
    color = mpTrigger->mpHost->mLightWhite;
    palette.setColor(QPalette::Button, color);
    styleSheet = QStringLiteral("QPushButton{background-color:") + color.name() + QStringLiteral(";}");
    pushButton_Lwhite->setStyleSheet(styleSheet);
}

void dlgColorTrigger::setColorBlack()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 2;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mBlack;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 2;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mBlack;
    }
    close();
}

void dlgColorTrigger::setColorLightBlack()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 1;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightBlack;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 1;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightBlack;
    }
    close();
}

void dlgColorTrigger::setColorRed()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 4;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mRed;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 4;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mRed;
    }
    close();
}

void dlgColorTrigger::setColorLightRed()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 3;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightRed;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 3;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightRed;
    }
    close();
}

void dlgColorTrigger::setColorGreen()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 6;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mGreen;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 6;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mGreen;
    }
    close();
}
void dlgColorTrigger::setColorLightGreen()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 5;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightGreen;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 5;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightGreen;
    }
    close();
}

void dlgColorTrigger::setColorBlue()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 10;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mBlue;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 10;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mBlue;
    }
    close();
}
void dlgColorTrigger::setColorLightBlue()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 9;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightBlue;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 9;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightBlue;
    }
    close();
}

void dlgColorTrigger::setColorYellow()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 8;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mYellow;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 8;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mYellow;
    }
    close();
}
void dlgColorTrigger::setColorLightYellow()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 7;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightYellow;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 7;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightYellow;
    }
    close();
}

void dlgColorTrigger::setColorCyan()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 14;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mCyan;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 14;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mCyan;
    }
    close();
}
void dlgColorTrigger::setColorLightCyan()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 13;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightCyan;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 13;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightCyan;
    }
    close();
}

void dlgColorTrigger::setColorMagenta()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 12;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mMagenta;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 12;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mMagenta;
    }
    close();
}
void dlgColorTrigger::setColorLightMagenta()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 11;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightMagenta;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 11;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightMagenta;
    }
    close();
}

void dlgColorTrigger::setColorWhite()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 16;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mWhite;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 16;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mWhite;
    }
    close();
}
void dlgColorTrigger::setColorLightWhite()
{
    if (!mpTrigger) {
        return;
    }
    mpTrigger->mColorTrigger = true;
    if (mMode == 0) {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 15;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightWhite;
    } else {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 15;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightWhite;
    }
    close();
}

void dlgColorTrigger::slot_save_and_exit()
{
    if (mMode == 0) {
        mpTrigger->mColorTriggerBg = false;
    } else {
        mpTrigger->mColorTriggerFg = false;
    }

    close();
}
