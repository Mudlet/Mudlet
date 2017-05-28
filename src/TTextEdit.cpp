/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
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


#include "TTextEdit.h"


#include "Host.h"
#include "TConsole.h"
#include "TEvent.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QPainter>
#include <QScrollBar>
#include <QString>
#include <QToolTip>
#include "post_guard.h"


TTextEdit::TTextEdit( TConsole * pC, QWidget * pW, TBuffer * pB, Host * pH, bool isDebugConsole, bool isSplitScreen )
: QWidget( pW )
, mCursorY( 0 )
, mIsCommandPopup( false )
, mIsTailMode( true )
, mShowTimeStamps( isDebugConsole )
, mForceUpdate( false )
, mHighlight_on( false )
, mHighlightingBegin( false )
, mHighlightingEnd( false )
, mInit_OK( false )
, mInversOn( false )
, mIsDebugConsole( isDebugConsole )
, mIsMiniConsole( false )
, mIsSplitScreen( isSplitScreen )
, mLastRenderBottom( 0 )
, mMouseTracking( false )
, mPainterInit( false )
, mpBuffer( pB )
, mpConsole( pC )
, mpHost( pH )
, mpScrollBar( 0 )
{
    mLastClickTimer.start();
    if( ! mIsDebugConsole )
    {
        mFontHeight = QFontMetrics( mpHost->mDisplayFont ).height();
        mFontWidth = QFontMetrics( mpHost->mDisplayFont ).width( QChar('W') );
        mScreenWidth = 100;
        if( (width()/mFontWidth ) < mScreenWidth )
        {

            mScreenWidth = 100;//width()/mFontWidth;
        }

        mpHost->mDisplayFont.setFixedPitch(true);
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap( mScreenWidth*mFontWidth*2, mFontHeight*2 );
        QPainter p(&pixmap);
        p.setFont(mpHost->mDisplayFont);
        const QRectF r = QRectF(0,0,mScreenWidth*mFontWidth*2,mFontHeight*2);
        QRectF r2;
        const QString t = "1234";
        p.drawText(r,1,t,&r2);
        mLetterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
        mpHost->mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
#endif
        setFont( mpHost->mDisplayFont );
    }
    else
    {
        mIsDebugConsole = true;
        mFontHeight = QFontMetrics( mDisplayFont ).height();
        mFontWidth = QFontMetrics( mDisplayFont ).width( QChar('W') );
        mScreenWidth = 100;
        mDisplayFont.setFixedPitch(true);
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap( mScreenWidth*mFontWidth*2, mFontHeight*2 );
        QPainter p(&pixmap);
        p.setFont(mDisplayFont);
        const QRectF r = QRectF(0,0,mScreenWidth*mFontWidth*2,mFontHeight*2);
        QRectF r2;
        const QString t = "1234";
        p.drawText(r,1,t,&r2);
        mLetterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
        mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
#endif
        setFont( mDisplayFont );
        // initialize after mFontHeight and mFontWidth have been set, because the function uses them!
        initDefaultSettings();
    }
    mScreenHeight = height() / mFontHeight;

    mScreenWidth = 100;

    setMouseTracking( true );
    setFocusPolicy( Qt::NoFocus );
    QCursor cursor;
    cursor.setShape(Qt::IBeamCursor);
    setCursor( cursor );
    setAttribute( Qt::WA_OpaquePaintEvent );//was disabled
    setAttribute( Qt::WA_DeleteOnClose );

    QPalette palette;
    palette.setColor( QPalette::Text, mFgColor );
    palette.setColor( QPalette::Highlight, QColor(55,55,255) );
    palette.setColor( QPalette::Base, mBgColor );
    setPalette(palette);
    showNewLines();
    setMouseTracking( true ); // test fix for MAC
    setEnabled( true ); //test fix for MAC
}

void TTextEdit::forceUpdate()
{
    mForceUpdate = true;
    update();
}

void TTextEdit::needUpdate( int y1, int y2 )
{
    if( ! mIsTailMode )
    {
        return;
    }

    if( mScreenHeight == 0 ) return;
    int top = imageTopLine();
    int bottom = y2-y1;

    if( top > 0 )
    {
        top = (y1 - top) % mScreenHeight;
    }
    else
    {
        top = y1 % mScreenHeight;
    }
    QRect r( 0, top*mFontHeight, mScreenWidth*mFontWidth, bottom*mFontHeight );
    mForceUpdate = true;
    update( r );
}

void TTextEdit::focusInEvent ( QFocusEvent * event )
{
    update();
    QWidget::focusInEvent( event );
}


void TTextEdit::slot_toggleTimeStamps()
{
    mShowTimeStamps = !mShowTimeStamps;
    forceUpdate();
    update();
}

void TTextEdit::slot_scrollBarMoved( int line )
{
    if( mpConsole->mpScrollBar )
    {
        disconnect( mpConsole->mpScrollBar, SIGNAL(valueChanged(int)), this, SLOT(slot_scrollBarMoved(int)));
        mpConsole->mpScrollBar->setRange( 0, mpBuffer->getLastLineNumber() );
        mpConsole->mpScrollBar->setSingleStep( 1 );
        mpConsole->mpScrollBar->setPageStep( mScreenHeight );
        mpConsole->mpScrollBar->setValue( line );
        scrollTo( line );
        connect( mpConsole->mpScrollBar, SIGNAL(valueChanged(int)), this, SLOT(slot_scrollBarMoved(int)));
    }
}

void TTextEdit::initDefaultSettings()
{
    mFgColor = QColor(192,192,192);
    mBgColor = QColor(Qt::black);
    mDisplayFont = QFont("Bitstream Vera Sans Mono", 10, QFont::Normal);
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        int width = mScreenWidth*mFontWidth*2;
        int height = mFontHeight*2;
        // sometimes mScreenWidth is 0, and QPainter doesn't like dimensions of 0x#. Need to work out why is
        // mScreenWidth ever zero and it gets used in the follow calculations.
        if ( width > 0 && height > 0 ) {
            QPixmap pixmap = QPixmap( width, height );
            QPainter p(&pixmap);
            p.setFont(mDisplayFont);
            const QRectF r = QRectF( 0,0,width,height );
            QRectF r2;
            const QString t = "1234";
            p.drawText(r,1,t,&r2);
            mLetterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
            mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
        }
#endif
    mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
    mDisplayFont.setFixedPitch(true);
    setFont( mDisplayFont );
    mCommandLineFont = QFont("Bitstream Vera Sans Mono", 10, QFont::Normal);
    mCommandSeperator = QString(";");
    mWrapAt = 100;
    mWrapIndentCount = 5;
}

