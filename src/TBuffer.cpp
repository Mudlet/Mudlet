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
//#define NDEBUG
#include <assert.h>
#include <QDebug>
#include <stdio.h>
#include <iostream>
#include <string>
#include <deque>
#include <queue>
#include "TBuffer.h"
#include "Host.h"

using namespace std;

const QColor TCharDefaultFgColor = QColor(255,255,255);
const QColor TCharDefaultBgColor = QColor(0,0,0); 


TChar::TChar()
{
    fgColor = QColor(255,255,255);
    bgColor = QColor(0,0,0);
    italics = false;
    bold = false;
    underline = false;
}


TChar::TChar( Host * pH )
{
    if( pH )
    {
        fgColor = pH->mFgColor;
        bgColor = pH->mBgColor;
    }
    else
    {
        
        fgColor = TCharDefaultFgColor;
        bgColor = TCharDefaultBgColor;   
    }
    italics = false;
    bold = false;
    underline = false;    
}


TChar::TChar( TChar const & copy )
{
    fgColor = copy.fgColor;
    bgColor = copy.bgColor;
    italics = copy.italics;
    bold = copy.bold;
    underline = copy.underline;     
}


TBuffer::TBuffer( Host * pH )
: mpHost( pH )
, mCursorMoved( false )
, mWrapAt( 100 )
, mWrapIndent( 5 )
, mLinesLimit( 100000 )
{   
    buffer.clear();
    lineBuffer.clear();
    newLines = 0;
    mLastLine = 0;
}


int TBuffer::getLastLineNumber()
{
    if( ((int)buffer.size())-1 > 0 )
    {
        return ((int)buffer.size())-1;
    }
    else
    {
        return 0;
    }
}


void TBuffer::append( QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline )
{
    if( (int)buffer.size() > mLinesLimit )
    {
        while( buffer.size() > mLinesLimit-10000 )
        {
            deleteLine( 0 );
        }
    }
    for( int i=0; i<text.size(); i++ )
    {
        int last = buffer.size()-1;
        if( last < 0 )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = bgColor;    // make the <LF>-marker invisible
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            timeBuffer << QTime::currentTime().toString() + "   ";
            last = 0;
        }
        if( mCursorMoved ) 
        {
            if(lineBuffer[last].size() == 1) // <LF> at beginning of new line marker
            {
                if( lineBuffer[last].at( 0 ) == QChar( 0x21af ) )
                {
                    if( text.at( i ) != QChar( '\n' ) )
                    {
                        mCursorMoved = false;
                        lineBuffer[last].replace( 0, 1, text.at( i ) );
                        TChar * pC = buffer[last][0];
                        pC->fgColor = fgColor;
                        pC->bgColor = bgColor;
                        pC->italics = italics;
                        pC->bold = bold;
                        pC->underline = underline;
                        timeBuffer[last] = QTime::currentTime().toString()+"   ";
                        continue;
                    }
                }
            }
        }
        else
        {
            lineBuffer[last].append( text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            buffer[last].push_back( pC );
        }
        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = bgColor;    // make the <LF>-marker invisible
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            timeBuffer << QTime::currentTime().toString()+"   ";
            mLastLine++;
            newLines++;
            mCursorMoved = true;
        }
    }
}

QPoint TBuffer::insert( QPoint & where, QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline )
{
    QPoint P(-1, -1);
    
    int x = where.x();
    int y = where.y();
    
    if( y < 0 ) return P;
    if( y >= buffer.size() ) return P;
    
    
    for( int i=0; i<text.size(); i++ )
    {
        if( mCursorMoved ) 
        {
            if(lineBuffer[y].size() == 1) // <LF> at beginning of new line marker
            {
                if( lineBuffer[y][0] == QChar( 0x21af ) )
                {
                    if( text.at( i ) != QChar( '\n' ) )
                    {
                        mCursorMoved = false;
                        x = 0;
                        lineBuffer[y].replace( 0, 1, text.at( i ) );
                        TChar * pC = new TChar;
                        pC->fgColor = fgColor;
                        pC->bgColor = bgColor;
                        pC->italics = italics;
                        pC->bold = bold;
                        pC->underline = underline;
                        buffer[y].push_back( pC );
                        buffer[y].pop_front();
                        continue;
                    }
                }
            }
        }
        else
        {
            lineBuffer[y].insert( x, text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            typedef std::deque<TChar *>::iterator IT;
            IT it = buffer[y].begin();
            buffer[y].insert( it+x, pC );
        }
        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            timeBuffer << QTime::currentTime().toString()+"   ";
            mLastLine++;
            newLines++;
            x = 0;
            y++;
            mCursorMoved = true;
        }
    }
    P.setX( x );
    P.setY( y );
    return P;
}


