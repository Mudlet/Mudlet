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

#include <iostream>
#include <string>
#include <sstream>
#include <QMap>
#include <QString>
#include <QWidget>
#include <QPainter>
#include <QClipboard>
#include "mudlet.h"
#include "TDebug.h"
#include "TTextEdit.h"
#include <math.h>

TTextEdit::TTextEdit( TConsole * pC, QWidget * pW, TBuffer * pB, Host * pH, bool isDebugConsole ) 
: QWidget( pW )
, mpBuffer( pB )
, mpHost( pH )
, mpConsole( pC )
, mCursorY( 0 )
, mIsTailMode( true )
, mHighlight_on( false )
, mHighlightingBegin( false )
, mHighlightingEnd( false )
, mMouseTracking( false )
, mIsSplitScreen( false )
, mInversOn( false )
{
    if( ! isDebugConsole )
    {
        mFontHeight = QFontMetrics( mpHost->mDisplayFont ).height();
        mFontWidth = QFontMetrics( mpHost->mDisplayFont ).width( QChar('W') );    
        mIsDebugConsole = false;
    }
    else
    {
        initDefaultSettings();
        mIsDebugConsole = true;
        mFontHeight = QFontMetrics( mDisplayFont ).height();
        mFontWidth = QFontMetrics( mDisplayFont ).width( QChar('W') );    
    }
    mScreenHeight = height() / mFontHeight;
    //FIXME
    mScreenWidth = 100;//width() / mFontWidth; //TODO: user defined value is much faster than dynamically calculated values 
                                                   //      because we can cut away large side rectangles this way that doent have
                                                   //      to be painted. performance gain is substantial
    setMaximumHeight(100*mFontHeight);
    setMouseTracking( true );
    setFocusPolicy( Qt::WheelFocus );
    setAutoFillBackground( true ); //experimental
    setAttribute( Qt::WA_InputMethodEnabled, true );
    setAttribute( Qt::WA_OpaquePaintEvent );
    setAttribute( Qt::WA_DeleteOnClose );
    showNewLines();
}

void TTextEdit::initDefaultSettings()
{
    mFgColor = QColor(255,255,255);
    mBgColor = QColor(0,0,0);
    mDisplayFont = QFont("Monospace", 10, QFont::Courier);
    mCommandLineFont = QFont("Monospace", 10, QFont::Courier);
    mCommandSeperator = QString(";");
    mWrapAt = 100;    
    mWrapIndentCount = 5;
}

/*
void TTextEdit::setScroll(int cursor, int lines)
{
    disconnect( mpScrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollBarPositionChanged(int)));
    mpScrollBar->setRange( 0, lines - mScreenHeight );
    mpScrollBar->setSingleStep( 1 );
    mpScrollBar->setPageStep( mScreenHeight );
    mpScrollBar->setValue( cursor );
    connect( mpScrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollBarPositionChanged(int)));
}
*/

