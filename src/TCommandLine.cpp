/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2019 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "Host.h"
#include <QRegularExpression>
#include "TConsole.h"
#include "TSplitter.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "mudlet.h"


TCommandLine::TCommandLine(Host* pHost, TConsole* pConsole, QWidget* parent)
: QPlainTextEdit(parent)
, mpHost(pHost)
, mpKeyUnit(pHost->getKeyUnit())
, mpConsole(pConsole)
, mTabCompletionCount()
, mAutoCompletionCount()
, mUserKeptOnTyping()
, mHistoryBuffer()
, mSelectionStart(0)
, mpHunspell(nullptr)
, mHunspellSuggestionNumber()
, mpHunspellSuggestionList()
{
    slot_changeSpellDict(mpHost->getSpellDic());

    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);

    setFont(mpHost->mDisplayFont);

    mRegularPalette.setColor(QPalette::Text, mpHost->mCommandLineFgColor); //QColor(0,0,192));
    mRegularPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    mRegularPalette.setColor(QPalette::Base, mpHost->mCommandLineBgColor); //QColor(255,255,225));

    setPalette(mRegularPalette);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setCenterOnScroll(false);
    setWordWrapMode(QTextOption::WrapAnywhere);
    setContentsMargins(0, 0, 0, 0);

    // We do NOT want the standard context menu to happen as we generate it
    // ourself:
    setContextMenuPolicy(Qt::PreventContextMenu);

    connect(mpHost, &Host::signal_changeSpellDict, this, &TCommandLine::slot_changeSpellDict);
}

TCommandLine::~TCommandLine()
{
    Hunspell_destroy(mpHunspell);
}

void TCommandLine::processNormalKey(QEvent* event)
{
    QPlainTextEdit::event(event);
    adjustHeight();

    if (mpHost->mAutoClearCommandLineAfterSend) {
        mHistoryBuffer = -1;
    } else {
        mHistoryBuffer = 0;
    }
    if (mTabCompletionOld != toPlainText()) {
        mUserKeptOnTyping = true;
        mAutoCompletionCount = -1;
    } else {
        mUserKeptOnTyping = false;
    }
    spellCheck();
}

bool TCommandLine::processPotentialKeyBinding(QKeyEvent* keyEvent)
{
    if (mpKeyUnit->processDataStream(keyEvent->key(), (int)keyEvent->modifiers())) {
        keyEvent->accept();
        return true;
    } else {
        return false;
    }
}

