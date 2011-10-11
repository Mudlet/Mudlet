/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
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
      void              resetMainConsole();
      void              echoUserWindow( QString & );
      Host *            getHost() { return mpHost; }
      TCommandLine *    mpCommandLine;
      void              replace( QString );
      void              insertHTML( QString );
      void              insertText( QString );
      void              insertText( QString, QPoint );
      void              insertLink( QString, QStringList &, QStringList &, QPoint, bool customFormat=false );
      void              insertLink( QString, QStringList &, QStringList &, bool customFormat=false );
      void              echoLink( QString & text, QStringList & func, QStringList & hint, bool customFormat=false );
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
      void              createMapper( int, int, int, int );
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
      TConsole *        createBuffer( QString & name );
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
      void              setLink( QString & linkText, QStringList & linkFunction, QStringList & linkHint );
      void              setItalics( bool );
      void              setUnderline( bool );
      void              finalize();
      void              runTriggers( int );
      void              showStatistics();
      void              showEvent( QShowEvent * event );
      void              hideEvent( QHideEvent * event );
      void              echoConsole( QString & msg );
      void              setConsoleBgColor( int, int, int );
      void              setConsoleFgColor( int, int, int );
      std::list<int>    _getFgColor();
      std::list<int>    _getBgColor();
      void              _luaWrapLine( int );
      QString           getCurrentLine();
      void              selectCurrentLine();
      bool              saveMap(QString location);
      bool              loadMap(QString location);
      QString           logger_translate( QString & );
      void              logger_set_text_properties( QString );
      QString           assemble_html_font_specs();

      Host *            mpHost;

      TBuffer           buffer;
      static const QString     cmLuaLineVariable;
      TTextEdit *       console;
      TTextEdit *       console2;
      int               currentFgColorProperty;
      QToolButton *     emergencyStop;
      bool              isUserScrollBack;
      QWidget *         layer;
      QWidget *         layerCommandLine;
      QWidget *         layerEdit;
      TFontSpecsLogger  m_LoggerfontSpecs;
      QColor            mBgColor;
      int               mButtonState;
      TBuffer           mClipboard;
      QColor            mCommandBgColor;
      QColor            mCommandFgColor;

      QString           mConsoleName;
      QString           mCurrentLine;
      int               mDeletedLines;
      QFont             mDisplayFont;
      int               mEngineCursor;
      QColor            mFgColor;
      TChar             mFormatBasic;
      TChar             mFormatSystemMessage;

      int               mIndentCount;
      bool              mIsDebugConsole;
      bool              mIsHighColorMode;
      bool              mIsSubConsole;
      std::map<std::string, TLabel *> mLabelMap;
      QFile             mLogFile;
      QString           mLogFileName;
      QTextStream       mLogStream;
      bool              mLogToLogFile;
      int               mMainFrameBottomHeight;
      int               mMainFrameLeftWidth;
      int               mMainFrameRightWidth;
      int               mMainFrameTopHeight;
      int               mOldX;
      int               mOldY;


      TChar             mFormatCurrent;
      QString           mFormatSequenceRest;
      bool              mHighColorModeBackground;
      bool              mHighColorModeForeground;



      QWidget *         mpBaseVFrame;
      QWidget *         mpTopToolBar;
      QWidget *         mpBaseHFrame;
      QWidget *         mpLeftToolBar;
      QWidget *         mpMainFrame;
      QWidget *         mpRightToolBar;
      QWidget *         mpMainDisplay;

      dlgMapper *       mpMapper;
      dlgNotepad *      mpNotePad;

      QScrollBar *      mpScrollBar;




      QTime             mProcessingTime;
      bool              mRecordReplay;
      QFile             mReplayFile;
      QDataStream       mReplayStream;
      TChar             mStandardFormat;
      QList<TConsole *> mSubConsoleList;
      std::map<std::string, TConsole *> mSubConsoleMap;

      QColor            mSystemMessageBgColor;
      QColor            mSystemMessageFgColor;
      bool              mTriggerEngineMode;
      bool              mUserConsole;
      QPoint            mUserCursor;
      bool              mWaitingForHighColorCode;
      bool              mWindowIsHidden;
      int               mWrapAt;
      QLineEdit *       networkLatency;
      QPoint            P_begin;
      QPoint            P_end;
      QString           profile_name;
      TSplitter *       splitter;
      int               mLastBufferLogLine;
      bool              mIsPromptLine;
      QToolButton *     logButton;
      bool              mUserAgreedToCloseConsole;
      QLineEdit *       mpBufferSearchBox;
      QToolButton *     mpBufferSearchUp;
      QToolButton *     mpBufferSearchDown;
      int               mCurrentSearchResult;
      QList<int>        mSearchResults;
      QString           mSearchQuery;

signals:


public slots:

      void              slot_searchBufferUp();
      void              slot_searchBufferDown();
      void              slot_toggleReplayRecording();
      void              slot_stop_all_triggers( bool );
      void              slot_toggleLogging();

};

#endif

