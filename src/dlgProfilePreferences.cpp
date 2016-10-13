/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016 by Stephen Lyons - slysven@virginmedia.com   *
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
#include "TRoomDB.h"
#include "TTextEdit.h"

#include "pre_guard.h"
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QMainWindow>
#include <QPalette>
#include <QRegExp>
#include <QTextOption>
#include <QToolBar>
#include <QVariant>
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
    checkBox_showSpacesAndTabs->setChecked( mudlet::self()->mEditorTextOptions & QTextOption::ShowTabsAndSpaces );
    checkBox_showLineFeedsAndParagraphs->setChecked( mudlet::self()->mEditorTextOptions & QTextOption::ShowLineAndParagraphSeparators );

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
//    connect(pushButton_command_line_font, SIGNAL(clicked()), this, SLOT(setCommandLineFont()));
//    connect(pushButtonBorderColor, SIGNAL(clicked()), this, SLOT(setBorderColor()));
//    connect(pushButtonBorderImage, SIGNAL(clicked()), this, SLOT(setBorderImage()));



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
    need_reconnect_for_data_protocol->hide();
    connect(mEnableGMCP, SIGNAL(clicked()), need_reconnect_for_data_protocol, SLOT(show()));
    connect(mEnableMSDP, SIGNAL(clicked()), need_reconnect_for_data_protocol, SLOT(show()));

    // same with special connection warnings
    need_reconnect_for_specialoption->hide();
    connect(mFORCE_MCCP_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));
    connect(mFORCE_GA_OFF, SIGNAL(clicked()), need_reconnect_for_specialoption, SLOT(show()));

    comboBox_statusBarSetting->addItem( tr( "Off" ), QVariant( mudlet::self()->statusBarHidden ) );
    comboBox_statusBarSetting->addItem( tr( "Auto" ), QVariant( mudlet::self()->statusBarAutoShown ) );
    comboBox_statusBarSetting->addItem( tr( "On" ), QVariant( mudlet::self()->statusBarAlwaysShown ) );
    comboBox_statusBarSetting->setMaxCount( 3 );
    comboBox_statusBarSetting->setInsertPolicy( QComboBox::NoInsert );
    comboBox_statusBarSetting->setMaxVisibleItems( 3 );
    int _indexForStatusBarSetting = comboBox_statusBarSetting->findData( QVariant(mudlet::self()->mStatusBarState), Qt::UserRole );
    if( _indexForStatusBarSetting >=0 ) {
        comboBox_statusBarSetting->setCurrentIndex( _indexForStatusBarSetting );
    }

    checkBox_reportMapIssuesOnScreen->setChecked( mudlet::self()->getAuditErrorsToConsoleEnabled() );
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
        mIsToLogInHtml->setChecked( pHost->mIsNextLogFileInHtmlFormat );
        commandLineMinimumHeight->setValue( pHost->commandLineMinimumHeight );
        mNoAntiAlias->setChecked( ! pHost->mNoAntiAlias );
        mFORCE_MCCP_OFF->setChecked( pHost->mFORCE_NO_COMPRESSION );
        mFORCE_GA_OFF->setChecked( pHost->mFORCE_GA_OFF );
        mAlertOnNewData->setChecked( pHost->mAlertOnNewData );
        //mMXPMode->setCurrentIndex( pHost->mMXPMode );
        //encoding->setCurrentIndex( pHost->mEncoding );
        mFORCE_SAVE_ON_EXIT->setChecked( pHost->mFORCE_SAVE_ON_EXIT );
        mEnableGMCP->setChecked( pHost->mEnableGMCP );
        mEnableMSDP->setChecked( pHost->mEnableMSDP );

        // load profiles into mappers "copy map to profile" combobox
        // this feature should work seamlessly both for online and offline profiles
        QStringList profileList = QDir( QStringLiteral( "%1/.config/mudlet/profiles" ).arg( QDir::homePath() ) )
                                   .entryList( QDir::Dirs|QDir::NoDotAndDotDot , QDir::Time ); // sort by profile "hotness"
        pushButton_chooseProfiles->setEnabled( false );
        pushButton_copyMap->setEnabled( false );
        QMenu * pMenu = new QMenu( tr( "Other profiles to Map to:" ) );
        for( unsigned int i=0, total = profileList.size(); i < total; ++i ) {
            QString s = profileList.at( i );
            if(    s.isEmpty()
              || ! s.compare( pHost->getName() )
              || ! s.compare( QStringLiteral( "default_host" ) ) ) {
                // Do not include THIS profile in the list - it will
                // automatically get saved - as the file to copy to the other
                // profiles!  Also exclude the dummy "default_host" one
                continue;
            }

            QAction * pItem = new QAction( s, 0 );
            pItem->setCheckable( true );
            pItem->setChecked( false );
            pMenu->addAction( pItem );
            //Enable it as we now have at least one profile to copy to
            pushButton_chooseProfiles->setEnabled( true );
        }

        pushButton_chooseProfiles->setMenu( pMenu );
        connect(pMenu, SIGNAL(triggered(QAction *)), this, SLOT(slot_chooseProfilesChanged(QAction *)));

        connect(pushButton_copyMap, SIGNAL(clicked()), this, SLOT(copyMap()));

        // label to show on sucessful map file action
        label_mapFileActionResult->hide();

        connect(pushButton_loadMap, SIGNAL(clicked()), this, SLOT(loadMap()));
        connect(pushButton_saveMap, SIGNAL(clicked()), this, SLOT(saveMap()));

        //doubleclick ignore
        QString ignore;
        QSetIterator<QChar> it(pHost->mDoubleClickIgnore);
        while( it.hasNext() )
            ignore = ignore.append(it.next());
        doubleclick_ignore_lineedit->setText( ignore );

        timeEdit_timerDebugOutputMinimumInterval->setTime( QTime( 0, 0, 0, 1).addMSecs( pHost->mTimerDebugOutputSuppressionInterval - 1 ) );
        // The above funky setting method is because a zero time is INVALID
        // and since Qt 5.0-ish adding any value to an invalid time still leaves
        // the time as "invalid".
        timeEdit_timerDebugOutputMinimumInterval->setCurrentSection( QDateTimeEdit::SecondSection );
        // Added so that the control is initially set to edit seconds (was
        // defaulting to first shown which was hours which is not the most
        // likely that the user would want to use)!
        connect(timeEdit_timerDebugOutputMinimumInterval, SIGNAL(timeChanged(QTime)), this, SLOT(slot_timeValueChanged(QTime)));

        // FIXME: Check this each time that it is appropriate for THIS build version
        comboBox_mapFileSaveFormatVersion->clear();
        // Add default version:
        comboBox_mapFileSaveFormatVersion->addItem( tr( "%1 {Default, recommended}" ).arg( pHost->mpMap->mDefaultVersion ),
                                                    QVariant( pHost->mpMap->mDefaultVersion ) );
        comboBox_mapFileSaveFormatVersion->setEnabled( false );
        label_mapFileSaveFormatVersion->setEnabled( false );
        if(  pHost->mpMap->mMaxVersion > pHost->mpMap->mDefaultVersion
          || pHost->mpMap->mMinVersion < pHost->mpMap->mDefaultVersion ) {
            for( short int i = pHost->mpMap->mMinVersion; i <= pHost->mpMap->mMaxVersion; ++i ) {
                if( i == pHost->mpMap->mDefaultVersion ) {
                    continue;
                }
                comboBox_mapFileSaveFormatVersion->setEnabled( true );
                label_mapFileSaveFormatVersion->setEnabled( true );
                if( i > pHost->mpMap->mDefaultVersion ) {
                    comboBox_mapFileSaveFormatVersion->addItem( tr( "%1 {Upgraded, experimental/testing, NOT recommended}" ).arg( i ),
                                                                QVariant( i ) );
                }
                else {
                    comboBox_mapFileSaveFormatVersion->addItem( tr( "%1 {Downgraded, for sharing with older version users, NOT recommended}" ).arg( i ),
                                                                QVariant( i ) );
                }
            }
            int _indexForCurrentSaveFormat = comboBox_mapFileSaveFormatVersion->findData( pHost->mpMap->mSaveVersion, Qt::UserRole );
            if( _indexForCurrentSaveFormat >=0 ) {
                comboBox_mapFileSaveFormatVersion->setCurrentIndex( _indexForCurrentSaveFormat );
            }
        }
        if( pHost->mpMap->mpMapper ) {
            checkBox_showDefaultArea->show();
            checkBox_showDefaultArea->setText( tr( "If checked (normal case) the \"%1\" IS shown in the map Area selection control." )
                                               .arg( pHost->mpMap->mpRoomDB->getDefaultAreaName() ) );
            checkBox_showDefaultArea->setChecked( pHost->mpMap->mpMapper->getDefaultAreaShown() );
        }
        else {
            checkBox_showDefaultArea->hide();
        }
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

