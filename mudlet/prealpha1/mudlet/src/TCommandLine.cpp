/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn   *
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
#include "TCommandLine.h"
#include "TConsole.h"


TCommandLine::TCommandLine( Host * pHost, TConsole * pConsole ) 
: QLineEdit( (QWidget *) pConsole )
, mpHost( pHost )
, mpConsole( pConsole )
{
    mpKeyUnit = mpHost->getKeyUnit();
    
    setFocusPolicy(Qt::StrongFocus);
    
    QFont font("Courier New", 10, QFont::Courier);
    setFont(font);
    
    mRegularPalette.setColor(QPalette::Text,QColor(0,0,192));
    mRegularPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mRegularPalette.setColor(QPalette::Base,QColor(255,255,225));
    
    mTabCompletionPalette.setColor(QPalette::Text,QColor(0,0,192));
    mTabCompletionPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mTabCompletionPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mTabCompletionPalette.setColor(QPalette::Base,QColor(235,255,235));    
    
    mAutoCompletionPalette.setColor(QPalette::Text,QColor(0,0,192));
    mAutoCompletionPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mAutoCompletionPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mAutoCompletionPalette.setColor(QPalette::Base,QColor(255,235,235));    
    
    
    mHistoryBuffer = 0;
    //TODO: historyList is goint to get restored via host restore

    mAutoCompletion = false;
}

void TCommandLine::slot_textChanged( const QString & text )
{
}

bool TCommandLine::event( QEvent * event )
{
    if( event->type() == QEvent::KeyPress ) 
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>( event );
      
        switch( ke->key() ) 
        {
            
        case Qt::Key_Tab: 
            
            if( ke->modifiers() == Qt::ShiftModifier )
            {
                handleTabCompletion( false );
                ke->accept();
                return true;
            }
            else
            {
                handleTabCompletion( true );
                ke->accept(); 
                return true;
            }
            break;
            
        case Qt::Key_unknown:
            qWarning()<<"ERROR: key unknown!";
            break;
        
        case Qt::Key_Backspace:
            if( mTabCompletionTyped.size() > 1 )
            {
                mTabCompletionTyped.chop(1);
                mAutoCompletionTyped.chop(1);
            }
            break;
            
        case Qt::Key_Delete:
            if( mTabCompletionTyped.size() > 1 )
            {
                mTabCompletionTyped.chop(1);
                mAutoCompletionTyped.chop(1);
            }
            break;
            
        //case Qt::Key_Insert:
        //break;
            
        case Qt::Key_Enter:
            enterCommand(ke);
            ke->accept();
            return true;
            
        case Qt::Key_Return:
            enterCommand(ke);
            ke->accept();
            return true;
            
        case Qt::Key_Down:
            historyDown(ke);
            ke->accept();
            return true;
            
        case Qt::Key_Up:
            historyUp(ke);
            ke->accept();   
            return true;
            
        case Qt::Key_Escape:
            
            mAutoCompletion = false;
            mTabCompletion = false;
            mTabCompletionTyped = "";
            mUserKeptOnTyping = false;
            mTabCompletionCount = -1;
            mAutoCompletionCount = -1;
            setPalette( mRegularPalette );
            ke->accept();
            return true;
            
        case Qt::Key_PageUp:
            mpConsole->scrollUp( 40 );
            ke->accept();
            return true;
            
        case Qt::Key_PageDown:
            mpConsole->scrollDown( 40 );
            ke->accept();
            return true;
            
            
        default:
            
            if( mpKeyUnit->processDataStream( ke->key(), (int)ke->modifiers() ) )
            {
                ke->accept();
                return true;
            }
            else
            {
                mUserKeptOnTyping = true;
                QLineEdit::event( event );
                handleAutoCompletion();
                return false;
            }
        }
    } 
    return QLineEdit::event( event );
}

void TCommandLine::enterCommand( QKeyEvent * event )
{
    mHistoryBuffer = 0;
    setPalette( mRegularPalette );
    
    QStringList commandList = text().split( QString(mpHost->mCommandSeparator), QString::SkipEmptyParts );
    if( commandList.size() == 0 ) 
        mpHost->send( "\n" );
    
    for( int i=0; i<commandList.size(); i++ )
    {
        mHistoryMap[ commandList[i] ] = 0;
        mHistoryList.push_front( commandList[i].append("\n" ) );
        //qDebug()<<"TCommandLine:enterCommand() sending to Host:"<<mpHost<<" text="<<commandList[i];
        mpHost->send( commandList[i].replace(QChar('\n'),"") );
    }
    mAutoCompletionTyped = "";
    mAutoCompletion = false;
    mTabCompletion = false;
    mTabCompletionCount = -1;
    mAutoCompletionCount = -1;
    selectAll();
}

void TCommandLine::slot_sendCommand(const char * pS)
{
    mpHost->sendRaw( QString(pS) );    
}

