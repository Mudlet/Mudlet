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
#include <QColorDialog>
#include <QPalette>
#include <QFontDialog>
#include <QFont>
#include <QToolBar>
#include "dlgProfilePreferences.h"
#include <QtCore>
#include <QDir>
#include <QRegExp>
#include "Host.h"
#include "mudlet.h"
#include "TTextEdit.h"

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

#ifdef Q_OS_LINUX
    QDir dir( "/usr/share/hunspell/" );
#else
    QDir dir( "./" );
#endif
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
    connect(pushButton_black, SIGNAL(clicked()), this, SLOT(setColorBlack()));
    QPalette palette;
    QString styleSheet;
    QColor color;

    color = mpHost->mCommandLineFgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_command_line_foreground_color->setStyleSheet( styleSheet );
    connect(pushButton_command_line_foreground_color, SIGNAL(clicked()), this, SLOT(setCommandLineFgColor()));

    color = mpHost->mCommandLineBgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_command_line_background_color->setStyleSheet( styleSheet );
    connect(pushButton_command_line_background_color, SIGNAL(clicked()), this, SLOT(setCommandLineBgColor()));

    color = mpHost->mBlack;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_black->setStyleSheet( styleSheet );

    connect(pushButton_Lblack, SIGNAL(clicked()), this, SLOT(setColorLightBlack()));
    color = mpHost->mLightBlack;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblack->setStyleSheet( styleSheet );

    connect(pushButton_green, SIGNAL(clicked()), this, SLOT(setColorGreen()));
    color = mpHost->mGreen;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_green->setStyleSheet( styleSheet );

    connect(pushButton_Lgreen, SIGNAL(clicked()), this, SLOT(setColorLightGreen()));
    color = mpHost->mLightGreen;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lgreen->setStyleSheet( styleSheet );

    connect(pushButton_red, SIGNAL(clicked()), this, SLOT(setColorRed()));
    color = mpHost->mRed;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_red->setStyleSheet( styleSheet );

    connect(pushButton_Lred, SIGNAL(clicked()), this, SLOT(setColorLightRed()));
    color = mpHost->mLightRed;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lred->setStyleSheet( styleSheet );

    connect(pushButton_blue, SIGNAL(clicked()), this, SLOT(setColorBlue()));
    color = mpHost->mBlue;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_blue->setStyleSheet( styleSheet );

    connect(pushButton_Lblue, SIGNAL(clicked()), this, SLOT(setColorLightBlue()));
    color = mpHost->mLightBlue;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblue->setStyleSheet( styleSheet );

    connect(pushButton_yellow, SIGNAL(clicked()), this, SLOT(setColorYellow()));
    color = mpHost->mYellow;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_yellow->setStyleSheet( styleSheet );

    connect(pushButton_Lyellow, SIGNAL(clicked()), this, SLOT(setColorLightYellow()));
    color = mpHost->mLightYellow;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lyellow->setStyleSheet( styleSheet );

    connect(pushButton_cyan, SIGNAL(clicked()), this, SLOT(setColorCyan()));
    color = mpHost->mCyan;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_cyan->setStyleSheet( styleSheet );

    connect(pushButton_Lcyan, SIGNAL(clicked()), this, SLOT(setColorLightCyan()));
    color = mpHost->mLightCyan;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lcyan->setStyleSheet( styleSheet );

    connect(pushButton_magenta, SIGNAL(clicked()), this, SLOT(setColorMagenta()));
    color = mpHost->mMagenta;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_magenta->setStyleSheet( styleSheet );

    connect(pushButton_Lmagenta, SIGNAL(clicked()), this, SLOT(setColorLightMagenta()));
    color = mpHost->mLightMagenta;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lmagenta->setStyleSheet( styleSheet );

    connect(pushButton_white, SIGNAL(clicked()), this, SLOT(setColorWhite()));
    color = mpHost->mWhite;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_white->setStyleSheet( styleSheet );

    connect(pushButton_Lwhite, SIGNAL(clicked()), this, SLOT(setColorLightWhite()));
    color = mpHost->mLightWhite;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lwhite->setStyleSheet( styleSheet );
    connect(pushButton_foreground_color, SIGNAL(clicked()), this, SLOT(setFgColor()));
    color = mpHost->mFgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_foreground_color->setStyleSheet( styleSheet );
    connect(pushButton_background_color, SIGNAL(clicked()), this, SLOT(setBgColor()));
    color = mpHost->mBgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_background_color->setStyleSheet( styleSheet );



    connect(pushButton_command_foreground_color, SIGNAL(clicked()), this, SLOT(setCommandFgColor()));
    color = mpHost->mCommandFgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_command_foreground_color->setStyleSheet( styleSheet );


    connect(pushButton_command_background_color, SIGNAL(clicked()), this, SLOT(setCommandBgColor()));
    color = mpHost->mCommandBgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_command_background_color->setStyleSheet( styleSheet );




    connect(reset_colors_button, SIGNAL(clicked()), this, SLOT(resetColors()));
    connect(fontComboBox, SIGNAL( currentFontChanged( const QFont & ) ), this, SLOT(setDisplayFont()));
    QStringList sizeList;
    for( int i=1; i<40; i++ ) sizeList << QString::number(i);
    fontSize->insertItems( 1, sizeList );
    connect(fontSize, SIGNAL(currentIndexChanged(int)), this, SLOT(setFontSize()));
    //connect(pushButton_command_line_font, SIGNAL(clicked()), this, SLOT(setCommandLineFont()));
    connect(pushButtonBorderColor, SIGNAL(clicked()), this, SLOT(setBorderColor()));
    connect(pushButtonBorderImage, SIGNAL(clicked()), this, SLOT(setBorderImage()));



    color = mpHost->mBlack_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_black_2->setStyleSheet( styleSheet );
    connect(pushButton_black_2, SIGNAL(clicked()), this, SLOT(setColorBlack2()));

    connect(pushButton_Lblack_2, SIGNAL(clicked()), this, SLOT(setColorLightBlack2()));
    color = mpHost->mLightBlack_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblack_2->setStyleSheet( styleSheet );

    connect(pushButton_green_2, SIGNAL(clicked()), this, SLOT(setColorGreen2()));
    color = mpHost->mGreen_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_green_2->setStyleSheet( styleSheet );

    connect(pushButton_Lgreen_2, SIGNAL(clicked()), this, SLOT(setColorLightGreen2()));
    color = mpHost->mLightGreen_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lgreen_2->setStyleSheet( styleSheet );

    connect(pushButton_red_2, SIGNAL(clicked()), this, SLOT(setColorRed2()));
    color = mpHost->mRed_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_red_2->setStyleSheet( styleSheet );

    connect(pushButton_Lred_2, SIGNAL(clicked()), this, SLOT(setColorLightRed2()));
    color = mpHost->mLightRed_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lred_2->setStyleSheet( styleSheet );

    connect(pushButton_blue_2, SIGNAL(clicked()), this, SLOT(setColorBlue2()));
    color = mpHost->mBlue_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_blue_2->setStyleSheet( styleSheet );

    connect(pushButton_Lblue_2, SIGNAL(clicked()), this, SLOT(setColorLightBlue2()));
    color = mpHost->mLightBlue_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblue_2->setStyleSheet( styleSheet );

    connect(pushButton_yellow_2, SIGNAL(clicked()), this, SLOT(setColorYellow2()));
    color = mpHost->mYellow_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_yellow_2->setStyleSheet( styleSheet );

    connect(pushButton_Lyellow_2, SIGNAL(clicked()), this, SLOT(setColorLightYellow2()));
    color = mpHost->mLightYellow_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lyellow_2->setStyleSheet( styleSheet );

    connect(pushButton_cyan_2, SIGNAL(clicked()), this, SLOT(setColorCyan2()));
    color = mpHost->mCyan_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_cyan_2->setStyleSheet( styleSheet );

    connect(pushButton_Lcyan_2, SIGNAL(clicked()), this, SLOT(setColorLightCyan2()));
    color = mpHost->mLightCyan_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lcyan_2->setStyleSheet( styleSheet );

    connect(pushButton_magenta_2, SIGNAL(clicked()), this, SLOT(setColorMagenta2()));
    color = mpHost->mMagenta_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_magenta_2->setStyleSheet( styleSheet );

    connect(pushButton_Lmagenta_2, SIGNAL(clicked()), this, SLOT(setColorLightMagenta2()));
    color = mpHost->mLightMagenta_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lmagenta_2->setStyleSheet( styleSheet );

    connect(pushButton_white_2, SIGNAL(clicked()), this, SLOT(setColorWhite2()));
    color = mpHost->mWhite_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_white_2->setStyleSheet( styleSheet );

    connect(pushButton_Lwhite_2, SIGNAL(clicked()), this, SLOT(setColorLightWhite2()));
    color = mpHost->mLightWhite_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lwhite_2->setStyleSheet( styleSheet );
    connect(pushButton_foreground_color_2, SIGNAL(clicked()), this, SLOT(setFgColor2()));
    color = mpHost->mFgColor_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_foreground_color_2->setStyleSheet( styleSheet );
    connect(pushButton_background_color_2, SIGNAL(clicked()), this, SLOT(setBgColor2()));
    color = mpHost->mBgColor_2;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_background_color_2->setStyleSheet( styleSheet );


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
    QPalette palette;
    QColor color;
    color = pHost->mFgColor;
    palette.setColor( QPalette::Button, color );
    pushButton_foreground_color->setPalette( palette );
    color = mpHost->mFgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_foreground_color->setStyleSheet( styleSheet );

    color = pHost->mBgColor;
    palette.setColor( QPalette::Button, color );
    pushButton_background_color->setPalette( palette );
    color = mpHost->mBgColor;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_background_color->setStyleSheet( styleSheet );

    color = pHost->mBlack;
    palette.setColor( QPalette::Button, color );
    pushButton_black->setPalette( palette );
    color = mpHost->mBlack;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_black->setStyleSheet( styleSheet );

    color = pHost->mLightBlack;
    palette.setColor( QPalette::Button, color );
    pushButton_Lblack->setPalette( palette );
    color = mpHost->mLightBlack;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblack->setStyleSheet( styleSheet );

    color = pHost->mRed;
    palette.setColor( QPalette::Button, color );
    pushButton_red->setPalette( palette );
    color = mpHost->mRed;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_red->setStyleSheet( styleSheet );

    color = pHost->mLightRed;
    palette.setColor( QPalette::Button, color );
    pushButton_Lred->setPalette( palette );
    color = mpHost->mLightRed;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lred->setStyleSheet( styleSheet );

    color = pHost->mGreen;
    palette.setColor( QPalette::Button, color );
    pushButton_green->setPalette( palette );
    color = mpHost->mGreen;
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_green->setStyleSheet( styleSheet );

    color = pHost->mLightGreen;
    palette.setColor( QPalette::Button, color );
    pushButton_Lgreen->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lgreen->setStyleSheet( styleSheet );

    color = pHost->mBlue;
    palette.setColor( QPalette::Button, color );
    pushButton_blue->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_blue->setStyleSheet( styleSheet );


    color = pHost->mLightBlue;
    palette.setColor( QPalette::Button, color );
    pushButton_Lblue->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lblue->setStyleSheet( styleSheet );

    color = pHost->mYellow;
    palette.setColor( QPalette::Button, color );
    pushButton_yellow->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_yellow->setStyleSheet( styleSheet );

    color = pHost->mLightYellow;
    palette.setColor( QPalette::Button, color );
    pushButton_Lyellow->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lyellow->setStyleSheet( styleSheet );

    color = pHost->mCyan;
    palette.setColor( QPalette::Button, color );
    pushButton_cyan->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_cyan->setStyleSheet( styleSheet );

    color = pHost->mLightCyan;
    palette.setColor( QPalette::Button, color );
    pushButton_Lcyan->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lcyan->setStyleSheet( styleSheet );

    color = pHost->mMagenta;
    palette.setColor( QPalette::Button, color );
    pushButton_magenta->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_magenta->setStyleSheet( styleSheet );

    color = pHost->mLightMagenta;
    palette.setColor( QPalette::Button, color );
    pushButton_Lmagenta->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lmagenta->setStyleSheet( styleSheet );

    color = pHost->mWhite;
    palette.setColor( QPalette::Button, color );
    pushButton_white->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_white->setStyleSheet( styleSheet );

    color = pHost->mLightWhite;
    palette.setColor( QPalette::Button, color );
    pushButton_Lwhite->setPalette( palette );
    palette.setColor( QPalette::Button, color );
    styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
    pushButton_Lwhite->setStyleSheet( styleSheet );
}

