/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
 *                                                                         *
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




#ifndef TCONSOLE_H
#define TCONSOLE_H

//#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <QMainWindow>
#include <QCloseEvent>
//#include "ui_console.h"
//#include <QtWebKit>
#include <iostream>
#include "ctelnet.h"
#include "TCommandLine.h"
#include <QPlainTextEdit>
#include <QTextDocumentFragment>
#include <QPoint>
#include "TBuffer.h"
#include "Host.h"
#include "TLabel.h"

class Host;
class mudlet;
class TTextEdit;
class TBuffer;
class TLabel;
class TSplitter;
class dlgNotepad;


class TFontSpecsLogger
{
public:
    TFontSpecsLogger(){ reset(); }
    QString getFontWeight()
    {
        if(bold)
        {
            return QString("bold");
        }
        else return QString("normal");
    }
    QString getFontStyle(){ return (italics) ? QString("italics") : QString("normal");}
    QString getFontDecoration(){ return (underline) ? QString("underline") : QString("normal");}
    void reset()
    {
        bold = false;
        italics = false;
        underline = false;
        m_bgColorHasChanged = false;
        m_fgColorHasChanged = false;
    }
    void bg_color_change(){ m_bgColorHasChanged=true; }
    void fg_color_change(){ m_fgColorHasChanged=true; }
    QColor fgColor;
    QColor fgColorLight;
    QColor bgColor;
    bool m_bgColorHasChanged;
    bool m_fgColorHasChanged;
    bool bold;
    bool italics;
    bool underline;

};

class TConsole : public QWidget 
{
Q_OBJECT

public:
   
                        TConsole( Host *, bool isDebugConsole, QWidget * parent=0 );

      void              reset();
      void              echoUserWindow( QString & );
      Host *            getHost() { return mpHost; }
      TCommandLine *    mpCommandLine;
      void              replace( QString );
      void              insertHTML( QString );
      void              insertText( QString );
      void              insertText( QString, QPoint );
      void              setLabelStyleSheet( std::string & buf, std::string & sh );
      void              copy();
      void              cut();
      void              paste();
      void              appendBuffer();
      void              appendBuffer( TBuffer );
      int               getButtonState();
      void              closeEvent( QCloseEvent *event );
      void              resizeEvent( QResizeEvent * event );
      void              pasteWindow( TBuffer );
      void              setUserWindow();
      QStringList       getLines( int from, int to );
      int               getLineNumber();
      int               getLineCount();
      bool              deleteLine( int );
      std::list<int>    getFgColor( std::string & buf );
      std::list<int>    getBgColor( std::string & buf );
      void              luaWrapLine( std::string & buf, int line );

