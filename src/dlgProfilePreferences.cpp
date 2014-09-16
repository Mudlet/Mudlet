/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "dlgProfilePreferences.h"


#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "Host.h"
#include "mudlet.h"
#include "TConsole.h"
#include "TMap.h"
#include "TTextEdit.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QMainWindow>
#include <QPalette>
#include <QRegExp>
#include <QToolBar>
#include "post_guard.h"


dlgProfilePreferences::dlgProfilePreferences( QWidget * pF, Host * pH )
: QDialog( pF )
, mFontSize( 10 )
, mpHost( pH )
{
    // init generated dialog
    setupUi(this);

    mFORCE_MXP_NEGOTIATION_OFF->setChecked(mpHost->mFORCE_MXP_NEGOTIATION_OFF);
    mMapperUseAntiAlias->setChecked(mpHost->mMapperUseAntiAlias);
    acceptServerGUI->setChecked(mpHost->mAcceptServerGUI);
    QString nick = tr("Mudlet%1").arg(QString::number(rand()%10000));
    QFile file( QDir::homePath()+"/.config/mudlet/irc_nick" );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( & file );
    ifs >> nick;
    file.close();
    if( nick.isEmpty() )
        nick = tr("Mudlet%1").arg(QString::number(rand()%10000));
    ircNick->setText( nick );

    dictList->setSelectionMode( QAbstractItemView::SingleSelection );
    enableSpellCheck->setChecked( pH->mEnableSpellCheck );

    QString path;
#ifdef Q_OS_LINUX
    if ( QFile::exists("/usr/share/hunspell/"+ mpHost->mSpellDic + ".aff") )
        path = "/usr/share/hunspell/";
    else
        path = "./";
#elif defined(Q_OS_MAC)
    path = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    path = "./";
#endif

    QDir dir(path);

    QStringList entries = dir.entryList( QDir::Files, QDir::Time );
    QRegExp rex("\\.dic$");
    entries = entries.filter( rex );
    for( int i=0; i<entries.size(); i++ )
    {
        QString n = entries[i].replace( ".dic", "" );
        QListWidgetItem * item = new QListWidgetItem( entries[i] );
        dictList->addItem( item );
        if( entries[i] == mpHost->mSpellDic )
        {
            item->setSelected( true );
        }
    }

    if( pH->mUrl.toLower().contains("achaea.com") || pH->mUrl.toLower().contains("aetolia.com") || pH->mUrl.toLower().contains("imperian.com") || pH->mUrl.toLower().contains("midkemiaonline.com") || pH->mUrl.toLower().contains("lusternia.com") )
    {
        downloadMapOptions->setVisible( true );
        connect(buttonDownloadMap, SIGNAL(clicked()), this, SLOT(downloadMap()));
    } else
        downloadMapOptions->setVisible( false );


    connect(closeButton, SIGNAL(pressed()), this, SLOT(slot_save_and_exit()));

    connect(pushButton_command_line_foreground_color, SIGNAL(clicked()), this, SLOT(setCommandLineFgColor()));
    connect(pushButton_command_line_background_color, SIGNAL(clicked()), this, SLOT(setCommandLineBgColor()));

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
    connect(pushButton_command_foreground_color, SIGNAL(clicked()), this, SLOT(setCommandFgColor()));
    connect(pushButton_command_background_color, SIGNAL(clicked()), this, SLOT(setCommandBgColor()));

    connect(reset_colors_button, SIGNAL(clicked()), this, SLOT(resetColors()));

    connect(fontComboBox, SIGNAL( currentFontChanged( const QFont & ) ), this, SLOT(setDisplayFont()));
    QStringList sizeList;
    for( int i=1; i<40; i++ ) sizeList << QString::number(i);
    fontSize->insertItems( 1, sizeList );
    connect(fontSize, SIGNAL(currentIndexChanged(int)), this, SLOT(setFontSize()));
    //connect(pushButton_command_line_font, SIGNAL(clicked()), this, SLOT(setCommandLineFont()));
    connect(pushButtonBorderColor, SIGNAL(clicked()), this, SLOT(setBorderColor()));
    connect(pushButtonBorderImage, SIGNAL(clicked()), this, SLOT(setBorderImage()));

    connect(pushButton_black_2, SIGNAL(clicked()), this, SLOT(setColorBlack2()));
    connect(pushButton_Lblack_2, SIGNAL(clicked()), this, SLOT(setColorLightBlack2()));
    connect(pushButton_green_2, SIGNAL(clicked()), this, SLOT(setColorGreen2()));
    connect(pushButton_Lgreen_2, SIGNAL(clicked()), this, SLOT(setColorLightGreen2()));
    connect(pushButton_red_2, SIGNAL(clicked()), this, SLOT(setColorRed2()));
    connect(pushButton_Lred_2, SIGNAL(clicked()), this, SLOT(setColorLightRed2()));
    connect(pushButton_blue_2, SIGNAL(clicked()), this, SLOT(setColorBlue2()));
    connect(pushButton_Lblue_2, SIGNAL(clicked()), this, SLOT(setColorLightBlue2()));
    connect(pushButton_yellow_2, SIGNAL(clicked()), this, SLOT(setColorYellow2()));
    connect(pushButton_Lyellow_2, SIGNAL(clicked()), this, SLOT(setColorLightYellow2()));
    connect(pushButton_cyan_2, SIGNAL(clicked()), this, SLOT(setColorCyan2()));
    connect(pushButton_Lcyan_2, SIGNAL(clicked()), this, SLOT(setColorLightCyan2()));
    connect(pushButton_magenta_2, SIGNAL(clicked()), this, SLOT(setColorMagenta2()));
    connect(pushButton_Lmagenta_2, SIGNAL(clicked()), this, SLOT(setColorLightMagenta2()));
    connect(pushButton_white_2, SIGNAL(clicked()), this, SLOT(setColorWhite2()));
    connect(pushButton_Lwhite_2, SIGNAL(clicked()), this, SLOT(setColorLightWhite2()));
    connect(pushButton_foreground_color_2, SIGNAL(clicked()), this, SLOT(setFgColor2()));
    connect(pushButton_background_color_2, SIGNAL(clicked()), this, SLOT(setBgColor2()));

    connect(reset_colors_button_2, SIGNAL(clicked()), this, SLOT(resetColors2()));

    // the GMCP warning is hidden by default and is only enabled when the value is toggled
    need_reconnect_for_gmcp->hide();
    connect(mEnableGMCP, SIGNAL(clicked()), need_reconnect_for_gmcp, SLOT(show()));

    // same with special connection warnings
    need_reconnect_for_specialoption->hide();
    connect(mFORCE_MCCP_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));
    connect(mFORCE_GA_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));

    Host * pHost = mpHost;
    if( pHost )
    {
        mFontSize = pHost->mDisplayFont.pointSize();
        fontComboBox->setCurrentFont( pHost->mDisplayFont );
        if( mFontSize < 0 )
        {
            mFontSize = 10;
        }
        if( mFontSize <= 40 ) fontSize->setCurrentIndex( mFontSize );

        setColors();

        wrap_at_spinBox->setValue(pHost->mWrapAt);
        indent_wrapped_spinBox->setValue(pHost->mWrapIndentCount);

        show_sent_text_checkbox->setChecked(pHost->mPrintCommand);
        auto_clear_input_line_checkbox->setChecked(pHost->mAutoClearCommandLineAfterSend);
        command_separator_lineedit->setText( pHost->mCommandSeparator);
        //disable_auto_completion_checkbox->setChecked(pHost->mDisableAutoCompletion);

        checkBox_USE_IRE_DRIVER_BUGFIX->setChecked( pHost->mUSE_IRE_DRIVER_BUGFIX );
        //this option is changed into a forced option for GA enabled drivers as triggers wont run on prompt lines otherwise
        //checkBox_LF_ON_GA->setChecked( pHost->mLF_ON_GA );
        checkBox_mUSE_FORCE_LF_AFTER_PROMPT->setChecked( pHost->mUSE_FORCE_LF_AFTER_PROMPT );
        USE_UNIX_EOL->setChecked( pHost->mUSE_UNIX_EOL );
        QFile file_use_smallscreen( QDir::homePath()+"/.config/mudlet/mudlet_option_use_smallscreen" );
        if( file_use_smallscreen.exists() )
            checkBox_USE_SMALL_SCREEN->setChecked( true );
        else
            checkBox_USE_SMALL_SCREEN->setChecked( false );
        topBorderHeight->setValue(pHost->mBorderTopHeight);
        bottomBorderHeight->setValue(pHost->mBorderBottomHeight);
        leftBorderWidth->setValue(pHost->mBorderLeftWidth);
        qDebug()<<"loading: left border width:"<<pHost->mBorderLeftWidth;
        rightBorderWidth->setValue(pHost->mBorderRightWidth);
        MainIconSize->setValue(mudlet::self()->mMainIconSize);
        TEFolderIconSize->setValue(mudlet::self()->mTEFolderIconSize);
        showMenuBar->setChecked( mudlet::self()->mShowMenuBar );
        if( ! showMenuBar->isChecked() )
            showToolbar->setChecked( true );
        else
            showToolbar->setChecked( mudlet::self()->mShowToolbar );
        mRawStreamDump->setChecked( pHost->mRawStreamDump );
        commandLineMinimumHeight->setValue( pHost->commandLineMinimumHeight );
        mNoAntiAlias->setChecked( ! pHost->mNoAntiAlias );
        mFORCE_MCCP_OFF->setChecked( pHost->mFORCE_NO_COMPRESSION );
        mFORCE_GA_OFF->setChecked( pHost->mFORCE_GA_OFF );
        mAlertOnNewData->setChecked( pHost->mAlertOnNewData );
        //mMXPMode->setCurrentIndex( pHost->mMXPMode );
        //encoding->setCurrentIndex( pHost->mEncoding );
        mFORCE_SAVE_ON_EXIT->setChecked( pHost->mFORCE_SAVE_ON_EXIT );
        mEnableGMCP->setChecked( pHost->mEnableGMCP );

        // load profiles into mappers "copy map to profile" combobox
        // this feature should worm seamlessly both for online and offline profiles
        QStringList mProfileList = QDir(QDir::homePath()+"/.config/mudlet/profiles").entryList(QDir::Dirs, QDir::Time); // sort by profile "hotness"
        for( int i=0; i<mProfileList.size(); i++ )
        {
            QString s = mProfileList[i];
            if( s.size() < 1 )
                continue;
            if( (mProfileList[i] == ".") || (mProfileList[i] == ".." ) )
                continue;

            mapper_profiles_combobox->addItem( mProfileList[i] );
        }

        connect(copy_map_profile, SIGNAL(clicked()), this, SLOT(copyMap()));

        // label to show on sucessful map file action
        map_file_action->hide();

        connect(load_map_button, SIGNAL(clicked()), this, SLOT(loadMap()));
        connect(save_map_button, SIGNAL(clicked()), this, SLOT(saveMap()));

        //doubleclick ignore
        QString ignore;
        QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
        while( it.hasNext() )
            ignore = ignore.append(it.next());
        doubleclick_ignore_lineedit->setText( ignore );

    }
}