void dlgProfilePreferences::resetColors()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;

    pHost->mFgColor       = QColor(255,255,255);
    pHost->mBgColor       = QColor(  0,  0,  0);
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
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}
void dlgProfilePreferences::setFgColor()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mFgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_foreground_color->setPalette( palette );
        pHost->mFgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
        color = mpHost->mFgColor;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_foreground_color->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setBgColor()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mBgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_background_color->setPalette( palette );
        pHost->mBgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
        color = mpHost->mBgColor;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_background_color->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setCommandFgColor()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mCommandFgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_command_foreground_color->setPalette( palette );
        pHost->mCommandFgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
        color = mpHost->mCommandFgColor;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_command_foreground_color->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setCommandLineFgColor()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mCommandLineFgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_command_line_foreground_color->setPalette( palette );
        pHost->mCommandLineFgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
        color = mpHost->mCommandLineFgColor;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_command_line_foreground_color->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setCommandLineBgColor()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mCommandLineBgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_command_line_background_color->setPalette( palette );
        pHost->mCommandLineBgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
        color = mpHost->mCommandLineBgColor;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_command_line_background_color->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setCommandBgColor()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mCommandBgColor, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_command_background_color->setPalette( palette );
        pHost->mCommandBgColor = color;
        if( mudlet::self()->mConsoleMap.contains( pHost ) ) mudlet::self()->mConsoleMap[pHost]->changeColors();
        color = mpHost->mCommandBgColor;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_command_background_color->setStyleSheet( styleSheet );
    }
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
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mBlack, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_black->setPalette( palette );
        pHost->mBlack = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_black->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorLightBlack()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightBlack, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lblack->setPalette( palette );
        pHost->mLightBlack = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lblack->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorRed()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mRed, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_red->setPalette( palette );
        pHost->mRed = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_red->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorLightRed()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightRed, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lred->setPalette( palette );
        pHost->mLightRed = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lred->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorGreen()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mGreen, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_green->setPalette( palette );
        pHost->mGreen = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_green->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightGreen()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightGreen, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lgreen->setPalette( palette );
        pHost->mLightGreen = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lgreen->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorBlue()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mBlue, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_blue->setPalette( palette );
        pHost->mBlue = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_blue->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightBlue()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightBlue, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lblue->setPalette( palette );
        pHost->mLightBlue = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lblue->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorYellow()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mYellow, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_yellow->setPalette( palette );
        pHost->mYellow = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_yellow->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightYellow()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightYellow, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lyellow->setPalette( palette );
        pHost->mLightYellow = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lyellow->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorCyan()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mCyan, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_cyan->setPalette( palette );
        pHost->mCyan = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_cyan->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightCyan()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightCyan, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lcyan->setPalette( palette );
        pHost->mLightCyan = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lcyan->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorMagenta()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mMagenta, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_magenta->setPalette( palette );
        pHost->mMagenta = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_magenta->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightMagenta()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightMagenta, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lmagenta->setPalette( palette );
        pHost->mLightMagenta = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lmagenta->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorWhite()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mWhite, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_white->setPalette( palette );
        pHost->mWhite = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_white->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightWhite()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightWhite, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lwhite->setPalette( palette );
        pHost->mLightWhite = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lwhite->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setFgColor2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mFgColor_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_foreground_color_2->setPalette( palette );
        pHost->mFgColor_2 = color;
        color = mpHost->mFgColor_2;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_foreground_color_2->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setBgColor2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mBgColor_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_background_color_2->setPalette( palette );
        pHost->mBgColor_2 = color;
        color = mpHost->mBgColor_2;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_background_color_2->setStyleSheet( styleSheet );
    }
}