void dlgProfilePreferences::downloadMap()
{
    Host * pHost = mpHost;
    if( ! pHost )
    {
        return;
    }
    if( ! pHost->mpMap->mpMapper ) {
        // CHECK: What happens if we are NOT the current profile anymore?
        mudlet::self()->createMapper( false );
    }

    pHost->mpMap->downloadMap();
}

void dlgProfilePreferences::loadMap()
{
    Host * pHost = mpHost;
    if( ! pHost )
    {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr( "Load Mudlet map" ),
                                                    QDir::homePath(),
                                                    tr( "Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)", "Do not change extensions (in braces) they are used programmatically!" ) );
    if( fileName.isEmpty() ) {
        return;
    }

    label_mapFileActionResult->show();

    // Ensure the setting is already made as the loadMap(...) uses the set value
    bool savedOldAuditErrorsToConsoleEnabledSetting = mudlet::self()->getAuditErrorsToConsoleEnabled();
    mudlet::self()->setAuditErrorsToConsoleEnabled( checkBox_reportMapIssuesOnScreen->isChecked() );

    if( fileName.endsWith( QStringLiteral( ".xml" ), Qt::CaseInsensitive ) ) {
        label_mapFileActionResult->setText( tr( "Importing map - please wait..." ) );
        qApp->processEvents(); // Needed to make the above message show up when loading big maps

        if ( mpHost->mpConsole->importMap(fileName) ) {
            label_mapFileActionResult->setText( tr( "Imported map from %1." ).arg( fileName ) );
        }
        else {
            label_mapFileActionResult->setText( tr( "Could not import map from %1." ).arg( fileName ) );
        }
    }
    else {
        label_mapFileActionResult->setText( tr( "Loading map - please wait..." ) );
        qApp->processEvents(); // Needed to make the above message show up when loading big maps


        if ( mpHost->mpConsole->loadMap(fileName) ) {
            label_mapFileActionResult->setText( tr( "Loaded map from %1." ).arg( fileName ) );
        }
        else {
            label_mapFileActionResult->setText( tr( "Could not load map from %1." ).arg( fileName ) );
        }
    }
    QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));

    // Restore setting immediately before we used it
    mudlet::self()->setAuditErrorsToConsoleEnabled( savedOldAuditErrorsToConsoleEnabledSetting );
}