// This function overrides the QWidget::event() and should return true if the
// event was recognized, otherwise it should return false. If the recognized
// event was accepted (see QEvent::accepted), any further processing such as
// event propagation to the parent widget stops.
bool TCommandLine::event(QEvent* event)
{
    const Qt::KeyboardModifiers allModifiers = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier;
    if (event->type() == QEvent::KeyPress) {
        auto * ke = dynamic_cast<QKeyEvent*>(event);

        // Shortcut for keypad keys
        if ((ke->modifiers() & Qt::KeypadModifier) && mpKeyUnit->processDataStream(ke->key(), (int)ke->modifiers())) {
            ke->accept();
            return true;

        }

        switch (ke->key()) {
        case Qt::Key_Space:
            if ((ke->modifiers() & (allModifiers & ~(Qt::ShiftModifier))) == Qt::NoModifier) {
                // Ignore the <SHIFT> modifier only - keeps some users happy!
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                mTabCompletionTyped.clear();
                if (mpHost->mAutoClearCommandLineAfterSend) {
                    mHistoryBuffer = -1;
                } else {
                    mHistoryBuffer = 0;
                }
                mLastCompletion.clear();
                break;

            } else {
                // Process as a possible key binding if there are ANY modifiers
                // other than just a <SHIFT> one; may actaully be configured as
                // a non-breaking space when used with a modifier!
                return processPotentialKeyBinding(ke);

            }

        case Qt::Key_Backtab:
            // <BACKTAB> is usually internally generated by SHIFT used in
            // conjunction with TAB - so ignore just the SHIFT key:
            if ((ke->modifiers() & (allModifiers & ~(Qt::ShiftModifier))) == Qt::ControlModifier) {
                // Switch to PREVIOUS profile tab when used with <CTRL> (and
                // implicit <SHIFT>):
                int currentIndex = mudlet::self()->mpTabBar->currentIndex();
                int count = mudlet::self()->mpTabBar->count();
                if (currentIndex - 1 < 0) {
                    mudlet::self()->mpTabBar->setCurrentIndex(count - 1);
                } else {
                    mudlet::self()->mpTabBar->setCurrentIndex(currentIndex - 1);
                }
                ke->accept();
                return true;

            } else if ((ke->modifiers() & (allModifiers & ~(Qt::ShiftModifier))) ==  Qt::NoModifier) {
                // Process as plain <BACKTAB> - (ignoring implicit <SHIFT>)
                handleTabCompletion(false);
                adjustHeight();
                ke->accept();
                return true;

            } else {
                // Process as a possible key binding if there are ANY modifiers
                // other than just the ignored <SHIFT> and the possible <CTRL>:
                return processPotentialKeyBinding(ke);

            }

        case Qt::Key_Tab:
            if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
                // Switch to NEXT profile tab
                int currentIndex = mudlet::self()->mpTabBar->currentIndex();
                int count = mudlet::self()->mpTabBar->count();
                if (currentIndex + 1 < count) {
                    mudlet::self()->mpTabBar->setCurrentIndex(currentIndex + 1);
                } else {
                    mudlet::self()->mpTabBar->setCurrentIndex(0);
                }
                ke->accept();
                return true;

            }

            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                handleTabCompletion(true);
                ke->accept();
                return true;

            }

            // Process as a possible key binding if there are ANY modifiers
            // other than just the Ctrl one
            // CHECKME: What about system foreground application switching?
            return processPotentialKeyBinding(ke);

        case Qt::Key_unknown:
            qWarning() << "ERROR: key unknown!";
            break;

        case Qt::Key_Backspace:
            if ((ke->modifiers() & (allModifiers & ~(Qt::ControlModifier|Qt::ShiftModifier))) == Qt::NoModifier) {
                // Ignore state of <CTRL> and <SHIFT> keys
                if (mpHost->mAutoClearCommandLineAfterSend) {
                    mHistoryBuffer = -1;
                } else {
                    mHistoryBuffer = 0;
                }

                if (mTabCompletionTyped.size() >= 1) {
                    mTabCompletionTyped.chop(1);
                }
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                mLastCompletion.clear();
                QPlainTextEdit::event(event);

                adjustHeight();

                return true;
            }
            // Process as a possible key binding if there are ANY modifiers
            // other than <CTRL> and/or <SHIFT>
            return processPotentialKeyBinding(ke);

        case Qt::Key_Delete:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                if (mpHost->mAutoClearCommandLineAfterSend) {
                    mHistoryBuffer = -1;
                } else {
                    mHistoryBuffer = 0;
                }

                if (mTabCompletionTyped.size() >= 1) {
                    mTabCompletionTyped.chop(1);
                } else {
                    mTabCompletionTyped.clear();
                    mUserKeptOnTyping = false;
                }
                mAutoCompletionCount = -1;
                mTabCompletionCount = -1;
                mLastCompletion.clear();
                QPlainTextEdit::event(event);
                adjustHeight();
                return true;

            }
            // Process as a possible key binding if there are ANY modifiers
            return processPotentialKeyBinding(ke);

        case Qt::Key_Return: // This is the main one (not the keypad)
            if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
                // If Ctrl-Return is pressed - scroll to the bottom of text:
                mpConsole->mLowerPane->mCursorY = mpConsole->buffer.size();
                mpConsole->mLowerPane->hide();
                mpConsole->buffer.mCursorY = mpConsole->buffer.size();
                mpConsole->mUpperPane->mCursorY = mpConsole->buffer.size();
                mpConsole->mUpperPane->mIsTailMode = true;
                mpConsole->mUpperPane->updateScreenView();
                mpConsole->mUpperPane->forceUpdate();
                ke->accept();
                return true;

            }

            if ((ke->modifiers() & allModifiers) == Qt::ShiftModifier) {
                textCursor().insertBlock();
                ke->accept();
                return true;

            }

            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                // Do the normal return key stuff only if NO modifiers are used:
                enterCommand(ke);
                mAutoCompletionCount = -1;
                mLastCompletion.clear();
                mTabCompletionTyped.clear();
                mUserKeptOnTyping = false;
                mTabCompletionCount = -1;
                if (mpHost->mAutoClearCommandLineAfterSend) {
                    clear();
                    mHistoryBuffer = -1;
                } else {
                    mHistoryBuffer = 0;
                }
                adjustHeight();
                ke->accept();
                return true;

            }

            // Process as a possible key binding if there are ANY modifiers,
            // other than just the Shift or just the Control modifiers
            return processPotentialKeyBinding(ke);

        case Qt::Key_Enter:
            // This is usually the Keypad one, so may come with
            // the keypad modifier - but if so it may never be reached because
            // of the keypad modifier interception done before this switch...
            if ((ke->modifiers() & (allModifiers & ~(Qt::KeypadModifier))) == Qt::NoModifier) {
                // Do the "normal" return key action if no or just the keypad
                // modifier is present:
                enterCommand(ke);
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                mLastCompletion.clear();
                mTabCompletionTyped.clear();
                mUserKeptOnTyping = false;
                if (mpHost->mAutoClearCommandLineAfterSend) {
                    clear();
                    mHistoryBuffer = -1;
                } else {
                    mHistoryBuffer = 0;
                }
                adjustHeight();
                ke->accept();
                return true;

            }
            // Process as a possible key binding if there are ANY modifiers,
            // other than just the Keypad modifier
            return processPotentialKeyBinding(ke);

        case Qt::Key_Down:
#if defined(Q_OS_MACOS)
            if ((ke->modifiers() & allModifiers) == (Qt::ControlModifier|Qt::KeypadModifier)) {
#else
            if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
#endif
                // If EXACTLY <Ctrl>-Down is pressed (special case for macOs -
                // also sets KeyPad modifier)
                moveCursor(QTextCursor::Down, QTextCursor::MoveAnchor);
                ke->accept();
                return true;
            }

#if defined(Q_OS_MACOS)
            if ((ke->modifiers() & allModifiers) == Qt::KeypadModifier) {
#else
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
#endif
                // If EXACTLY Down is pressed without modifiers (special case
                // for macOs - also sets KeyPad modifier)
                historyDown(ke);
                ke->accept();
                return true;

            }
            // Process as a possible key binding if there are ANY modifiers,
            // other than just the Control modifier (or keypad modifier on
            // macOs)
            return processPotentialKeyBinding(ke);

        case Qt::Key_Up:
#if defined(Q_OS_MACOS)
            if ((ke->modifiers() & allModifiers) == (Qt::ControlModifier|Qt::KeypadModifier)) {
#else
            if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
#endif
                // If EXACTLY <Ctrl>-Up is pressed (special case for macOs -
                // also sets KeyPad modifier)
                moveCursor(QTextCursor::Up, QTextCursor::MoveAnchor);
                ke->accept();
                return true;

            }