      int               getColumnNumber();
      void              loadRawFile( std::string );
      void              setWrapAt( int pos ){ mWrapAt = pos; buffer.setWrapAt( pos ); }
      void              setIndentCount( int count ){ mIndentCount = count; buffer.setWrapIndent( count ); }
      void              echo( QString & );
      bool              moveCursor( int x, int y );
      int               select( QString, int numOfMatch = 1 );
      void              deselect();
      bool              selectSection( int, int );
      void              skipLine();
      void              setFgColor( int, int, int );
      void              setBgColor( int, int, int );
      void              changeColors();
      void              scrollDown( int lines );
      void              scrollUp( int lines );
      void              print( QString &, int, int, int, int, int, int );
      void              print( QString & msg );
      void              print( const char * );
      void              printDebug( QColor &, QColor &, QString & );
      void              printSystemMessage( QString & msg );
      void              printOnDisplay( std::string & );
      void              printCommand( QString & );
      bool              hasSelection();
      void              moveCursorEnd();
      int               getLastLineNumber();
      void              refresh();
      TLabel *          createLabel( QString & name, int x, int y, int width, int height, bool fillBackground );
      TConsole *        createMiniConsole( QString & name, int x, int y, int width, int height );
      bool              createButton( QString & name, int x, int y, int width, int height, bool fillBackground );
      bool              showWindow( QString & name );
      bool              hideWindow( QString & name );
      bool              printWindow( QString & name, QString & text );
      bool              setBackgroundImage( QString & name, QString & path );
      bool              setBackgroundColor( QString & name, int r, int g, int b, int alpha );
      QString           getCurrentLine( std::string & );
      void              selectCurrentLine( std::string & );
      bool              setMiniConsoleFontSize( std::string &, int );
      void              setBold( bool );
      void              setItalics( bool );
      void              setUnderline( bool );
      void              finalize();
      void              runTriggers( int );
      void              showStatistics();
      void              showEvent( QShowEvent * event );
      void              hideEvent( QHideEvent * event );
      QString           mConsoleName;
      bool              mWindowIsHidden;
      int               mOldX;
      int               mOldY;
      TTextEdit *       console;
      TTextEdit *       console2;
      Host *            mpHost; 
      TBuffer           buffer;
      TBuffer           mClipboard;
      QScrollBar *      mpScrollBar;
      QWidget *         layerEdit;
      QWidget *         layer;
      QWidget *         layerCommandLine;  
      //QPushButton *     emergencyStop;
      QToolButton *     emergencyStop;
      QLineEdit *       networkLatency;
      QWidget *         mpBaseVFrame;
      QWidget *         mpBaseHFrame;
      QWidget *         mpMainFrame;
      QWidget *         mpTopToolBar;
      QWidget *         mpLeftToolBar;
      QWidget *         mpRightToolBar;
      TChar             mFormatCurrent;
      void echoConsole( QString & msg );
      void              setConsoleBgColor( int, int, int );
      void              setConsoleFgColor( int, int, int );
      QList<TConsole *> mSubConsoleList;
      std::map<std::string, TConsole *> mSubConsoleMap;
      std::map<std::string, TLabel *> mLabelMap;
      //QMap<QString, TButton *> mButtonMap;
      int               mButtonState;
      TSplitter *       splitter;
      QFile             mLogFile;
      QFile             mReplayFile;
      QTextStream       mLogStream;
      QDataStream       mReplayStream;
      bool              mLogToLogFile;
      bool              mRecordReplay;
      QString           mLogFileName;
      dlgNotepad *      mpNotePad;


//private:

      std::list<int>    _getFgColor();
      std::list<int>    _getBgColor();
      void              _luaWrapLine( int );
      QTime             mProcessingTime;
      QString           getCurrentLine();
      void              selectCurrentLine();
      QString           profile_name; 
      TChar             mStandardFormat;
      //std::string       getCurrentTime();
      //void              translateToPlainText( QString & );
     
      QString           logger_translate( QString & );
      void              logger_set_text_properties( QString );
      //void              set_text_properties( int formatPropertyCode );
      QString           assemble_html_font_specs();
      QString           mCurrentLine;
      QPoint            mUserCursor;
      QColor            mCommandFgColor;
      QColor            mCommandBgColor;
      QColor            mSystemMessageFgColor;
      QColor            mSystemMessageBgColor;
      bool              mWaitingForHighColorCode;
      bool              mHighColorModeForeground;
      bool              mHighColorModeBackground;
      bool              mIsHighColorMode;
      bool              isUserScrollBack;
      //TFontSpecs        m_fontSpecs;
      TFontSpecsLogger  m_LoggerfontSpecs;
      int               currentFgColorProperty;
      QString           mFormatSequenceRest;
      QFont             mDisplayFont;
      QColor            mFgColor;
      QColor            mBgColor;
      bool              mIsDebugConsole;
      int               mWrapAt;
      int               mIndentCount;
      bool              mTriggerEngineMode;
      static const QString     cmLuaLineVariable;
      QPoint            P_begin;
      QPoint            P_end;

      TChar             mFormatBasic;
      TChar             mFormatSystemMessage;

      int               mDeletedLines;
      int               mEngineCursor;

      QWidget *         mpMainDisplay;
      int               mMainFrameTopHeight;
      int               mMainFrameBottomHeight;
      int               mMainFrameLeftWidth;
      int               mMainFrameRightWidth;
      bool              mIsSubConsole;

signals:
    
    
public slots:    
      
      void              slot_toggleReplayRecording();
      void              slot_stop_all_triggers( bool );
      void              slot_toggleLogging();
      void              slot_user_scrolling( int );
      
};

#endif