void dlgProfilePreferences::saveMap()
{
    Host * pHost = mpHost;
    if( ! pHost ) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName( this,
                                                     tr( "Save Mudlet map" ),
                                                     QDir::homePath(),
                                                     tr( "Mudlet map (*.dat)", "Do not change the extension text (in braces) - it is needed programatically!" ) );
    if( fileName.isEmpty() ) {
        return;
    }

    if( ! fileName.endsWith( QStringLiteral( ".dat" ), Qt::CaseInsensitive) ) {
        fileName.append( QStringLiteral( ".dat" ) );
    }

    label_mapFileActionResult->show();
    label_mapFileActionResult->setText( tr( "Saving map - please wait..." ) );
    qApp->processEvents(); // Copied from "Loading map - please wait..." case
                           // Just in case is needed to make the above message
                           // show up when saving big maps

    // Temporarily use whatever version is currently set
    int oldSaveVersionFormat = pHost->mpMap->mSaveVersion;
#if QT_VERSION >= 0x050200
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();
#else
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->itemData( comboBox_mapFileSaveFormatVersion->currentIndex() ).toInt();
#endif

    // Ensure the setting is already made as the saveMap(...) uses the set value
    bool savedOldAuditErrorsToConsoleEnabledSetting = mudlet::self()->getAuditErrorsToConsoleEnabled();
    mudlet::self()->setAuditErrorsToConsoleEnabled( checkBox_reportMapIssuesOnScreen->isChecked() );

    if( pHost->mpConsole->saveMap(fileName) ) {
        label_mapFileActionResult->setText( tr( "Saved map to %1." ).arg( fileName ) );
    } else {
        label_mapFileActionResult->setText( tr( "Could not save map to %1." ).arg( fileName ) );
    }
    // Then restore prior version
    pHost->mpMap->mSaveVersion = oldSaveVersionFormat;
    mudlet::self()->setAuditErrorsToConsoleEnabled( savedOldAuditErrorsToConsoleEnabledSetting );

    QTimer::singleShot(10*1000, this, SLOT(hideActionLabel()));
}