void dlgProfilePreferences::setColors()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;

    QString styleSheet;

    styleSheet = QString("QPushButton{background-color:")+pHost->mCommandLineFgColor.name()+QString(";}");
    pushButton_command_line_foreground_color->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mCommandLineBgColor.name()+QString(";}");
    pushButton_command_line_background_color->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mCommandFgColor.name()+QString(";}");
    pushButton_command_foreground_color->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mCommandBgColor.name()+QString(";}");
    pushButton_command_background_color->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mFgColor.name()+QString(";}");
    pushButton_foreground_color->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mBgColor.name()+QString(";}");
    pushButton_background_color->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mBlack.name()+QString(";}");
    pushButton_black->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightBlack.name()+QString(";}");
    pushButton_Lblack->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mRed.name()+QString(";}");
    pushButton_red->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightRed.name()+QString(";}");
    pushButton_Lred->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mGreen.name()+QString(";}");
    pushButton_green->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightGreen.name()+QString(";}");
    pushButton_Lgreen->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mBlue.name()+QString(";}");
    pushButton_blue->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightBlue.name()+QString(";}");
    pushButton_Lblue->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mYellow.name()+QString(";}");
    pushButton_yellow->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightYellow.name()+QString(";}");
    pushButton_Lyellow->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mCyan.name()+QString(";}");
    pushButton_cyan->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightCyan.name()+QString(";}");
    pushButton_Lcyan->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mMagenta.name()+QString(";}");
    pushButton_magenta->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightMagenta.name()+QString(";}");
    pushButton_Lmagenta->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mWhite.name()+QString(";}");
    pushButton_white->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightWhite.name()+QString(";}");
    pushButton_Lwhite->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mBlack_2.name()+QString(";}");
    pushButton_black_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightBlack_2.name()+QString(";}");
    pushButton_Lblack_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mGreen_2.name()+QString(";}");
    pushButton_green_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightGreen_2.name()+QString(";}");
    pushButton_Lgreen_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mRed_2.name()+QString(";}");
    pushButton_red_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightRed_2.name()+QString(";}");
    pushButton_Lred_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mBlue_2.name()+QString(";}");
    pushButton_blue_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightBlue_2.name()+QString(";}");
    pushButton_Lblue_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mYellow_2.name()+QString(";}");
    pushButton_yellow_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightYellow_2.name()+QString(";}");
    pushButton_Lyellow_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mCyan_2.name()+QString(";}");
    pushButton_cyan_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightCyan_2.name()+QString(";}");
    pushButton_Lcyan_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mMagenta_2.name()+QString(";}");
    pushButton_magenta_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightMagenta_2.name()+QString(";}");
    pushButton_Lmagenta_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mWhite_2.name()+QString(";}");
    pushButton_white_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mLightWhite_2.name()+QString(";}");
    pushButton_Lwhite_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mFgColor_2.name()+QString(";}");
    pushButton_foreground_color_2->setStyleSheet( styleSheet );

    styleSheet = QString("QPushButton{background-color:")+pHost->mBgColor_2.name()+QString(";}");
    pushButton_background_color_2->setStyleSheet( styleSheet );
}