#if defined(Q_OS_MACOS)
            if ((ke->modifiers() & allModifiers) == Qt::KeypadModifier) {
#else
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
#endif
                // If EXACTLY Up is pressed without modifiers (special case for
                // macOs - also sets KeyPad modifier)
                historyUp(ke);
                ke->accept();
                return true;

            }

            // Process as a possible key binding if there are ANY modifiers,
            // other than just the Control modifier (or keypad modifier on
            // macOs)
            return processPotentialKeyBinding(ke);

        case Qt::Key_Escape:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                // Escape from tab completion mode if used with NO modifiers
                selectAll();
                mTabCompletionTyped.clear();
                mUserKeptOnTyping = false;
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                setPalette(mRegularPalette);
                if (mpHost->mAutoClearCommandLineAfterSend) {
                    mHistoryBuffer = -1;
                } else {
                    mHistoryBuffer = 0;
                }
                ke->accept();
                return true;

            }

            // Process as a possible key binding if there are ANY modifiers,
            return processPotentialKeyBinding(ke);

        case Qt::Key_PageUp:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                mpConsole->scrollUp(mpHost->mScreenHeight);
                ke->accept();
                return true;

            }

            // Process as a possible key binding if there are ANY modifiers,
            return processPotentialKeyBinding(ke);

        case Qt::Key_PageDown:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                mpConsole->scrollDown(mpHost->mScreenHeight);
                ke->accept();
                return true;

            }

            // Process as a possible key binding if there are ANY modifiers,
            return processPotentialKeyBinding(ke);

        case Qt::Key_C:
            if (((ke->modifiers() & allModifiers) == Qt::ControlModifier)
                && (mpConsole->mUpperPane->mSelectedRegion != QRegion(0, 0, 0, 0))) {

                // Only process as a Control-C if it is EXACTLY those two keys
                // and no other AND there is a selection active in the TConsole
                mpConsole->mUpperPane->slot_copySelectionToClipboard();
                ke->accept();
                return true;

            }

            // Process as a possible key binding if there are ANY modifiers,
            if (processPotentialKeyBinding(ke)) {
                return true;

            }

            processNormalKey(event);
            return false;

        default:
            // Process as a possible key binding if there are ANY modifiers,
            if (processPotentialKeyBinding(ke)) {
                return true;

            }

            processNormalKey(event);
            return false;

        }
    }

    return QPlainTextEdit::event(event);
}

void TCommandLine::focusInEvent(QFocusEvent* event)
{
    textCursor().movePosition(QTextCursor::Start);
    textCursor().movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, mSelectedText.length());

    mpConsole->mUpperPane->forceUpdate();
    mpConsole->mLowerPane->forceUpdate();
    QPlainTextEdit::focusInEvent(event);
}

void TCommandLine::focusOutEvent(QFocusEvent* event)
{
    if (textCursor().hasSelection()) {
        mSelectionStart = textCursor().selectionStart();
        mSelectedText = textCursor().selectedText();
    } else {
        mSelectionStart = 0;
        mSelectedText.clear();
    }
    QPlainTextEdit::focusOutEvent(event);
}

void TCommandLine::adjustHeight()
{
    int lines = document()->size().height();
    int fontH = QFontMetrics(mpHost->mDisplayFont).height();
    if (lines < 1) {
        lines = 1;
    }
    if (lines > 10) {
        lines = 10;
    }
    int _baseHeight = fontH * lines;
    int _height = _baseHeight + fontH;
    if (_height < mpHost->commandLineMinimumHeight) {
        _height = mpHost->commandLineMinimumHeight;
    }
    if (_height > height() || _height < height()) {
        mpConsole->layerCommandLine->setMinimumHeight(_height);
        mpConsole->layerCommandLine->setMaximumHeight(_height);
        int x = mpConsole->width();
        int y = mpConsole->height();
        QSize s = QSize(x, y);
        QResizeEvent event(s, s);
        QApplication::sendEvent(mpConsole, &event);
    }
}

void TCommandLine::spellCheck()
{
    if (!mpHost->mEnableSpellCheck) {
        return;
    }

    QTextCursor oldCursor = textCursor();
    QTextCharFormat f;
    QTextCursor c = textCursor();
    c.select(QTextCursor::WordUnderCursor);
    QByteArray encodedText = mpHunspellCodec->fromUnicode(c.selectedText());
    if (!Hunspell_spell(mpHunspell, encodedText.constData())) {
        // Word is misspelt
        f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        f.setUnderlineColor(Qt::red);
        f.setFontUnderline(true);
    } else {
        // Word is spelt correctly
        f.setFontUnderline(false);
    }
    c.setCharFormat(f);
    setTextCursor(c);
    f.setFontUnderline(false);
    oldCursor.setCharFormat(f);
    setTextCursor(oldCursor);
}

