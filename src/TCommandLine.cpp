/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn (KoehnHeiko@googlemail.com)    *
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
#include "TCommandLine.h"
#include <QShortcut>
#include "TConsole.h"
#include "TTextEdit.h"
#include <QPlainTextEdit>
#include "TSplitter.h"
#include <hunspell/hunspell.h>

TCommandLine::TCommandLine( Host * pHost, TConsole * pConsole, QWidget * parent )
: QPlainTextEdit( parent )
, mpHost( pHost )
, mpConsole( pConsole )
, mSelectedText( "" )
, mSelectionStart( 0 )

{
    QString path;
#ifdef Q_OS_LINUX
    path = "/usr/share/hunspell/";
#else
    path = "./";
#endif

    QString spell_aff = path + pHost->mSpellDic + ".aff";
    QString spell_dic = path + pHost->mSpellDic + ".dic";
    mpHunspell = Hunspell_create( spell_aff.toLatin1().data(), spell_dic.toLatin1().data() );//"en_US.aff", "en_US.dic");
    mpKeyUnit = mpHost->getKeyUnit();
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);

    QFont font = mpHost->mDisplayFont;
    setFont(font);

    mRegularPalette.setColor(QPalette::Text, mpHost->mCommandLineFgColor );//QColor(0,0,192));
    mRegularPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mRegularPalette.setColor(QPalette::Base,mpHost->mCommandLineBgColor);//QColor(255,255,225));

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
    mAutoCompletion = false;
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setCenterOnScroll( false );
    setWordWrapMode( QTextOption::WrapAnywhere );