void TTextEdit::updateScreenView()
{
    if( isHidden() )
    {
        mFontWidth = QFontMetrics( mDisplayFont ).width( QChar(' ') );
        mFontDescent = QFontMetrics( mDisplayFont ).descent();
        mFontAscent = QFontMetrics( mDisplayFont ).ascent();
        mFontHeight = mFontAscent + mFontDescent;
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap( 2000,600 );
        QPainter p(&pixmap);
        mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
        if( ! p.isActive() ) return;
        p.setFont(mDisplayFont);
        const QRectF r = QRectF(0,0,2000,600);
        QRectF r2;
        const QString t = "1234";
        p.drawText(r,1,t,&r2);
        mLetterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
        mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
#endif
        return; //NOTE: das ist wichtig, damit ich keine floating point exception bekomme, wenn mScreenHeight==0, was hier der Fall wäre
    }
    if( ! mIsDebugConsole && ! mIsMiniConsole )
    {
        mFontWidth = QFontMetrics( mpHost->mDisplayFont ).width( QChar('W') );
        mFontDescent = QFontMetrics( mpHost->mDisplayFont ).descent();
        mFontAscent = QFontMetrics( mpHost->mDisplayFont ).ascent();
        mFontHeight = mFontAscent + mFontDescent;
        mBgColor = mpHost->mBgColor;
        mFgColor = mpHost->mFgColor;
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        QPixmap pixmap = QPixmap( mScreenWidth*mFontWidth*2, mFontHeight*2 );
        QPainter p(&pixmap);
        mpHost->mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
        if( p.isActive() )
        {
            p.setFont(mpHost->mDisplayFont);
            const QRectF r = QRectF(0,0,mScreenWidth*mFontWidth*2,mFontHeight*2);
            QRectF r2;
            const QString t = "1234";
            p.drawText(r,1,t,&r2);
            mLetterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
            mpHost->mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
        }
#endif
    }
    else
    {
        mFontWidth = QFontMetrics( mDisplayFont ).width( QChar('W') );
        mFontDescent = QFontMetrics( mDisplayFont ).descent();
        mFontAscent = QFontMetrics( mDisplayFont ).ascent();
        mFontHeight = mFontAscent + mFontDescent;
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        int width = mScreenWidth*mFontWidth*2;
        int height = mFontHeight*2;
        // sometimes mScreenWidth is 0, and QPainter doesn't like dimensions of 0x#. Need to work out why is
        // mScreenWidth ever zero and it gets used in the follow calculations.
        if ( width > 0 && height > 0 ) {
            QPixmap pixmap = QPixmap( width, height );
            QPainter p(&pixmap);
            mDisplayFont.setLetterSpacing(QFont::AbsoluteSpacing, 0);
            if( p.isActive() )
            {
                p.setFont(mDisplayFont);
                const QRectF r = QRectF( 0,0,width,height );
                QRectF r2;
                const QString t = "1234";
                p.drawText(r,1,t,&r2);
                mLetterSpacing = (qreal)((qreal)mFontWidth-(qreal)(r2.width()/t.size()));
                mDisplayFont.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
            }
        }
#endif
    }
    mScreenHeight = visibleRegion().boundingRect().height()/mFontHeight;
    int currentScreenWidth = visibleRegion().boundingRect().width() / mFontWidth;
    if( ! mIsDebugConsole && ! mIsMiniConsole )
    {
        if( mpHost->mScreenWidth > currentScreenWidth )
        {
            if( currentScreenWidth < 100 )
            {
                mScreenWidth = 100;
            }
            else
            {
                mScreenWidth = currentScreenWidth;
                mpHost->mScreenWidth = mScreenWidth;
            }
        }
        else
        {
            mpHost->mScreenWidth = currentScreenWidth;
            mScreenWidth = currentScreenWidth;
        }

        mpHost->mScreenHeight = mScreenHeight;
    }
    else
    {
        mScreenWidth = currentScreenWidth;
    }
}

void TTextEdit::showNewLines()
{
    if( ! mIsSplitScreen )
    {
        if( ! isTailMode() )
        {
            return;
        }
    }
    else
    {
        if( isHidden() )
        {
            return;
        }
    }

    mCursorY = mpBuffer->size();
    if( ! mIsSplitScreen )
    {
        mpBuffer->mCursorY = mpBuffer->size();
    }
    if( mCursorY > mScreenHeight )
    {
        mScrollUp = true;
    }
    mOldScrollPos = mpBuffer->getLastLineNumber();
    if( ! mIsSplitScreen )
    {
        if( mpConsole->mpScrollBar && mOldScrollPos > 0 )
        {
            disconnect( mpConsole->mpScrollBar, SIGNAL(valueChanged(int)), mpConsole->console, SLOT(slot_scrollBarMoved(int)));
            mpConsole->mpScrollBar->setRange( 0, mpBuffer->getLastLineNumber() );
            mpConsole->mpScrollBar->setSingleStep( 1 );
            mpConsole->mpScrollBar->setPageStep( mScreenHeight );
            if( mpConsole->console->isTailMode() )
            {
                mpConsole->mpScrollBar->setValue( mpBuffer->mCursorY );
            }
            connect( mpConsole->mpScrollBar, SIGNAL(valueChanged(int)), mpConsole->console, SLOT(slot_scrollBarMoved(int)));
        }
    }
    update();
}

void TTextEdit::scrollTo( int line )
{
    if( (line > -1) && (line < mpBuffer->size()) )
    {
        if( (line < (mpBuffer->getLastLineNumber()-mScreenHeight) && mIsTailMode ) )
        {

            mpConsole->console2->mCursorY = mpBuffer->size();
            mpConsole->console2->mIsTailMode = true;
            mIsTailMode = false;
            mpConsole->console2->show();
            mpConsole->console2->forceUpdate();
        }
        else if( (line > (mpBuffer->getLastLineNumber()-mScreenHeight)) && !mIsTailMode )
        {
            mpConsole->console2->mCursorY = mpConsole->buffer.getLastLineNumber();
            mpConsole->console2->hide();
            mpConsole->console->mCursorY = mpConsole->buffer.getLastLineNumber();
            mpConsole->console->mIsTailMode = true;
            mpConsole->console->updateScreenView();
            mpConsole->console->forceUpdate();
        }
        mpBuffer->mCursorY = line;

        mScrollVector = 0;
        update();
    }
}

void TTextEdit::scrollUp( int lines )
{
    if( mIsSplitScreen )
        return;
    lines = bufferScrollUp( lines );
    if( lines == 0 )
        return;
    else
    {
        mIsTailMode = false;
        lines = mScreenHeight;
        mScrollVector = 0;
        update();
    }
}

void TTextEdit::scrollDown( int lines )
{
    if( mIsSplitScreen )
        return;
    lines = bufferScrollDown( lines );
    if( lines == 0 )
        return;
    else
    {
        mScrollVector = 0;
        update();
    }
}

inline void TTextEdit::drawBackground( QPainter & painter,
                                const QRect & rect,
                                const QColor & bgColor )
{
    QRect bR = rect;
    painter.fillRect( bR.x(), bR.y(), bR.width(), bR.height(), bgColor );
}

