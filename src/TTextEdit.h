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

#ifndef _T_TEXTEDIT_H
#define _T_TEXTEDIT_H

//#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <iostream>
//#include <strstream>

#include <QMap>
#include <QString>
#include <QScrollBar>
#include "mudlet.h"
#include "TBuffer.h"

class Host;
class TConsole;

class TTextEdit : public QWidget
{
Q_OBJECT
        
public:
    
                      TTextEdit( TConsole *, 
                                 QWidget *, 
                                 TBuffer * pB, 
                                 Host * pH, 
                                 bool isDebugConsole,
                                 bool isSplitScreen );
    
    void              paintEvent( QPaintEvent* );
    void              contextMenuEvent ( QContextMenuEvent * event );
    void              drawForeground(QPainter &, const QRect & );
    void              drawFrame(QPainter &, const QRect & );
    void              drawBackground( QPainter &, const QRect &, const QColor & );
    void              drawCharacters( QPainter & painter,
                                      const QRect & rect,
                                      QString & text,
                                      bool isBold,
                                      bool isUnderline,
                                      bool isItalics,
                                      QColor & fgColor,
                                      QColor & bgColor );
    std::string       getCurrentTime();
    void              showNewLines();
    void              forceUpdate();
    void              needUpdate( int, int );
    void              scrollTo( int );    
    void              scrollUp( int lines );
    void              scrollDown( int lines );
    void              wheelEvent( QWheelEvent * e ); 
    void              resizeEvent( QResizeEvent * event );
    void              mousePressEvent( QMouseEvent *  );
    void              mouseReleaseEvent( QMouseEvent * );
    void              mouseMoveEvent( QMouseEvent * );
    void              showEvent ( QShowEvent * event );
    void              updateScreenView();
    void              highlight();
    void              unHighlight( QRegion & );
    void              swap( QPoint & p1, QPoint & p2 );
    void              focusInEvent( QFocusEvent * event );
    int               imageTopLine();
    int               bufferScrollUp( int lines );
    int               bufferScrollDown( int lines );
    bool              isTailMode();
    void              copySelectionToClipboard();
    void              setConsoleFgColor( int r, int g, int b ){mFgColor = QColor(r,g,b);}
    void              setConsoleBgColor( int r, int g, int b ){mBgColor = QColor(r,g,b);}
    void              setIsMiniConsole(){ mIsMiniConsole = true; }
    bool              mShowTimeStamps;
    int               mScrollVector;
    int               mCursorY;
    bool              mIsTailMode;
    QFont             mDisplayFont;
    int               mFontAscent;
    int               mFontDescent;
    int               mWrapAt;
    int               mWrapIndentCount;

signals:
    
public slots:
    
    void              slot_toggleTimeStamps();
    void              slot_copySelectionToClipboard();
    void              slot_scrollBarMoved( int );



private:
    
    bool              mForceUpdate;
    void              initDefaultSettings();
    bool              mScrollUp;
    QColor            mFgColor;
    QColor            mBgColor;

    QFont             mCommandLineFont;
    QFont             mCommandSeperator;

    bool              mIsDebugConsole;    
    
    int               mLeftMargin;
    int               mTopMargin;
    int               mScreenHeight;
    int               mScreenWidth;
    int               mFontHeight;
    int               mFontWidth;
    Host *            mpHost;
    TBuffer *         mpBuffer;
    TConsole *        mpConsole;


    bool              mHighlight_on;
    bool              mHighlightingBegin;
    bool              mHighlightingEnd;
    bool              mMouseTracking;
    bool              mIsSplitScreen; 
    bool              mInversOn;
    QPoint            mPA;
    QPoint            mPB;
    QRegion           mSelectedRegion;

    bool              mPainterInit;
    QPixmap           mScreenMap;
    QScrollBar *      mpScrollBar;
    int               mOldScrollPos;
    bool              mInit_OK;
    bool              mIsMiniConsole;
};

#endif