void dlgProfilePreferences::resetColors()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;

    pHost->mCommandLineFgColor = Qt::darkGray;
    pHost->mCommandLineBgColor = Qt::black;
    pHost->mCommandFgColor     = QColor(113, 113, 0);
    pHost->mCommandBgColor     = Qt::black;
    pHost->mFgColor            = Qt::lightGray;
    pHost->mBgColor            = Qt::black;
    pHost->mBlack              = Qt::black;
    pHost->mLightBlack         = Qt::darkGray;
    pHost->mRed                = Qt::darkRed;
    pHost->mLightRed           = Qt::red;
    pHost->mGreen              = Qt::darkGreen;
    pHost->mLightGreen         = Qt::green;
    pHost->mBlue               = Qt::darkBlue;
    pHost->mLightBlue          = Qt::blue;
    pHost->mYellow             = Qt::darkYellow;
    pHost->mLightYellow        = Qt::yellow;
    pHost->mCyan               = Qt::darkCyan;
    pHost->mLightCyan          = Qt::cyan;
    pHost->mMagenta            = Qt::darkMagenta;
    pHost->mLightMagenta       = Qt::magenta;
    pHost->mWhite              = Qt::lightGray;
    pHost->mLightWhite         = Qt::white;

    setColors();
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}

void dlgProfilePreferences::resetColors2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;

    pHost->mFgColor_2      = Qt::lightGray;
    pHost->mBgColor_2      = Qt::black;
    pHost->mBlack_2        = Qt::black;
    pHost->mLightBlack_2   = Qt::darkGray;
    pHost->mRed_2          = Qt::darkRed;
    pHost->mLightRed_2     = Qt::red;
    pHost->mGreen_2        = Qt::darkGreen;
    pHost->mLightGreen_2   = Qt::green;
    pHost->mBlue_2         = Qt::darkBlue;
    pHost->mLightBlue_2    = Qt::blue;
    pHost->mYellow_2       = Qt::darkYellow;
    pHost->mLightYellow_2  = Qt::yellow;
    pHost->mCyan_2         = Qt::darkCyan;
    pHost->mLightCyan_2    = Qt::cyan;
    pHost->mMagenta_2      = Qt::darkMagenta;
    pHost->mLightMagenta_2 = Qt::magenta;
    pHost->mWhite_2        = Qt::lightGray;
    pHost->mLightWhite_2   = Qt::white;

    setColors();
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}

