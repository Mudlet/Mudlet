/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn  KoehnHeiko@googlemail.com     *
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
#include <QTime>
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
    void              updateLastLine();
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
    void              copySelectionToClipboardHTML();

    QColor            mBgColor;
    int               mCursorY;
    QFont             mDisplayFont;
    QColor            mFgColor;
    int               mFontAscent;
    int               mFontDescent;
    bool              mIsCommandPopup;
    bool              mIsTailMode;
    QMap<QString, QString> mPopupCommands;
    int               mScrollVector;
    QRegion           mSelectedRegion;
    bool              mShowTimeStamps;
    int               mWrapAt;
    int               mWrapIndentCount;
    qreal             mLetterSpacing;

signals:

public slots:

    void              slot_toggleTimeStamps();
    void              slot_copySelectionToClipboard();
    void              slot_scrollBarMoved( int );
    void              slot_popupMenu();
    void              slot_copySelectionToClipboardHTML();

private:

    void              initDefaultSettings();

    QFont             mCommandLineFont;
    QFont             mCommandSeperator;
    int               mFontHeight;
    int               mFontWidth;
    bool              mForceUpdate;
    bool              mHighlight_on;
    bool              mHighlightingBegin;
    bool              mHighlightingEnd;
    bool              mInit_OK;
    bool              mInversOn;
    bool              mIsDebugConsole;
    bool              mIsMiniConsole;
    bool              mIsSplitScreen;
    int               mLastRenderBottom;
    int               mLeftMargin;
    bool              mMouseTracking;
    int               mOldScrollPos;
    QPoint            mP_aussen;
    QPoint            mPA;
    bool              mPainterInit;
    QPoint            mPB;
    TBuffer *         mpBuffer;
    TConsole *        mpConsole;
    Host *            mpHost;
    QScrollBar *      mpScrollBar;
    int               mScreenHeight;
    QPixmap           mScreenMap;
    int               mScreenWidth;
    bool              mScrollUp;
    int               mTopMargin;
    bool              mUpdateSlice;
    QTime             mLastClickTimer;

};

#endif