void TCommandLine::slot_popupMenu()
{
    auto* pA = qobject_cast<QAction*>(sender());
    if (!pA) {
        return;
    }
#if defined(Q_OS_FREEBSD)
    QString t = pA->data().toString();
#else
    QString t = pA->text();
#endif
    QTextCursor c = cursorForPosition(mPopupPosition);
    c.select(QTextCursor::WordUnderCursor);

    c.removeSelectedText();
    c.insertText(t);
    c.clearSelection();
    Hunspell_free_list(mpHunspell, &mpHunspellSuggestionList, mHunspellSuggestionNumber);
}

void TCommandLine::mousePressEvent(QMouseEvent* event)
{

    if (event->button() == Qt::RightButton) {
        auto popup = createStandardContextMenu(event->globalPos());
        QTextCursor c = cursorForPosition(event->pos());
        c.select(QTextCursor::WordUnderCursor);
        QByteArray encodedText = mpHunspellCodec->fromUnicode(c.selectedText());
        if (!Hunspell_spell(mpHunspell, encodedText.constData())) {
            // The word is NOT in the dictionary:
            char** sl;
            auto separator = popup->actions().first();
            separator = popup->insertSeparator(separator);
            // separator is now the QAction of, indeed is, a separator:
            QList<QAction*> spellings;

            // The return value is the count of suggestions:
            mHunspellSuggestionNumber = Hunspell_suggest(mpHunspell, &sl, encodedText.constData());
            if (mHunspellSuggestionNumber) {
                for (int i = 0; i < mHunspellSuggestionNumber; ++i) {
                    auto pA = new QAction(mpHunspellCodec->toUnicode(sl[i]));
#if defined(Q_OS_FREEBSD)
                    // Adding the text afterwards as user data as well as in the
                    // constructor is to fix a bug(?) in FreeBSD that
                    // automagically adds a '&' somewhere in the text to be a
                    // shortcut - but doesn't show it and forgets to remove
                    // it when asked for the text later:
                    pA->setData(mpHunspellCodec->toUnicode(sl[i]));
#endif
                    connect(pA, &QAction::triggered, this, &TCommandLine::slot_popupMenu);
                    spellings << pA;
                }

            } else {
                auto pA = new QAction(tr("no suggestions",
                                         // Intentional comment
                                         "used when the command spelling checker has no words to suggest"));
                pA->setEnabled(false);
                spellings << pA;
            }

            mpHunspellSuggestionList = sl;
            popup->insertActions(separator, spellings);
        }
        // else the word is in the dictionary - in either case show the context
        // menu - either the one with the prefixed spellings, or the standard
        // one:

        mPopupPosition = event->pos();
        popup->popup(event->globalPos());
        event->accept();
        return;
    }

    // Process any other possible mousePressEvent
    QPlainTextEdit::mousePressEvent(event);
}

void TCommandLine::enterCommand(QKeyEvent* event)
{
    QString _t = toPlainText();
    mTabCompletionCount = -1;
    mAutoCompletionCount = -1;
    mTabCompletionTyped.clear();

    QStringList _l = _t.split(QChar::LineFeed);
    for (int i = 0; i < _l.size(); i++) {
        mpHost->send(_l[i]);
    }
    if (!toPlainText().isEmpty()) {
        mHistoryBuffer = 0;
        setPalette(mRegularPalette);

        mHistoryList.removeAll(toPlainText());
        mHistoryList.push_front(toPlainText());
    }
    if (mpHost->mAutoClearCommandLineAfterSend) {
        clear();
    } else {
        selectAll();
    }
    adjustHeight();
}