void dlgProfilePreferences::setColor(QPushButton* b, QColor& c)
{
    Host* pHost = mpHost;
    if(!pHost) return;

    QColor color = QColorDialog::getColor(c, this);
    if (color.isValid()) {
        c = color;
        if(mudlet::self()->mConsoleMap.contains(pHost)) {
            mudlet::self()->mConsoleMap[pHost]->changeColors();
        }

        QString styleSheet = QString("QPushButton{background-color:") + color.name() + QString(";}");
        b->setStyleSheet(styleSheet);
    }
}

void dlgProfilePreferences::setFgColor()
{
    if( mpHost )
        setColor(pushButton_foreground_color, mpHost->mFgColor);
}
void dlgProfilePreferences::setBgColor()
{
    if( mpHost )
        setColor(pushButton_background_color, mpHost->mBgColor);
}

void dlgProfilePreferences::setCommandFgColor()
{
    if( mpHost )
        setColor(pushButton_command_foreground_color, mpHost->mCommandFgColor);
}

void dlgProfilePreferences::setCommandLineFgColor()
{
    if( mpHost )
        setColor(pushButton_command_line_foreground_color, mpHost->mCommandLineFgColor);
}

void dlgProfilePreferences::setCommandLineBgColor()
{
    if( mpHost )
        setColor(pushButton_command_line_background_color, mpHost->mCommandLineBgColor);
}