//    setMaximumBlockCount(1);
    setContentsMargins(0,0,0,0);
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
                if( mpHost->mAutoClearCommandLineAfterSend )
                    mHistoryBuffer = -1;
                else
                    mHistoryBuffer = 0;
                mLastCompletion = "";
                break;

            case Qt::Key_Backtab:
                handleTabCompletion( false );
                ke->accept();
                adjustHeight();
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
                if( mpHost->mAutoClearCommandLineAfterSend )
                    mHistoryBuffer = -1;
                else
                    mHistoryBuffer = 0;
                if( mTabCompletionTyped.size() >= 1 )
                {
                    mTabCompletionTyped.chop(1);
                    mAutoCompletionTyped.chop(1);
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    mLastCompletion = "";
                }
                else
                {
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    mLastCompletion = "";
                }
                QPlainTextEdit::event(event);

                adjustHeight();

                return true;

            case Qt::Key_Delete:
                if( mpHost->mAutoClearCommandLineAfterSend )
                    mHistoryBuffer = -1;
                else
                    mHistoryBuffer = 0;
                if( mTabCompletionTyped.size() >= 1 )
                {
                    mTabCompletionTyped.chop(1);
                    mAutoCompletionTyped.chop(1);
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    mLastCompletion = "";
                }
                else
                {
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    mLastCompletion = "";
                    mTabCompletionTyped = "";
                    mAutoCompletionTyped = "";
                    mUserKeptOnTyping = false;
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                }
                QPlainTextEdit::event(event);
                adjustHeight();
                return true;
                break;

            case Qt::Key_Return:
                if( ke->modifiers() & Qt::ControlModifier )
                {
                    mpConsole->console2->mCursorY = mpConsole->buffer.size();//
                    mpConsole->console2->hide();
                    mpConsole->buffer.mCursorY = mpConsole->buffer.size();
                    mpConsole->console->mCursorY = mpConsole->buffer.size();//
                    mpConsole->console->mIsTailMode = true;
                    mpConsole->console->updateScreenView();
                    mpConsole->console->forceUpdate();
                    ke->accept();
                    return true;
                }
                else if( ke->modifiers() & Qt::ShiftModifier )
                {
                    textCursor().insertBlock();
                    /*if( ! textCursor().movePosition(QTextCursor::Down, QTextCursor::KeepAnchor) )
                    {
                        textCursor().insertBlock();
                    }*/
                    ke->accept();
                    return true;
                }
                else
                {
                    enterCommand(ke);
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    mLastCompletion = "";
                    mTabCompletionTyped = "";
                    mAutoCompletionTyped = "";
                    mUserKeptOnTyping = false;
                    mTabCompletionCount = -1;
                    mAutoCompletionCount = -1;
                    if( mpHost->mAutoClearCommandLineAfterSend )
                    {
                        clear();
                        mHistoryBuffer = -1;
                    }
                    else
                        mHistoryBuffer = 0;
                    adjustHeight();
                    ke->accept();
                    return true;
                }
                break;

            case Qt::Key_Enter:
                enterCommand(ke);
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                mLastCompletion = "";
                mTabCompletionTyped = "";
                mAutoCompletionTyped = "";
                mUserKeptOnTyping = false;
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                if( mpHost->mAutoClearCommandLineAfterSend )
                {
                    clear();
                    mHistoryBuffer = -1;
                }
                else
                    mHistoryBuffer = 0;
                adjustHeight();
                ke->accept();
                return true;
                break;

            case Qt::Key_Down:
                if( ke->modifiers() & Qt::ControlModifier )
                {
                    moveCursor(QTextCursor::Down, QTextCursor::MoveAnchor);
                    ke->accept();
                    return true;
                }
                else
                {
                    historyDown(ke);
                    ke->accept();
                    return true;
                }
                break;

            case Qt::Key_Up:
                if( ke->modifiers() & Qt::ControlModifier )
                {
                    moveCursor(QTextCursor::Up, QTextCursor::MoveAnchor );
                    ke->accept();
                    return true;
                }
                else
                {
                    historyUp(ke);
                    ke->accept();
                    return true;
                }
                break;

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
                if( mpHost->mAutoClearCommandLineAfterSend )
                    mHistoryBuffer = -1;
                else
                    mHistoryBuffer = 0;
                ke->accept();
                return true;
                break;

            case Qt::Key_PageUp:
                mpConsole->scrollUp( mpHost->mScreenHeight );
                ke->accept();
                return true;
                break;

            case Qt::Key_PageDown:
                mpConsole->scrollDown( mpHost->mScreenHeight );
                ke->accept();
                return true;
                break;

            case Qt::Key_C:
                if( ke->modifiers() & Qt::ControlModifier )
                {
                     if( mpConsole->console->mSelectedRegion != QRegion( 0, 0, 0, 0 ) )
                     {
                         mpConsole->console->copySelectionToClipboard();
                         ke->accept();
                         return true;
                     }
                }
                break;

            default:

                if( mpKeyUnit->processDataStream( ke->key(), (int)ke->modifiers() ) )
                {
                    ke->accept();
                    return true;
                }
                else
                {
                    QPlainTextEdit::event( event );
                    adjustHeight();

                    if( mpHost->mAutoClearCommandLineAfterSend )
                        mHistoryBuffer = -1;
                    else
                        mHistoryBuffer = 0;
                    if( mTabCompletionOld != toPlainText() )//text() )
                    {
                        mUserKeptOnTyping = true;
                        mAutoCompletionCount = -1;
                    }
                    else
                    {
                        mUserKeptOnTyping = false;
                    }
                    spellCheck();
                    return false;
                }
        }

    }

    return QPlainTextEdit::event( event );
}

void TCommandLine::focusInEvent( QFocusEvent * event )
{
    //setSelection( mSelectionStart, mSelectedText.length() );
    textCursor().movePosition(QTextCursor::Start);
    textCursor().movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, mSelectedText.length() );

    mpConsole->console->forceUpdate();
    mpConsole->console2->forceUpdate();
    QPlainTextEdit::focusInEvent( event );
}

void TCommandLine::focusOutEvent( QFocusEvent * event )
{
    if( textCursor().hasSelection() )//hasSelectedText() )
    {
        mSelectionStart = textCursor().selectionStart();
        mSelectedText = textCursor().selectedText();
    }
    else
    {
        mSelectionStart = 0;
        mSelectedText = "";
    }
    QPlainTextEdit::focusOutEvent( event );
}