void dlgProfilePreferences::hideActionLabel()
{
    label_mapFileActionResult->hide();
}

void dlgProfilePreferences::copyMap()
{
    Host * pHost = mpHost;
    if( ! pHost ) {
        return;
    }

    QMap<QString, int> toProfilesRoomIdMap;
    QMenu * _menu = pushButton_chooseProfiles->menu();
    QListIterator<QAction *> itAction( _menu->actions() );
    while( itAction.hasNext() ) {
        QAction * _action = itAction.next();
        if( _action->isChecked() ) {
            QString toProfileName = _action->text();
            toProfilesRoomIdMap.insert( toProfileName, 0 );
            // 0 is used as sentinel value that we don't have a valid Id yet
            // for the given Host - the contents of this map will be used to
            // update, or rather REPLACE TMap::mRoomIdHash

            // Check for the destination directory for the other profiles
            QDir toProfileDir;
            QString toProfileDirPathString = QStringLiteral( "%1/.config/mudlet/profiles/%2/map" )
                                                    .arg( QDir::homePath() )
                                                    .arg( toProfileName );
            if( ! toProfileDir.exists( toProfileDirPathString ) ) {
                if( ! toProfileDir.mkpath( toProfileDirPathString ) ) {
                    QString errMsg = tr( "[ ERROR ] - Unable to use or create directory to store map for other profile \"%1\".\n"
                                                     "Please check that you have permissions/access to:\n"
                                                     "\"%2\"\n"
                                                     "and there is enough space. The copying operation has failed." )
                                         .arg( toProfileName )
                                         .arg( toProfileDirPathString );
                    pHost->postMessage( errMsg );
                    label_mapFileActionResult->show();
                    label_mapFileActionResult->setText( tr( "Creating a destination directory failed..." ) );
                    return;
                }
            }
        }
    }

    // Identify which, if any, of the toProfilesRoomIdMap is active and get the current room
    QSet<Host *> activeHosts( mudlet::self()->mConsoleMap.keys().toSet() );
    QMap<QString, Host *> activeOtherHostMap;
    QSetIterator<Host *> itActiveHost( activeHosts );
    while( itActiveHost.hasNext() ) {
        Host * pOtherHost = itActiveHost.next();
        if( pOtherHost && pHost != pOtherHost && pOtherHost ) {
            QString otherHostName = pOtherHost->getName();
            if( toProfilesRoomIdMap.contains( otherHostName ) ) {
                activeOtherHostMap.insert( otherHostName, pOtherHost );
                toProfilesRoomIdMap.insert( otherHostName, pOtherHost->mpMap->mRoomIdHash.value( otherHostName, -1 ) );
            }
        }
    }
    // otherProfileCurrentRoomId will be -1 if tried and failed to get it from
    // current running profile, > 0 on sucess or 0 if not running as another profile

    // Ensure the setting is already made as the value could be used in the
    // code following after
    bool savedOldAuditErrorsToConsoleEnabledSetting = mudlet::self()->getAuditErrorsToConsoleEnabled();
    mudlet::self()->setAuditErrorsToConsoleEnabled( checkBox_reportMapIssuesOnScreen->isChecked() );

    // We now KNOW there are places where the destination profiles will/have
    // stored their maps - if we do not already know where the player is in the
    // other profiles - because they aren't active - or have not set it - try
    // and find out what the rooms are from the last saved files - ignoring
    // other details that we have also obtained.
    QMutableMapIterator<QString, int> itOtherProfile( toProfilesRoomIdMap );
    while( itOtherProfile.hasNext() ) {
        itOtherProfile.next();
        if( itOtherProfile.value() > 0 ) {
            // Skip the ones where we have already got the player room from the
            // active profile
            qDebug() << "dlgProfilePreference::copyMap() in other ACTIVE profile:"
                     << itOtherProfile.key()
                     << "\n    the player was located in:"
                     << itOtherProfile.value();
            if( pHost->mpMap->mpRoomDB->getRoom( itOtherProfile.value() ) ) {
                // That room IS in the map we are copying across, so update the
                // local record of it for the map for that profile:
                pHost->mpMap->mRoomIdHash[ itOtherProfile.key() ] = itOtherProfile.value();
            }
            continue;
        }

        // Most of these we'll just get for debugging!
        QString otherProfileFileUsed;
        int otherProfileRoomCount;
        int otherProfileAreaCount;
        int otherProfileVersion;
        int otherProfileCurrentRoomId; // What we are looking for!
        if( pHost->mpMap->retrieveMapFileStats( itOtherProfile.key(),
                                                 & otherProfileFileUsed,
                                                 & otherProfileVersion,
                                                 & otherProfileCurrentRoomId,
                                                 & otherProfileAreaCount,
                                                 & otherProfileRoomCount ) ) {

            qDebug() << "dlgProfilePreference::copyMap() in other INACTIVE profile:"
                     << itOtherProfile.key()
                     << "\n    the file examined was:"
                     << otherProfileFileUsed
                     << "\n    it was of version:"
                     << otherProfileVersion
                     << "\n    it had an area count of:"
                     << otherProfileAreaCount
                     << "\n    it had a room count of:"
                     << otherProfileRoomCount
                     << "\n    the player was located in:"
                     << otherProfileCurrentRoomId;
            itOtherProfile.setValue( otherProfileCurrentRoomId );
            // Using a mutable iterator we must modify (mutate) the data through
            // the iterator!
            if( pHost->mpMap->mpRoomDB->getRoom( otherProfileCurrentRoomId ) ) {
                // That room IS in the map we are copying across, so update the
                // local record of it for the map for that profile:
                pHost->mpMap->mRoomIdHash[ itOtherProfile.key() ] = otherProfileCurrentRoomId;
            }
        }
    }

    // Now, we can save our current map with all the profiles' player room data
    label_mapFileActionResult->show();
    label_mapFileActionResult->setText( tr( "Backing up current map - please wait..." ) );
    qApp->processEvents(); // Copied from "Loading map - please wait..." case
                           // Just in case is needed to make the above message
                           // show up when saving big maps

    // Temporarily use whatever version is currently set
    int oldSaveVersionFormat = pHost->mpMap->mSaveVersion;
#if QT_VERSION >= 0x050200
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();
#else
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->itemData( comboBox_mapFileSaveFormatVersion->currentIndex() ).toInt();
#endif

    if ( ! pHost->mpConsole->saveMap( QString() ) ) {
        label_mapFileActionResult->setText( tr( "Could not backup the map - saving it failed." ) );
        QTimer::singleShot( 10*1000, this, SLOT( hideActionLabel() ) );
        return;
    }

    // Then restore prior version
    pHost->mpMap->mSaveVersion = oldSaveVersionFormat;

    // work out which map is latest in THIS profile - which SHOULD be the one
    // we just saved!
    QString thisProfileLatestMapPathFileName;
    QFile thisProfileLatestMapFile;
    QString sourceMapFolder( QStringLiteral( "%1/.config/mudlet/profiles/%2/map" )
                                 .arg( QDir::homePath() )
                                 .arg( pHost->getName() ) );
    QStringList mProfileList = QDir( sourceMapFolder )
                                   .entryList( QDir::Files | QDir::NoDotAndDotDot, QDir::Time );
    for( unsigned int i = 0, total = mProfileList.size(); i < total; ++i ) {
        thisProfileLatestMapPathFileName = mProfileList.at( i );
        if( thisProfileLatestMapPathFileName.isEmpty() ) {
            continue;
        }

        thisProfileLatestMapFile.setFileName( QStringLiteral( "%1/%2" )
                                                  .arg( sourceMapFolder )
                                                  .arg( thisProfileLatestMapPathFileName ) );
        break;
    }

    if( thisProfileLatestMapFile.fileName().isEmpty() ) {
        label_mapFileActionResult->setText( tr( "Could not copy the map - failed to work out which map file we just saved the map as!" ) );
        QTimer::singleShot( 10 * 1000, this, SLOT( hideActionLabel() ) );
        return;
    }

    // Make the copies into the destination profiles (for all to profiles whether
    // in use or not):
    itOtherProfile.toFront();
    while( itOtherProfile.hasNext() ) {
        itOtherProfile.next();
        QString otherHostName = itOtherProfile.key();
        // Copy over into the profiles map folder, so it is loaded first when map is open - this covers the offline case
        label_mapFileActionResult->setText( tr( "Copying over map to %1 - please wait..." )
                                                .arg( otherHostName ) );
        qApp->processEvents(); // Copied from "Loading map - please wait..." case
                               // Just in case is needed to make the above message
                               // show up when saving big maps

        if( ! thisProfileLatestMapFile.copy( QStringLiteral( "%1/.config/mudlet/profiles/%2/map/%3" )
                                                 .arg( QDir::homePath() )
                                                 .arg( otherHostName )
                                                 .arg( thisProfileLatestMapPathFileName ) ) ) {
            label_mapFileActionResult->setText( tr( "Could not copy the map to %1 - unable to copy the new map file over." )
                                                        .arg( otherHostName ));
            QTimer::singleShot( 10*1000, this, SLOT( hideActionLabel() ) );
            continue; // Try again with next profile
        }
        else {
            label_mapFileActionResult->setText( tr( "Map copied successfully to other profile %1." )
                                                    .arg( otherHostName ) );
            qApp->processEvents(); // Copied from "Loading map - please wait..." case
                                   // Just in case is needed to make the above message
                                   // show up when saving big maps
        }
    }

    // Finally, signal the other profiles to reload their maps:
    mudlet::self()->requestProfilesToReloadMaps( toProfilesRoomIdMap.keys() );
    // GOTCHA: keys() is a QList<QString>, however, though it IS equivalent to a
    // QStringList in many ways, the SLOT/SIGNAL system treats them as different
    // - I thinK - so use QList<QString> thoughout the SIGNAL/SLOT links Slysven!
    label_mapFileActionResult->setText( tr( "Map copied, now signalling other profiles to reload it." ) );
    QTimer::singleShot( 10*1000, this, SLOT( hideActionLabel() ) );

    // CHECK: Race condition? We might be changing this whilst other profile
    // are accessing it...
    mudlet::self()->setAuditErrorsToConsoleEnabled( savedOldAuditErrorsToConsoleEnabledSetting );
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
    pHost->mEnableMSDP = mEnableMSDP->isChecked();
    pHost->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
    if( pHost->mpMap && pHost->mpMap->mpMapper ) {
        pHost->mpMap->mpMapper->mp2dMap->mMapperUseAntiAlias = mMapperUseAntiAlias->isChecked();
        bool isAreaWidgetInNeedOfResetting = false;
        if(  ( ! pHost->mpMap->mpMapper->getDefaultAreaShown() )
          && ( checkBox_showDefaultArea->isChecked() )
          && ( pHost->mpMap->mpMapper->mp2dMap->mAID == -1 ) ) {
            isAreaWidgetInNeedOfResetting = true;
        }

        pHost->mpMap->mpMapper->setDefaultAreaShown( checkBox_showDefaultArea->isChecked() );
        if( isAreaWidgetInNeedOfResetting ) {
            // Corner case fixup:
            pHost->mpMap->mpMapper->showArea->setCurrentText( pHost->mpMap->mpRoomDB->getDefaultAreaName() );
        }
        pHost->mpMap->mpMapper->mp2dMap->repaint(); // Forceably redraw it as we ARE currently showing default area
        pHost->mpMap->mpMapper->update();
    }
    pHost->mBorderTopHeight = topBorderHeight->value();
    pHost->mBorderBottomHeight = bottomBorderHeight->value();
    pHost->mBorderLeftWidth = leftBorderWidth->value();
    pHost->mBorderRightWidth = rightBorderWidth->value();
//qDebug()<<"Left border width:"<<pHost->mBorderLeftWidth<<" right:"<<pHost->mBorderRightWidth;
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
    pHost->mIsNextLogFileInHtmlFormat = mIsToLogInHtml->isChecked();
    pHost->mNoAntiAlias = !mNoAntiAlias->isChecked();
    pHost->mAlertOnNewData = mAlertOnNewData->isChecked();
    if( mudlet::self()->mConsoleMap.contains( pHost ) )
    {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
    QString lIgnore = doubleclick_ignore_lineedit->text();
    pHost->mDoubleClickIgnore.clear();
    for(int i=0;i<lIgnore.size();i++){
        pHost->mDoubleClickIgnore.insert(lIgnore.at(i));
    }
    QTime _midnight( 0, 0, 0, 1 );
    // zero time is NOT valid and QTime::msecsTo( const QTime & other ) returns
    // 0 if EITHER time is invalid
    pHost->mTimerDebugOutputSuppressionInterval = _midnight.msecsTo( timeEdit_timerDebugOutputMinimumInterval->time() ) - 1;
    if( pHost->mTimerDebugOutputSuppressionInterval < 0 ) {
        // Clean up if the control IS at zero, so the msecTo() fails and returns 0
        pHost->mTimerDebugOutputSuppressionInterval = 0;
    }

#if QT_VERSION >= 0x050200
    mudlet::self()->mStatusBarState = mudlet::StatusBarOptions( comboBox_statusBarSetting->currentData().toInt() );
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->currentData().toInt();
#else
    mudlet::self()->mStatusBarState = mudlet::StatusBarOptions( comboBox_statusBarSetting->itemData( comboBox_statusBarSetting->currentIndex() ).toInt() );
    pHost->mpMap->mSaveVersion = comboBox_mapFileSaveFormatVersion->itemData( comboBox_mapFileSaveFormatVersion->currentIndex() ).toInt();
#endif
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
//qDebug()<<"after console refresh: Left border width:"<<pHost->mBorderLeftWidth<<" right:"<<pHost->mBorderRightWidth;
    }
    mudlet::self()->setEditorTextoptions( checkBox_showSpacesAndTabs->isChecked(), checkBox_showLineFeedsAndParagraphs->isChecked() );
    mudlet::self()->setAuditErrorsToConsoleEnabled( checkBox_reportMapIssuesOnScreen->isChecked() );
    close();
}

