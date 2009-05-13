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
#include "TCommandLine.h"
#include <QShortcut>
#include "TConsole.h"
#include "TTextEdit.h"

TCommandLine::TCommandLine( Host * pHost, TConsole * pConsole, QWidget * parent )
: QLineEdit( parent )
, mpHost( pHost )
, mpConsole( pConsole )
, mSelectionStart( 0 )
, mSelectedText( "" )
{
    mpKeyUnit = mpHost->getKeyUnit();
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);

    QFont font("Courier New", 10, QFont::Courier);
    setFont(font);
    
    mRegularPalette.setColor(QPalette::Text,QColor(0,0,192));
    mRegularPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mRegularPalette.setColor(QPalette::Base,QColor(255,255,225));
    
    setPalette( mRegularPalette );

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
        //qDebug()<<"modifier="<<ke->modifiers()<<" key="<<ke->key();
        switch( ke->key() ) 
        {
            case Qt::Key_Space:
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                mTabCompletionTyped = "";
                mAutoCompletionTyped = "";
                mHistoryBuffer = 0;
                mLastCompletion = "";
                break;

            case Qt::Key_Backtab:
                handleTabCompletion( false );
                ke->accept();
                return true;
                break;

            case Qt::Key_Tab:
                handleTabCompletion( true );
                ke->accept();
                return true;
                break;

            case Qt::Key_unknown:
                qWarning()<<"ERROR: key unknown!";
                break;

            case Qt::Key_Backspace:
                if( mTabCompletionTyped.size() > 1 )
                {
                    mTabCompletionTyped.chop(1);
                    mAutoCompletionTyped.chop(1);
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    mLastCompletion = "";
                }
                break;

            case Qt::Key_Delete:
                if( mTabCompletionTyped.size() > 1 )
                {
                    mTabCompletionTyped.chop(1);
                    mAutoCompletionTyped.chop(1);
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    mLastCompletion = "";
                }
                break;

            case Qt::Key_Return:
                if( ke->modifiers() & Qt::ControlModifier )
                {
                     mpConsole->console->mCursorY = mpConsole->buffer.getLastLineNumber();
                     mpConsole->console->mIsTailMode = true;
                     mpConsole->console2->mCursorY = mpConsole->buffer.getLastLineNumber();
                     mpConsole->console2->hide();
                     ke->accept();
                     return true;
                }
                else
                {
                    enterCommand(ke);
                    ke->accept();
                    return true;
                }
                break;

            case Qt::Key_Enter:
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

                selectAll();
                mAutoCompletion = false;
                mTabCompletion = false;
                mTabCompletionTyped = "";
                mAutoCompletionTyped = "";
                mUserKeptOnTyping = false;
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                setPalette( mRegularPalette );
                ke->accept();
                return true;

            case Qt::Key_PageUp:
                mpConsole->scrollUp( mpHost->mScreenHeight );
                ke->accept();
                return true;

            case Qt::Key_PageDown:
                mpConsole->scrollDown( mpHost->mScreenHeight );
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
                    QLineEdit::event( event );

                    if( mTabCompletionOld.size() < text().size() )
                    {
                        mUserKeptOnTyping = true;
                        mAutoCompletionCount = 0;
                    }
                    else
                    {
                        mUserKeptOnTyping = false;
                    }
                    return false;
                }
        }
    } 
    return QLineEdit::event( event );
}

void TCommandLine::focusInEvent( QFocusEvent * event )
{
    setSelection( mSelectionStart, mSelectedText.length() );
    mpConsole->console->forceUpdate();
    mpConsole->console2->forceUpdate();
    QLineEdit::focusInEvent( event );
}

void TCommandLine::focusOutEvent( QFocusEvent * event )
{
    if( hasSelectedText() )
    {
        mSelectionStart = selectionStart();
        mSelectedText = selectedText();
    }
    QLineEdit::focusOutEvent( event );
}

