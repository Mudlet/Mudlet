#include "dlgColorTrigger.h"
/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

#include <QMainWindow>
#include <QPalette>
#include "TTrigger.h"
#include "dlgColorTrigger.h"
#include "Host.h"
#include "mudlet.h"
#include "TTextEdit.h"

dlgColorTrigger::dlgColorTrigger( QWidget * pF, TTrigger * pT, int mode )
: QDialog( pF )
, mpTrigger( pT )
, mMode( mode )
{
    // init generated dialog
    setupUi(this);
    connect(closeButton, SIGNAL(pressed()), this, SLOT(slot_save_and_exit()));
    connect(pushButton_black, SIGNAL(clicked()), this, SLOT(setColorBlack()));
    connect(pushButton_Lblack, SIGNAL(clicked()), this, SLOT(setColorLightBlack()));
    connect(pushButton_green, SIGNAL(clicked()), this, SLOT(setColorGreen()));
    connect(pushButton_Lgreen, SIGNAL(clicked()), this, SLOT(setColorLightGreen()));
    connect(pushButton_red, SIGNAL(clicked()), this, SLOT(setColorRed()));
    connect(pushButton_Lred, SIGNAL(clicked()), this, SLOT(setColorLightRed()));
    connect(pushButton_blue, SIGNAL(clicked()), this, SLOT(setColorBlue()));
    connect(pushButton_Lblue, SIGNAL(clicked()), this, SLOT(setColorLightBlue()));
    connect(pushButton_yellow, SIGNAL(clicked()), this, SLOT(setColorYellow()));
    connect(pushButton_Lyellow, SIGNAL(clicked()), this, SLOT(setColorLightYellow()));
    connect(pushButton_cyan, SIGNAL(clicked()), this, SLOT(setColorCyan()));
    connect(pushButton_Lcyan, SIGNAL(clicked()), this, SLOT(setColorLightCyan()));
    connect(pushButton_magenta, SIGNAL(clicked()), this, SLOT(setColorMagenta()));
    connect(pushButton_Lmagenta, SIGNAL(clicked()), this, SLOT(setColorLightMagenta()));
    connect(pushButton_white, SIGNAL(clicked()), this, SLOT(setColorWhite()));
    connect(pushButton_Lwhite, SIGNAL(clicked()), this, SLOT(setColorLightWhite()));
}

void dlgColorTrigger::setColorBlack()
{
    if( ! mpTrigger ) return;
    mpTrigger->mColorTrigger = true;
    if( mMode == 0 )
    {
        mpTrigger->mColorTriggerFg = true;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mBlack;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mBlack;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightBlack;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mRed;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightRed;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mGreen;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightGreen;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mBlue;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightBlue;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mYellow;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightYellow;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mCyan;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightCyan;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mMagenta;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightMagenta;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mWhite;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
        mpTrigger->mColorTriggerFgColor = mpTrigger->mpHost->mLightWhite;
    }
    else
    {
        mpTrigger->mColorTriggerBg = true;
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
