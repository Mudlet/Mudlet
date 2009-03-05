/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn   *
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
#include <QColorDialog>
#include <QPalette>
#include <QFontDialog>
#include <QFont>

#include "dlgProfilePreferences.h"
#include "Host.h"
#include "mudlet.h"

dlgProfilePreferences::dlgProfilePreferences( QWidget * pF ):QDialog( pF )
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
    connect(pushButton_foreground_color, SIGNAL(clicked()), this, SLOT(setFgColor()));
    connect(pushButton_background_color, SIGNAL(clicked()), this, SLOT(setBgColor()));
    connect(reset_colors_button, SIGNAL(clicked()), this, SLOT(resetColors()));
    connect(pushButton_display_font, SIGNAL(clicked()), this, SLOT(setDisplayFont()));
    connect(pushButton_command_line_font, SIGNAL(clicked()), this, SLOT(setCommandLineFont()));

    Host * pHost = mudlet::self()->getActiveHost();
    if( pHost )
    {
        setColors();

        wrap_at_lineEdit->setText( QString::number(pHost->mWrapAt));    
        indent_wrapped_lineEdit->setText(QString::number(pHost->mWrapIndentCount));

        show_sent_text_checkbox->setChecked(pHost->mPrintCommand);
        auto_clear_input_line_checkbox->setChecked(pHost->mAutoClearCommandLineAfterSend);
        command_separator_lineedit->setText( pHost->mCommandSeparator);
        disable_auto_completion_checkbox->setChecked(pHost->mDisableAutoCompletion);
        
        checkBox_USE_IRE_DRIVER_BUGFIX->setChecked( pHost->mUSE_IRE_DRIVER_BUGFIX );
        
        QFile file_use_smallscreen( QDir::homePath()+"/.config/mudlet/mudlet_option_use_smallscreen" );
        if( file_use_smallscreen.exists() )
            checkBox_USE_SMALL_SCREEN->setChecked( true );
        else 
            checkBox_USE_SMALL_SCREEN->setChecked( false );
    }
}

void dlgProfilePreferences::setColors()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;

    QPalette palette;
    QColor color;
    color = pHost->mFgColor;
    palette.setColor( QPalette::Button, color );
    pushButton_foreground_color->setPalette( palette );

    color = pHost->mBgColor;
    palette.setColor( QPalette::Button, color );
    pushButton_background_color->setPalette( palette );

    color = pHost->mBlack;
    palette.setColor( QPalette::Button, color );
    pushButton_black->setPalette( palette );

    color = pHost->mLightBlack;
    palette.setColor( QPalette::Button, color );
    pushButton_Lblack->setPalette( palette );

    color = pHost->mRed;
    palette.setColor( QPalette::Button, color );
    pushButton_red->setPalette( palette );

    color = pHost->mLightRed;
    palette.setColor( QPalette::Button, color );
    pushButton_Lred->setPalette( palette );

    color = pHost->mGreen;
    palette.setColor( QPalette::Button, color );
    pushButton_green->setPalette( palette );

    color = pHost->mLightGreen;
    palette.setColor( QPalette::Button, color );
    pushButton_Lgreen->setPalette( palette );

    color = pHost->mBlue;
    palette.setColor( QPalette::Button, color );
    pushButton_blue->setPalette( palette );

    color = pHost->mLightBlue;
    palette.setColor( QPalette::Button, color );
    pushButton_Lblue->setPalette( palette );

    color = pHost->mYellow;
    palette.setColor( QPalette::Button, color );
    pushButton_yellow->setPalette( palette );

    color = pHost->mLightYellow;
    palette.setColor( QPalette::Button, color );
    pushButton_Lyellow->setPalette( palette );

    color = pHost->mCyan;
    palette.setColor( QPalette::Button, color );
    pushButton_cyan->setPalette( palette );

    color = pHost->mLightCyan;
    palette.setColor( QPalette::Button, color );
    pushButton_Lcyan->setPalette( palette );

    color = pHost->mMagenta;
    palette.setColor( QPalette::Button, color );
    pushButton_magenta->setPalette( palette );

    color = pHost->mLightMagenta;
    palette.setColor( QPalette::Button, color );
    pushButton_Lmagenta->setPalette( palette );

    color = pHost->mWhite;
    palette.setColor( QPalette::Button, color );
    pushButton_white->setPalette( palette );

    color = pHost->mLightWhite;
    palette.setColor( QPalette::Button, color );
    pushButton_Lwhite->setPalette( palette );
}

void dlgProfilePreferences::resetColors()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;

    pHost->mFgColor       = QColor(255,255,255);
    pHost->mBgColor       = QColor(  0,  0,  0);
    if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
    pHost->mBlack         = QColor(  0,  0,  0);
    pHost->mLightBlack    = QColor(128,128,128);
    pHost->mRed           = QColor(128,  0,  0);
    pHost->mLightRed      = QColor(255,  0,  0);
    pHost->mGreen         = QColor(  0,179,  0);
    pHost->mLightGreen    = QColor(  0,255  ,0);
    pHost->mBlue          = QColor(  0,  0,128);
    pHost->mLightBlue     = QColor(  0,  0,255);
    pHost->mYellow        = QColor(128,128,  0);
    pHost->mLightYellow   = QColor(255,255,  0);
    pHost->mCyan          = QColor(  0,128,128);
    pHost->mLightCyan     = QColor(  0,255,255);
    pHost->mMagenta       = QColor(128,  0,128);
    pHost->mLightMagenta  = QColor(255,  0,255);
    pHost->mWhite         = QColor(192,192,192);
    pHost->mLightWhite    = QColor(255,255,255);

    setColors();
}
void dlgProfilePreferences::setFgColor()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mFgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_foreground_color->setPalette( palette );
        pHost->mFgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}
