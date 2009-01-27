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

#include <QDebug>
#include <stdio.h>
#include <iostream>
#include <string>
#include <deque>
#include <queue>
#include "TBuffer.h"
#include "Host.h"

using namespace std;

TChar::TChar(){}

TChar::TChar( Host * pH )
{
    fgColor = pH->mFgColor;
    bgColor = pH->mBgColor;
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
{   
    mpHost->mScreenHeight = 40;
    mpHost->mScreenWidth = 80; //TODO: make this a user option
    
    buffer.clear();
    buffer.push_back( bufferLine );
    lineBuffer.clear();
    QString nothing;
    lineBuffer.append( nothing );
    newLines = 0;
    mLastLine = buffer.size()-1;
}

void TBuffer::handleNewLine()
{
    int y = lineBuffer.size();
    if( y > 0 )
    {
        if( lineBuffer[y-1].size() == 1 )
        {
            lineBuffer[y-1].chop( 1 );
            delete buffer[y-1][0];
            buffer[y-1].pop_back();    
        }
    }
    std::deque<TChar *> newLine;
    newLine.push_back( new TChar( mpHost ) );
    QString nextLine = "\n";
    buffer.push_back( newLine );
    lineBuffer.append( nextLine );
    mLastLine++;
    newLines++;
}

int TBuffer::getLastLineNumber()
{
    return lineBuffer.size()-1;
}

void TBuffer::addText( QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline )
{
    // special case if line ends on <LF> with a prior format code change
    if( text == QString("\n") )
    {
        handleNewLine();
        return;
    }
    
    QStringList lines = text.split( '\n' );
    
    int y;
    for( int i=0; i<lines.size(); i++ )
    {
        // the content of lines[0] is to be added to the last line in the buffer
        // unless lines[0] is itself an <LF>
        // all other lines represent corresponding <LF>
        if( lines[i].size() == 0 )
        {
            handleNewLine();
            continue;
        }
        else if( i > 0 ) 
        {
            handleNewLine();
        }
            
        y = getLastLineNumber();
        
        // replace preceding <LF>
        if( lineBuffer[y].size() == 1 )
        {
            lineBuffer[y].chop( 1 );
            delete buffer[y][0];
            buffer[y].pop_back();
        }
        
        lineBuffer[y].append( lines[i] );
        for( int i2=0; i2<lines[i].size(); i2++ )
        {
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            buffer[y].push_back( pC );
        }
    }
}

void TBuffer::wrap( unsigned int startLine, unsigned int screenWidth, unsigned int indentSize )
{
    if( buffer.size() <= startLine ) return;
    
    std::queue<std::deque<TChar *> > queue;
    QStringList tempList;
    int lineCount = 0;
    
    for( unsigned int i=startLine; i<buffer.size(); i++ )
    {
        std::deque<TChar *> newLine;
        QString lineText;
        
        int indent = 0;
        if( buffer[i].size() >= screenWidth )
        {
            for( unsigned int i3=0; i3<indentSize; i3++ )
            {
                TChar * pSpace = new TChar;
                pSpace->fgColor = mpHost->mFgColor;
                pSpace->bgColor = mpHost->mBgColor;
                pSpace->italics = false;
                pSpace->bold = false;
                pSpace->underline = false;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        for( unsigned int i2=0; i2<buffer[i].size();  )
        {  
            for( unsigned int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( i2 >= buffer[i].size() )
                {
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
    }
    
    newLines -= lineCount;
    newLines += queue.size();
    
    while( ! queue.empty() )
    {
        buffer.push_back( queue.front() );
        queue.pop();
    }
    
    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.append( tempList[i] );
    }
}

QString badLineError = QString("ERROR: invalid line number");

QString & TBuffer::line( int n )
{
    if( (n > lineBuffer.size()) || (n<0) ) return badLineError;
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

bool TBuffer::deleteLines( int from, int to )
{
    if( ( from >= 0 ) 
     && ( from <= buffer.size() )
     && ( from < to )   
     && ( to >=0 )
     && ( to <= buffer.size() ) )
    {
        int delta = to - from;
        
        for( int i=from; i<from+delta; i++ )
        {
            lineBuffer.removeAt( i );
            for( int k=0; k<buffer[i].size(); k++ )
            {
                delete buffer[i][k];    
            }
        }
        
        int i = buffer.size();
        
        // we do reverse lookup as the wanted lines are usually at the end of the buffer
        // std::revers_iterator is not defined for usage in erase()
        
        typedef std::deque<std::deque<TChar *> >::iterator IT;
        for( IT it=buffer.end(); it!=buffer.begin(); it-- )
        {
            if( --i >= to ) 
                continue;
            
            if( --delta >= 0 ) 
                buffer.erase( it );
            else
                break;
        }
        
        return true;
    }
    else 
        return false;
}

bool TBuffer::applyFormat( int line, int from, int to, TChar & format )
{
    if( ( from >= 0 ) 
     && ( from <= buffer.size() )
     && ( to >=0 )
     && ( to <= buffer.size() ) ) 
    {
        for( int i=from; i<to; i++ )
        {
            *buffer[line][i] = format;
        }
        return true;
    }
    else 
        return false;            
}