void dlgProfilePreferences::setColorBlack2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mBlack_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_black_2->setPalette( palette );
        pHost->mBlack_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_black_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorLightBlack2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightBlack_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lblack_2->setPalette( palette );
        pHost->mLightBlack_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lblack_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorRed2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mRed_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_red_2->setPalette( palette );
        pHost->mRed_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_red_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorLightRed2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightRed_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lred_2->setPalette( palette );
        pHost->mLightRed_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lred_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorGreen2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mGreen_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_green_2->setPalette( palette );
        pHost->mGreen_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_green_2->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightGreen2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightGreen_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lgreen_2->setPalette( palette );
        pHost->mLightGreen_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lgreen_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorBlue2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mBlue_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_blue_2->setPalette( palette );
        pHost->mBlue_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_blue_2->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightBlue2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightBlue_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lblue_2->setPalette( palette );
        pHost->mLightBlue_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lblue_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorYellow2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mYellow_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_yellow_2->setPalette( palette );
        pHost->mYellow_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_yellow_2->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightYellow2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightYellow_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lyellow_2->setPalette( palette );
        pHost->mLightYellow_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lyellow_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorCyan2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mCyan_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_cyan_2->setPalette( palette );
        pHost->mCyan_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_cyan_2->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightCyan2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightCyan_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lcyan_2->setPalette( palette );
        pHost->mLightCyan_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lcyan_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorMagenta2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mMagenta_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_magenta_2->setPalette( palette );
        pHost->mMagenta_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_magenta_2->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightMagenta2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightMagenta_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lmagenta_2->setPalette( palette );
        pHost->mLightMagenta_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lmagenta_2->setStyleSheet( styleSheet );
    }
}