// Needed to fixup the jump from zero (special "show all") to a non-zero time
// value always jumping to the most significant section (Hours) when for our
// purposes we would like it to be Seconds!
void dlgProfilePreferences::slot_timeValueChanged(QTime newTime)
{
    static QTime oldTime = QTime( 0, 0, 0, 0);

    // Has the value changed?
    if( oldTime != newTime ) {
        // Yes - was the old time zero (the special case "show all")?
        if( oldTime == QTime( 0, 0, 0, 0 ) && newTime == QTime( 1, 0, 0, 0) ) {
            // Yes - then set the section to be the seconds one:
            timeEdit_timerDebugOutputMinimumInterval->setCurrentSection( QDateTimeEdit::SecondSection );
            // And fix the fact that the +1 has already been applied but to the
            // hours and not the seconds:
            timeEdit_timerDebugOutputMinimumInterval->setTime( QTime( 0, 0, 1, 0 ) );
        }
        oldTime = newTime;
    }
}

void dlgProfilePreferences::slot_chooseProfilesChanged( QAction * _action )
{
    Q_UNUSED( _action );

    QMenu * _menu = pushButton_chooseProfiles->menu();
    QListIterator<QAction *> itAction( _menu->actions() );
    unsigned int selectionCount = 0;
    while( itAction.hasNext() ) {
        QAction * _currentAction = itAction.next();
        if( _currentAction->isChecked() ) {
            ++selectionCount;
        }
    }
    if( selectionCount ) {
        pushButton_copyMap->setEnabled( true );
        pushButton_chooseProfiles->setText( tr( "%1 selected - press to change" ).arg( selectionCount ) );
    }
    else {
        pushButton_copyMap->setEnabled( false );
        pushButton_chooseProfiles->setText( tr( "Press to pick destination(s)" ) );
    }
}