void TCommandLine::adjustHeight()
{
    int lines = document()->size().height();
    int fontH = QFontMetrics( mpHost->mDisplayFont ).height();
    if( lines < 1 ) lines = 1;
    if( lines > 10 ) lines = 10;
    int _baseHeight = fontH * lines;
    int _height = _baseHeight + fontH;
    if( _height < mpHost->commandLineMinimumHeight )
        _height = mpHost->commandLineMinimumHeight;
    if( _height > height() || _height < height() )
    {
        mpConsole->layerCommandLine->setMinimumHeight( _height );
        mpConsole->layerCommandLine->setMaximumHeight( _height );
        int x = mpConsole->width();
        int y = mpConsole->height();
        QSize s = QSize(x,y);
        QResizeEvent event(s, s);
        QApplication::sendEvent( mpConsole, &event);
    }
}

void TCommandLine::spellCheck()
{
    if( ! mpHost->mEnableSpellCheck ) return;

    QTextCursor oldCursor = textCursor();
    QTextCharFormat f;
    QColor cred = QColor(255,0,0);
    f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    f.setUnderlineColor(cred);
    QTextCursor c = textCursor();
    c.select(QTextCursor::WordUnderCursor);

    if( ! Hunspell_spell( mpHunspell, c.selectedText().toLatin1().data()) )
    {
        f.setFontUnderline(true);
        c.setCharFormat(f);
    }
    else
    {
        f.setFontUnderline(false);
        c.setCharFormat(f);
    }
    setTextCursor(c);
    f.setFontUnderline(false);
    oldCursor.setCharFormat(f);
    setTextCursor(oldCursor);
}

void TCommandLine::slot_popupMenu()
{
    QAction * pA = (QAction *)sender();
    if( ! pA )
    {
        return;
    }
    QString t = pA->text();
    QTextCursor c = cursorForPosition( mPopupPosition );
    c.select(QTextCursor::WordUnderCursor);

    c.removeSelectedText();
    c.insertText( t );
    c.clearSelection();
    Hunspell_free_list( mpHunspell, &mpHunspellSuggestionList, mHunspellSuggestionNumber );
}

void TCommandLine::mousePressEvent( QMouseEvent * event )
{
    if( event->button() == Qt::RightButton )
    {
        QTextCursor c = cursorForPosition( event->pos() );
        c.select(QTextCursor::WordUnderCursor);

        if( ! Hunspell_spell( mpHunspell, c.selectedText().toLatin1().data()) )
        {
            char ** sl;
            mHunspellSuggestionNumber = Hunspell_suggest( mpHunspell, &sl, c.selectedText().toLatin1().data() );
            QMenu * popup = new QMenu( this );
            for( int i=0; i<mHunspellSuggestionNumber; i++ )
            {
                QAction * pA;
                pA = popup->addAction( sl[i] );
                connect( pA, SIGNAL(triggered()), this, SLOT(slot_popupMenu()));
            }
            mpHunspellSuggestionList = sl;
            mPopupPosition = event->pos();
            popup->popup( event->globalPos() );
        }

        event->accept();
        return;
    }
    QPlainTextEdit::mousePressEvent( event );
}