void dlgProfilePreferences::setColorWhite2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mWhite_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_white_2->setPalette( palette );
        pHost->mWhite_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_white_2->setStyleSheet( styleSheet );
    }
}
void dlgProfilePreferences::setColorLightWhite2()
{
    Host * pHost = mpHost;
    if( ! pHost ) return;
    QColor color = QColorDialog::getColor( pHost->mLightWhite_2, this );
    if ( color.isValid() )
    {
        QPalette palette;
        palette.setColor( QPalette::Button, color );
        pushButton_Lwhite_2->setPalette( palette );
        pHost->mLightWhite_2 = color;
        palette.setColor( QPalette::Button, color );
        QString styleSheet = QString("QPushButton{background-color:")+color.name()+QString(";}");
        pushButton_Lwhite_2->setStyleSheet( styleSheet );
    }
}

#include "dlgMapper.h"

void dlgProfilePreferences::downloadMap()
{
    if( ! mpHost->mpMap->mpMapper ) mudlet::self()->slot_mapper();

    mpHost->mpMap->mpMapper->downloadMap();
}

#include <QFileDialog>

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

#include "dlgIRC.h"

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
    pHost->mpConsole->changeColors();
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
    pHost->mpConsole->console->updateScreenView();
    pHost->mpConsole->console->forceUpdate();
    pHost->mpConsole->refresh();
    int x = pHost->mpConsole->width();
    int y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
qDebug()<<"after console refresh: Left border width:"<<pHost->mBorderLeftWidth<<" right:"<<pHost->mBorderRightWidth;
    close();
}




