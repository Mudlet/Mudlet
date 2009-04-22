/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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


#include <QMessageBox>
#include <QDebug>
#include "TConsole.h"
#include "mudlet.h"
#include <QScrollBar>
#include "TCommandLine.h"
#include <QVBoxLayout>
#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <time.h>
#include <unistd.h>
#include <QTextCodec>
#include <QHostAddress>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <stdio.h>
#include "TDebug.h"
#include "TTextEdit.h"
#include <QGraphicsSimpleTextItem>
#include "XMLexport.h"
#include <QShortcut>

using namespace std;

const QString TConsole::cmLuaLineVariable("line");

TConsole::TConsole( Host * pH, bool isDebugConsole ) 
: mpHost( pH )
, m_fontSpecs( pH )
, buffer( pH )
, mIsDebugConsole( isDebugConsole )
, mDisplayFont( QFont("Bitstream Vera Sans Mono", 10, QFont::Courier ) )//mDisplayFont( QFont("Monospace", 10, QFont::Courier ) )
, mFgColor( QColor( 0, 0, 0 ) )
, mBgColor( QColor( 255, 255, 255 ) )
, mCommandFgColor( QColor( 213, 195, 0 ) )
, mCommandBgColor( mBgColor )
, mSystemMessageFgColor( QColor( 255,0,0 ) )
, mSystemMessageBgColor( mBgColor )
, mWrapAt( 100 )
, mIndentCount( 0 )
, mTriggerEngineMode( false )
, mClipboard( mpHost )
, mpScrollBar( new QScrollBar )
, emergencyStop( new QPushButton )
, layerCommandLine( 0 )
, mLogFileName(QString(""))
, mLogToLogFile( false )
, networkLatency( new QLineEdit )
{
    QShortcut * ps = new QShortcut(this);
    ps->setKey(Qt::CTRL + Qt::Key_W);
    ps->setContext(Qt::WidgetShortcut);

    if( mIsDebugConsole )
    {
        mStandardFormat.bgColor = mBgColor;
        mStandardFormat.fgColor = mFgColor;
        mStandardFormat.bold = false;
        mStandardFormat.italics = false;
        mStandardFormat.underline = false;
    }
    else
    {
        mStandardFormat.bgColor = mpHost->mBgColor;
        mStandardFormat.fgColor = mpHost->mFgColor;
        mStandardFormat.bold = false;
        mStandardFormat.italics = false;
        mStandardFormat.underline = false;
    }
    setContentsMargins(0,0,0,0);
    profile_name = mpHost->getName();
    mFormatSystemMessage.bgColor = mBgColor;
    mFormatSystemMessage.fgColor = QColor( 255, 0, 0 );
    setAttribute( Qt::WA_DeleteOnClose );
    mWaitingForHighColorCode = false;
    mHighColorModeForeground = false;
    mHighColorModeBackground = false;
    mIsHighColorMode = false;
    
    QVBoxLayout * layout = new QVBoxLayout( this );
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy sizePolicy3( QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy sizePolicy2( QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    mpCommandLine = new TCommandLine( pH, this );
    mpCommandLine->setContentsMargins(0,0,0,0);
    mpCommandLine->setSizePolicy( sizePolicy );
    mpCommandLine->setMaximumHeight( 30 );
    mpCommandLine->setFocusPolicy( Qt::StrongFocus );
    
    layer = new QWidget( this );
    layer->setContentsMargins(0,0,0,0);
    layer->setSizePolicy( sizePolicy );
    layer->setFocusPolicy( Qt::NoFocus );
        
    QHBoxLayout * layoutLayer = new QHBoxLayout( layer );
    layoutLayer->setMargin(0);
    QSizePolicy sizePolicyLayer(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
    mpScrollBar->setFixedWidth( 15 );
    
    QSplitter * splitter = new QSplitter( Qt::Vertical, layer );
    splitter->setContentsMargins(0,0,0,0);
    splitter->setSizePolicy( sizePolicy );
    splitter->setOrientation( Qt::Vertical );
    splitter->setHandleWidth( 3 );
    setFocusProxy( mpCommandLine );
    
    console = new TTextEdit( this, splitter, &buffer, mpHost, isDebugConsole, false );
    console->setContentsMargins(0,0,0,0);
    console->setSizePolicy( sizePolicy3 );
    console->setFocusPolicy( Qt::NoFocus );
    
    console2 = new TTextEdit( this, splitter, &buffer, mpHost, isDebugConsole, true );
    console2->setContentsMargins(0,0,0,0);
    console2->setSizePolicy( sizePolicy3 );
    console2->setFocusPolicy( Qt::NoFocus );
    
    splitter->addWidget( console );
    splitter->addWidget( console2 );
    
    splitter->setCollapsible( 1, false );
    splitter->setCollapsible( 0, false );
    splitter->setStretchFactor(0,6);
    splitter->setStretchFactor(1,1);
    
    layoutLayer->addWidget( splitter );
    layoutLayer->addWidget( mpScrollBar );
    
    layerCommandLine = new QWidget( layer );
    layerCommandLine->setContentsMargins(0,0,0,0);
    layerCommandLine->setSizePolicy( sizePolicy2 );
    layerCommandLine->setMaximumHeight(31);
    QHBoxLayout * layoutLayer2 = new QHBoxLayout( layerCommandLine );
    layoutLayer2->setMargin(0);
    layoutLayer2->setSpacing(0);
    
    QPushButton * timeStampButton = new QPushButton;
    timeStampButton->setCheckable( true );
    timeStampButton->setFocusPolicy( Qt::NoFocus );
    timeStampButton->setToolTip("Show Time Stamps");
    QIcon icon(":/icons/dialog-information.png");
    timeStampButton->setIcon( icon );
    connect( timeStampButton, SIGNAL(pressed()), console, SLOT(slot_toggleTimeStamps()));
    
    QPushButton * logButton = new QPushButton;
    logButton->setCheckable( true );
    logButton->setFocusPolicy( Qt::NoFocus );
    logButton->setToolTip("start logging MUD output to log file");
    QIcon icon3(":/icons/folder-downloads.png");
    logButton->setIcon( icon3 );
    connect( logButton, SIGNAL(pressed()), this, SLOT(slot_toggleLogging()));

    networkLatency->setReadOnly( true );
    networkLatency->setFocusPolicy( Qt::NoFocus );
    networkLatency->setToolTip("latency of the MUD-server and network (current/average)");
    networkLatency->setMaximumWidth( 120 );
    networkLatency->setMinimumWidth( 120 );
    networkLatency->setAutoFillBackground( false );
    /*QPalette lagPalette;
    lagPalette.setColor( QPalette::Text, QColor(100,255,100) );
    lagPalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    lagPalette.setColor( QPalette::Base, QColor(0,0,0) );
    networkLatency->setPalette(lagPalette);*/
    QFont latencyFont = QFont("Bitstream Vera Sans Mono", 6, QFont::Courier);
    networkLatency->setFont( latencyFont );

    QIcon icon2(":/icons/edit-bomb.png");
    emergencyStop->setIcon( icon2 );
    emergencyStop->setFocusPolicy( Qt::NoFocus );
    emergencyStop->setCheckable( true );
    emergencyStop->setToolTip("Emergency Stop. Stop All Timers and Triggers");
    connect( emergencyStop, SIGNAL(clicked(bool)), this, SLOT(slot_stop_all_triggers( bool )));

    layoutLayer2->addWidget( mpCommandLine );
    layoutLayer2->addWidget( timeStampButton );
    layoutLayer2->addWidget( logButton );
    layoutLayer2->addWidget( emergencyStop );
    layoutLayer2->addWidget( networkLatency );

    layout->addWidget( layer );
    layout->addWidget( layerCommandLine );
          
    QList<int> sizeList;
    sizeList << 6 << 2;
    splitter->setSizes( sizeList );
    
    console->show();
    console2->show();
    console2->hide();
    if( mIsDebugConsole )
        mpCommandLine->hide();
  
    isUserScrollBack = false;
       
    m_fontSpecs.init();
    connect( mpScrollBar, SIGNAL(valueChanged(int)), console, SLOT(slot_scrollBarMoved(int)));
    changeColors();
}

void TConsole::closeEvent( QCloseEvent *event )
{
    if( profile_name != "default_host" )
    {
        ASK: int choice = QMessageBox::question( this, "Exiting Session: Question", "Do you want to save the profile "+profile_name, QMessageBox::Yes|QMessageBox::No );
        if( choice == QMessageBox::Yes )
        {
            QString directory_xml = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current";
            QString filename_xml = directory_xml + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+".xml";
            QDir dir_xml;
            if( ! dir_xml.exists( directory_xml ) )
            {
                dir_xml.mkpath( directory_xml );    
            }
            QFile file_xml( filename_xml );
            if( file_xml.open( QIODevice::WriteOnly ) )
            {
                XMLexport writer( mpHost );
                writer.exportHost( & file_xml );
                file_xml.close();
                event->accept();
                return;
            }
            else
            {
                QMessageBox::critical( this, "ERROR: Not closing profile.", "Failed to save "+profile_name+" to location "+filename_xml+" because of the following error: "+file_xml.errorString() );
                goto ASK;
            }
        }
        else if( choice == QMessageBox::No )
        {
            event->accept();
            return;
        }
        else
        {
            if( ! mudlet::self()->isGoingDown() )
            {
                QMessageBox::warning( this, "Aborting exit","Session exit aborted on user request." );
                event->ignore();
                return;
            }
            else
            {
                event->accept();
                return;
            }
        }
    }
}

void TConsole::slot_toggleLogging()
{
    mLogToLogFile = ! mLogToLogFile;
    if( mLogToLogFile )
    {
        QString directoryLogFile = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/log";
        QString mLogFileName = directoryLogFile + "/"+QDateTime::currentDateTime().toString("dd-MM-yyyy#hh-mm-ss")+".html";
        QDir dirLogFile;
        if( ! dirLogFile.exists( directoryLogFile ) )
        {
            dirLogFile.mkpath( directoryLogFile );
        }
        mLogFile.setFileName( mLogFileName );
        mLogFile.open( QIODevice::WriteOnly );
        mLogStream.setDevice( &mLogFile );
        mLogStream << "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'><html><head><style><!-- *{ font-family: 'Courier New', 'Monospace', 'Courier';} *{ white-space: pre-wrap; } *{background-color:rgb("<<mpHost->mBgColor.red()<<","<<mpHost->mBgColor.green()<<","<<mpHost->mBgColor.blue()<<");} --></style><meta http-equiv='content-type' content='text/html; charset=utf-8'></head><body>";
        QString message = QString("Logging has started. Log file is ") + mLogFile.fileName();
        printSystemMessage( message );
    }
    else
    {
        mLogStream << "</pre></body></html>";
        mLogFile.close();
        QString message = QString("Logging has been stopped. Log file is ") + mLogFile.fileName() ;
        printSystemMessage( message );
    }
}

void TConsole::changeColors()
{
    if( mIsDebugConsole )
    {
        mDisplayFont.setStyleStrategy( (QFont::StyleStrategy)(QFont::PreferAntialias | QFont::PreferQuality) );
        console->setFont( mDisplayFont );
        console2->setFont( mDisplayFont );
        QPalette palette;
        palette.setColor( QPalette::Text, mFgColor );
        palette.setColor( QPalette::Highlight, QColor(55,55,255) );
        palette.setColor( QPalette::Base, mBgColor );
        console->setPalette( palette );
        console2->setPalette( palette );    
    }
    else
    {
        mpHost->mDisplayFont.setStyleStrategy( (QFont::StyleStrategy)(QFont::PreferAntialias | QFont::PreferQuality) );
        console->setFont( mpHost->mDisplayFont );
        console2->setFont( mpHost->mDisplayFont );
        QPalette palette;
        palette.setColor( QPalette::Text, mpHost->mFgColor );
        palette.setColor( QPalette::Highlight, QColor(55,55,255) );
        palette.setColor( QPalette::Base, mpHost->mBgColor );
        setPalette( palette );
        layer->setPalette( palette );
        console->setPalette( palette );
        console2->setPalette( palette );
    }
}

/*std::string TConsole::getCurrentTime()
{
    time_t t;
    time(&t);
    tm lt;
    ostringstream s;
    s.str("");
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    localtime_r( &t, &lt ); 
    s << "["<<lt.tm_hour<<":"<<lt.tm_min<<":"<<lt.tm_sec<<":"<<tv.tv_usec<<"]";
    string time = s.str();
    return time;
} */

void TConsole::set_text_properties(int tag)
{
    //qDebug()<<"tag="<<tag;
    // are we dealing with 256 color mode enabled servers or standard ANSI colors?
    if( mWaitingForHighColorCode )
    {
        if( mHighColorModeForeground )
        {
            if( tag < 16 )
            {
                mHighColorModeForeground = false;
                mWaitingForHighColorCode = false;
                mIsHighColorMode = false;
                goto NORMAL_ANSI_COLOR_TAG;
            }
            if( tag < 232 )
            {
                tag-=16; // because color 1-15 behave like normal ANSI colors
                // 6x6 RGB color space 
                int r = tag / 36; 
                int g = (tag-(r*36)) / 6; 
                int b = (tag-(r*36))-(g*6); 
                m_fontSpecs.fgColor = QColor( r*42, g*42, b*42 ); 
            }
            else
            {
                // black + 23 tone grayscale from dark to light gray
                tag -= 232;
                m_fontSpecs.fgColor = QColor( tag*10, tag*10, tag*10 );
            }
            mHighColorModeForeground = false;
            mWaitingForHighColorCode = false;
            mIsHighColorMode = false;
            return;
        }
        if( mHighColorModeBackground )
        {
            if( tag < 16 )
            {
                mHighColorModeBackground = false;
                mWaitingForHighColorCode = false;
                mIsHighColorMode = false;
                goto NORMAL_ANSI_COLOR_TAG;
            }
            if( tag < 232 )
            {
                tag-=16;
                int r = tag / 36; 
                int g = (tag-(r*36)) / 6; 
                int b = (tag-(r*36))-(g*6); 
                m_fontSpecs.bgColor = QColor( r*42, g*42, b*42 ); 
            }
            else
            {
                // black + 23 tone grayscale from dark to light gray
                tag -= 232;
                m_fontSpecs.fgColor = QColor( tag*10, tag*10, tag*10 );
            }
            mHighColorModeBackground = false;
            mWaitingForHighColorCode = false;
            mIsHighColorMode = false;
            return;
        }
    }
    
    if( tag == 38 ) 
    {
        mIsHighColorMode = true; 
        mHighColorModeForeground = true;
        return;
    }
    if( tag == 48 )    
    {
        mIsHighColorMode = true;
        mHighColorModeBackground = true;
    }
    if( ( mIsHighColorMode ) && ( tag == 5 ) )
    {
        mWaitingForHighColorCode = true;    
        return;
    }
    
    // we are dealing with standard ANSI colors
NORMAL_ANSI_COLOR_TAG:
    
    switch( tag )
    {
    case 0: 
        mHighColorModeForeground = false;
        mHighColorModeBackground = false;
        mWaitingForHighColorCode = false;
        mIsHighColorMode = false;
        m_fontSpecs.reset();
        break;
    case 1: 
        m_fontSpecs.bold = true;
        break;
    case 2: 
        m_fontSpecs.bold = false;
        break;
    case 3: 
        m_fontSpecs.italics = true;
        break;
    case 4:
        m_fontSpecs.underline = true;
    case 5: 
        break; //FIXME support blinking
    case 6:
        break; //FIXME support fast blinking
    case 7:
        break; //FIXME support inverse
    case 9:
        break; //FIXME support strikethrough
    case 22:
        m_fontSpecs.bold = false;
        break;
    case 23: 
        m_fontSpecs.italics = false;
        break;
    case 24: 
        m_fontSpecs.underline = false;
        break;
    case 27: 
        break; //FIXME inverse off
    case 29: 
        break; //FIXME
    case 30:
        m_fontSpecs.fgColor = mpHost->mBlack;
        m_fontSpecs.isDefaultColor = false;
        m_fontSpecs.fgColorLight = mpHost->mLightBlack;
        break;
    case 31:
        m_fontSpecs.fgColor = mpHost->mRed;
        m_fontSpecs.fgColorLight = mpHost->mLightRed;
        m_fontSpecs.isDefaultColor = false;
        break;
    case 32:
        m_fontSpecs.fgColor = mpHost->mGreen;
        m_fontSpecs.fgColorLight = mpHost->mLightGreen;
        m_fontSpecs.isDefaultColor = false;
        break;
    case 33:
        m_fontSpecs.fgColor = mpHost->mYellow;
        m_fontSpecs.fgColorLight = mpHost->mLightYellow;
        m_fontSpecs.isDefaultColor = false;
        break;
    case 34:
        m_fontSpecs.fgColor = mpHost->mBlue;
        m_fontSpecs.fgColorLight = mpHost->mLightBlue;
        m_fontSpecs.isDefaultColor = false;
        break;
    case 35:
        m_fontSpecs.fgColor = mpHost->mMagenta;
        m_fontSpecs.fgColorLight = mpHost->mLightMagenta;
        m_fontSpecs.isDefaultColor = false;
        break;
    case 36:
        m_fontSpecs.fgColor = mpHost->mCyan; 
        m_fontSpecs.fgColorLight = mpHost->mLightCyan;
        m_fontSpecs.isDefaultColor = false;
        break;
    case 37:
        m_fontSpecs.fgColor = mpHost->mWhite;
        m_fontSpecs.fgColorLight = mpHost->mLightWhite;
        m_fontSpecs.isDefaultColor = false;
        break;
    case 39:
        m_fontSpecs.bgColor = mpHost->mBgColor;//mWhite
        break;
    case 40:
        m_fontSpecs.bgColor = mpHost->mBlack;
        break;
    case 41:
        m_fontSpecs.bgColor = mpHost->mRed;
        break;
    case 42:
        m_fontSpecs.bgColor = mpHost->mGreen;
        break;
    case 43:
        m_fontSpecs.bgColor = mpHost->mYellow;
        break;
    case 44:
        m_fontSpecs.bgColor = mpHost->mBlue;
        break;
    case 45:
        m_fontSpecs.bgColor = mpHost->mMagenta;
        break;
    case 46:
        m_fontSpecs.bgColor = mpHost->mCyan;
        break;
    case 47:
        m_fontSpecs.bgColor = mpHost->mWhite;
        break;
    };
}


QString TConsole::translate( QString & s )
{
}

/*  QTextDocument *document = edit->document();
   QTextCursor cursor(document);

   cursor.movePosition(QTextCursor::Start);
   cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

   QTextCharFormat format;
   format.setFontWeight(QFont::Bold); 
   cursor.mergeCharFormat(format);
*/

/* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on 
      2 intensity faint
      3 italics on       
      4 underline on     
      5 blink slow
      6 blink fast
      7 inverse on       
      9 strikethrough    
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      27 inverse off
      28 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 bg default white
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg black    

      sequences for 256 Color support:
      38;5;0-256 foreground color
      48;5;0-256 background color */

void TConsole::translateToPlainText( QString & s )
{
    int cursorX=0, cursorY=0;
    if( mFormatSequenceRest.size() > 0 ) 
    {
        s.prepend( mFormatSequenceRest );
        mFormatSequenceRest.clear();
    }
    mFormatSequenceRest="";
    int sequence_begin = 0;
    int sequence_end = 0;
    int pos = 0;
    QString sequence;
    sequence_begin = s.indexOf(QString("\033["), 0);
    if( ( sequence_begin == -1 ) || ( sequence_begin > 0 ) ) 
    {
        if( sequence_begin == -1 )
        {
            buffer.append( s, m_fontSpecs.fgColor, m_fontSpecs.bgColor, m_fontSpecs.bold, m_fontSpecs.italics, m_fontSpecs.underline );
            return;
        }
        else
        {
            buffer.append( s.mid( 0, sequence_begin ), m_fontSpecs.fgColor, m_fontSpecs.bgColor, m_fontSpecs.bold, m_fontSpecs.italics, m_fontSpecs.underline );
        }
    }
    while( sequence_begin != -1 )
    {
        int t1,t2,t3,t4;
        t1=t2=t3=t4=0;
        QTime time;
        time.restart();
        // what kind of control sequence is this?
        bool isCursorMove = false;
        QChar control_sequence_type = 'x';
        int control_sequence_type_minor = -1;
        for( int k=0; k<s.size(); k++ )
        {
            QChar c_k = s[sequence_begin+k];
            if( c_k == QChar('m') )
            {
                control_sequence_type = QChar('m');
                break;
            }
            if( c_k == QChar('H') )
            {
                isCursorMove = true;
                control_sequence_type = QChar('H');
                break;
            }
            if( c_k == QChar('J') )
            {
                isCursorMove = true;
                control_sequence_type = QChar('J');
                break;
            }
            if( c_k == QChar('f') )
            {
                isCursorMove = true;
                control_sequence_type = QChar('H');
                break;
            }
            if( c_k == QChar('A') )
            {
                isCursorMove = true;
                control_sequence_type = QChar('A');
                break;
            }
            if( c_k == QChar('B') )
            {        
                isCursorMove = true;
                control_sequence_type = QChar('B');
                break;
            }
            if( c_k == QChar('C') )
            {
                isCursorMove = true;
                control_sequence_type = QChar('C');
                break;
            }
            if( c_k == QChar('D') )
            {
                isCursorMove = true;
                control_sequence_type = QChar('D');
                break;
            }
           
            if( c_k == QChar('K') )
            {        
                isCursorMove = true;
                control_sequence_type = QChar('K');
                break;
            }
        }
        QStringList textPropertyList;
        sequence_end = s.indexOf( control_sequence_type ,sequence_begin );
        int sequence_length = abs(sequence_begin - sequence_end );
        if( sequence_end != -1 )
        {
            sequence = s.mid(sequence_begin+2,sequence_length-2); // weil 3 elemente ausgelassen werden
            if( sequence.indexOf(QChar(';'),0) != -1 )
            {
                if( control_sequence_type == QChar('m') )
                {
                    textPropertyList = sequence.split(QChar(';'),QString::SkipEmptyParts);
                }
                if( control_sequence_type == QChar('H') )
                {
                    textPropertyList = sequence.split(QChar(';'),QString::SkipEmptyParts);
                } 
            }
            else
            {
                if( control_sequence_type == QChar('J') )
                {
                    control_sequence_type_minor = sequence.toInt();
                    textPropertyList << sequence;
                }
                if( control_sequence_type == QChar('D') )
                {
                    control_sequence_type_minor = sequence.toInt();
                    textPropertyList << sequence;
                } 
                if( control_sequence_type == QChar('m') )
                {
                    textPropertyList << sequence;
                }
            }            
            if( textPropertyList.size() == 0 )
            {
                if( control_sequence_type == QChar('H') )
                {
                    cursorX=0;
                    cursorY=0;
                } 
                if( control_sequence_type == QChar('D') )
                {
                   cursorX--;
                } 
                if( control_sequence_type == QChar('J') )
                {
                    // ESC[J or [0J -> clear screen from cursor downwards
                    // clear screen from cursor down
                    //FIXME
                }    
            } 
            for( int i=0; i<textPropertyList.size(); i++ )
            {
                if( control_sequence_type == QChar('m') )
                {
                    set_text_properties( textPropertyList[i].toInt() );
                    continue;
                }
                if( control_sequence_type == QChar('D') )
                {
                    cursorX-=textPropertyList[i].toInt();
                } 
                if( control_sequence_type == QChar('H') )
                {
                    if( textPropertyList.size()-i < 2 ) break;
                    cursorY = textPropertyList[i].toInt();
                    cursorX = textPropertyList[++i].toInt();
                    break;
                } 
                if( control_sequence_type == QChar('J') )
                {
                    // ANSI(ESC[2J): clear screen -> our screen size is fixed at 80x25
                    // other ESC[0J or ESC[1J or ESC[J are not ANSI but VT100 control sequences, but they are being used e.g. by LD-Driver Muds
                    // ESC[J or [0J -> clear screen from cursor downwards
                    // ESC[1J -> clear screen from cursor upwareds
                    if( control_sequence_type_minor == 2 )
                    {
                        // clear screen (ESC[2J)
                        cursorX = 0;
                        cursorY = 0;
                        //FIXME buffer.clearScreen();
                    }
                    if( control_sequence_type_minor ==  1 )
                    {
                        //FIXME buffer.clearScreenUpwards();
                    }  
                    if( control_sequence_type_minor ==  0 )
                    {
                        //FIXME buffer.clearSceenDownwards();
                    }    
                }
            }
            pos = sequence_begin + sequence_length;
            sequence_begin = s.indexOf( QString( "\033[" ), pos );
            int dangling_esc = -1;
            // retrieve dangling <ESC> sequence headers in case of package splits
            if( sequence_begin == -1 )
            {
                dangling_esc = s.indexOf( QChar('\033'), pos );
            }
            if( (sequence_begin > pos + 1 ) || ( sequence_begin == -1 ) )
            {
                if( dangling_esc > -1 )
                {
                     // sequence_end is in next TCP/IPpacket keep translation state
                     mFormatSequenceRest = QChar('\033');
                     if( ! m_fontSpecs.bold || m_fontSpecs.isDefaultColor )
                     {
                         buffer.append( s.mid( pos+1, dangling_esc ),
                                        m_fontSpecs.fgColor,
                                        m_fontSpecs.bgColor,
                                        m_fontSpecs.isDefaultColor ? m_fontSpecs.bold : false,
                                        m_fontSpecs.italics,
                                        m_fontSpecs.underline );
                     }
                     else
                     {
                          buffer.append( s.mid( pos+1, dangling_esc ),
                                         m_fontSpecs.fgColorLight,
                                         m_fontSpecs.bgColor,
                                         false,
                                         m_fontSpecs.italics,
                                         m_fontSpecs.underline );
                     }
                     return;
                }
                if( ! m_fontSpecs.bold || m_fontSpecs.isDefaultColor )
                {
                    buffer.append( s.mid( pos+1, sequence_begin-pos-1 ),
                                   m_fontSpecs.fgColor,
                                   m_fontSpecs.bgColor,
                                   m_fontSpecs.isDefaultColor ? m_fontSpecs.bold : false,
                                   m_fontSpecs.italics,
                                   m_fontSpecs.underline );
                }
                else
                {
                    buffer.append( s.mid( pos+1, sequence_begin-pos-1 ),
                                   m_fontSpecs.fgColorLight,
                                   m_fontSpecs.bgColor,
                                   false,
                                   m_fontSpecs.italics,
                                   m_fontSpecs.underline );
                }
            }
        }// is sequence_end included in this data packet?
        else
        {
            // sequence_end is in next TCP/IPpacket keep translation state
            mFormatSequenceRest = s.mid( sequence_begin, -1 );
            return;
        }
    }//while
}

void TConsole::printOnDisplay( QString & incomingSocketData )  
{
    QString prompt ="";//FIXME
    if( mLogToLogFile )
    {
        QString log = incomingSocketData;
        mLogStream << logger_translate( log );
    }
    int lineBeforeNewContent = buffer.getLastLineNumber();
    translateToPlainText( incomingSocketData );
    
    mTriggerEngineMode = true;
    mDeletedLines = 0;
    int lastLineNumber = buffer.getLastLineNumber();
    mProcessingTime.restart();
    for( int i=lineBeforeNewContent; i<=lastLineNumber; i++ )
    {
        mUserCursor.setY( i );
        mEngineCursor = i;
        mUserCursor.setX( 0 );
        mCurrentLine = buffer.line( i );
        mCurrentLine.replace( QChar( 0x21af ), QChar('\n') );
        //qDebug()<<"line<"<<mCurrentLine<<">";
        mpHost->getLuaInterpreter()->set_lua_string( cmLuaLineVariable, mCurrentLine );
        if( mudlet::debugMode ) TDebug() << "new line = " << mCurrentLine;
        mpHost->incomingStreamProcessor( mCurrentLine, prompt );

        if( mDeletedLines > 0 )
        {
            i = i - mDeletedLines;
            mDeletedLines = 0;
            buffer.newLines--;
            continue;
        }
    }
    double processT = mProcessingTime.elapsed();
    mTriggerEngineMode = false;    
    buffer.wrap( lineBeforeNewContent, mpHost->mWrapAt, mpHost->mWrapIndentCount, mStandardFormat );
    console->showNewLines();
    console2->showNewLines();
    moveCursorEnd();
    networkLatency->setText( QString("net:%1 sys:%2").arg(mpHost->mTelnet.networkLatency,0,'f',3)
                                                     .arg(processT/1000,0,'f',3));
}

QString TConsole::assemble_html_font_specs()
{
    QString s;
    s = "</span><span style=\"";
    if( m_LoggerfontSpecs.m_fgColorHasChanged )
    {
        s+="color: rgb("+
            QString::number(m_LoggerfontSpecs.fgColor.red())+","+
            QString::number(m_LoggerfontSpecs.fgColor.green())+","+
            QString::number(m_LoggerfontSpecs.fgColor.blue()) + ");";
    }
    if( m_LoggerfontSpecs.m_bgColorHasChanged )
    {
        s += " background: rgb("+
            QString::number(m_LoggerfontSpecs.bgColor.red())+","+
            QString::number(m_LoggerfontSpecs.bgColor.green())+","+
            QString::number(m_LoggerfontSpecs.bgColor.blue()) +");";
    }
    s += " font-weight: " + m_LoggerfontSpecs.getFontWeight() +
        "; font-style: " + m_LoggerfontSpecs.getFontStyle() +
        "; font-decoration: " + m_LoggerfontSpecs.getFontDecoration() +
        "\">";
    return s;
}

void TConsole::logger_set_text_properties( QString tags )
{
    switch( tags.toInt() )
    {
    case 0:
        m_LoggerfontSpecs.reset();
        break;
    case 1:
        m_LoggerfontSpecs.bold = true;
        break;
    case 2:
        m_LoggerfontSpecs.bold = false;
        break;
    case 3:
        m_LoggerfontSpecs.italics = true;
        break;
    case 4:
        m_LoggerfontSpecs.underline = true;
    case 5:
        break; //FIXME support blinking
    case 6:
        break; //FIXME support fast blinking
    case 7:
        break; //FIXME support inverse
    case 9:
        break; //FIXME support strikethrough
    case 22:
        m_LoggerfontSpecs.bold = false;
        break;
    case 23:
        m_LoggerfontSpecs.italics = false;
        break;
    case 24:
        m_LoggerfontSpecs.underline = false;
        break;
    case 27:
        break; //FIXME inverse off
    case 29:
        break; //FIXME
    case 30:
        m_LoggerfontSpecs.fgColor = mpHost->mBlack;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightBlack;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 31:
        m_LoggerfontSpecs.fgColor = mpHost->mRed;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightRed;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 32:
        m_LoggerfontSpecs.fgColor = mpHost->mGreen;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightGreen;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 33:
        m_LoggerfontSpecs.fgColor = mpHost->mYellow;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightYellow;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 34:
        m_LoggerfontSpecs.fgColor = mpHost->mBlue;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightBlue;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 35:
        m_LoggerfontSpecs.fgColor = mpHost->mMagenta;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightMagenta;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 36:
        m_LoggerfontSpecs.fgColor = mpHost->mCyan;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightCyan;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 37:
        m_LoggerfontSpecs.fgColor = mpHost->mWhite;
        m_LoggerfontSpecs.fgColorLight = mpHost->mLightWhite;
        m_LoggerfontSpecs.fg_color_change();
        break;
    case 39:
        m_LoggerfontSpecs.bgColor = mpHost->mBgColor;//mWhite
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 40:
        m_LoggerfontSpecs.bgColor = mpHost->mBlack;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 41:
        m_LoggerfontSpecs.bgColor = mpHost->mRed;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 42:
        m_LoggerfontSpecs.bgColor = mpHost->mGreen;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 43:
        m_LoggerfontSpecs.bgColor = mpHost->mYellow;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 44:
        m_LoggerfontSpecs.bgColor = mpHost->mBlue;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 45:
        m_LoggerfontSpecs.bgColor = mpHost->mMagenta;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 46:
        m_LoggerfontSpecs.bgColor = mpHost->mCyan;
        m_LoggerfontSpecs.bg_color_change();
        break;
    case 47:
        m_LoggerfontSpecs.bgColor = mpHost->mWhite;
        m_LoggerfontSpecs.bg_color_change();
        break;
    };
}


QString TConsole::logger_translate( QString & s )
{
     /* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on
      2 intensity faint
      3 italics on
      4 underline on
      5 blink slow
      6 blink fast
      7 inverse on
      9 strikethrough
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      27 inverse off
      28 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 bg default white
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg black     */


    //s.replace(QChar('\\'), "\\\\");
    s.replace(QChar('\n'), "<br />");
    s.replace(QChar('\t'), "     ");
    int sequence_begin = 0;
    int sequence_end = 0;
    QString sequence;
    while( (sequence_begin = s.indexOf(QString("\033["),0) ) != -1 )
    {
        sequence_end = s.indexOf(QChar('m'),sequence_begin);
        int sequence_length = abs(sequence_begin - sequence_end )+1;
        if( sequence_end != -1 )
        {
            sequence = s.mid(sequence_begin+2,sequence_length-3); // weil 3 elemente ausgelassen werden
            QStringList textPropertyList;
            if( sequence.indexOf(QChar(';'),0) )
            {
                textPropertyList = sequence.split(QChar(';'),QString::SkipEmptyParts);
            }
            else
            {
                textPropertyList << sequence;
            }
            for( int i=0; i<textPropertyList.size(); i++ )
            {
                m_LoggerfontSpecs.m_fgColorHasChanged = false;
                m_LoggerfontSpecs.m_bgColorHasChanged = false;
                logger_set_text_properties(textPropertyList[i]);
                //            qDebug()<<"set property:"<<textPropertyList[i];
            }
            QString html_tags = assemble_html_font_specs();
            s.replace(sequence_begin,sequence_length,html_tags);
        }
        else
        {
            break; // sequenzende befindet sich im naechsten tcp/ip packet
        }
    }

    return s;
}


void TConsole::scrollDown( int lines )
{
    console->scrollDown( lines );
    if( console->isTailMode() ) console2->hide();
}

void TConsole::scrollUp( int lines )
{
    console2->show();
    console->scrollUp( lines );    
}

void TConsole::reset()
{
    mFormatCurrent.bgColor = mStandardFormat.bgColor;
    mFormatCurrent.fgColor = mStandardFormat.fgColor;
    mFormatCurrent.bold = false;
    mFormatCurrent.italics = false;
    mFormatCurrent.underline = false;
}

void TConsole::insertText( QString text, QPoint P )
{
    int x = P.x();
    int y = P.y();
    int o = 0;//FIXME: das ist ein fehler bei mehrzeiliger selection
    int r = text.size();
    if( mTriggerEngineMode )
    {
        if( hasSelection() )
        {
            if( r < o )
            {
                int a = -1*(o-r);
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
            if( r > o )
            {
                int a = r-o;
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
        }
        else
        {
            mpHost->getLuaInterpreter()->adjustCaptureGroups( x, r );
        }
        if( y < mEngineCursor )
        {
            buffer.insertInLine( P, text, mFormatCurrent );
            console->needUpdate(mUserCursor.y(),mUserCursor.y()+1);
        }
        else if( y >= mEngineCursor )
        {
            buffer.insertInLine( P, text, mFormatCurrent );
        }
        return;
    }
    else
    {
        if( mUserCursor.y() == buffer.getLastLineNumber() )
        {
            buffer.append( text,
                           mFormatCurrent.fgColor,
                           mFormatCurrent.bgColor,
                           false,
                           false,
                           false );
            console->showNewLines();
        }
        else
        {
            buffer.insertInLine( mUserCursor,
                                 text,
                                 mFormatCurrent );
            if( text.indexOf("\n") != -1 )
            {
                int y_tmp = mUserCursor.y();
                int down = buffer.wrapLine( mUserCursor.y(),mpHost->mScreenWidth, mpHost->mWrapIndentCount, mFormatCurrent );
                console->needUpdate( y_tmp, y_tmp+down+1 );
                int y_neu = y_tmp+down;
                int x_adjust = text.lastIndexOf("\n");
                int x_neu = 0;
                if( x_adjust != -1 )
                {
                    x_neu = text.size()-x_adjust-1 > 0 ? : 0;
                }
                moveCursor( x_neu, y_neu );
            }
            else
            {
                console->needUpdate( mUserCursor.y(),mUserCursor.y()+1 );
                moveCursor( mUserCursor.x()+text.size(), mUserCursor.y() );
            }
        }
    }
}


void TConsole::replace( QString text )
{
    int x = P_begin.x();
    int o = P_end.x() - P_begin.x();
    int r = text.size();
    
    if( mTriggerEngineMode )
    {
        if( hasSelection() )
        {
            if( r < o )
            {
                int a = -1*(o-r);
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );        
            }
            if( r > o )
            {
                int a = r-o;
                mpHost->getLuaInterpreter()->adjustCaptureGroups( x, a );
            }
        }
        else
        {
            mpHost->getLuaInterpreter()->adjustCaptureGroups( x, r );    
        }
    }

    buffer.replaceInLine( P_begin, P_end, text, mFormatCurrent );
}

void TConsole::skipLine()
{
    if( deleteLine( mUserCursor.y() ) )
    {
        mDeletedLines++;
    }
}

bool TConsole::deleteLine( int y )
{
    return buffer.deleteLine( y );
}

bool TConsole::hasSelection() 
{
    if( mP_start != mP_end )
        return true;
    else
        return false;
}

void TConsole::insertText( QString msg )
{
    insertText( msg, mUserCursor );    
}

void TConsole::insertHTML( QString text )
{
    insertText( text );
}

int TConsole::getLineNumber()
{
    return mUserCursor.y(); 
}

int TConsole::getColumnNumber()
{
    return mUserCursor.x();
}

int TConsole::getLineCount()
{
    return buffer.getLastLineNumber();
}

QStringList TConsole::getLines( int from, int to )
{
    QStringList ret;
    int pos = mUserCursor.y();
    int delta = abs( from - to );
    for( int i=0; i<delta; i++ )
    {
        ret << buffer.line( from + i );
    }
    return ret;
}

int TConsole::getLastLineNumber()
{
    return buffer.getLastLineNumber();
}

void TConsole::moveCursorEnd()
{
    int y = buffer.getLastLineNumber();
    int x = buffer.line( y ).size()-1;
    x = x >= 0 ? x : 0;
    moveCursor( x, y );
}

bool TConsole::moveCursor( int x, int y )
{
    QPoint P( x, y );
    if( buffer.moveCursor( P ) )
    {
        mUserCursor.setX( x );
        mUserCursor.setY( y );
        return true;
    }
    else
    {
        return false;
    }
}


void TConsole::setUserWindow()
{
}

int TConsole::select( QString text, int numOfMatch )
{
    if( mudlet::debugMode ) 
        TDebug() << "\nline under current user cursor: "<<mUserCursor.y()<<"#:" << buffer.line( mUserCursor.y() ) << "\n" >> 0;
    
    int begin = 0;
    for( int i=0;i<numOfMatch; i++ )
    {
        begin = buffer.line( mUserCursor.y() ).indexOf( text, begin );
        
        if( begin == -1 )
        {
            P_begin.setX( 0 );
            P_begin.setY( 0 );
            P_end.setX( 0 );
            P_end.setY( 0 );
            return -1;
        }
    }   
    int end = begin + text.size();
    P_begin.setX( begin );
    P_begin.setY( mUserCursor.y() );
    P_end.setX( end );
    P_end.setY( mUserCursor.y() );
    
    if( mudlet::debugMode ) 
        TDebug()<<"P_begin("<<P_begin.x()<<"/"<<P_begin.y()<<"), P_end("<<P_end.x()<<"/"<<P_end.y()<<") selectedText = " << buffer.line( mUserCursor.y() ).mid(P_begin.x(), P_end.x()-P_begin.x() ) <<"\n" >> 0;
    return begin;
}

bool TConsole::selectSection( int from, int to )
{
    if( mudlet::debugMode )
    {
        if( mudlet::debugMode ) 
            TDebug() <<"\nselectSection("<<from<<","<<to<<"): line under current user cursor: " << buffer.line( mUserCursor.y() ) << "\n" >> 0;
    }
    if( from < 0 ) 
        return false;
    
    if( from >= buffer.buffer[mUserCursor.y()].size() ) 
        return false;
    
 
    P_begin.setX( from );
    P_begin.setY( mUserCursor.y() );
    P_end.setX( from + to );
    P_end.setY( mUserCursor.y() );
    
    if( mudlet::debugMode ) 
        TDebug()<<"P_begin("<<P_begin.x()<<"/"<<P_begin.y()<<"), P_end("<<P_end.x()<<"/"<<P_end.y()<<") selectedText = " << buffer.line( mUserCursor.y() ).mid(P_begin.x(), P_end.x()-P_begin.x() ) <<"\n" >> 0;
    
    return true;
}

void TConsole::setFgColor( int r, int g, int b )
{
    mFormatCurrent.fgColor = QColor( r, g, b );
    buffer.applyFormat( P_begin, P_end, mFormatCurrent );
}

void TConsole::setBgColor( int r, int g, int b )
{
    mFormatCurrent.bgColor = QColor( r, g, b );
    buffer.applyFormat( P_begin, P_end, mFormatCurrent );
}

void TConsole::printCommand( QString & msg )
{
    msg.append("\n");
    print( msg, mCommandFgColor, mCommandBgColor );
}

void TConsole::echo( QString & msg )
{
    QPoint P;
    if( mTriggerEngineMode )
    {
        P.setY( mEngineCursor );
        P.setX( (buffer.line(mEngineCursor)).size()-1 );
        insertText( msg, P );
    }
    else
    {
        print( msg );
    }
}

void TConsole::print( const char * msg )
{
    int lineBeforeNewContent = buffer.getLastLineNumber();
    buffer.append( msg, 
                   mFormatCurrent.fgColor,
                   mFormatCurrent.bgColor, 
                   false, 
                   false,
                   false );
    buffer.wrap( lineBeforeNewContent, mWrapAt, mIndentCount, mFormatCurrent );
    console->showNewLines();
}

void TConsole::printDebug( QString & msg )
{
    QColor fgColor = QColor(0,0,0);
    QColor bgColor = QColor(255,255,255);
    int lineBeforeNewContent = buffer.getLastLineNumber();
    buffer.append( msg, 
                   fgColor,
                   bgColor, 
                   false, 
                   false,
                   false );
    buffer.wrap( lineBeforeNewContent, mWrapAt, mIndentCount, mFormatCurrent );
    console->showNewLines();
}

void TConsole::print( QString & msg )
{
    int lineBeforeNewContent = buffer.getLastLineNumber();
    buffer.append(  msg, 
                    mFormatCurrent.fgColor,
                    mFormatCurrent.bgColor, 
                    false, 
                    false,
                    false );
    buffer.wrap( lineBeforeNewContent, mWrapAt, mIndentCount, mFormatCurrent );
    console->showNewLines();
}

void TConsole::print( QString & msg, QColor & fgColor, QColor & bgColor )
{
    int lineBeforeNewContent = buffer.getLastLineNumber();
    buffer.append(  msg, 
                    fgColor,
                    bgColor, 
                    false, 
                    false,
                    false );
    buffer.wrap( lineBeforeNewContent, mWrapAt, mIndentCount, mFormatCurrent );
    console->showNewLines();
}


void TConsole::printSystemMessage( QString & msg )
{
    QColor bgColor;
    QColor fgColor;
    
    if( mIsDebugConsole ) 
    {
        bgColor = mBgColor;
        fgColor = mFgColor;
    }
    else
    {
        bgColor = mpHost->mBgColor;
    }
    
    int lineBeforeNewContent = buffer.getLastLineNumber();
    buffer.append(  QString("System Message: ")+msg,
                    mSystemMessageFgColor,
                    mSystemMessageBgColor,
                    false, 
                    false,
                    false );
    buffer.wrap( lineBeforeNewContent, mWrapAt, mIndentCount, mFormatSystemMessage );
    console->showNewLines();
}

void TConsole::echoUserWindow( QString & msg )
{
    print( msg );
}

void TConsole::copy()
{
    mClipboard = buffer.copy( P_begin, P_end );    
}

void TConsole::cut()
{
    mClipboard = buffer.cut( P_begin, P_end );
}

void TConsole::paste()
{
    QPoint P = P_begin;
    buffer.paste( P_begin, mClipboard );     //TODO: P_begin & P_end to replace selection
    console->showNewLines();
}

void TConsole::pasteWindow( TBuffer bufferSlice )
{
    mClipboard = bufferSlice;
    paste();
}

void TConsole::slot_user_scrolling( int action )
{
}

void TConsole::slot_stop_all_triggers( bool b )
{
    if( b )
    {
        mpHost->stopAllTriggers();
        QIcon icon2(":/icons/red-bomb.png");
        emergencyStop->setIcon( icon2 );
    }
    else
    {
        mpHost->reenableAllTriggers();
        QIcon icon2(":/icons/edit-bomb.png");
        emergencyStop->setIcon( icon2 );
    }
}