inline void TTextEdit::drawCharacters( QPainter & painter,
                                       const QRect & rect,
                                       QString & text,
                                       bool isBold,
                                       bool isUnderline,
                                       bool isItalics,
                                       bool isStrikeOut,
                                       QColor & fgColor,
                                       QColor & bgColor )
{
    if( ( painter.font().bold() != isBold )
     || ( painter.font().underline() != isUnderline )
     || ( painter.font().italic() != isItalics )
     || ( painter.font().strikeOut() != isStrikeOut) )
    {
        QFont font = painter.font();
        font.setBold( isBold );
        font.setUnderline( isUnderline );
        font.setItalic( isItalics );
        font.setStrikeOut( isStrikeOut );
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        font.setLetterSpacing(QFont::AbsoluteSpacing, mLetterSpacing);
#endif
        painter.setFont( font );
    }
    if( painter.pen().color() != fgColor )
    {
        painter.setPen( fgColor );
    }
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    QPointF _p(rect.x(), rect.bottom()-mFontDescent);
    painter.drawText( _p, text );
#else
    painter.drawText( rect.x(), rect.bottom()-mFontDescent, text );
#endif
}


void TTextEdit::drawFrame( QPainter & p, const QRect & rect )
{
    QPoint P_topLeft  = rect.topLeft();
    QPoint P_bottomRight = rect.bottomRight();
    int x_topLeft = P_topLeft.x();
    int x_bottomRight = P_bottomRight.x();

    if( x_bottomRight > mScreenWidth * mFontWidth ) x_bottomRight = mScreenWidth * mFontWidth;

    int x1 = x_topLeft / mFontWidth;
    int x2 = x_bottomRight / mFontWidth;

    int lineOffset = imageTopLine();
    bool invers = false;

    if( mHighlight_on && mInversOn )
    {
        invers = true;
    }

    int from = 0;
    for( int i=from; i<mScreenHeight; i++ )
    {
        if( static_cast<int>(mpBuffer->buffer.size()) <= i+lineOffset )
        {
            break;
        }
        int timeOffset = 0;
        if( mShowTimeStamps )
        {
            if( mpBuffer->timeBuffer.size() > i+lineOffset )
            {
                timeOffset = mpBuffer->timeBuffer[i+lineOffset].size()-1;
            }
        }
        int lineLength = mpBuffer->buffer[i+lineOffset].size() + timeOffset;
        for( int i2=x1; i2<lineLength; )
        {
            QString text;
            if( i2 < timeOffset )
            {
                text = mpBuffer->timeBuffer[i+lineOffset];
                bool isBold = false;
                bool isUnderline = false;
                bool isItalics = false;
                bool isStrikeOut = false;
                QRect textRect = QRect( mFontWidth * i2,
                                        mFontHeight * i,
                                        mFontWidth * timeOffset,
                                        mFontHeight );
                auto bgTime = QColor(22,22,22);
                auto fgTime = QColor(200,150,0);
                drawBackground( p, textRect, bgTime );
                drawCharacters( p, textRect, text, isBold, isUnderline, isItalics, isStrikeOut, fgTime, bgTime );
                i2+=timeOffset;
            }
            else
            {
                if( i2 >= x2 )
                {
                    break;
                }
                text = mpBuffer->lineBuffer[i+lineOffset].at(i2-timeOffset);
                TChar & f = mpBuffer->buffer[i+lineOffset][i2-timeOffset];
                int delta = 1;
                auto fgColor = QColor(f.fgR, f.fgG, f.fgB );
                auto bgColor = QColor(f.bgR, f.bgG, f.bgB );
                while( i2+delta+timeOffset < lineLength )
                {
                    if( mpBuffer->buffer[i+lineOffset][i2+delta-timeOffset] == f )
                    {
                        text.append( mpBuffer->lineBuffer[i+lineOffset].at(i2+delta-timeOffset) );
                        delta++;
                    }
                    else
                    {
                        break;
                    }

                }
                if( invers || ( bgColor != mBgColor ) )
                {
                    QRect textRect = QRect( mFontWidth * i2,
                                        mFontHeight * i,
                                        mFontWidth * delta,
                                        mFontHeight );
                    if( invers )
                        drawBackground( p, textRect, fgColor );
                    else
                        drawBackground( p, textRect, bgColor );
                }
                if( ( p.font().bold() != static_cast<bool>(f.flags & TCHAR_BOLD) )
                 || ( p.font().underline() != static_cast<bool>(f.flags & TCHAR_UNDERLINE) )
                 || ( p.font().italic() != static_cast<bool>(f.flags & TCHAR_ITALICS) )
                 || ( p.font().strikeOut() != static_cast<bool>(f.flags & TCHAR_STRIKEOUT) ) )
                {
                    QFont font = p.font();
                    font.setBold( f.flags & TCHAR_BOLD );
                    font.setUnderline( f.flags & TCHAR_UNDERLINE );
                    font.setItalic( f.flags & TCHAR_ITALICS );
                    font.setStrikeOut( f.flags & TCHAR_STRIKEOUT );
                    font.setLetterSpacing( QFont::AbsoluteSpacing, mLetterSpacing );
                    p.setFont( font );
                }
                if( ( p.pen().color() != fgColor ) || ( invers ) )
                {
                    if( invers )
                        p.setPen( bgColor );
                    else
                        p.setPen( fgColor );
                }
                p.drawText( mFontWidth*i2,(mFontHeight*i)-mFontDescent, text );
                i2+=delta;
            }
        }
    }
    mScrollVector = 0;
}

void TTextEdit::updateLastLine()
{
    qDebug()<<"--->ACHTUNG: error: updateLastLine() called";
    QRect r( 0, (mScreenHeight-1)*mFontHeight, mScreenWidth*mFontWidth, mScreenHeight*mFontHeight );
    mForceUpdate = true;
    update( r );
}