bool TBuffer::insertInLine( QPoint & P, QString & text, TChar & format )
{
    if( text.size() < 1 ) return false;
    int x = P.x();
    int y = P.y();
    if( ( y > 0 ) && ( y <= (int)buffer.size()-1 ) )
    {
        if( x < 0 )
        {
            return false;
        }
        if( x >= buffer[y].size() )
        {
            TChar c;
            expandLine( y, x-buffer[y].size(), & c );
        }
        for( int i=0; i<text.size(); i++ )
        {
            lineBuffer[y].insert( x+i, text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = format.fgColor;
            pC->bgColor = format.bgColor;
            pC->italics = format.italics;
            pC->bold = format.bold;
            pC->underline = format.underline;
            typedef std::deque<TChar *>::iterator IT;
            IT it = buffer[y].begin();
            buffer[y].insert( it+x+i, pC );
        }   
    }
    return true;
}

TBuffer TBuffer::copy( QPoint & P1, QPoint & P2 )
{
    TBuffer slice( mpHost );
    slice.clear();
    int y = P1.y();
    int x = P1.x();
    if( y < 0 || y >= buffer.size() )
        return slice;
    
    if( ( x < 0 ) 
        || ( x >= buffer[y].size() )
        || ( P2.x() < 0 ) 
        || ( P2.x() > buffer[y].size() ) )
    {
        x=0;
    }
    for( ; x<P2.x(); x++ )
    {
        if( lineBuffer[y][x] == QChar( 0x21af ) )
        {
            continue;
        }
        slice.append(QString(lineBuffer[y][x]), 
                     buffer[y][x]->fgColor, 
                     buffer[y][x]->bgColor, 
                     (buffer[y][x]->bold == true), 
                     (buffer[y][x]->italics == true), 
                     (buffer[y][x]->underline == true) );
    }
    return slice;
}

TBuffer TBuffer::cut( QPoint & P1, QPoint & P2 )
{
    TBuffer slice = copy( P1, P2 );
    QString nothing = "";
    TChar format;
    replaceInLine( P1, P2, nothing, format );
    return slice;
}

void TBuffer::paste( QPoint & P, TBuffer chunk )
{
    bool hasAppended = false;
    int y = P.y();
    int x = P.x();
    if( chunk.buffer.size() < 1 )
    {
        return;
    }
    if( y < 0 || y > getLastLineNumber() )
    {
        y=getLastLineNumber();
    }
    if( x < 0 || x >= buffer[y].size() )
    {
        return;
    }
    for( int cx=0; cx<(int)chunk.buffer[0].size(); cx++ )
    {
        QPoint P_current(cx, y);
        if( chunk.buffer[0][cx] )
        {
            if( y < getLastLineNumber() )
            {
                TChar format;
                format.fgColor = chunk.buffer[0][cx]->fgColor;
                format.bgColor = chunk.buffer[0][cx]->bgColor;
                format.bold = chunk.buffer[0][cx]->bold;
                format.italics = chunk.buffer[0][cx]->italics;
                format.underline = chunk.buffer[0][cx]->underline;
                QString s = QString(chunk.lineBuffer[0][cx]);
                insertInLine( P_current, s, format );
            }
            else
            {
                hasAppended = true;
                append(QString(chunk.lineBuffer[0][cx]),
                       chunk.buffer[0][cx]->fgColor,
                       chunk.buffer[0][cx]->bgColor,
                       (chunk.buffer[0][cx]->bold == true),
                       (chunk.buffer[0][cx]->italics == true),
                       (chunk.buffer[0][cx]->underline == true) );
            }
        }
    }
    cout << endl;
    if( hasAppended )
    {
        TChar format;
        wrapLine( y, mWrapAt, mWrapIndent, format );
    }
}

int TBuffer::calcWrapPos( int line, int begin, int end )
{
    const QString lineBreaks = ",.- ";
    if( lineBuffer.size() < line ) return -1;
    if( lineBuffer[line].size() < end )
    {
        end = lineBuffer[line].size()-1;
    }
    for( int i=end; i>=begin; i-- )
    {
        if( lineBreaks.indexOf(lineBuffer[line][i]) > -1 )
        {
            return i;
        }
    }
    return -1;
}

// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrap( int startLine, int screenWidth, int indentSize, TChar & format )
{
    if( buffer.size() <= startLine ) return 0;
    std::queue<std::deque<TChar *> > queue;
    QStringList tempList;
    int lineCount = 0;
    
    for( int i=startLine; i<buffer.size(); i++ )
    {
        assert( buffer[i].size() == lineBuffer[i].size() );
        std::deque<TChar *> newLine;
        QString lineText;
        
        int indent = 0;
        if( buffer[i].size() >= screenWidth )
        {
            for( unsigned int i3=0; i3<indentSize; i3++ )
            {
                TChar * pSpace = new TChar;
                pSpace->fgColor = format.fgColor;
                pSpace->bgColor = format.bgColor;
                pSpace->italics = format.italics;
                pSpace->bold = format.bold;
                pSpace->underline = format.underline;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        int length = buffer[i].size();
        for( int i2=0; i2<buffer[i].size();  )
        {
            if( length-i2 > screenWidth-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+screenWidth-indent );
                if( wrapPos > -1 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = -1;
                }
            }
            else
            {
                lastSpace = -1;
            }
            for( int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 >= lastSpace )
                    {
                        i2++;
                        break;
                    }
                }
                if( i2 >= buffer[i].size() )
                {
                    break;
                }
                if( lineBuffer[i][i2] == QChar('\n') )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );
            
            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }
    
    for( int i=0; i<lineCount; i++ )
    {
        buffer.pop_back();    
        lineBuffer.pop_back();
        timeBuffer.pop_back();
    }
    
    newLines -= lineCount;
    newLines += queue.size();
    int insertedLines = queue.size()-1;
    
    while( ! queue.empty() )
    {
        buffer.push_back( queue.front() );
        queue.pop();
    }
    
    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.append( tempList[i] );
        timeBuffer.append( QTime::currentTime().toString()+"   " );
    }
    return insertedLines > 0 ? insertedLines : 0;
}

// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrapLine( int startLine, int screenWidth, int indentSize, TChar & format )
{
    if( buffer.size() <= startLine ) return 0;
    std::queue<std::deque<TChar *> > queue;
    QStringList tempList;
    int lineCount = 0;

    for( int i=startLine; i<buffer.size(); i++ )
    {
        if( i > startLine ) break; //only wrap one line of text

        assert( buffer[i].size() == lineBuffer[i].size() );
        std::deque<TChar *> newLine;
        QString lineText;

        int indent = 0;
        if( buffer[i].size() >= screenWidth )
        {
            for( unsigned int i3=0; i3<indentSize; i3++ )
            {
                TChar * pSpace = new TChar;
                pSpace->fgColor = format.fgColor;
                pSpace->bgColor = format.bgColor;
                pSpace->italics = format.italics;
                pSpace->bold = format.bold;
                pSpace->underline = format.underline;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        int length = buffer[i].size();
        for( int i2=0; i2<buffer[i].size();  )
        {
            if( length-i2 > screenWidth-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+screenWidth-indent );
                if( wrapPos > -1 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = -1;
                }
            }
            else
            {
                lastSpace = -1;
            }
            for( int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 >= lastSpace )
                    {
                        i2++;
                        break;
                    }
                }
                if( i2 >= buffer[i].size() )
                {
                    break;
                }
                if( lineBuffer[i][i2] == QChar('\n') )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );

            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }

    buffer.erase( buffer.begin()+startLine );
    lineBuffer.removeAt( startLine );
    timeBuffer.removeAt( startLine );

    int insertedLines = queue.size()-1;

    int i=0;
    while( ! queue.empty() )
    {
        buffer.insert(buffer.begin()+startLine+i, queue.front());
        queue.pop();
        i++;
    }

    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.insert( startLine+i, tempList[i] );
        timeBuffer.insert( startLine+i, QTime::currentTime().toString()+"   " );
    }
    return insertedLines > 0 ? insertedLines : 0;
}


bool TBuffer::moveCursor( QPoint & where )
{
    int x = where.x();
    int y = where.y();
    if( y < 0 ) return false;
    if( y >= buffer.size() ) return false;
    
    if( buffer[y].size()-1 >  x )
    {
        TChar c;
        expandLine( y, x-buffer[y].size()-1, & c );
    }
    return true;
}

QString badLineError = QString("ERROR: invalid line number");


QString & TBuffer::line( int n )
{
    if( (n >= lineBuffer.size()) || (n<0) ) return badLineError;
    return lineBuffer[n];
}


int TBuffer::find( int line, QString what, int pos=0 )
{
    if( lineBuffer[line].size() >= pos ) return -1;
    if( pos < 0 ) return -1;
    if( ( line >= buffer.size() ) || ( line < 0 ) ) return -1;
    return lineBuffer[line].indexOf( what, pos );
}


QStringList TBuffer::split( int line, QString splitter )
{
    if( ( line >= buffer.size() ) || ( line < 0 ) ) return QStringList();   
    return lineBuffer[line].split( splitter );
}


QStringList TBuffer::split( int line, QRegExp splitter )
{
    if( ( line >= buffer.size() ) || ( line < 0 ) ) return QStringList();   
    return lineBuffer[line].split( splitter );
}