/*std::string TTextEdit::getCurrentTime()
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

void TTextEdit::updateScreenView()
{
    mScreenHeight = visibleRegion().boundingRect().height() / mFontHeight;
    mScreenWidth = visibleRegion().boundingRect().width() / mFontWidth;
    mpHost->mScreenHeight = mScreenHeight;
    mpHost->mScreenWidth = mScreenWidth;
    if( ! mIsDebugConsole )
    {
        mFontHeight = QFontMetrics( mpHost->mDisplayFont ).height();
        mFontWidth = QFontMetrics( mpHost->mDisplayFont ).width( QChar('W') );
    }
    else
    {
        mFontHeight = QFontMetrics( mDisplayFont ).height();
        mFontWidth = QFontMetrics( mDisplayFont ).width( QChar('W') );
    }
    
    //update( visibleRegion().boundingRect() );    
}

void TTextEdit::showNewLines() 
{   
    if( ! isTailMode() ) return;
    
    mCursorY = mpBuffer->size()-1;
    
    QRect widgetRect = visibleRegion().boundingRect();
    
    QRect scrollRect;
    scrollRect.setLeft( 0 );
    scrollRect.setRight( mScreenWidth * mFontWidth );
    scrollRect.setTop( widgetRect.top() ); 
    scrollRect.setHeight( mScreenHeight * mFontHeight );
    
    QRect drawRect;
    drawRect.setLeft( 0 );
    drawRect.setRight( mScreenWidth * mFontWidth );
    int lines = mpBuffer->newLines;
           
    drawRect.setTop( abs(mScreenHeight - lines ) * mFontHeight );  //old:abs(mScreenHeight - lines -1 )
    drawRect.setHeight( lines * mFontHeight );
    if( scrollRect.height() < widgetRect.height() )
    {
        scroll( 0, -1 * (scrollRect.height()), scrollRect );// mFontHeight * ( -1 * ( mpBuffer->newLines ) );
    }
    update( drawRect );
    mpBuffer->newLines = 0;
}

void TTextEdit::scrollUp( int lines )
{
    lines = bufferScrollUp( lines );
    if( lines == 0 ) 
        return;
    
    if( lines > 0 )
    {
        scroll( 0, mFontHeight * lines );        
        return;
    }
    else
    {
        // lines < 0 => skip scrolling and paint frame directly,
        //              as scrollRect covers the entire area of the screen
        lines = mScreenHeight;
        
        QRect drawRect;
        drawRect.setLeft( 0 );
        drawRect.setRight( mScreenWidth * mFontWidth );
        drawRect.setTop( abs(mScreenHeight - lines ) * mFontHeight );
        drawRect.setHeight( mScreenHeight * mFontHeight );
    
        update( drawRect );
    }
}

void TTextEdit::scrollDown( int lines )
{
    lines = bufferScrollDown( lines );
    if( lines == 0 ) return;
        
    if( ( lines < 0 ) || ( imageTopLine() == 0 ) )
    {
        // lines < 0 => skip scrolling and paint frame directly,
        //              as scrollRect covers the entire area of the screen
        lines = lines * -1;
        QRect drawRect;
        drawRect.setLeft( 0 );
        drawRect.setRight( mScreenWidth * mFontWidth );
        drawRect.setBottom( mScreenHeight*mFontHeight );
        drawRect.setHeight( lines*mFontHeight );
    
        update( drawRect );
    }
    else
    {
        scroll( 0, mFontHeight * ( -1 * lines ) );        
        return;
    }    
}

void TTextEdit::drawBackground( QPainter & painter, 
                               const QRect & rect, 
                               const QColor & bgColor )
{
    QRect bR = rect;
    if( rect.width() > mScreenWidth * mFontWidth ) bR.setWidth( mScreenWidth * mFontWidth );
    
    painter.fillRect( bR.x(), bR.y(), bR.width(), bR.height(), bgColor );//, QColor(rand()%255,rand()%255,rand()%255));//bgColor);
}

void TTextEdit::drawCharacters( QPainter & painter,
                                const QRect & rect,
                                QString & text,
                                bool isBold,
                                bool isUnderline,
                                bool isItalics,
                                QColor & fgColor,
                                QColor & bgColor )
{
    QFont font = painter.font();
    if( ( font.bold() != isBold ) || ( font.underline() != isUnderline ) )
    {
        font.setBold( isBold );
        font.setUnderline( isUnderline );
        painter.setFont( font );
    }
    
    QPen pen = painter.pen();
    if ( pen.color() != fgColor )
    {
        pen.setColor( fgColor );
        painter.setPen( fgColor );
    }
    
    painter.drawText( rect.x(), rect.bottom()-5, text );
}

void TTextEdit::drawForeground( QPainter & painter, const QRect & rect )
{
    QPoint P_topLeft  = rect.topLeft();
    QPoint P_bottomRight = rect.bottomRight();
    int x_topLeft = P_topLeft.x();
    int y_topLeft = P_topLeft.y();
    int x_bottomRight = P_bottomRight.x();
    int y_bottomRight = P_bottomRight.y();
    
    if( x_bottomRight > mScreenWidth * mFontWidth ) x_bottomRight = mScreenWidth * mFontWidth;
    
    int x1 = x_topLeft / mFontWidth;
    int y1 = y_topLeft / mFontHeight;
    int x2 = x_bottomRight / mFontWidth;
    int y2 = y_bottomRight / mFontHeight;
    
    int lineOffset = imageTopLine(); 
    bool invers = false;
    
    if( mHighlight_on && mInversOn ) 
    {
        invers = true;
    }
    
    for( int i=0; i<mScreenHeight; i++ )
    {
        if( mpBuffer->buffer.size() <= i+lineOffset ) 
        {
            break;
        }
        
        int lineLength = mpBuffer->buffer[i+lineOffset].size();
        for( int i2=x1; i2<lineLength; )
        {
            if( i2 >= x2 )
            {
                break;
            }
            QString text = mpBuffer->lineBuffer[i+lineOffset].at(i2);
            bool isBold = mpBuffer->buffer[i+lineOffset][i2]->bold;
            bool isUnderline = mpBuffer->buffer[i+lineOffset][i2]->underline;
            bool isItalics = mpBuffer->buffer[i+lineOffset][i2]->italics;
            QColor fgColor = mpBuffer->buffer[i+lineOffset][i2]->fgColor;
            QColor bgColor = mpBuffer->buffer[i+lineOffset][i2]->bgColor;
            int delta = 1;
            
            while( i2+delta < lineLength )
            {
                if( ( mpBuffer->buffer[i+lineOffset][i2+delta]->bold == isBold ) 
                   && ( mpBuffer->buffer[i+lineOffset][i2+delta]->underline == isUnderline ) 
                   && ( mpBuffer->buffer[i+lineOffset][i2+delta]->italics == isItalics ) 
                   && ( mpBuffer->buffer[i+lineOffset][i2+delta]->fgColor == fgColor )     
                   && ( mpBuffer->buffer[i+lineOffset][i2+delta]->bgColor == bgColor ) )
                {
                    text.append( mpBuffer->lineBuffer[i+lineOffset].at(i2+delta) );
                    delta++;
                }
                else
                {
                    break;
                }
                
            }
            
            QRect textRect = QRect( mFontWidth * i2, 
                                    mFontHeight * i, 
                                    mFontWidth * delta, 
                                    mFontHeight );
            
            if( (bgColor != palette().base().color()) || invers )
            {
                if( invers )
                {
                    QColor tmpColor = bgColor;
                    bgColor = fgColor;
                    fgColor = tmpColor;
                }
                drawBackground( painter, textRect, bgColor );
            }
            if( text[0] != cLF )
            {
                drawCharacters( painter, textRect, text, isBold, isUnderline, isItalics, fgColor, bgColor );  
            }
            
            i2+=delta;
        }
    }
}

void TTextEdit::paintEvent( QPaintEvent* e )
{
    //cout << "Time before render = " << getCurrentTime() <<endl;
    QPainter painter( this );
    
    const QRect & rect = e->rect();
    drawBackground( painter, rect, palette().base().color() );
    drawForeground( painter, rect );
    
    //cout << " Time after render = " << getCurrentTime()<<endl;
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
    mSelectedRegion = mSelectedRegion - newRegion;
    unHighlight( mSelectedRegion );
     
    mHighlight_on = true;
    mInversOn = true;
    mSelectedRegion = newRegion;
    foreach( const QRect & rect, mSelectedRegion.rects()  )
    {
        repaint( rect );
    }
    
    mInversOn = false; 
}

void TTextEdit::unHighlight( QRegion & region )
{
    mHighlight_on = false;
    mInversOn = false;
    foreach( const QRect & rect, region.rects()  )
    {
        repaint( rect );
    }
}

void TTextEdit::swap( QPoint & p1, QPoint & p2 )
{
    QPoint tmp = p1;
    p1 = p2;
    p2 = tmp;
}

void TTextEdit::mouseMoveEvent( QMouseEvent * event )
{
    if( ! mMouseTracking ) return;
    
    int x = event->x() / mFontWidth;
    int y = ( event->y() / mFontHeight ) + imageTopLine();
    if( ( x < 0 ) || ( y < 0 ) || ( y > (int) mpBuffer->size()-1 ) ) return;
    
    QPoint PC( x, y );
       
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
        mPA = PC;    
    }
    else mPB = PC;
    
    highlight();
}

void TTextEdit::mousePressEvent( QMouseEvent * event )
{
    if( event->button() == Qt::MidButton )
    {
        mpConsole->console->mCursorY = mpBuffer->size()-1;
        mpConsole->console->mIsTailMode = true;
        mpConsole->console2->mCursorY = mpBuffer->size()-1;
        mpConsole->console2->hide();    
        event->accept();
        return;
    }
    
    if( event->button() == Qt::LeftButton )
    {
        mMouseTracking = true;
        int x = event->x() / mFontWidth;
        int y = ( event->y() / mFontHeight ) + imageTopLine();
        unHighlight( mSelectedRegion );
        mSelectedRegion = QRegion( 0, 0, 0, 0 );
        mPA.setX( x );
        mPA.setY( y );
        mPB = mPA;
        event->accept();
        return;
    }
    
    if( event->button() == Qt::RightButton )
    {
        QAction * action = new QAction("copy", this );
        action->setStatusTip(tr("copy selected text to clipboard"));
        connect( action, SIGNAL(triggered()), this, SLOT(slot_copySelectionToClipboard()));
        QMenu * popup = new QMenu( this );
        popup->addAction( action );
        popup->popup( mapToGlobal( event->pos() ), action );
        event->accept();
        return;
        
    }
    QWidget::mousePressEvent( event );
}

void TTextEdit::slot_copySelectionToClipboard()
{
    copySelectionToClipboard();        
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
        int x = 0;
        if( y == mPA.y() ) x = mPA.x();
        while( x < (int) mpBuffer->buffer[y].size() )
        {
            text.append( mpBuffer->lineBuffer[y].at(x) );
            if( y >= mPB.y() )
            {
                if( ( x == mPB.x()-1 ) || ( x >= (int) mpBuffer->buffer[y].size()-1 ) )
                {
                    QClipboard * clipboard = QApplication::clipboard();
                    clipboard->setText( text );
                    qDebug()<<"copied to clipboard: "<<text;
                    return;
                }
            }
            x++;
        }
        text.append("\n");
    }
}

void TTextEdit::mouseReleaseEvent( QMouseEvent * event )
{
    if( event->button() == Qt::LeftButton )
    {
        mMouseTracking = false;
    }
}

void TTextEdit::showEvent( QShowEvent * event ) 
{
    if( mIsSplitScreen ) 
    {
        mIsSplitScreen = false;
        resize( width(), mpConsole->height()/4 );
    }
    updateScreenView();
    repaint( contentsRect() );
    QWidget::showEvent( event );
}

void TTextEdit::resizeEvent( QResizeEvent * event ) 
{
    updateScreenView();    
    QWidget::resizeEvent( event );
}

void TTextEdit::wheelEvent ( QWheelEvent * e ) 
{
    int delta = e->delta() / 8 / 15;
    
    int k = 10 + (abs(delta) * 50) - 50;
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
    if( mCursorY > mScreenHeight  )
    {
        return mCursorY - mScreenHeight + 1;
    }
    else
    {   
        return 0;
    }
} 

bool TTextEdit::isTailMode()
{
    if( ( mCursorY == (int) mpBuffer->size()-1 ) 
         
        //|| ( mIsDebugConsole ) 
        || ( mIsTailMode ) 
        || ( (int)mpBuffer->size() <= mScreenHeight) )
    {
        mIsTailMode = true;
        return true;    
    }
    else 
        return false;
}

int TTextEdit::bufferScrollUp( int lines )
{
    if( (mCursorY - lines) >= mScreenHeight  )
    {
        mCursorY -= lines;
        mIsTailMode = false;
        return lines;
    }
    else
    {
        mCursorY -= lines;  
        if( mCursorY < 0 )
        {
            int delta = mCursorY;
            mCursorY = 0;
            return delta; 
        }
        else
            return 0;
    }
}

int TTextEdit::bufferScrollDown( int lines )
{
    if( ( mCursorY + lines ) < (int)(mpBuffer->size()-1 - mScreenHeight) )
    {
        if( mCursorY + lines < mScreenHeight + lines )
        {
            mCursorY = mScreenHeight+lines;
            if( mCursorY > (int)(mpBuffer->size()-1 ) )
            {
                mCursorY = mpBuffer->size()-1;    
            }
        }
        else
        {
            mCursorY += lines;
        }
        mIsTailMode = false;
        qDebug()<<"mCursorY="<<mCursorY;
        return lines;
    }
    else
    {
        mCursorY += lines;
        if( mCursorY > (int)(mpBuffer->size()-1) )
        {
            int delta = mCursorY;
            mCursorY = mpBuffer->size()-1;
            return (-1 * delta);
        }
        mIsTailMode = true;
        return 0;
    }
}