void dlgProfilePreferences::setCommandBgColor()
{
    if( mpHost )
        setColor(pushButton_command_background_color, mpHost->mCommandBgColor);
}


void dlgProfilePreferences::setFontSize()
{
    mFontSize = fontSize->currentIndex();
    setDisplayFont();
}

void dlgProfilePreferences::setDisplayFont()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QFont font = fontComboBox->currentFont();
    font.setPointSize( mFontSize );
    pHost->mDisplayFont = font;
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}
void dlgProfilePreferences::setCommandLineFont()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    bool ok;
    QFont font = QFontDialog::getFont( &ok, pHost->mCommandLineFont, this );
    pHost->mCommandLineFont = font;
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }

}


void dlgProfilePreferences::setColorBlack()
{
    if( mpHost )
        setColor(pushButton_black, mpHost->mBlack);
}

void dlgProfilePreferences::setColorLightBlack()
{
    if( mpHost )
        setColor(pushButton_Lblack, mpHost->mLightBlack);
}

void dlgProfilePreferences::setColorRed()
{
    if( mpHost )
        setColor(pushButton_red, mpHost->mRed);
}
void dlgProfilePreferences::setColorLightRed()
{
    if( mpHost )
        setColor(pushButton_Lred, mpHost->mLightRed);
}

void dlgProfilePreferences::setColorGreen()
{
    if( mpHost )
        setColor(pushButton_green, mpHost->mGreen);
}
void dlgProfilePreferences::setColorLightGreen()
{
    if( mpHost )
        setColor(pushButton_Lgreen, mpHost->mLightGreen);
}

void dlgProfilePreferences::setColorBlue()
{
    if( mpHost )
        setColor(pushButton_blue, mpHost->mBlue);
}
void dlgProfilePreferences::setColorLightBlue()
{
    if( mpHost )
        setColor(pushButton_Lblue, mpHost->mLightBlue);
}

void dlgProfilePreferences::setColorYellow()
{
    if( mpHost )
        setColor(pushButton_yellow, mpHost->mYellow);
}
void dlgProfilePreferences::setColorLightYellow()
{
    if( mpHost )
        setColor(pushButton_Lyellow, mpHost->mLightYellow);
}

void dlgProfilePreferences::setColorCyan()
{
    if( mpHost )
        setColor(pushButton_cyan, mpHost->mCyan);
}
void dlgProfilePreferences::setColorLightCyan()
{
    if( mpHost )
        setColor(pushButton_Lcyan, mpHost->mLightCyan);
}

void dlgProfilePreferences::setColorMagenta()
{
    if( mpHost )
        setColor(pushButton_magenta, mpHost->mMagenta);
}
void dlgProfilePreferences::setColorLightMagenta()
{
    if( mpHost )
        setColor(pushButton_Lmagenta, mpHost->mLightMagenta);
}

void dlgProfilePreferences::setColorWhite()
{
    if( mpHost )
        setColor(pushButton_white, mpHost->mWhite);
}
void dlgProfilePreferences::setColorLightWhite()
{
    if( mpHost )
        setColor(pushButton_Lwhite, mpHost->mLightWhite);
}

