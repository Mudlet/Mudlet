/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "TTrigger.h"
#include "Host.h"
#include "mudlet.h"
#include "TTextEdit.h"

#include "pre_guard.h"
#include <QPalette>
#include "post_guard.h"


dlgColorTrigger::dlgColorTrigger( QWidget * pF, TTrigger * pT, int mode )
: QDialog( pF )
, mpTrigger( pT )
, mMode( mode )
{
    // init generated dialog
    setupUi(this);
    connect(closeButton, SIGNAL(pressed()), this, SLOT(slot_save_and_exit()));
    connect(pushButton_black, SIGNAL(clicked()), this, SLOT(setColorBlack()));
    QPalette palette;
    QString styleSheet;
    QColor color;
    color = mpTrigger->mpHost->mBlack;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_black->setStyleSheet( styleSheet );

    connect(pushButton_Lblack, SIGNAL(clicked()), this, SLOT(setColorLightBlack()));
    color = mpTrigger->mpHost->mLightBlack;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblack->setStyleSheet( styleSheet );

    connect(pushButton_green, SIGNAL(clicked()), this, SLOT(setColorGreen()));
    color = mpTrigger->mpHost->mGreen;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_green->setStyleSheet( styleSheet );

    connect(pushButton_Lgreen, SIGNAL(clicked()), this, SLOT(setColorLightGreen()));
    color = mpTrigger->mpHost->mLightGreen;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lgreen->setStyleSheet( styleSheet );

    connect(pushButton_red, SIGNAL(clicked()), this, SLOT(setColorRed()));
    color = mpTrigger->mpHost->mRed;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_red->setStyleSheet( styleSheet );

    connect(pushButton_Lred, SIGNAL(clicked()), this, SLOT(setColorLightRed()));
    color = mpTrigger->mpHost->mLightRed;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lred->setStyleSheet( styleSheet );

    connect(pushButton_blue, SIGNAL(clicked()), this, SLOT(setColorBlue()));
    color = mpTrigger->mpHost->mBlue;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_blue->setStyleSheet( styleSheet );

    connect(pushButton_Lblue, SIGNAL(clicked()), this, SLOT(setColorLightBlue()));
    color = mpTrigger->mpHost->mLightBlue;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblue->setStyleSheet( styleSheet );

    connect(pushButton_yellow, SIGNAL(clicked()), this, SLOT(setColorYellow()));
    color = mpTrigger->mpHost->mYellow;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_yellow->setStyleSheet( styleSheet );

    connect(pushButton_Lyellow, SIGNAL(clicked()), this, SLOT(setColorLightYellow()));
    color = mpTrigger->mpHost->mLightYellow;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lyellow->setStyleSheet( styleSheet );

    connect(pushButton_cyan, SIGNAL(clicked()), this, SLOT(setColorCyan()));
    color = mpTrigger->mpHost->mCyan;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_cyan->setStyleSheet( styleSheet );

    connect(pushButton_Lcyan, SIGNAL(clicked()), this, SLOT(setColorLightCyan()));
    color = mpTrigger->mpHost->mLightCyan;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lcyan->setStyleSheet( styleSheet );

    connect(pushButton_magenta, SIGNAL(clicked()), this, SLOT(setColorMagenta()));
    color = mpTrigger->mpHost->mMagenta;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_magenta->setStyleSheet( styleSheet );

    connect(pushButton_Lmagenta, SIGNAL(clicked()), this, SLOT(setColorLightMagenta()));
    color = mpTrigger->mpHost->mLightMagenta;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lmagenta->setStyleSheet( styleSheet );

    connect(pushButton_white, SIGNAL(clicked()), this, SLOT(setColorWhite()));
    color = mpTrigger->mpHost->mWhite;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_white->setStyleSheet( styleSheet );

    connect(pushButton_Lwhite, SIGNAL(clicked()), this, SLOT(setColorLightWhite()));
    color = mpTrigger->mpHost->mLightWhite;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lwhite->setStyleSheet( styleSheet );
}

void dlgColorTrigger::setColorBlack()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 2;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mBlack;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 2;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mBlack;
    }
    close();
}

void dlgColorTrigger::setColorLightBlack()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 1;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightBlack;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 1;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightBlack;
    }
    close();
}

void dlgColorTrigger::setColorRed()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 4;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mRed;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 4;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mRed;
    }
    close();
}

void dlgColorTrigger::setColorLightRed()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 3;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightRed;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 3;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightRed;
    }
    close();
}

void dlgColorTrigger::setColorGreen()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 6;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mGreen;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 6;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mGreen;
    }
    close();
}
void dlgColorTrigger::setColorLightGreen()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 5;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightGreen;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 5;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightGreen;
    }
    close();
}

void dlgColorTrigger::setColorBlue()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 10;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mBlue;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 10;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mBlue;
    }
    close();
}
void dlgColorTrigger::setColorLightBlue()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 9;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightBlue;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 9;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightBlue;
    }
    close();
}

void dlgColorTrigger::setColorYellow()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 8;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mYellow;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 8;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mYellow;
    }
    close();
}
void dlgColorTrigger::setColorLightYellow()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 7;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightYellow;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 7;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightYellow;
    }
    close();
}

void dlgColorTrigger::setColorCyan()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 14;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mCyan;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 14;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mCyan;
    }
    close();
}
void dlgColorTrigger::setColorLightCyan()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 13;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightCyan;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 13;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightCyan;
    }
    close();
}

void dlgColorTrigger::setColorMagenta()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 12;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mMagenta;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 12;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mMagenta;
    }
    close();
}
void dlgColorTrigger::setColorLightMagenta()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 11;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightMagenta;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 11;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightMagenta;
    }
    close();
}

void dlgColorTrigger::setColorWhite()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 16;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mWhite;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 16;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mWhite;
    }
    close();
}
void dlgColorTrigger::setColorLightWhite()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgAnsi = 15;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightWhite;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerBgAnsi = 15;
        mpTrigger->mColorTriggerBgColor = mpTrigger->mpHost->mLightWhite;
    }
    close();
}

void dlgColorTrigger::slot_save_and_exit()
{
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerBg = false;
    }
    else
    {
        mpTrigger->mColorTriggerFg = false;
    }

    close();
}