// TAB completion mode gets turned on by the tab key.
// This mode tries to find suitable matches for the letters being typed by the user
// in the output buffer of data being sent by the MUD. This helps the user
// to quickly type complicated names by only having to type the first letters.
// You can cycle through all possible matches of the currently typed letters
// with by repeatedly pressing tab or shift+tab. ESC-key brings you back into regular mode

void TCommandLine::handleTabCompletion(bool direction)
{
    if ((mTabCompletionCount < 0) || (mUserKeptOnTyping)) {
        mTabCompletionTyped = toPlainText();
        if (mTabCompletionTyped.size() == 0) {
            return;
        }
        mUserKeptOnTyping = false;
        mTabCompletionCount = -1;
    }
    int amount = mpHost->mpConsole->buffer.size();
    if (amount > 500) {
        amount = 500;
    }

    QStringList bufferList = mpHost->mpConsole->buffer.getEndLines(amount);
    QString buffer = bufferList.join(QChar::Space);

    buffer.replace(QChar(0x21af), QChar::LineFeed);
    buffer.replace(QChar::LineFeed, QChar::Space);

    QStringList wordList = buffer.split(QRegularExpression(QStringLiteral(R"(\b)"), QRegularExpression::UseUnicodePropertiesOption), QString::SkipEmptyParts);
    if (direction) {
        mTabCompletionCount++;
    } else {
        mTabCompletionCount--;
    }
    if (!wordList.empty()) {
        if (mTabCompletionTyped.endsWith(QChar::Space)) {
            return;
        }
        QString lastWord;
        QRegularExpression reg = QRegularExpression(QStringLiteral(R"(\b(\w+)$)"), QRegularExpression::UseUnicodePropertiesOption);
        QRegularExpressionMatch match = reg.match(mTabCompletionTyped);
        int typePosition = match.capturedStart();
        if (reg.captureCount() >= 1) {
            lastWord = match.captured(1);
        } else {
            lastWord = QString();
        }

        QStringList filterList = wordList.filter(QRegularExpression(QStringLiteral(R"(^%1\w+)").arg(lastWord), QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption));
        if (filterList.empty()) {
            return;
        }
        int offset = 0;
        forever {
            QString tmp = filterList.back();
            filterList.removeAll(tmp);
            filterList.insert(offset, tmp);
            ++offset;
            if (offset >= filterList.size()) {
                break;
            }
        }

        if (!filterList.empty()) {
            if (mTabCompletionCount >= filterList.size()) {
                mTabCompletionCount = filterList.size() - 1;
            }
            if (mTabCompletionCount < 0) {
                mTabCompletionCount = 0;
            }
            QString proposal = filterList[mTabCompletionCount];
            QString userWords = mTabCompletionTyped.left(typePosition);
            setPlainText(QString(userWords + proposal).trimmed());
            moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
            mTabCompletionOld = toPlainText();
        }
    }
}

// Hitting the cursor up key gets you in autocompletion mode.
// in this mode mudlet tries to find matching substrings in the user's
// command history. Repeated usage of the cursor up key cycles through
// all possible matches. If the user keeps on typing more letters the
// the set of possible matches is getting smaller. The ESC key brings you back to regular mode

void TCommandLine::handleAutoCompletion()
{
    QString neu = toPlainText();
    neu.chop(textCursor().selectedText().size());
    setPlainText(neu);
    mTabCompletionOld = neu;
    int oldLength = toPlainText().size();
    if (mAutoCompletionCount >= mHistoryList.size()) {
        mAutoCompletionCount = mHistoryList.size() - 1;
    }
    if (mAutoCompletionCount < 0) {
        mAutoCompletionCount = 0;
    }
    for (int i = mAutoCompletionCount; i < mHistoryList.size(); i++) {
        QString h = mHistoryList[i].mid(0, neu.size());
        if (neu == h) {
            mAutoCompletionCount = i;
            mLastCompletion = mHistoryList[i];
            setPlainText(mHistoryList[i]);
            moveCursor(QTextCursor::Start);
            for (int k = 0; k < oldLength; k++) {
                moveCursor(QTextCursor::Right, QTextCursor::MoveAnchor);
            }
            moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
            return;
        } else {
            moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
        }
    }
    mAutoCompletionCount = -1;
}