void dlgProfilePreferences::setFgColor2()
{
    if( mpHost )
        setColor(pushButton_foreground_color_2, mpHost->mFgColor_2);
}
void dlgProfilePreferences::setBgColor2()
{
    if( mpHost )
        setColor(pushButton_background_color_2, mpHost->mBgColor_2);
}


void dlgProfilePreferences::setColorBlack2()
{
    if( mpHost )
        setColor(pushButton_black_2, mpHost->mBlack_2);
}
void dlgProfilePreferences::setColorLightBlack2()
{
    if( mpHost )
        setColor(pushButton_Lblack_2, mpHost->mLightBlack_2);
}

void dlgProfilePreferences::setColorRed2()
{
    if( mpHost )
        setColor(pushButton_red_2, mpHost->mRed_2);
}
void dlgProfilePreferences::setColorLightRed2()
{
    if( mpHost )
        setColor(pushButton_Lred_2, mpHost->mLightRed_2);
}

void dlgProfilePreferences::setColorGreen2()
{
    if( mpHost )
        setColor(pushButton_green_2, mpHost->mGreen_2);
}
void dlgProfilePreferences::setColorLightGreen2()
{
    if( mpHost )
        setColor(pushButton_Lgreen_2, mpHost->mLightGreen_2);
}

void dlgProfilePreferences::setColorBlue2()
{
    if( mpHost )
        setColor(pushButton_blue_2, mpHost->mBlue_2);
}
void dlgProfilePreferences::setColorLightBlue2()
{
    if( mpHost )
        setColor(pushButton_Lblue_2, mpHost->mLightBlue_2);
}

void dlgProfilePreferences::setColorYellow2()
{
    if( mpHost )
        setColor(pushButton_yellow_2, mpHost->mYellow_2);
}
void dlgProfilePreferences::setColorLightYellow2()
{
    if( mpHost )
        setColor(pushButton_Lyellow_2, mpHost->mLightYellow_2);
}

void dlgProfilePreferences::setColorCyan2()
{
    if( mpHost )
        setColor(pushButton_cyan_2, mpHost->mCyan_2);
}
void dlgProfilePreferences::setColorLightCyan2()
{
    if( mpHost )
        setColor(pushButton_Lcyan_2, mpHost->mLightCyan_2);
}

void dlgProfilePreferences::setColorMagenta2()
{
    if( mpHost )
        setColor(pushButton_magenta_2, mpHost->mMagenta_2);
}
void dlgProfilePreferences::setColorLightMagenta2()
{
    if( mpHost )
        setColor(pushButton_Lmagenta_2, mpHost->mLightMagenta_2);
}

void dlgProfilePreferences::setColorWhite2()
{
    if( mpHost )
        setColor(pushButton_white_2, mpHost->mWhite_2);
}
void dlgProfilePreferences::setColorLightWhite2()
{
    if( mpHost )
        setColor(pushButton_Lwhite_2, mpHost->mLightWhite_2);
}

void dlgProfilePreferences::downloadMap()
{
    if (!mpHost) {
        return;
    }
    if( ! mpHost->mpMap->mpMapper ) {
        mudlet::self()->slot_mapper();
    }

    mpHost->mpMap->mpMapper->downloadMap();
}

void dlgProfilePreferences::loadMap()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Mudlet map"),
                                                    QDir::homePath(), "Mudlet map (*.dat);;Any file (*)");
    if( fileName.isEmpty() ) return;

    map_file_action->show();
    map_file_action->setText("Loading map...");

    if ( mpHost->mpConsole->loadMap(fileName) )
    {
        map_file_action->setText("Loaded map from "+fileName);
        QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
    }
    else
    {
        map_file_action->setText("Couldn't load map from "+fileName);
        QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
    }
    if( mpHost->mpMap )
        if( mpHost->mpMap->mpMapper )
            mpHost->mpMap->mpMapper->updateAreaComboBox();
}