void TBuffer::expandLine( int y, int count, TChar * pC )
{
    int size = buffer[y].size()-1;
    for( int i=size; i<size+count; i++ )
    {
        if( ! pC ) pC = new TChar;
        buffer[y].push_back( pC );
        lineBuffer[y].append( " " );
    }
}

bool TBuffer::replaceInLine( QPoint & P_begin,
                             QPoint & P_end,
                             QString & with,
                             TChar & format )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    if( ( x2 > buffer[y2].size() ) || ( x1 > buffer[y1].size() ) )
    {
        return false;
    }
    if( x1 < 0 || x2 < 0 )
    {
        return false;
    }

    int xb,xe, yb, ye;
    if( y1 <= y2 )
    {
        yb = y1;
        ye = y2;
        xb = x1;
        xe = x2;
    }
    else
    {
        yb = y2;
        ye = y1;
        xb = x2;
        xe = x1;
    }

    for( int y=yb; y<=ye; y++ )
    {
        int x = 0;
        if( y == yb )
        {
            x = xb;
        }
        int x_end = buffer[y].size()-1;
        if( y == ye )
        {
            x_end = xe;
        }
        lineBuffer[y].remove( x, x_end-x );
        typedef std::deque<TChar *>::iterator IT;
        IT it1 = buffer[y].begin()+x;
        IT it2 = buffer[y].begin()+x_end;
        buffer[y].erase( it1, it2 );
    }

    // insert replacement
    insertInLine( P_begin, with, format );
    return true;
}


bool TBuffer::replace( int line, QString what, QString with )
{
    if( ( line >= buffer.size() ) || ( line < 0 ) ) 
        return false;
    lineBuffer[line].replace( what, with );
    
    // fix size of the corresponding format buffer
    
    int delta = lineBuffer[line].size() - buffer[line].size();
    
    if( delta > 0 )
    {
        for( int i=0; i<delta; i++ )
        {
            TChar * pC = new TChar( mpHost ); // cloning default char format according to profile
                                              // because a lookup would be too expensive as
                                              // this is a very often used function and this standard
                                              // behaviour is acceptable. If the user wants special colors
                                              // he can apply format changes
            buffer[line].push_back( pC );    
        }
    }
    else if( delta < 0 )
    {
        for( int i=0; i<delta; i++ )
        {
            buffer[line].pop_back();
        }
    }
    return true;
}

void TBuffer::clear()
{
    while( (getLastLineNumber() > -1 ) )   
    {
        if( ! deleteLines( 0, 0 ) )
        {
            break;
        }
    }
}

bool TBuffer::deleteLine( int y )
{ 
    return deleteLines( y, y );
}


bool TBuffer::deleteLines( int from, int to )
{
    if( ( from >= 0 ) 
     && ( from < buffer.size() )
     && ( from <= to )   
     && ( to >=0 )
     && ( to < buffer.size() ) )
    {
        int delta = to - from + 1;
        
        for( int i=from; i<from+delta; i++ )
        {
            lineBuffer.removeAt( i ); 
            timeBuffer.removeAt( i );
            for( int k=0; k<buffer[i].size(); k++ )
            {
                delete buffer[i][k];    
            }
        }
        
        int i = (int)buffer.size();
        // we do reverse lookup as the wanted lines are usually at the end of the buffer
        // std::reverse_iterator is not defined for usage in erase()
        
        typedef std::deque<std::deque<TChar *> >::iterator IT;
        for( IT it=buffer.end(); it!=buffer.begin(); )
        {
            it--;
            i--;
            if( i > to ) 
                continue;
            
            if( --delta >= 0 )
                buffer.erase( it );
            else
                break;
        }
        return true;
    }
    else 
    {
        return false;
    }
}


bool TBuffer::applyFormat( QPoint & P_begin, QPoint & P_end, TChar & format )
{
    if( ( P_begin.x() >= 0 ) 
        && ( ( P_end.y() < buffer.size() ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < buffer[y].size() ) 
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }
            
                *buffer[y][x] = format;
                x++;
            }
        }
        return true;
    }
    else 
        return false;            
}

bool TBuffer::applyFgColor( QPoint & P_begin, QPoint & P_end, QColor & fgColor )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < buffer.size() ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < buffer[y].size() )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x]->fgColor = fgColor;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyBgColor( QPoint & P_begin, QPoint & P_end, QColor & bgColor )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < buffer.size() ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < buffer[y].size() )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x]->bgColor = bgColor;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

QStringList TBuffer::getEndLines( int n )
{
    QStringList linesList;
    for( int i=getLastLineNumber()-n; i<getLastLineNumber(); i++ )
    {
        linesList << line( i );
    }
    return linesList;
}




