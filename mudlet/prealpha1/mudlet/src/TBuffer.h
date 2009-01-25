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




#ifndef TBUFFER_H
#define TBUFFER_H

#include <QColor>
#include <QChar>
#include <QString>
#include <QStringList>
#include <deque>

class Host;

class TChar
{
public:
           TChar();
           TChar( Host * );
           TChar( TChar const & copy );
    QColor fgColor;
    QColor bgColor;
    bool   italics;
    bool   bold;
    bool   underline;
};

class Host;

class TBuffer
{
public: 
    
    TBuffer( Host * pH );
    void addText( QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline );
    void wrap( unsigned int startLine, unsigned int screenWidth, unsigned int indentSize );
    unsigned int size(){ return buffer.size(); }    
    QString & line( int n );
    int find( int line, QString what, int pos );
    QStringList split( int line, QString splitter );
    QStringList split( int line, QRegExp splitter );
    bool replace( int line, QString what, QString with );
    bool deleteLines( int from, int to );
    bool applyFormat( int line, int x1, int x2, TChar & format );
    int getLastLineNumber();
    
    std::deque<TChar *> bufferLine;
    std::deque< std::deque<TChar*> > buffer; 
    
    QStringList lineBuffer;
    
    int newLines;

    
private:  
    
    void handleNewLine();
    
    Host * mpHost;
    int maxx;
    int maxy;
    bool hadLF;
    unsigned int mLastLine;
    
    
};

#endif
