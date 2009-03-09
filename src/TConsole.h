/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn (KoehnHeiko@googlemail.com)         *
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
#include "ui_console.h"
#include <QtWebKit>
#include <iostream>
#include "ctelnet.h"
#include "TCommandLine.h"
#include <QPlainTextEdit>
#include <QTextDocumentFragment>
#include <QPoint>
#include "TBuffer.h"

class Host;
class mudlet;
class TTextEdit;
class TBuffer;

class TFontSpecs 
{
public:
    TFontSpecs( Host * pH ){ mpHost = pH; init(); }
    QString getFontWeight()
    { 
        if(bold)
        { 
            return QString("<b>"); 
        }
        else return QString("</b>"); 
    }
    QString getFontStyle(){ return (italics) ? QString("<i>") : QString("</i>");}
    QString getFontDecoration(){ return (underline) ? QString("<u>") : QString("</u>");}
    void init()
    { 
        bold = false;
        italics = false;
        underline = false;
        m_reset = false;
        m_bgColorHasChanged = false;
        m_fgColorHasChanged = false;
        m_bgColorHasChanged_old = false;
        m_fgColorHasChanged_old = false;
        bold_old = false;
        italics_old = false;
        underline_old = false;
        fgColor = mpHost->mFgColor;
        fgColorLight = mpHost->mLightBlue;
        bgColor = mpHost->mBgColor;
    }
    void reset()
    {
        init();
    }
    void bg_color_change(){ m_bgColorHasChanged=true; }
    void fg_color_change(){ m_fgColorHasChanged=true; }
    QColor fgColor;
    QColor fgColorLight;
    QColor fgColor_old;
    QColor bgColor;
    QColor bgColor_old;
    bool m_bgColorHasChanged;
    bool m_bgColorHasChanged_old;
    bool m_fgColorHasChanged;
    bool m_fgColorHasChanged_old;
    bool bold;
    bool bold_old;
    bool italics;
    bool italics_old;
    bool underline;
    bool underline_old;
    bool m_reset;
    QString text;
    Host * mpHost;
};

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
   
                        TConsole( Host *, bool isDebugConsole );
      
      void              echoUserWindow( QString & );
      Host *            getHost() { return mpHost; }
      TCommandLine *    mpCommandLine;
      void              replace( QString );
      void              insertHTML( QString );
      void              insertText( QString );
      void              insertText( QString, QPoint );
      void              copy();
      void              cut();
      void              paste();
      void              closeEvent( QCloseEvent *event );
      void              pasteWindow( TBuffer );
      void              setUserWindow();
      QStringList       getLines( int from, int to );
      int               getLineNumber();
      int               getLineCount();
      bool              deleteLine( int );
      int               getColumnNumber();
      void              setWrapAt( int pos ){ mWrapAt = pos; buffer.setWrapAt( pos ); }
      void              setIndentCount( int count ){ mIndentCount = count; buffer.setWrapIndent( count ); }
      void              echo( QString & );
      bool              moveCursor( int x, int y );
      int               select( QString, int numOfMatch = 1 );
      bool              selectSection( int, int );
      void              skipLine();
      void              setFgColor( int, int, int );
      void              setBgColor( int, int, int );
      void              changeColors();
      void              scrollDown( int lines );
      void              scrollUp( int lines );
      void              print( QString &, QColor &, QColor & );
      void              print( QString & msg );
      void              print( const char * );
      void              printDebug( QString & );
      void              printSystemMessage( QString & msg );
      void              printOnDisplay(QString  &);
      void              printCommand( QString & );
      bool              hasSelection();
      
      TTextEdit *       console;
      TTextEdit *       console2;
      Host *            mpHost; 
      TBuffer           buffer;
      TBuffer           mClipboard;
      QScrollBar *      mpScrollBar;
      QWidget *         layerEdit;
      QWidget *         layer;
      QWidget *         layerCommandLine;  
      QPushButton *     emergencyStop;
    
private:
      QString           profile_name; 
      TChar             mStandardFormat;
      //std::string       getCurrentTime();
      void              translateToPlainText( QString & );
      QString           translate( QString & );
      QString           logger_translate( QString & );
      void              logger_set_text_properties( QString );
      void              set_text_properties( int formatPropertyCode );  
      QString           assemble_html_font_specs();
      QString           mCurrentLine;
      QPoint            mP_start;
      QPoint            mP_end;
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
      TFontSpecs        m_fontSpecs;
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
      TChar             mFormatCurrent;
      TChar             mFormatBasic;
      TChar             mFormatSystemMessage;
      QFile             mLogFile;
      QTextStream       mLogStream;
      bool              mLogToLogFile;
      QString           mLogFileName;
      

signals:
    
    
public slots:    
      
      void              slot_toggleLogging();
      void              slot_user_scrolling( int );
      
};

#endif