void TTextEdit::drawForeground( QPainter & painter, const QRect & r )
{
    QPixmap screenPixmap;
    QPixmap pixmap = QPixmap( mScreenWidth*mFontWidth, mScreenHeight*mFontHeight );
    pixmap.fill( palette().base().color() );

    QPainter p( &pixmap );
    p.setCompositionMode( QPainter::CompositionMode_Source );
    if( ! mIsDebugConsole && ! mIsMiniConsole )
    {
        p.setFont( mpHost->mDisplayFont );
        p.setRenderHint( QPainter::TextAntialiasing, !mpHost->mNoAntiAlias );
    }
    else
    {
        p.setFont( mDisplayFont );
        p.setRenderHint( QPainter::TextAntialiasing, false );
    }

    QPoint P_topLeft  = r.topLeft();
    QPoint P_bottomRight = r.bottomRight();
    int x_topLeft = 0;
    int y_topLeft = P_topLeft.y();
    int x_bottomRight = P_bottomRight.x();
    int y_bottomRight = P_bottomRight.y();

    if( x_bottomRight > mScreenWidth * mFontWidth )
    {
        x_bottomRight = mScreenWidth * mFontWidth;
    }

    int x1 = x_topLeft / mFontWidth;
    int y1 = y_topLeft / mFontHeight;
    int x2 = x_bottomRight / mFontWidth;
    int y2 = y_bottomRight / mFontHeight;

    int lineOffset = imageTopLine();
    int from = 0;
    if( lineOffset == 0 )
    {
        mScrollVector = 0;
    }
    else
    {
        mScrollVector = lineOffset - mLastRenderBottom;
    }

    bool noScroll = false;
    bool noCopy = false;
    if( abs( mScrollVector ) > mScreenHeight || mForceUpdate || lineOffset < 10 )
    {
        mScrollVector = 0;
        noScroll = true;
    }
    if( ( r.height() < rect().height() ) && ( lineOffset > 0 ) )
    {
        p.drawPixmap( 0, 0, mScreenMap );
        if( ! mForceUpdate && ! mMouseTracking )
        {
            from = y1;
            noScroll = true;
            noCopy = true;
        }
        else
        {
            from = y1;
            y2 = mScreenHeight;
            noScroll = true;
            mScrollVector = 0;
        }
    }
    if( ( ! noScroll ) && ( mScrollVector >= 0 ) && ( mScrollVector <= mScreenHeight ) && ( ! mForceUpdate ) )
    {

        if( mScrollVector*mFontHeight < mScreenMap.height()
            && mScreenWidth*mFontWidth <= mScreenMap.width()
            && (mScreenHeight-mScrollVector)*mFontHeight > 0
            && (mScreenHeight-mScrollVector)*mFontHeight <= mScreenMap.height() )
        {
            screenPixmap = mScreenMap.copy( 0,
                                            mScrollVector*mFontHeight,
                                            mScreenWidth*mFontWidth,
                                            (mScreenHeight-mScrollVector)*mFontHeight );
            p.drawPixmap( 0, 0, screenPixmap );
            from = mScreenHeight - mScrollVector - 1;
        }
    }
    else if( ( ! noScroll ) && ( mScrollVector < 0 && mScrollVector >= ((-1)*mScreenHeight) ) && ( ! mForceUpdate ) )
    {
        if( abs(mScrollVector)*mFontHeight < mScreenMap.height()
            && mScreenWidth*mFontWidth <= mScreenMap.width()
            && (mScreenHeight-abs(mScrollVector))*mFontHeight > 0
            && (mScreenHeight-abs(mScrollVector))*mFontHeight <= mScreenMap.height() )
        {
            screenPixmap = mScreenMap.copy( 0,
                                            0,
                                            mScreenWidth*mFontWidth,
                                            (mScreenHeight-abs(mScrollVector))*mFontHeight );
            p.drawPixmap( 0, abs(mScrollVector)*mFontHeight, screenPixmap );
            from = 0;
            y2 = abs(mScrollVector);
        }
    }
    QRect deleteRect = QRect( 0, from*mFontHeight, x2*mFontHeight, (y2+1)*mFontHeight);
    drawBackground( p, deleteRect, mBgColor );
    for( int i=from; i<=y2; i++ )
    {
        if( static_cast<int>(mpBuffer->buffer.size()) <= i+lineOffset )
        {
            break;
        }
        mpBuffer->dirty[lineOffset+i] = false;
        int timeOffset = 0;
        if( mShowTimeStamps )
        {
            timeOffset = 13;
        }
        int lineLength = mpBuffer->buffer[i+lineOffset].size() + timeOffset;
        for( int i2=x1; i2<lineLength; )
        {
            QString text;
            if( i2 < timeOffset )
            {
                text = mpBuffer->timeBuffer[i+lineOffset];
                bool isBold = false;
                bool isUnderline = false;
                bool isItalics = false;
                bool isStrikeOut = false;
                QRect textRect = QRect( mFontWidth * i2,
                                        mFontHeight * i,
                                        mFontWidth * timeOffset,
                                        mFontHeight );
                auto bgTime = QColor(22,22,22);
                auto fgTime = QColor(200,150,0);
                drawBackground( p, textRect, bgTime );
                drawCharacters( p, textRect, text, isBold, isUnderline, isItalics, isStrikeOut, fgTime, bgTime );
                i2+=timeOffset;
            }
            else
            {
                if( i2 >= x2 )
                {
                    break;
                }
                text = mpBuffer->lineBuffer[i+lineOffset].at(i2-timeOffset);
                TChar & f = mpBuffer->buffer[i+lineOffset][i2-timeOffset];
                int delta = 1;
                QColor fgColor;
                QColor bgColor;
                if( f.flags & TCHAR_INVERSE )
                {
                    bgColor = QColor(f.fgR, f.fgG, f.fgB );
                    fgColor = QColor(f.bgR, f.bgG, f.bgB );
                }
                else
                {
                    fgColor = QColor(f.fgR, f.fgG, f.fgB );
                    bgColor = QColor(f.bgR, f.bgG, f.bgB );
                }
                while( i2+delta+timeOffset < lineLength )
                {
                    if( mpBuffer->buffer[i+lineOffset][i2+delta-timeOffset] == f )
                    {
                        text.append( mpBuffer->lineBuffer[i+lineOffset].at(i2+delta-timeOffset) );
                        delta++;
                    }
                    else
                    {
                        break;
                    }

                }
                QRect textRect;
                textRect = QRect( mFontWidth * i2,
                                  mFontHeight * i,
                                  mFontWidth * delta,
                                  mFontHeight );
                if( f.flags & TCHAR_INVERSE || ( bgColor != mBgColor ) )
                {
                    drawBackground( p, textRect, bgColor );
                }
                drawCharacters( p, textRect, text, f.flags & TCHAR_BOLD, f.flags & TCHAR_UNDERLINE, f.flags & TCHAR_ITALICS, f.flags & TCHAR_STRIKEOUT, fgColor, bgColor );
                i2+=delta;
            }
        }
    }
    p.end();
    painter.setCompositionMode( QPainter::CompositionMode_Source );
    painter.drawPixmap( 0, 0, pixmap );
    if( ! noCopy )
    {
        mScreenMap = pixmap.copy();
    }
    mScrollVector = 0;
    mLastRenderBottom = lineOffset;
    mForceUpdate = false;
}


void TTextEdit::paintEvent( QPaintEvent* e )
{
    const QRect & rect = e->rect();

    if( mFontWidth <= 0 || mFontHeight <= 0 ) return;

    if( mScreenHeight <= 0 || mScreenWidth <= 0 )
    {
        mScreenHeight = height()/mFontHeight;
        mScreenWidth = 100;
        if( mScreenHeight <= 0 || mScreenWidth <= 0 ) return;
    }
    QPainter painter( this );
    if( ! painter.isActive() ) return;

    QRect borderRect = QRect( 0, mScreenHeight*mFontHeight, rect.width(), rect.height() );
    drawBackground( painter, borderRect, mBgColor );
    QRect borderRect2 = QRect( rect.width()-mScreenWidth, 0, rect.width(), rect.height() );
    drawBackground( painter, borderRect2, mBgColor );
    drawForeground( painter, rect );
    mUpdateSlice = false;
}