void dlgProfilePreferences::setBgColor()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mBgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_background_color->setPalette( palette );
        pHost->mBgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}

void dlgProfilePreferences::setDisplayFont()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    bool ok;
    QFont font = QFontDialog::getFont(&ok, pHost->mDisplayFont, this);
    pHost->mDisplayFont = font;
    if( mudlet::self()->mConsoleMap.contains( pHost ) ) 
    {
        qDebug()<<"OK font changed";
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}
void dlgProfilePreferences::setCommandLineFont()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    bool ok;
    QFont font = QFontDialog::getFont( &ok, pHost->mCommandLineFont, this );
    pHost->mCommandLineFont = font;
    if( mudlet::self()->mConsoleMap.contains( pHost ) ) 
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    
}


void dlgProfilePreferences::setColorBlack()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mBlack, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_black->setPalette( palette );
        pHost->mBlack = color;
    }
}

void dlgProfilePreferences::setColorLightBlack()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightBlack, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lblack->setPalette( palette );
        pHost->mLightBlack = color;
    }
}

void dlgProfilePreferences::setColorRed()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mRed, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_red->setPalette( palette );
        pHost->mRed = color;
    }
}

void dlgProfilePreferences::setColorLightRed()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightRed, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lred->setPalette( palette );
        pHost->mLightRed = color;
    }
}

void dlgProfilePreferences::setColorGreen()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mGreen, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_green->setPalette( palette );
        pHost->mGreen = color;
    }
}
void dlgProfilePreferences::setColorLightGreen()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightGreen, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lgreen->setPalette( palette );
        pHost->mLightGreen = color;
    }
}

void dlgProfilePreferences::setColorBlue()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mBlue, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_blue->setPalette( palette );
        pHost->mBlue = color;
    }
}
void dlgProfilePreferences::setColorLightBlue()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightBlue, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lblue->setPalette( palette );
        pHost->mLightBlue = color;
    }
}

void dlgProfilePreferences::setColorYellow()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mYellow, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_yellow->setPalette( palette );
        pHost->mYellow = color;
    }
}
void dlgProfilePreferences::setColorLightYellow()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightYellow, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lyellow->setPalette( palette );
        pHost->mLightYellow = color;
    }
}

void dlgProfilePreferences::setColorCyan()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mCyan, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_cyan->setPalette( palette );
        pHost->mCyan = color;
    }
}
void dlgProfilePreferences::setColorLightCyan()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightCyan, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lcyan->setPalette( palette );
        pHost->mLightCyan = color;
    }
}

void dlgProfilePreferences::setColorMagenta()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mMagenta, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_magenta->setPalette( palette );
        pHost->mMagenta = color;
    }
}
void dlgProfilePreferences::setColorLightMagenta()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightMagenta, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lmagenta->setPalette( palette );
        pHost->mLightMagenta = color;
    }
}

void dlgProfilePreferences::setColorWhite()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mWhite, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_white->setPalette( palette );
        pHost->mWhite = color;
    }
}
void dlgProfilePreferences::setColorLightWhite()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;    
    QColor color = QColorDialog::getColor( pHost->mLightWhite, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lwhite->setPalette( palette );
        pHost->mLightWhite = color;
    }
}

void dlgProfilePreferences::slot_save_and_exit()
{
    Host * pHost = mudlet::self()->getActiveHost();
    if( ! pHost ) return;
    if( wrap_at_lineEdit->text().toInt() != 0 ) pHost->mWrapAt = wrap_at_lineEdit->text().toInt();    
    if( indent_wrapped_lineEdit->text().toInt() != 0 )pHost->mWrapIndentCount = indent_wrapped_lineEdit->text().toInt();

    pHost->mPrintCommand = show_sent_text_checkbox->isChecked();
    pHost->mAutoClearCommandLineAfterSend = auto_clear_input_line_checkbox->isChecked();
    pHost->mCommandSeparator = command_separator_lineedit->text();
    pHost->mDisableAutoCompletion = disable_auto_completion_checkbox->isChecked();
    pHost->mUSE_IRE_DRIVER_BUGFIX = checkBox_USE_IRE_DRIVER_BUGFIX->isChecked();
    pHost->set_USE_IRE_DRIVER_BUGFIX( checkBox_USE_IRE_DRIVER_BUGFIX->isChecked() );
    if( checkBox_USE_SMALL_SCREEN->isChecked() )
    {
        std::cout << "SMALL_SCREEN"<<std::endl;
        QFile file_use_smallscreen( QDir::homePath()+"/.config/mudlet/mudlet_option_use_smallscreen" );
        file_use_smallscreen.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file_use_smallscreen);
        out << "use_small_screen";
        file_use_smallscreen.close();
    }
    close();
}