// TAB completion mode gets turned on by the tab key.
// This mode tries to find suitable matches for the letters being typed by the user
// in the output buffer of data being sent by the MUD. This helps the user
// to quickly type complicated names by only having to type the first letters.
// You can cycle through all possible matches of the currently typed letters
// with by repeatedly pressing tab or shift+tab. ESC-key brings you back into regular mode

void TCommandLine::handleTabCompletion( bool direction )
{
    if( ( mTabCompletionCount < 0 ) || ( mUserKeptOnTyping ) )
    {
        mTabCompletionTyped = text();
        if( mTabCompletionTyped.size() == 0 ) return;
        mUserKeptOnTyping = false;
        mTabCompletionCount = 0;
    }
    QStringList bufferList = mpHost->getLastBuffer();
    QString buffer = bufferList.join("");
    buffer.replace(QChar('\n'), "" );
    QStringList wordList = buffer.split( QRegExp( "\\W+" ), QString::SkipEmptyParts );
    if( direction )
    {
        mTabCompletionCount++;
    }
    else
    {
        mTabCompletionCount--;
    }
    if( wordList.size() > 0 )
    {
        if( mTabCompletionTyped.endsWith(" ") ) return;
        QStringList typedList = mTabCompletionTyped.split( QRegExp( "\\W+" ), QString::SkipEmptyParts );
        QString lastWord;
        if( typedList.size() > 1 )
        {
            lastWord = typedList[typedList.size()-1];
        }
        else
        {
            lastWord = mTabCompletionTyped;
        }
        //TODO: speed optimization
        QStringList filterList = wordList.filter( QRegExp( "\\b"+lastWord+".*" ) );
        QMap<QString, int> propsalMap;
        for( int i=0; i<filterList.size(); i++ )
        {
            propsalMap[filterList[i]]=i;
        }
        filterList = propsalMap.uniqueKeys();
        setPalette( mTabCompletionPalette );
        
        if( filterList.size() > 0 )
        {
            if( mTabCompletionCount < 0 ) mTabCompletionCount ? (filterList.size()-1)>0 : 0;
            if( mTabCompletionCount >= filterList.size() ) mTabCompletionCount = 0;
            QString proposal = filterList[mTabCompletionCount];
            if( proposal.size() > 2 )
            {
                if( typedList.size() > 0 ) typedList.pop_back();
                QString userWords = typedList.join(" ");
                setText( QString( userWords + " " + proposal ).trimmed() );
            }
        }
    }
}

// Hitting the cursor up key gets you in autocompletion mode.
// in this mode mudlet tries to find matching substrings in the user's
// command history. Repeated usage of the cursur up key cycles through
// all possible matches. If the user keeps on typing more letters the
// the set of possible matches is getting smaller. The ESC key brings you back to regular mode

void TCommandLine::handleAutoCompletion()
{
    if( ! mAutoCompletion ) return; // enabled via cursor up
    if( hasSelectedText() ) setText( text().remove( selectedText()) );
    mAutoCompletionTyped = text();
    if( mAutoCompletionCount >= mHistoryMap.size() ) mAutoCompletionCount = 0;    
    if( mAutoCompletionCount < 0 )  
    {
        mUserKeptOnTyping = false;
        mAutoCompletionCount = 0;
        if( mAutoCompletionTyped.size() == 0 ) return;
    }
    mAutoCompletionCount++;
    QStringList wordList = mHistoryMap.uniqueKeys();
    if( wordList.size() > 0 )
    {
        QStringList filterList = wordList.filter( QRegExp( "^"+mAutoCompletionTyped ) );
        QMap<QString, int> propsalMap;
        for( int i=0; i<filterList.size(); i++ )
        {
            propsalMap[filterList[i]]=i;
        }
        filterList = propsalMap.uniqueKeys();
        setPalette( mAutoCompletionPalette );
        if( filterList.size() > 0 )
        {
            if( ( mAutoCompletionCount > (filterList.size() - 1 ) ) ) mAutoCompletionCount = 0;
            QString proposal = filterList[mAutoCompletionCount].replace( mAutoCompletionTyped, 0 );
            setText( mAutoCompletionTyped+proposal.trimmed() );
            setCursorPosition( mAutoCompletionTyped.size() );
            setSelection( text().size(), (-1*(proposal.size())) );
            return;
        }
    }
}

// cursor down: cycles chronologically through the command history.

void TCommandLine::historyDown(QKeyEvent *event)
{
    if( mAutoCompletion )
    {
        mAutoCompletionCount -= 2;
        handleAutoCompletion();
        return;
    }
    mHistoryBuffer++;
    if( mHistoryBuffer < mHistoryList.size() )
    {
        setText( mHistoryList[mHistoryBuffer] );
    }
    else
    {
        mHistoryBuffer = 0;
    }
}

// cursor up: turns on autocompletion mode and cycles through all possible matches
// In case nothing has been typed it cycles through the command history in 
// reverse order compared to cursor down.

void TCommandLine::historyUp(QKeyEvent *event)
{
    if( text().size() > 0 )
    {
        mAutoCompletion = true;
        handleAutoCompletion();
    }
}