void TTextEdit::highlight()
{
    QRegion newRegion;
    int lineDelta = abs( mPA.y() - mPB.y() ) - 1;
    if( lineDelta > 0 )
    {

        QRect rectFirstLine( mPA.x()*mFontWidth, (mPA.y()-imageTopLine())*mFontHeight, mScreenWidth*mFontWidth, mFontHeight );
        newRegion += rectFirstLine;

        QRect rectMiddlePart( 0, (mPA.y()+1-imageTopLine())*mFontHeight, mScreenWidth*mFontWidth, lineDelta*mFontHeight );
        newRegion += rectMiddlePart;

        QRect rectLastLine( 0, (mPB.y()-imageTopLine())*mFontHeight, mPB.x()*mFontWidth, mFontHeight );
        newRegion += rectLastLine;
    }

    if( lineDelta == 0 )
    {

        QRect rectFirstLine( mPA.x()*mFontWidth, (mPA.y()-imageTopLine())*mFontHeight, mScreenWidth*mFontWidth, mFontHeight );
        newRegion += rectFirstLine;

        QRect rectLastLine( 0, (mPB.y()-imageTopLine())*mFontHeight, mPB.x()*mFontWidth, mFontHeight );
        newRegion += rectLastLine;
    }

    if( lineDelta < 0 )
    {

        QRect rectFirstLine( mPA.x()*mFontWidth, (mPA.y()-imageTopLine())*mFontHeight, (mPB.x()-mPA.x())*mFontWidth, mFontHeight );
        newRegion += rectFirstLine;
    }

    QRect _r = mSelectedRegion.boundingRect();
    if( lineDelta < 0 )
    {
        _r.setWidth( mScreenWidth*mFontWidth );
    }
    update( _r );

    mSelectedRegion = mSelectedRegion.subtracted( newRegion );

    int y1 = mPA.y();
    for( int y=y1; y<=mPB.y(); y++ )
    {
        int x = 0;
        if( y == y1 )
        {
            x = mPA.x();
        }
        for( ; ; x++ )
        {
            if( (y == mPB.y()) && (x > mPB.x()) )
            {
                break;
            }
            mpBuffer->dirty[y] = true;
            if( x < static_cast<int>(mpBuffer->buffer[y].size()) )
            {
                mpBuffer->buffer[y][x].flags |= TCHAR_INVERSE;
            }
            else
            {
                break;
            }
        }
    }

    update( mSelectedRegion.boundingRect() );
    mSelectedRegion = newRegion;
}

void TTextEdit::unHighlight( QRegion & region )
{
    int y1 = mPA.y();
    if( y1 < 0 )
    {
        return;
    }
    for( int y=y1; y<=mPB.y(); y++ )
    {
        int x = 0;
        if( y == y1 )
        {
            x = mPA.x();
        }
        for( ; ; x++ )
        {
            if( (y == mPB.y()) && (x > mPB.x()) )
            {
                break;
            }
            if( y >= static_cast<int>(mpBuffer->buffer.size()) )
            {
                break;
            }
            mpBuffer->dirty[y] = true;
            if( x < static_cast<int>(mpBuffer->buffer[y].size()) )
            {
                mpBuffer->buffer[y][x].flags &= ~(TCHAR_INVERSE);
                mpBuffer->dirty[y] = true;
            }
            else
            {
                break;
            }
        }
    }
    mForceUpdate = true;
    update();
}

void TTextEdit::swap( QPoint & p1, QPoint & p2 )
{
    QPoint tmp = p1;
    p1 = p2;
    p2 = tmp;
}