void dlgProfilePreferences::saveMap()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Mudlet map"),
                                                    QDir::homePath(), "Mudlet map (*.dat)");
    if( fileName.isEmpty() ) return;

    if ( ! fileName.endsWith(".dat", Qt::CaseInsensitive) )
        fileName.append(".dat");

    map_file_action->show();
    map_file_action->setText("Saving map...");

    if ( mpHost->mpConsole->saveMap(fileName) ) {
        map_file_action->setText("Saved map to "+fileName);
        QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
    } else {
        map_file_action->setText("Couldn't save map to "+fileName);
        QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
    }
}

void dlgProfilePreferences::hideActionLabel()
{
    map_file_action->hide();
}

void dlgProfilePreferences::copyMap()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;

    QString toProfile = mapper_profiles_combobox->itemText(mapper_profiles_combobox->currentIndex());

    // at first, save our current map
    map_file_action->show();
    map_file_action->setText("Copying map...");
    if ( ! mpHost->mpConsole->saveMap("") ) {
        map_file_action->setText("Couldn't copy the map - saving it failed.");
        QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
        return;
    }

    // then copy over into the profiles map folder, so it is loaded first when map is open - this covers the offline case
    QDir dir_map;
    QString directory_map = QDir::homePath()+"/.config/mudlet/profiles/"+toProfile+"/map";
    if( ! dir_map.exists( directory_map ) )
    {
        dir_map.mkpath( directory_map );
    }

    // work out which map is latest
    QFile latestMap;
    QString toMapFolder(QDir::homePath()+"/.config/mudlet/profiles/"+pHost->getName()+"/map");
    QStringList mProfileList = QDir(toMapFolder).entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    for( int i=0; i<mProfileList.size(); i++ )
    {
        QString s = mProfileList[i];
        if( s.size() < 1 )
            continue;

        latestMap.setFileName(toMapFolder+"/"+mProfileList[i]);
        break;
    }

    if ( latestMap.fileName() == "" ) {
        map_file_action->setText("Couldn't copy the map - failed to work out which map file did we just save the map as.");
        QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
        return;
    }

    QFileInfo lm(latestMap.fileName());

    if ( !latestMap.copy(directory_map+"/"+lm.fileName()) ) {
        map_file_action->setText("Couldn't copy the map - couldn't copy the offline map file over.");
        QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
        return;
    }
    map_file_action->setText("Map copied successfully."); // don't mention offline here, would be a bit confusing

    // then force that profile to reload it's latest map - this covers the online case
    QMap<Host *, TConsole *> activeSessions = mudlet::self()->mConsoleMap;
    QMapIterator<Host *, TConsole *> it2(activeSessions);
    while (it2.hasNext()){
        it2.next();
        Host * host = it2.key();
        if (host->mHostName != toProfile)
            continue;

        if ( host->mpConsole->loadMap(directory_map+"/"+lm.fileName()) ) {
            map_file_action->setText("Map copied and reloaded on "+toProfile+".");
            QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
        } else {
            map_file_action->setText("Map copied, but couldn't be reloaded on "+toProfile+".");
            QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
        }

        return;
    }

}