void TCommandLine::enterCommand( QKeyEvent * event )
{
    QString _t = toPlainText();
    mAutoCompletion = false;
    mTabCompletion = false;
    mTabCompletionCount = -1;
    mAutoCompletionCount = -1;
    mTabCompletionTyped = "";
    if( mpHost->mAutoClearCommandLineAfterSend )
        clear();
    else
    {
        selectAll();
    }
    adjustHeight();

    QStringList _l = _t.split("\n");
    for( int i=0; i<_l.size(); i++ )
    {
        mpHost->send( _l[i] );
    }
    if( toPlainText().size() > 0 )
    {
        mHistoryBuffer = 0;
        setPalette( mRegularPalette );

        mHistoryList.removeAll( toPlainText() );
        mHistoryList.push_front( toPlainText() );
    }





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
        mTabCompletionTyped = toPlainText();
        if( mTabCompletionTyped.size() == 0 ) return;
        mUserKeptOnTyping = false;
        mTabCompletionCount = -1;
    }
    int amount = mpHost->mpConsole->buffer.size();
    if( amount > 500 ) amount = 500;

    QStringList bufferList = mpHost->mpConsole->buffer.getEndLines( amount );
    QString buffer = bufferList.join(" ");

    buffer.replace(QChar( 0x21af ), "\n");
    buffer.replace(QChar('\n'), " " );

    QStringList wordList = buffer.split( QRegExp("\\b"), QString::SkipEmptyParts );
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
        QString lastWord;
        QRegExp reg = QRegExp("\\b(\\w+)$");
        int typePosition = reg.indexIn( mTabCompletionTyped );
        if( reg.numCaptures() >= 1 )
            lastWord = reg.cap( 1 );
        else
            lastWord = "";
        QStringList filterList = wordList.filter( QRegExp( "^"+lastWord+"\\w+",Qt::CaseInsensitive  ) );
        if( filterList.size() < 1 ) return;
        int offset = 0;
        for( ; ; )
        {
            QString tmp = filterList.back();
            filterList.removeAll( tmp );
            filterList.insert( offset, tmp );
            ++offset;
            if( offset >= filterList.size() ) break;
        }

        if( filterList.size() > 0 )
        {
            if( mTabCompletionCount >= filterList.size() ) mTabCompletionCount = filterList.size()-1;
            if( mTabCompletionCount < 0 ) mTabCompletionCount = 0;
            QString proposal = filterList[mTabCompletionCount];
            QString userWords = mTabCompletionTyped.left( typePosition );
            setPlainText( QString( userWords + proposal ).trimmed() );
            moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
            mTabCompletionOld = toPlainText();
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
    QString neu = toPlainText();
    neu.chop( textCursor().selectedText().size() );
    setPlainText( neu );
    mTabCompletionOld = neu;
    int oldLength = toPlainText().size();
    if( mAutoCompletionCount >= mHistoryList.size() ) mAutoCompletionCount = mHistoryList.size()-1;
    if( mAutoCompletionCount < 0 ) mAutoCompletionCount = 0;
    for( int i=mAutoCompletionCount; i<mHistoryList.size(); i++ )
    {
        QString h = mHistoryList[i].mid( 0, neu.size() );
        if( neu == h )
        {
            mAutoCompletionCount = i;
            mLastCompletion = mHistoryList[i];
            setPlainText( mHistoryList[i] );
            moveCursor( QTextCursor::Start );
            for( int k=0; k<oldLength; k++ )
            {
                moveCursor( QTextCursor::Right, QTextCursor::MoveAnchor );
            }
            moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
            return;
        }
        else
        {
            moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
        }
    }
    mAutoCompletionCount = -1;
}

// cursor down: cycles chronologically through the command history.

void TCommandLine::historyDown(QKeyEvent *event)
{
    if( mHistoryList.size() < 1 ) return;
    if( (textCursor().selectedText().size() == toPlainText().size()) || (toPlainText().size() == 0) )
    {
        mHistoryBuffer--;
        if( mHistoryBuffer >= mHistoryList.size() ) mHistoryBuffer = mHistoryList.size()-1;
        if( mHistoryBuffer < 0 ) mHistoryBuffer = 0;
        setPlainText( mHistoryList[mHistoryBuffer] );
        selectAll();
        adjustHeight();
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
    if( (textCursor().selectedText().size() == toPlainText().size()) || (toPlainText().size() == 0) )
    {
        if( toPlainText().size() != 0) mHistoryBuffer++;
        if( mHistoryBuffer >= mHistoryList.size() ) mHistoryBuffer = mHistoryList.size()-1;
        if( mHistoryBuffer < 0 ) mHistoryBuffer = 0;
        setPlainText( mHistoryList[mHistoryBuffer] );
        selectAll();
        adjustHeight();
    }
    else
    {
        mAutoCompletionCount++;
        handleAutoCompletion();
    }
}