void TTextEdit::mouseMoveEvent( QMouseEvent * event )
{
    if( (mFontWidth == 0) | (mFontHeight == 0) ) return;
    int x = event->x() / mFontWidth;// bugfix by BenH (used to be mFontWidth-1)
    if( mShowTimeStamps )
    {
        x -= 13;
    }
    int y = ( event->y() / mFontHeight ) + imageTopLine();
    if( x < 0 ) x = 0;
    if( y < 0 ) y = 0;
    if( y < static_cast<int>(mpBuffer->buffer.size()) )
    {
        if( x < static_cast<int>(mpBuffer->buffer[y].size()) )
        {
            if( mpBuffer->buffer[y][x].link > 0 )
            {
                setCursor( Qt::PointingHandCursor );
                QStringList tooltip = mpBuffer->mHintStore[mpBuffer->buffer[y][x].link];
                QToolTip::showText( event->globalPos(), tooltip.join("\n") );
            }
            else
            {
                setCursor( Qt::IBeamCursor );
                QToolTip::hideText();
            }
        }
    }

    if( ! mMouseTracking ) return;

    if( event->y() < 10 )
    {
        mpConsole->scrollUp( 3 );
    }
    if( event->y() >= height()-10 )
    {
        mpConsole->scrollDown( 3 );
    }
    if( ( y > (int) mpBuffer->size()-1 ) )
    {
        return;
    }

    QPoint PC( x, y );
    
    if( mCtrlSelecting )
    {
        int oldAY = mPA.y();
        int oldBY = mPB.y();
        if( PC.y() == mDragStartY ){
            mPA.setY( PC.y() );
            mPB.setY( PC.y() );
        } else if( PC.y() < mDragStartY ){
            mPA.setY( PC.y() );
            mPB.setY( mDragStartY );
        } else if( PC.y() > mDragStartY ){
            mPA.setY( mDragStartY );
            mPB.setY( PC.y() );
        }
        
        if( oldAY < mPA.y() ){
          for( int y = oldAY; y < mPA.y(); y++ ){
            for(auto & x : mpBuffer->buffer[y]){
              x.flags &= ~(TCHAR_INVERSE);
            }
          }
        }
        if( oldBY > mPB.y() ){
            for( int y = mPB.y()+1; y <= oldBY; y++ ){
                for(auto & x : mpBuffer->buffer[y]){
                    x.flags &= ~(TCHAR_INVERSE);
                }
            }
        }
        
        mPA.setX( 0 );
        mPB.setX( static_cast<int>(mpBuffer->buffer[mPB.y()].size())-1 );

        highlight();
        return;
    }
    if( ( mPA.y() == mPB.y() ) && ( mPA.x() > mPB.x() ) )
    {
        swap( mPA, mPB );
    }
    if( mPA.y() > mPB.y() )
    {
        swap( mPA, mPB );
    }
    QPoint p1 = mPA-PC;
    QPoint p2 = mPB-PC;
    if( p1.manhattanLength() < p2.manhattanLength() )
    {
        if( mPA.y() < PC.y() || ( (mPA.x() < PC.x()) && (mPA.y() == PC.y()) ) )
        {
            int y1 = PC.y();
            for( int y=y1; y>=mPA.y(); y-- )
            {
                if( y >= static_cast<int>(mpBuffer->buffer.size()) || y < 0 ) break;
                int x = mpBuffer->buffer[y].size()-1;
                if( y == y1 )
                {
                    x = PC.x();
                    if( x >= static_cast<int>(mpBuffer->buffer[y].size()) )
                        x = static_cast<int>(mpBuffer->buffer[y].size())-1;
                    if( x < 0 ) x = 0;
                }
                mpBuffer->dirty[y] = true;
                for( ; ; x-- )
                {
                    if( ( y == mPA.y() ) && ( x < mPA.x() ) )
                    {
                        break;
                    }

                    if( x < static_cast<int>(mpBuffer->buffer[y].size()) && x >= 0 )
                    {
                        mpBuffer->buffer[y][x].flags &= ~(TCHAR_INVERSE);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            mP_aussen = mPA;
        }
        else
            mP_aussen = PC;
        mPA = PC;
    }
    else
    {
        if( mPB.y() > PC.y() || (mPB.x() > PC.x() && mPB.y() == PC.y()) )
        {
            int y1 = PC.y();
            for( int y=y1; y<=mPB.y(); y++ )
            {
                int x = 0;
                if( y == y1 )
                {
                    x = PC.x();
                }
                if( y >= static_cast<int>(mpBuffer->buffer.size()) || y < 0 ) break;
                mpBuffer->dirty[y] = true;
                for( ; ; x++ )
                {
                    if( ( y == mPB.y() ) && ( x > mPB.x() ) )
                    {
                        break;
                    }
                    if( x < static_cast<int>(mpBuffer->buffer[y].size()) )
                    {
                        mpBuffer->buffer[y][x].flags &= ~(TCHAR_INVERSE);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            mP_aussen = mPB;
        }
        else
            mP_aussen = PC;
        mPB = PC;
    }
    if( ( mPA.y() == mPB.y() ) && ( mPA.x() > mPB.x() ) )
    {
        swap( mPA, mPB );
    }
    if( mPA.y() > mPB.y() )
    {
        swap( mPA, mPB );
    }
    mP_aussen = PC;
    highlight();
}


void TTextEdit::contextMenuEvent( QContextMenuEvent * event )
{
    event->accept();
    return;
}

void TTextEdit::slot_popupMenu()
{
    QAction * pA = (QAction *)sender();
    if( ! pA )
    {
        return;
    }
    QString cmd;
    if( mPopupCommands.contains( pA->text() ) )
    {
        cmd = mPopupCommands[pA->text()];
    }
    mpHost->mLuaInterpreter.compileAndExecuteScript( cmd );
}

void TTextEdit::mousePressEvent( QMouseEvent * event )
{
    if (!mpConsole->mIsSubConsole && !mpConsole->mIsDebugConsole) {
        TEvent mudletEvent;
        mudletEvent.mArgumentList.append(QLatin1String("sysWindowMousePressEvent"));
        switch (event->button()) {
        case Qt::LeftButton:
            mudletEvent.mArgumentList.append(QString::number(1));
            break;
        case Qt::RightButton:
            mudletEvent.mArgumentList.append(QString::number(2));
            break;
        case Qt::MidButton:
            mudletEvent.mArgumentList.append(QString::number(3));
            break;
        default: // TODO: What about those of us with more than three mouse buttons?
            mudletEvent.mArgumentList.append(0);
            break;
        }
        mudletEvent.mArgumentList.append(QString::number(event->x()));
        mudletEvent.mArgumentList.append(QString::number(event->y()));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mpHost->raiseEvent(mudletEvent);
    }

    if( event->button() == Qt::LeftButton )
    {
        if( event->modifiers() & Qt::ControlModifier ) {
            mCtrlSelecting = true;
        }
        int x = event->x() / mFontWidth;
        if( mShowTimeStamps )
        {
            if( x < 13 ) {
                mCtrlSelecting = true;
            }
            x -= 13;
        }
        int y = ( event->y() / mFontHeight ) + imageTopLine();
        if( x < 0 ) x = 0;
        if( y < 0 ) y = 0;
        if( y < static_cast<int>(mpBuffer->buffer.size()) )
        {
            if( x < static_cast<int>(mpBuffer->buffer[y].size()) )
            {
               if( mpBuffer->buffer[y][x].link > 0 )
               {
                    QStringList command = mpBuffer->mLinkStore[mpBuffer->buffer[y][x].link];
                    QString func;
                    if( command.size() > 0 )
                    {
                        func = command.at(0);
                        mpHost->mLuaInterpreter.compileAndExecuteScript( func );
                        return;
                    }
                }
            }
        }
        unHighlight( mSelectedRegion );
        mSelectedRegion = QRegion( 0, 0, 0, 0 );
        if ( mLastClickTimer.elapsed() < 300 )
        {
            int xind=x;
            int yind=y;

            if ( yind >= mpBuffer->lineBuffer.size() )
                return;
            if ( xind >= mpBuffer->lineBuffer[yind].size() )
                return;
            while( xind < static_cast<int>( mpBuffer->buffer[yind].size() ) )
            {
                QChar c = mpBuffer->lineBuffer[yind].at(xind);
                if ( c == ' ' )
                    break;
                xind++;
            }
            // For ignoring user specified characters, we first stop at space boundaries, then we
            // proceed to search within these spaces for ignored characters and chop off any we find.
            while(xind>0 && mpHost->mDoubleClickIgnore.contains(mpBuffer->lineBuffer[yind].at(xind-1)))
                xind--;
            mPB.setX ( xind-1 );
            mPB.setY ( yind );
            for( xind=x-1; xind>0; xind--)
            {
                QChar c = mpBuffer->lineBuffer[yind].at(xind);
                if (c == ' ')
                    break;
            }
            int lsize = mpBuffer->lineBuffer[yind].size();
            while(xind+1 < lsize && mpHost->mDoubleClickIgnore.contains(mpBuffer->lineBuffer[yind].at(xind+1)))
                xind++;
            if ( xind > 0 )
                mPA.setX ( xind+1 );
            else
                mPA.setX ( qMax( 0, xind ) );
            mPA.setY ( yind );
            highlight();
            event->accept();
            return;
        }
        else
        {
            mLastClickTimer.start();
            mMouseTracking = true;
            if( y >= mpBuffer->size() )
            {
                return;
            }
            if( mCtrlSelecting ) {
                mPA.setX( 0 );
                mPA.setY( y );
                mPB.setX( static_cast<int>(mpBuffer->buffer[y].size())-1 );
                mPB.setY( y );
                mDragStartY = y;
                highlight();
            } else {
                mPA.setX( x );
                mPA.setY( y );
                mPB = mPA;
            }
            event->accept();
            return;
        }
    }

    if( event->button() == Qt::RightButton )
    {
        int x = event->x() / mFontWidth;
        if( mShowTimeStamps )
        {
            x -= 13;
        }
        int y = ( event->y() / mFontHeight ) + imageTopLine();
        if( x < 0 ) x = 0;
        if( y < 0 ) y = 0;
        if( y < static_cast<int>(mpBuffer->buffer.size()) )
        {
            if( x < static_cast<int>(mpBuffer->buffer[y].size()) )
            {
                if( mpBuffer->buffer[y][x].link > 0 )
                {
                    QStringList command = mpBuffer->mLinkStore[mpBuffer->buffer[y][x].link];
                    QStringList hint = mpBuffer->mHintStore[mpBuffer->buffer[y][x].link];
                    if( command.size() > 1 )
                    {
                        auto popup = new QMenu( this );
                        for( int i=0; i<command.size(); i++ )
                        {
                            QAction * pA;
                            if( i < hint.size() )
                            {
                                pA = popup->addAction( hint[i] );
                                mPopupCommands[hint[i]] = command[i];
                            }
                            else
                            {
                                pA = popup->addAction( command[i] );
                                mPopupCommands[command[i]] = command[i];
                            }
                            connect( pA, SIGNAL(triggered()), this, SLOT(slot_popupMenu()));
                        }
                        popup->popup( event->globalPos() );
                    }
                    mIsCommandPopup = true;
                    return;
                }
            }
        }
        mIsCommandPopup = false;

        QAction * action = new QAction("copy", this );
        action->setStatusTip(tr("copy selected text to clipboard"));
        connect( action, SIGNAL(triggered()), this, SLOT(slot_copySelectionToClipboard()));
        QAction * action2 = new QAction("copy HTML", this );
        action2->setStatusTip(tr("copy selected text with colors as HTML (for web browsers)"));
        connect( action2, SIGNAL(triggered()), this, SLOT(slot_copySelectionToClipboardHTML()));
        auto popup = new QMenu( this );
        popup->addAction( action );
        popup->addAction( action2 );
        popup->popup( mapToGlobal( event->pos() ), action );
        event->accept();
        return;

    }

    if( event->button() == Qt::MidButton )
    {
        mpConsole->console2->mCursorY = mpConsole->buffer.size();//
        mpConsole->console2->hide();
        mpBuffer->mCursorY = mpBuffer->size();
        mpConsole->console->mCursorY = mpConsole->buffer.size();//
        mpConsole->console->mIsTailMode = true;
        mpConsole->console->updateScreenView();
        mpConsole->console->forceUpdate();
        event->accept();
        return;
    }
    QWidget::mousePressEvent( event );
}

void TTextEdit::slot_copySelectionToClipboard()
{
    copySelectionToClipboard();
}

void TTextEdit::slot_copySelectionToClipboardHTML()
{
    copySelectionToClipboardHTML();
}


void TTextEdit::copySelectionToClipboard()
{
    if( ( mPA.y() == mPB.y() ) && ( mPA.x() > mPB.x() ) )
    {
        swap( mPA, mPB );
    }
    if( mPA.y() > mPB.y() )
    {
        swap( mPA, mPB );
    }
    QString text;

    for( int y=mPA.y(); y<=mPB.y()+1; y++ )
    {
        if( y >= static_cast<int>(mpBuffer->buffer.size()) )
        {
            QClipboard * clipboard = QApplication::clipboard();
            clipboard->setText( text );
            mSelectedRegion = QRegion( 0, 0, 0, 0 );
            forceUpdate();
            return;
        }
        // add timestamps to clipboard when "Show Time Stamps" is on and it is not one-line selection
        if (mShowTimeStamps && !mpBuffer->timeBuffer[y].isEmpty() && mPA.y() != mPB.y())
        {
            text.append( mpBuffer->timeBuffer[y].left(13) );
        }
        int x = 0;
        if( y == mPA.y() ) x = mPA.x();
        while( x < static_cast<int>( mpBuffer->buffer[y].size() ) )
        {
            text.append( mpBuffer->lineBuffer[y].at(x) );
            if( y >= mPB.y() )
            {
                if( ( x == mPB.x() ) || ( x >= static_cast<int>( mpBuffer->buffer[y].size() - 1 ) ) )
                {
                    QClipboard * clipboard = QApplication::clipboard();
                    clipboard->setText( text );
                    mSelectedRegion = QRegion( 0, 0, 0, 0 );
                    forceUpdate();
                    return;
                }
            }
            x++;
        }
        text.append("\n");

    }
}

void TTextEdit::copySelectionToClipboardHTML()
{
    if( ( mPA.y() == mPB.y() ) && ( mPA.x() > mPB.x() ) ) {
        swap( mPA, mPB );
    }
    if( mPA.y() > mPB.y() ) {
        swap( mPA, mPB );
    }

    QString title;
    if( this->mIsDebugConsole ) {
        title = tr( "Mudlet, debug console extract" );
    }
    else if( this->mIsMiniConsole ) {
        if( ! this->mpHost->mpConsole->mSubConsoleMap.empty() ) {
            for( auto it = this->mpHost->mpConsole->mSubConsoleMap.cbegin();
                 it != this->mpHost->mpConsole->mSubConsoleMap.cend();
                 ++it ) {

                if( (*it).second == this->mpConsole ) {
                    title = tr( "Mudlet, %1 mini-console extract from %2 profile" ).arg((*it).first.data(), this->mpHost->getName() );
                    break;
                }
            }
        }
    }
    else {
        title = tr( "Mudlet, %1 console extract from %2 profile" ).arg(this->mpConsole->mConsoleName, this->mpHost->getName() );
    }

    QStringList fontsList; // List of fonts to become the font-family entry for
                           // the master css in the header
    fontsList << this->fontInfo().family(); // Seems to be the best way to get the
                                        // font in use, as different TConsole
                                        // instances within the same profile
                                        // might have different fonts in future,
                                        // and although the font is settable for
                                        // the main profile window, it is not yet
                                        // for user miniConsoles, or the Debug one
    fontsList << QStringLiteral( "Courier New" );
    fontsList << QStringLiteral( "Monospace" );
    fontsList << QStringLiteral( "Courier" );
    fontsList.removeDuplicates(); // In case the actual one is one of the defaults here

    QString text =   "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'>\n";
    text.append("<html>\n");
    text.append(" <head>\n");
    text.append("  <meta http-equiv='content-type' content='text/html; charset=utf-8'>");
    // put the charset as early as possible as the parser MUST restart when it
    // switches away from the ASCII default
    text.append("  <meta name='generator' content='Mudlet MUD Client version: ");
    text.append(APP_VERSION);
    text.append(APP_BUILD);
    text.append("'>\n");
    // Nice to identify what made the file!
    text.append("  <title>");
    text.append(title);
    text.append("</title>\n");
     // Web-page title
    text.append("  <style type='text/css'>\n");
    text.append("   <!-- body { font-family: '");
    text.append(fontsList.join("', '"));
    text.append("'; font-size: 100%; line-height: 1.125em; white-space: nowrap; color:rgb(255,255,255); background-color:rgb(");
    // Line height, I think, should be equal to 18 point for a 16 point default
    // font size by default but this seems to work even when the size is not 16
    // Use a "%age" for a IE compatible font size, 16 point is the default for
    // web-pages, but 14 seems to produce a more reasonable size but that could
    // just be the browsers I tested it on! - Slysven
    text.append(QString::number(mpHost->mBgColor.red()));
    text.append(",");
    text.append(QString::number(mpHost->mBgColor.green()));
    text.append(",");
    text.append(QString::number(mpHost->mBgColor.blue()));
    text.append(");}\n");
    text.append("        span { white-space: pre; } -->\n");
    text.append("  </style>\n");
    text.append("  </head>\n");
    text.append("  <body><div>");
    // <div></div> tags required around outside of the body <span></spans> for
    // strict HTML 4 as we do not use <p></p>s or anything else

    for( int y=mPA.y(); y<=mPB.y(); y++ ) {
        if( y >= static_cast<int>(mpBuffer->buffer.size()) ) {
            return;
        }
        int x = 0;
        if( y == mPA.y() ) {// First line of selection
            x = mPA.x();
            text.append(mpBuffer->bufferToHtml( QPoint(x,y), QPoint(-1,y), mShowTimeStamps, x));
        }
        else if ( y == mPB.y() ) {// Last line of selection
            x = mPB.x();
            text.append(mpBuffer->bufferToHtml( QPoint(0,y), QPoint(x,y), mShowTimeStamps));
        }
        else { // inside lines of selection
            text.append(mpBuffer->bufferToHtml( QPoint(0,y), QPoint(-1,y), mShowTimeStamps));
        }
    }
    text.append( QStringLiteral( " </div></body>\n"
                                 "</html>" ) );
    // The last two of these tags were missing and meant the HTML was not terminated properly
    QClipboard * clipboard = QApplication::clipboard();
    clipboard->setText( text );
    mSelectedRegion = QRegion( 0, 0, 0, 0 );
    forceUpdate();
    return;
}

void TTextEdit::mouseReleaseEvent( QMouseEvent * event )
{
    if (event->button() == Qt::LeftButton) {
        mMouseTracking = false;
        mCtrlSelecting = false;
    }

    if (!mpConsole->mIsSubConsole && !mpConsole->mIsDebugConsole) {
        TEvent mudletEvent;
        mudletEvent.mArgumentList.append(QLatin1String("sysWindowMouseReleaseEvent"));
        switch (event->button()) {
        case Qt::LeftButton:
            mudletEvent.mArgumentList.append(QString::number(1));
            break;
        case Qt::RightButton:
            mudletEvent.mArgumentList.append(QString::number(2));
            break;
        case Qt::MidButton:
            mudletEvent.mArgumentList.append(QString::number(3));
            break;
        default:
            mudletEvent.mArgumentList.append(QString::number(0));
            break;
        }
        mudletEvent.mArgumentList.append(QString::number(event->x()));
        mudletEvent.mArgumentList.append(QString::number(event->y()));
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mudletEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mpHost->raiseEvent(mudletEvent);
    }
}

void TTextEdit::showEvent( QShowEvent * event )
{
    updateScreenView();
    mScrollVector=0;
    repaint();
    QWidget::showEvent( event );
}

void TTextEdit::resizeEvent( QResizeEvent * event )
{
    updateScreenView();
    if( ! mIsSplitScreen  && ! mIsDebugConsole )
    {
        mpHost->adjustNAWS();
    }

    QWidget::resizeEvent( event );
}

void TTextEdit::wheelEvent( QWheelEvent * e )
{
    int k = 3;
    if( e->delta() < 0 )
    {
        mpConsole->scrollDown( abs( k ) );
        e->accept();
        return;
    }
    if( e->delta() > 0 )
    {
        mpConsole->scrollUp( k );
        e->accept();
        return;
    }
    e->ignore();
    return;
}

int TTextEdit::imageTopLine()
{
    if( ! mIsSplitScreen )
    {
        mCursorY = mpBuffer->mCursorY;
    }
    if( mCursorY > mScreenHeight  )
    {
        if( isTailMode() )
        {
            if( mpBuffer->lineBuffer[mpBuffer->getLastLineNumber()] == "" )
            {
                return mCursorY - mScreenHeight - 1;
            }
            else
            {
                return mCursorY - mScreenHeight;
            }
        }
        else
        {
            return mCursorY - mScreenHeight;
        }
    }
    else
    {
        return 0;
    }
}

bool TTextEdit::isTailMode()
{
    if( mIsTailMode )
    {
        mIsTailMode = true;
        return true;
    }
    else
    {
        return false;
    }
}

int TTextEdit::bufferScrollUp( int lines )
{
    if( (mpBuffer->mCursorY - lines) >= mScreenHeight  )
    {
        mpBuffer->mCursorY -= lines;
        mIsTailMode = true;
        return lines;
    }
    else
    {
        mpBuffer->mCursorY -= lines;
        if( mCursorY < 0 )
        {
            int delta = mCursorY;
            mpBuffer->mCursorY = 0;
            return delta;
        }
        else
            return 0;
    }
}

int TTextEdit::bufferScrollDown( int lines )
{
    if( ( mpBuffer->mCursorY + lines ) < (int)(mpBuffer->size()) )
    {
        if( mpBuffer->mCursorY + lines < mScreenHeight + lines )
        {
            mpBuffer->mCursorY = mScreenHeight+lines;
            if( mpBuffer->mCursorY > (int)(mpBuffer->size()-1 ) )
            {
                mpBuffer->mCursorY = mpBuffer->size()-1;
                mIsTailMode = true;
            }
        }
        else
        {
            mpBuffer->mCursorY += lines;
            mIsTailMode = false;
        }

        return lines;
    }
    else if( mpBuffer->mCursorY >= (int)(mpBuffer->size()-1) )
    {
        mIsTailMode = true;
        mpBuffer->mCursorY = mpBuffer->lineBuffer.size();
        forceUpdate();
        return 0;
    }
    else
    {
        lines = (int)(mpBuffer->size()-1) - mpBuffer->mCursorY;
        if( mpBuffer->mCursorY + lines < mScreenHeight + lines )
        {
            mpBuffer->mCursorY = mScreenHeight+lines;
            if( mpBuffer->mCursorY > (int)(mpBuffer->size()-1 ) )
            {
                mpBuffer->mCursorY = mpBuffer->size()-1;
                mIsTailMode = true;
            }
        }
        else
        {
            mpBuffer->mCursorY += lines;
            mIsTailMode = false;
        }

        return lines;
    }
}