void dlgProfilePreferences::slot_save_and_exit()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    if( dictList->currentItem() )
        pHost->mSpellDic = dictList->currentItem()->text();
    pHost->mEnableSpellCheck = enableSpellCheck->isChecked();
    pHost->mWrapAt = wrap_at_spinBox->value();
    pHost->mWrapIndentCount = indent_wrapped_spinBox->value();
    pHost->mPrintCommand = show_sent_text_checkbox->isChecked();
    pHost->mAutoClearCommandLineAfterSend = auto_clear_input_line_checkbox->isChecked();
    pHost->mCommandSeparator = command_separator_lineedit->text();
    pHost->mAcceptServerGUI = acceptServerGUI->isChecked();
    //pHost->mDisableAutoCompletion = disable_auto_completion_checkbox->isChecked();
    pHost->mUSE_IRE_DRIVER_BUGFIX = checkBox_USE_IRE_DRIVER_BUGFIX->isChecked();
    pHost->set_USE_IRE_DRIVER_BUGFIX( checkBox_USE_IRE_DRIVER_BUGFIX->isChecked() );
    //pHost->set_LF_ON_GA( checkBox_LF_ON_GA->isChecked() );
    pHost->mUSE_FORCE_LF_AFTER_PROMPT = checkBox_mUSE_FORCE_LF_AFTER_PROMPT->isChecked();
    pHost->mUSE_UNIX_EOL = USE_UNIX_EOL->isChecked();
    pHost->mFORCE_NO_COMPRESSION = mFORCE_MCCP_OFF->isChecked();
    pHost->mFORCE_GA_OFF = mFORCE_GA_OFF->isChecked();
    pHost->mFORCE_SAVE_ON_EXIT = mFORCE_SAVE_ON_EXIT->isChecked();
    pHost->mEnableGMCP = mEnableGMCP->isChecked();
    pHost->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
    if( pHost->mpMap )
        if( pHost->mpMap->mpMapper )
            pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
    pHost->mBorderTopHeight = topBorderHeight->value();
    pHost->mBorderBottomHeight = bottomBorderHeight->value();
    pHost->mBorderLeftWidth = leftBorderWidth->value();
    pHost->mBorderRightWidth = rightBorderWidth->value();
  qDebug()<<"Left border width:"<<pHost->mBorderLeftWidth<<" right:"<<pHost->mBorderRightWidth;
    pHost->commandLineMinimumHeight = commandLineMinimumHeight->value();
    //pHost->mMXPMode = mMXPMode->currentIndex();
    //pHost->mEncoding = encoding->currentIndex();
    pHost->mFORCE_MXP_NEGOTIATION_OFF = mFORCE_MXP_NEGOTIATION_OFF->isChecked();
    mudlet::self()->mMainIconSize = MainIconSize->value();
    mudlet::self()->mTEFolderIconSize = TEFolderIconSize->value();
    mudlet::self()->setIcoSize(MainIconSize->value());
    pHost->mpEditorDialog->setTBIconSize( 0 );
    mudlet::self()->mShowMenuBar = showMenuBar->isChecked();
    if( showMenuBar->isChecked() )
        mudlet::self()->menuBar()->show();
    else
        mudlet::self()->menuBar()->hide();
    mudlet::self()->mShowToolbar = showToolbar->isChecked();
    if( showToolbar->isChecked() )
        mudlet::self()->mpMainToolBar->show();
    else
        mudlet::self()->mpMainToolBar->hide();
    pHost->mRawStreamDump = mRawStreamDump->isChecked();
    pHost->mNoAntiAlias = !mNoAntiAlias->isChecked();
    pHost->mAlertOnNewData = mAlertOnNewData->isChecked();
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
    QString lIgnore = doubleclick_ignore_lineedit->text();
    for(int i=0;i<lIgnore.size();i++){
        mpHost->mDoubleClickIgnore.insert(lIgnore.at(i));
    }

    //pHost->mIRCNick = ircNick->text();
    QString old_nick = mudlet::self()->mIrcNick;
    QString new_nick = ircNick->text();
    if( new_nick.isEmpty() )
        new_nick = tr("Mudlet%1").arg(QString::number(rand()%10000));
    QFile file( QDir::homePath()+"/.config/mudlet/irc_nick" );
    file.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
    QDataStream ofs( & file );
    ofs << new_nick;
    file.close();
    if( mudlet::self()->mpIRC )
        mudlet::self()->mpIRC->session->setNick( new_nick );

    if( checkBox_USE_SMALL_SCREEN->isChecked() )
    {
        QFile file_use_smallscreen( QDir::homePath()+"/.config/mudlet/mudlet_option_use_smallscreen" );
        file_use_smallscreen.open( QIODevice::WriteOnly | QIODevice::Text );
        QTextStream out(&file_use_smallscreen);
        file_use_smallscreen.close();
    }
    else
    {
       QFile file_use_smallscreen( QDir::homePath()+"/.config/mudlet/mudlet_option_use_smallscreen" );
       file_use_smallscreen.remove();
    }
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->console->updateScreenView();
        mudlet::self()->mConsoleMap[pHost]->console->forceUpdate();
        mudlet::self()->mConsoleMap[pHost]->refresh();
        int x = mudlet::self()->mConsoleMap[pHost]->width();
        int y = mudlet::self()->mConsoleMap[pHost]->height();
        QSize s = QSize(x,y);
        QResizeEvent event(s, s);
        QApplication::sendEvent( mudlet::self()->mConsoleMap[pHost], &event);
qDebug()<<"after console refresh: Left border width:"<<pHost->mBorderLeftWidth<<" right:"<<pHost->mBorderRightWidth;
    }
    close();
}