// cursor down: cycles chronologically through the command history.

void TCommandLine::historyDown(QKeyEvent* event)
{
    if (mHistoryList.empty()) {
        return;
    }
    if ((textCursor().selectedText().size() == toPlainText().size()) || (toPlainText().size() == 0)) {
        mHistoryBuffer--;
        if (mHistoryBuffer >= mHistoryList.size()) {
            mHistoryBuffer = mHistoryList.size() - 1;
        }
        if (mHistoryBuffer < 0) {
            mHistoryBuffer = 0;
        }
        setPlainText(mHistoryList[mHistoryBuffer]);
        selectAll();
        adjustHeight();
    } else {
        mAutoCompletionCount--;
        handleAutoCompletion();
    }
}

// cursor up: turns on autocompletion mode and cycles through all possible matches
// In case nothing has been typed it cycles through the command history in
// reverse order compared to cursor down.

void TCommandLine::historyUp(QKeyEvent* event)
{
    if (mHistoryList.empty()) {
        return;
    }
    if ((textCursor().selectedText().size() == toPlainText().size()) || (toPlainText().size() == 0)) {
        if (toPlainText().size() != 0) {
            mHistoryBuffer++;
        }
        if (mHistoryBuffer >= mHistoryList.size()) {
            mHistoryBuffer = mHistoryList.size() - 1;
        }
        if (mHistoryBuffer < 0) {
            mHistoryBuffer = 0;
        }
        setPlainText(mHistoryList[mHistoryBuffer]);
        selectAll();
        adjustHeight();
    } else {
        mAutoCompletionCount++;
        handleAutoCompletion();
    }
}

void TCommandLine::slot_changeSpellDict(const QString& newDict)
{
    // This is duplicated (and should be the same as) the code in:
    // (void) dlgProfilePreferences::initWithHost(Host*)
    QString path;
#if defined(Q_OS_MACOS)
    path = QStringLiteral("%1/../Resources/").arg(QCoreApplication::applicationDirPath());
#elif defined(Q_OS_FREEBSD)
    if (QFile::exists(QStringLiteral("/usr/local/share/hunspell/%1.aff").arg(newDict))) {
        path = QLatin1String("/usr/local/share/hunspell/");
    } else if (QFile::exists(QStringLiteral("/usr/share/hunspell/%1.aff").arg(newDict))) {
        path = QLatin1String("/usr/share/hunspell/");
    } else {
        path = QLatin1String("./");
    }
#elif defined(Q_OS_LINUX)
    if (QFile::exists(QStringLiteral("/usr/share/hunspell/%1.aff").arg(newDict))) {
        path = QLatin1String("/usr/share/hunspell/");
    } else {
        path = QLatin1String("./");
    }
#else
    // Probably Windows!
    path = "./";
#endif

    QString spell_aff = QStringLiteral("%1%2.aff").arg(path, newDict);
    QString spell_dic = QStringLiteral("%1%2.dic").arg(path, newDict);
    // The man page for hunspell advises Utf8 encoding of the pathFileNames for
    // use on Windows platforms which can have non ASCII characters...
    if (mpHunspell) {
        Hunspell_destroy(mpHunspell);
    }
    mpHunspell = Hunspell_create(spell_aff.toUtf8().constData(), spell_dic.toUtf8().constData());
    if (mpHunspell) {
        mHunspellCodecName = QByteArray(Hunspell_get_dic_encoding(mpHunspell));
        qDebug().noquote().nospace() << "TCommandLine::slot_changeSpellDict(\"" << newDict << "\") INFO - Hunspell dictionary loaded, it uses a \"" << Hunspell_get_dic_encoding(mpHunspell) << "\" encoding...";
        mpHunspellCodec = QTextCodec::codecForName(mHunspellCodecName);
    }
}