void TCommandLine::enterCommand( QKeyEvent * event )
{
    mpHost->send( text() );
    if( text().size() > 0 )
    {
        mHistoryBuffer = 0;
        setPalette( mRegularPalette );

        mHistoryList.removeAll( text() );
        mHistoryList.push_front( text() );
    }

    mAutoCompletion = false;
    mTabCompletion = false;
    mTabCompletionCount = -1;
    mAutoCompletionCount = -1;
    mTabCompletionTyped = "";

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
    QStringList bufferList = mpHost->mpConsole->buffer.getEndLines( 200 );
    QString buffer = bufferList.join(" ");

    buffer.replace(QChar( 0x21af ), "\n");
    buffer.replace(QChar('\n'), " " );

    QStringList wordList = buffer.split( QChar(' '), QString::SkipEmptyParts );
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
        QStringList typedList = mTabCompletionTyped.split( " ", QString::SkipEmptyParts );
        QString lastWord;
        if( typedList.size() > 1 )
        {
            lastWord = typedList[typedList.size()-1];
        }
        else
        {
            lastWord = mTabCompletionTyped;
        }

        QStringList filterList = wordList.filter( QRegExp( "^"+lastWord+"\\w+",Qt::CaseInsensitive  ) );
        if( filterList.size() <= 1 ) return;
        //qDebug()<<"vor sortieren size="<<filterList.size()<<" filterList="<<filterList;
        int offset = 0;
        for( ; ; )
        {
            QString tmp = filterList.back();
            filterList.removeAll( tmp );
            filterList.insert( offset, tmp );
            ++offset;
            if( offset >= filterList.size() ) break;
        }
        //qDebug()<<"nachher size="<<filterList.size()<<" tab="<<mTabCompletionCount<<" filterList="<<filterList;

        if( filterList.size() > 0 )
        {
            if( mTabCompletionCount >= filterList.size() ) mTabCompletionCount = filterList.size()-1;
            if( mTabCompletionCount < filterList.size() )
            {
                if( mTabCompletionCount < 1 ) mTabCompletionCount = 1;
                QString proposal = filterList[mTabCompletionCount-1];
                if( typedList.size() > 0 ) typedList.pop_back();
                QString userWords = typedList.join(" ");
                setText( QString( userWords + " " + proposal ).trimmed() );
                mTabCompletionOld = text();
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
    QString neu = text();
    neu.chop( selectedText().size() );
    setText( neu );
    mTabCompletionOld = neu;
    qDebug()<<"after chop text=<"<<text()<<">";
    int oldLength = text().size();
    qDebug()<<"historyList="<<mHistoryList;
    if( mAutoCompletionCount >= mHistoryList.size() ) mAutoCompletionCount = mHistoryList.size()-1;
    if( mAutoCompletionCount <= 0 ) mAutoCompletionCount = 0;
    for( int i=mAutoCompletionCount; i<mHistoryList.size(); i++ )
    {
        if( text() == mHistoryList[i].mid( 0, text().size() ) )
        {
            mAutoCompletionCount = i;
            mLastCompletion = mHistoryList[i];
            setText( mHistoryList[i] );
            setSelection( oldLength, text().size() );
            return;
        }
    }
}

// cursor down: cycles chronologically through the command history.

void TCommandLine::historyDown(QKeyEvent *event)
{
    if( mHistoryList.size() < 1 ) return;
    if( (selectedText().size() == text().size()) || (text().size() == 0) )
    {
        if( mHistoryBuffer >= 1 )
        {
            mHistoryBuffer--;
            if( mHistoryBuffer >= 0 )
            {
                setText( mHistoryList[mHistoryBuffer] );
            }
            selectAll();
        }
        /*else
        {
            if( mHistoryBuffer < mHistoryList.size() )
            { 
                setText( mHistoryList[mHistoryBuffer] );
            }
        }*/
    }
    else
    {
        mAutoCompletionCount--;
        handleAutoCompletion();
    }
}

// cursor up: turns on autocompletion mode and cycles through all possible matches
// In case nothing has been typed it cycles through the command history in 
// reverse order compared to cursor down.

void TCommandLine::historyUp(QKeyEvent *event)
{
    if( mHistoryList.size() < 1 ) return;
    if( (selectedText().size() == text().size()) || (text().size() == 0) )
    {
        if( mHistoryBuffer < mHistoryList.size()-1 )
        {
            mHistoryBuffer++;
            setText( mHistoryList[mHistoryBuffer] );
            selectAll();
        }
        /*else
        {
            mHistoryBuffer = 0;
            if( mHistoryList.size() > 0 )
            {
                setText( mHistoryList[mHistoryBuffer]);
            }
            selectAll();
        }*/
    }
    else
    {
        mAutoCompletionCount++;
        handleAutoCompletion();
    }
}

