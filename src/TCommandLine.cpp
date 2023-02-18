/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2020, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2023 by Lecker Kebap - Leris@mudlet.org                 *
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
#include "TConsole.h"
#include "TMainConsole.h"
#include "TSplitter.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TEvent.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollBar>
#include "post_guard.h"

TCommandLine::TCommandLine(Host* pHost, const QString& name, CommandLineType type, TConsole* pConsole, QWidget* parent)
: QPlainTextEdit(parent)
, mCommandLineName(name)
, mpHost(pHost)
, mType(type)
, mpKeyUnit(pHost->getKeyUnit())
, mpConsole(pConsole)
{
    setObjectName(qsl("commandLine_%1_%2").arg(mpHost->getName(), name));

    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);

    setFont(mpHost->getDisplayFont());
    document()->setDocumentMargin(2);

    mRegularPalette.setColor(QPalette::Text, mpHost->mCommandLineFgColor);
    mRegularPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    mRegularPalette.setColor(QPalette::Base, mpHost->mCommandLineBgColor);

    setPalette(mRegularPalette);
    //style subCommandLines by stylesheet
    if (mType != MainCommandLine) {
        QColor c = mpHost->mCommandLineBgColor;
        QString styleSheet{qsl("QPlainTextEdit{background-color: rgb(%1, %2, %3);}").arg(c.red()).arg(c.green()).arg(c.blue())};
        setStyleSheet(styleSheet);
    }

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setCenterOnScroll(false);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setContentsMargins(0, 0, 0, 0);
    // clear console selection if selection in command line changes
    connect(this, &QPlainTextEdit::copyAvailable, this, &TCommandLine::slot_clearSelection);
    // We do NOT want the standard context menu to happen as we generate it
    // ourself:
    setContextMenuPolicy(Qt::PreventContextMenu);

    connect(mudlet::self(), &mudlet::signal_adjustAccessibleNames, this, &TCommandLine::slot_adjustAccessibleNames);
    slot_adjustAccessibleNames();
}

void TCommandLine::processNormalKey(QEvent* event)
{
    QPlainTextEdit::event(event);
    adjustHeight();

    mHistoryBuffer = 0;
    if (mTabCompletionOld != toPlainText()) {
        mUserKeptOnTyping = true;
        mAutoCompletionCount = -1;
    } else {
        mUserKeptOnTyping = false;
    }
    spellCheck();
}

bool TCommandLine::keybindingMatched(QKeyEvent* keyEvent)
{
    if (mpKeyUnit->processDataStream(static_cast<Qt::Key>(keyEvent->key()), static_cast<Qt::KeyboardModifiers>(keyEvent->modifiers()))) {
        keyEvent->accept();
        return true;
    }

    return false;
}

// This function overrides the QWidget::event() and should return true if the
// event was recognized, otherwise it should return false. If the recognized
// event was accepted (see QEvent::accepted), any further processing such as
// event propagation to the parent widget stops.
bool TCommandLine::event(QEvent* event)
{
    const Qt::KeyboardModifiers allModifiers = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier;
    if (event->type() == QEvent::KeyPress) {
        auto* ke = dynamic_cast<QKeyEvent*>(event);
        if (!ke) {
            // Something is wrong -
            qCritical().noquote() << "TCommandLine::event(QEvent*) CRITICAL - a QEvent that is supposed to be a QKeyEvent is not dynamically castable to the latter - so the processing of this event "
                                     "has been aborted - please report this to Mudlet Makers.";
            // Indicate that we don't want to touch this event with a barge-pole!
            return false;
        }

        if (ke->matches(QKeySequence::Copy)){ // Copy is Ctrl+C and possibly Ctrl+Ins, F16
            if (mpConsole->mUpperPane->mSelectedRegion != QRegion(0, 0, 0, 0)) {
                // Only process if there is a selection active in the TConsole
                mpConsole->mUpperPane->slot_copySelectionToClipboard();
                ke->accept();
                return true;
            }
        }

        if (ke->matches(QKeySequence::Find)){ // Find is Ctrl+F
            if (mudlet::self()->dactionInputLine->isChecked()) {
                // If hidden then reveal as if pressed Alt-L
                mudlet::self()->dactionInputLine->setChecked(false);
                mudlet::self()->mpCurrentActiveHost->setCompactInputLine(false);
            }
            mpConsole->mpBufferSearchBox->setFocus();
            ke->accept();
            return true;
        }

        // Shortcut for keypad keys
        if ((ke->modifiers() & Qt::KeypadModifier) && mpKeyUnit->processDataStream(static_cast<Qt::Key>(ke->key()), static_cast<Qt::KeyboardModifiers>(ke->modifiers()))) {
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
                mHistoryBuffer = 0;
                mLastCompletion.clear();
                break;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers
                // other than just a <SHIFT> one; may actually be configured as
                // a non-breaking space when used with a modifier!
                return true;
            }
            break;

        case Qt::Key_Backtab:
            // <BACKTAB> is usually internally generated by SHIFT used in
            // conjunction with TAB - so ignore just the SHIFT key:
            if ((ke->modifiers() & (allModifiers & ~(Qt::ShiftModifier))) == Qt::ControlModifier) {
                // Switch to PREVIOUS profile tab when used with <CTRL> (and
                // implicit <SHIFT>):
                int currentIndex = mudlet::self()->mpTabBar->currentIndex();
                int count = mudlet::self()->mpTabBar->count();
                if (currentIndex - 1 < 0) {
                    mudlet::self()->slot_tabChanged(count - 1);
                } else {
                    mudlet::self()->slot_tabChanged(currentIndex - 1);
                }
                ke->accept();
                return true;
            }

            if ((ke->modifiers() & (allModifiers & ~(Qt::ShiftModifier))) ==  Qt::NoModifier) {
                // Process as plain <BACKTAB> - (ignoring implicit <SHIFT>)
                handleTabCompletion(false);
                adjustHeight();
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers
                // other than just the ignored <SHIFT> and the possible <CTRL>:
                return true;
            }
            break;

        case Qt::Key_Tab:
            if ((mpHost->mCaretShortcut == Host::CaretShortcut::Tab && !(ke->modifiers() & Qt::ControlModifier)) ||
                (mpHost->mCaretShortcut == Host::CaretShortcut::CtrlTab && (ke->modifiers() & Qt::ControlModifier))) {
                mpHost->setCaretEnabled(true);
                ke->accept();
                return true;
            }

            if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
                // Switch to NEXT profile tab
                int currentIndex = mudlet::self()->mpTabBar->currentIndex();
                int count = mudlet::self()->mpTabBar->count();
                if (currentIndex + 1 < count) {
                    mudlet::self()->slot_tabChanged(currentIndex + 1);
                } else {
                    mudlet::self()->slot_tabChanged(0);
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
            if (keybindingMatched(ke)) {
                return true;
            }
            break;

        case Qt::Key_F6:
            if (mpHost->mCaretShortcut == Host::CaretShortcut::F6) {
                mpHost->setCaretEnabled(true);
                ke->accept();
                return true;
            }
            break;

        case Qt::Key_unknown:
            qWarning() << "ERROR: key unknown!";
            break;

        case Qt::Key_Backspace:
            if ((ke->modifiers() & (allModifiers & ~(Qt::ControlModifier|Qt::ShiftModifier))) == Qt::NoModifier) {
                // Ignore state of <CTRL> and <SHIFT> keys
                mHistoryBuffer = 0;

                if (!mTabCompletionTyped.isEmpty()) {
                    mTabCompletionTyped.chop(1);
                }
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                mLastCompletion.clear();
                QPlainTextEdit::event(event);

                adjustHeight();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers
                // other than <CTRL> and/or <SHIFT>
                return true;
            }
            break;

        case Qt::Key_Delete:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                mHistoryBuffer = 0;

                if (!mTabCompletionTyped.isEmpty()) {
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

            if (keybindingMatched(ke)) {
                return true;
            }
            break;

        case Qt::Key_Return: // This is the main one (not the keypad)
            if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
                // If Ctrl-Return is pressed - scroll to the bottom of text:
                mpConsole->mLowerPane->mCursorY = mpConsole->buffer.size();
                mpConsole->mLowerPane->hide();
                mpConsole->buffer.mCursorY = mpConsole->buffer.size();
                mpConsole->mUpperPane->mCursorY = mpConsole->buffer.size();
                mpConsole->mUpperPane->mCursorX = 0;
                mpConsole->mUpperPane->mIsTailMode = true;
                mpConsole->mUpperPane->updateScreenView();
                mpConsole->mUpperPane->forceUpdate();
                ke->accept();
                return true;

            }

            if ((ke->modifiers() & allModifiers) == Qt::ShiftModifier) {
                textCursor().insertBlock();
                ke->accept();
                adjustHeight();
                return true;

            }

            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                // Do the normal return key stuff only if NO modifiers are used:
                enterCommand(ke);
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers,
                // other than just the Shift or just the Control modifiers
                return true;
            }
            break;

        case Qt::Key_Enter:
            // This is usually the Keypad one, so may come with
            // the keypad modifier - but if so it may never be reached because
            // of the keypad modifier interception done before this switch...
            if ((ke->modifiers() & (allModifiers & ~(Qt::KeypadModifier))) == Qt::NoModifier) {
                // Do the "normal" return key action if no or just the keypad
                // modifier is present:
                enterCommand(ke);
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers,
                // other than just the Keypad modifier
                return true;
            }
            break;

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
                historyMove(MOVE_DOWN);
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers,
                // other than just the Control modifier (or keypad modifier on
                // macOs)
                return true;
            }
            break;

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
                historyMove(MOVE_UP);
                ke->accept();
                return true;

            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers,
                // other than just the Control modifier (or keypad modifier on
                // macOs)
                return true;
            }
            break;

        case Qt::Key_Escape:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                // Escape from tab completion mode if used with NO modifiers
                selectAll();
                mTabCompletionTyped.clear();
                mUserKeptOnTyping = false;
                mTabCompletionCount = -1;
                mAutoCompletionCount = -1;
                mHistoryBuffer = 0;
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers,
                return true;
            }
            break;

        case Qt::Key_PageUp:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                mpConsole->scrollUp(0);
                QTimer::singleShot(0, [this]() {  mpConsole->scrollUp(mpConsole->mUpperPane->getScreenHeight()); });
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers,
                return true;
            }
            break;

        case Qt::Key_PageDown:
            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                mpConsole->scrollDown(mpConsole->mUpperPane->getScreenHeight());
                ke->accept();
                return true;

            }
            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers,
                return true;
            }
            break;

        case Qt::Key_1:
            if (handleCtrlTabChange(ke, 1)) {
                return true;
            }
            return false;

        case Qt::Key_2:
            if (handleCtrlTabChange(ke, 2)) {
                return true;
            }
            return false;

        case Qt::Key_3:
            if (handleCtrlTabChange(ke, 3)) {
                return true;
            }
            return false;

        case Qt::Key_4:
            if (handleCtrlTabChange(ke, 4)) {
                return true;
            }
            return false;

        case Qt::Key_5:
            if (handleCtrlTabChange(ke, 5)) {
                return true;
            }
            return false;

        case Qt::Key_6:
            if (handleCtrlTabChange(ke, 6)) {
                return true;
            }
            return false;

        case Qt::Key_7:
            if (handleCtrlTabChange(ke, 7)) {
                return true;
            }
            return false;

        case Qt::Key_8:
            if (handleCtrlTabChange(ke, 8)) {
                return true;
            }
            return false;

        case Qt::Key_9:
            if (handleCtrlTabChange(ke, 9)) {
                return true;
            }
            return false;

        case Qt::Key_0:
            if (handleCtrlTabChange(ke, 10)) {
                return true;
            }
            return false;

        default:
            // Process as a possible key binding if there are ANY modifiers
            if (keybindingMatched(ke)) {
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

    // Make sure this profile's tab gets activated in multi-view mode:
    mudlet::self()->activateProfile(mpHost);

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

void TCommandLine::hideEvent(QHideEvent* event)
{
    if (hasFocus()) {
        mudlet::self()->mpCurrentActiveHost->mpConsole->mpCommandLine->setFocus();
    }
    QPlainTextEdit::hideEvent(event);
}

void TCommandLine::adjustHeight()
{
    // Make sure adjustHeight won't crash if it's used before mpConsole->layerCommandLine has a value
    if (!mpConsole->layerCommandLine) {
        qWarning() << "TCommandLine::adjustHeight() ERROR: mpConsole->layerCommandLine is NULL!";
        return;
    }
    int lines = document()->size().height();
    // Workaround for SubCommandLines textCursor not visible in some situations
    // SubCommandLines cannot autoresize
    if (mType == SubCommandLine) {
        if (lines <= 1) {
            verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
        }
        return;
    }
    if (lines < 1) {
        lines = 1;
    }
    if (lines > 10) {
        lines = 10;
    }
    int fontH = QFontMetrics(font()).height();
    // Adjust height margin based on font size and if it is more than one row
    int marginH = lines > 1 ? 2+fontH/3 : 5;
    if (lines > 1 && marginH < 8) {
        marginH = 8; // needed for very small fonts
    }
    int _height = fontH * lines + marginH;
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
    if (!mpHost || !mpHost->mEnableSpellCheck) {
        return;
    }

    QTextCursor oldCursor = textCursor();
    QTextCursor c = textCursor();
    spellCheckWord(c);
    QTextCharFormat f;
    f.setFontUnderline(false);
    oldCursor.setCharFormat(f);
    setTextCursor(oldCursor);
}

void TCommandLine::slot_popupMenu()
{
    auto* pA = qobject_cast<QAction*>(sender());
    if (!mpHost || !pA) {
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
    auto systemDictionaryHandle = mpHost->mpConsole->getHunspellHandle_system();
    if (systemDictionaryHandle) {
        Hunspell_free_list(mpHost->mpConsole->getHunspellHandle_system(), &mpSystemSuggestionsList, mSystemDictionarySuggestionsCount);
    }
    auto userDictionaryHandle = mpHost->mpConsole->getHunspellHandle_user();
    if (userDictionaryHandle) {
        Hunspell_free_list(userDictionaryHandle, &mpUserSuggestionsList, mUserDictionarySuggestionsCount);
    }

    // Call the function again so that the replaced word gets rechecked:
    spellCheck();
}

void TCommandLine::fillSpellCheckList(QMouseEvent* event, QMenu* popup) {
    QTextCursor c = cursorForPosition(event->pos());
    c.select(QTextCursor::WordUnderCursor);
    mSpellCheckedWord = c.selectedText();

    if (bool IsWordLongEnoughForSpellCheck = mSpellCheckedWord.size() > 2; !IsWordLongEnoughForSpellCheck) {
        return;
    }

    auto codec = mpHost->mpConsole->getHunspellCodec_system();
    auto handle_system = mpHost->mpConsole->getHunspellHandle_system();
    auto handle_profile = mpHost->mpConsole->getHunspellHandle_user();
    bool haveAddOption = false;
    bool haveRemoveOption = false;
    QAction* action_addWord = nullptr;
    QAction* action_removeWord = nullptr;
    QAction* action_dictionarySeparatorLine = nullptr;
    if (handle_profile) {
        // TODO: Make icons for these?
        // if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        //    action_addWord = new QAction(QIcon(QPixmap(qsl(":/icons/dictionary-add-word.png"))), tr("Add to user dictionary"));
        //    action_removeWord = new QAction(QIcon(QPixmap(qsl(":/icons/dictionary-remove-word.png"))), tr("Remove from user dictionary"));
        // } else {
        action_addWord = new QAction(tr("Add to user dictionary"));
        action_addWord->setEnabled(false);
        action_removeWord = new QAction(tr("Remove from user dictionary"));
        action_removeWord->setEnabled(false);
        // }
        if (mudlet::self()->mUsingMudletDictionaries) {
            action_dictionarySeparatorLine = new QAction(tr("▼Mudlet▼ │ dictionary suggestions │ ▲User▲",
                                                                    // Intentional separator
                                                                    "This line is shown in the list of spelling suggestions on the profile's command-"
                                                                    "line context menu to clearly divide up where the suggestions for correct "
                                                                    "spellings are coming from.  The precise format might be modified as long as it "
                                                                    "is clear that the entries below this line in the menu come from the spelling "
                                                                    "dictionary that the user has chosen in the profile setting which we have "
                                                                    "bundled with Mudlet; the entries about this line are the ones that the user "
                                                                    "has personally added."));
        } else {
            action_dictionarySeparatorLine = new QAction(tr("▼System▼ │ dictionary suggestions │ ▲User▲",
                                                                    // Intentional separator
                                                                    "This line is shown in the list of spelling suggestions on the profile's command-"
                                                                    "line context menu to clearly divide up where the suggestions for correct "
                                                                    "spellings are coming from.  The precise format might be modified as long as it "
                                                                    "is clear that the entries below this line in the menu come from the spelling "
                                                                    "dictionary that the user has chosen in the profile setting which is provided "
                                                                    "as part of the OS; the entries about this line are the ones that the user has "
                                                                    "personally added."));
        }
        action_dictionarySeparatorLine->setEnabled(false);
    }

    QList<QAction*> spellings_system;
    QList<QAction*> spellings_profile;
    if (!(handle_system && codec)) {
        mSystemDictionarySuggestionsCount = 0;
    } else {
        QByteArray encodedText = codec->fromUnicode(mSpellCheckedWord);
        if (!Hunspell_spell(handle_system, encodedText.constData())) {
            // The word is NOT in the main system dictionary:
            if (handle_profile) {
                // Have a user dictionary so check it:
                if (!Hunspell_spell(handle_profile, mSpellCheckedWord.toUtf8().constData())) {
                    // The word is NOT in the profile one either - so enable add option
                    haveAddOption = true;
                } else {
                    // However the word is in the profile one - so enable remove option
                    haveRemoveOption = true;
                }

                if (haveAddOption) {
                    action_addWord->setEnabled(true);
                    connect(action_addWord, &QAction::triggered, this, &TCommandLine::slot_addWord);
                }
                if (haveRemoveOption) {
                    action_removeWord->setEnabled(true);
                    connect(action_removeWord, &QAction::triggered, this, &TCommandLine::slot_removeWord);
                }
            }
        }

        mSystemDictionarySuggestionsCount = Hunspell_suggest(handle_system, &mpSystemSuggestionsList, encodedText.constData());
    }

    if (handle_profile) {
        mUserDictionarySuggestionsCount = Hunspell_suggest(handle_profile, &mpUserSuggestionsList, mSpellCheckedWord.toUtf8().constData());
    } else {
        mUserDictionarySuggestionsCount = 0;
    }

    if (mSystemDictionarySuggestionsCount) {
        for (int i = 0; i < mSystemDictionarySuggestionsCount; ++i) {
            auto pA = new QAction(codec->toUnicode(mpSystemSuggestionsList[i]));
#if defined(Q_OS_FREEBSD)
            // Adding the text afterwards as user data as well as in the
            // constructor is to fix a bug(?) in FreeBSD that
            // automagically adds a '&' somewhere in the text to be a
            // shortcut - but doesn't show it and forgets to remove
            // it when asked for the text later:
            pA->setData(codec->toUnicode(mpSystemSuggestionsList[i]));
#endif
            connect(pA, &QAction::triggered, this, &TCommandLine::slot_popupMenu);
            spellings_system << pA;
        }

    } else {
        auto pA = new QAction(tr("no suggestions (system)",
                                    // Intentional comment
                                    "used when the command spelling checker using the selected system dictionary has no words to suggest"));
        pA->setEnabled(false);
        spellings_system << pA;
    }

    if (handle_profile) {
        if (mUserDictionarySuggestionsCount) {
            for (int i = 0; i < mUserDictionarySuggestionsCount; ++i) {
                auto pA = new QAction(codec->toUnicode(mpUserSuggestionsList[i]));
#if defined(Q_OS_FREEBSD)
                // Adding the text afterwards as user data as well as in the
                // constructor is to fix a bug(?) in FreeBSD that
                // automagically adds a '&' somewhere in the text to be a
                // shortcut - but doesn't show it and forgets to remove
                // it when asked for the text later:
                pA->setData(codec->toUnicode(mpUserSuggestionsList[i]));
#endif
                connect(pA, &QAction::triggered, this, &TCommandLine::slot_popupMenu);
                spellings_profile << pA;
            }

        } else {
            QAction* pA = nullptr;
            auto mainConsole = mpConsole->mpHost->mpConsole;
            if (mainConsole->isUsingSharedDictionary()) {
                pA = new QAction(tr("no suggestions (shared)",
                                            // Intentional comment
                                            "used when the command spelling checker using the dictionary shared between profile has no words to suggest"));
            } else {
                pA = new QAction(tr("no suggestions (profile)",
                                            // Intentional comment
                                            "used when the command spelling checker using the profile's own dictionary has no words to suggest"));
            }
            pA->setEnabled(false);
            spellings_profile << pA;
        }
    }

    /*
    * Build up the extra context menu items from the BOTTOM up, so that
    * the top of the context menu looks like:
    *
    * profile dictionary suggestions
    * --------- separator_aboveDictionarySeparatorLine
    * \/ System dictionary suggestions /\ Profile  <== Text
    * --------- separator_aboveSystemDictionarySuggestions
    * system dictionary suggestions
    * --------- separator_aboveAddAndRemove
    * Add word action
    * Remove word action
    * --------- separator_aboveStandardMenu
    *
    * The insertAction[s](...)/(Separator(...)) insert their things
    * second argument (or generated by themself) before the first (or
    * only) argument given.
    */

    auto separator_aboveStandardMenu = popup->insertSeparator(popup->actions().first());
    if (handle_profile) {
        popup->insertAction(separator_aboveStandardMenu, action_removeWord);
        popup->insertAction(action_removeWord, action_addWord);
        auto separator_aboveAddAndRemove = popup->insertSeparator(action_addWord);
        popup->insertActions(separator_aboveAddAndRemove, spellings_system);
        auto separator_aboveSystemDictionarySuggestions = popup->insertSeparator(spellings_system.first());
        popup->insertAction(separator_aboveSystemDictionarySuggestions, action_dictionarySeparatorLine);
        auto separator_aboveDictionarySeparatorLine = popup->insertSeparator(action_dictionarySeparatorLine);
        popup->insertActions(separator_aboveDictionarySeparatorLine, spellings_profile);
    } else {
        popup->insertActions(separator_aboveStandardMenu, spellings_system);
    }
    // else the word is in the dictionary - in either case show the context
    // menu - either the one with the prefixed spellings, or the standard
    // one:
}

void TCommandLine::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        auto popup = createStandardContextMenu(event->globalPos());
        if (mpHost->mEnableSpellCheck) {
            fillSpellCheckList(event, popup);
        }

        popup->addSeparator();
        foreach(auto label, contextMenuItems.keys()) {
            auto eventName = contextMenuItems.value(label);
            auto action = new QAction(label, this);
            connect(action, &QAction::triggered, [=]() {
                TEvent event = {};
                event.mArgumentList << eventName;
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                mpHost->raiseEvent(event);
            });
            popup->addAction(action);
        }

        mPopupPosition = event->pos();
        popup->popup(event->globalPos());
        // The use of accept here is supposed to prevents this event from
        // reaching any parent widget - like the TConsole containing this
        // TCommandLine...
        event->accept();
    }

    // Process any other possible mousePressEvent - which is default popup
    // handling - and which accepts the event:
    QPlainTextEdit::mousePressEvent(event);
    mudlet::self()->activateProfile(mpHost);
    if (mType & (SubCommandLine|ConsoleCommandLine)) {
        // This is NOT the main TMainConsole so keep the focus in this
        // TConsole/TCommandLine - but due to the way things happen we
        // need to do it after other things have happened - by using a zero
        // time-out timer:
        QTimer::singleShot(0, this, [this]() {
            if (mpConsole) {
                mpConsole->setFocusOnAppropriateConsole();
                this->setFocus(Qt::OtherFocusReason);
            }
        });
    }
}

void TCommandLine::mouseReleaseEvent(QMouseEvent* event)
{
    // Process any other possible mousePressEvent - which is default popup
    // handling - and which accepts the event:
    QPlainTextEdit::mousePressEvent(event);
    mudlet::self()->activateProfile(mpHost);
    if (mType & (SubCommandLine|ConsoleCommandLine)) {
        // This is NOT the main TMainConsole so keep the focus in this
        // TConsole/TCommandLine - but due to the way things happen we
        // need to do it after other things have happened - by using a zero
        // time-out timer:
        QTimer::singleShot(0, this, [this]() {
            if (mpConsole) {
                mpConsole->setFocusOnAppropriateConsole();
                this->setFocus(Qt::OtherFocusReason);
            }
        });
    }
}

void TCommandLine::enterCommand(QKeyEvent* event)
{
    Q_UNUSED(event)
    QString _t = toPlainText();
    mTabCompletionCount = -1;
    mAutoCompletionCount = -1;
    mTabCompletionTyped.clear();
    mLastCompletion.clear();
    mUserKeptOnTyping = false;

    QStringList _l = _t.split(QChar::LineFeed);

    for (int i = 0; i < _l.size(); i++) {
        if (mType != MainCommandLine && mActionFunction) {
            mpHost->getLuaInterpreter()->callCmdLineAction(mActionFunction, _l[i]);
        } else {
            mpHost->send(_l[i]);
        }
        // send command to your MiniConsole
        if (mType == ConsoleCommandLine && !mActionFunction && mpHost->mPrintCommand){
            mpConsole->printCommand(_l[i]);
        }
    }

    if (!toPlainText().isEmpty()) {
        if (mpHost->mAutoClearCommandLineAfterSend) {
            mHistoryBuffer = 0;
        } else {
            mHistoryBuffer = 1;
        }

        mHistoryList.removeAll(toPlainText());
        if (!mHistoryList.isEmpty()) {
            mHistoryList[0] = toPlainText();
        } else {
            mHistoryList.push_front(toPlainText());
        }
        mHistoryList.push_front(QString());
    }
    if (mpHost->mAutoClearCommandLineAfterSend) {
#if defined (Q_OS_MACOS)
        // clearing the input line on macOS 11.6 makes VoiceOver announce the removed text,
        // essentially re-announcing everything we've typed. This workaround fixes this behaviour
        // and does not seem to negatively affect other platforms
        hide();
#endif
        clear();
#if defined (Q_OS_MACOS)
        show();
#endif
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
        if (mTabCompletionTyped.isEmpty()) {
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

    QStringList wordList = buffer.split(QRegularExpression(qsl(R"(\b)"), QRegularExpression::UseUnicodePropertiesOption), Qt::SkipEmptyParts);

    wordList.append(commandLineSuggestions.values());

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
        QRegularExpression reg = QRegularExpression(qsl(R"(\b(\w+)$)"), QRegularExpression::UseUnicodePropertiesOption);
        QRegularExpressionMatch match = reg.match(mTabCompletionTyped);
        int typePosition = match.capturedStart();
        if (reg.captureCount() >= 1) {
            lastWord = match.captured(1);
        } else {
            lastWord = QString();
        }

        QStringList filterList = wordList.filter(QRegularExpression(qsl(R"(^%1\w+)").arg(lastWord), QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption));
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
            setPlainText(QString(userWords + proposal));
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

// cursor up/down: turns on autocompletion mode and cycles through all possible matches
// In case nothing has been typed it cycles through the command history in
// reverse order compared to cursor down.

void TCommandLine::historyMove(MoveDirection direction)
{
    if (mHistoryList.empty()) {
        return;
    }
    int shift = (direction == MOVE_UP ? 1 : -1);
    if ((textCursor().selectedText().size() == toPlainText().size()) || (toPlainText().isEmpty()) || !mpHost->mHighlightHistory) {
        mHistoryBuffer += shift;
        if (mHistoryBuffer >= mHistoryList.size()) {
            mHistoryBuffer = mHistoryList.size() - 1;
        }
        if (mHistoryBuffer < 0) {
            mHistoryBuffer = 0;
        }
        setPlainText(mHistoryList[mHistoryBuffer]);
        if (mpHost->mHighlightHistory) {
            selectAll();
        } else {
            moveCursor(QTextCursor::End);
        }
    } else {
        mAutoCompletionCount += shift;
        handleAutoCompletion();
    }
    adjustHeight();
}

void TCommandLine::slot_clearSelection(bool yes)
{
    if (yes && !mSpellChecking) {
        mpConsole->clearSelection();
    }
}

void TCommandLine::slot_removeWord()
{
    if (mSpellCheckedWord.isEmpty()) {
        return;
    }

    mpHost->mpConsole->removeWordFromSet(mSpellCheckedWord);
    // Redo spell check to update underlining
    spellCheck();
}

void TCommandLine::slot_addWord()
{
    if (mSpellCheckedWord.isEmpty()) {
        return;
    }

    mpHost->mpConsole->addWordToSet(mSpellCheckedWord);
    // Redo spell check to update underlining
    spellCheck();
}

void TCommandLine::spellCheckWord(QTextCursor& c)
{
    if (!mpHost || !mpHost->mEnableSpellCheck) {
        return;
    }

    Hunhandle* systemDictionaryHandle = mpHost->mpConsole->getHunspellHandle_system();
    if (!systemDictionaryHandle) {
        return;
    }

    QTextCharFormat f;
    mSpellChecking = true;
    c.select(QTextCursor::WordUnderCursor);
    QByteArray encodedText = mpHost->mpConsole->getHunspellCodec_system()->fromUnicode(c.selectedText());
    if (!Hunspell_spell(systemDictionaryHandle, encodedText.constData())) {
        // Word is not in selected system dictionary
        Hunhandle* userDictionaryhandle = mpHost->mpConsole->getHunspellHandle_user();
        if (userDictionaryhandle) {
            if (Hunspell_spell(userDictionaryhandle, c.selectedText().toUtf8().constData())) {
                // We are using a user dictionary and it does contain this word - so
                // use a different underline, on many systems the spell-check underline is
                // a wavy line but on macOs it is a dotted line - so use dash underline
                f.setUnderlineStyle(QTextCharFormat::DashUnderline);
                f.setUnderlineColor(Qt::cyan);
            } else {
                // The word is not in it either:
                f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
                f.setUnderlineColor(Qt::red);
            }
        } else {
            // The word is not in the main dictionary and that is all we are using:
            f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
            f.setUnderlineColor(Qt::red);
        }

    } else {
        // Word is spelt correctly
        f.setFontUnderline(false);
    }
    c.setCharFormat(f);
    setTextCursor(c);
    mSpellChecking = false;
}

bool TCommandLine::handleCtrlTabChange(QKeyEvent* ke, int tabNumber)
{
    const Qt::KeyboardModifiers allModifiers = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier;

    if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
        // let user-defined Ctrl+# keys match first - and only if the user hasn't created
        // then we fallback to tab switching
        if (keybindingMatched(ke)) {
            // Ah the user HAS created a matching binding:
            return true;
        }

        if (mudlet::self()->mpTabBar->count() >= (tabNumber)) {
            mudlet::self()->slot_tabChanged(tabNumber - 1);
            ke->accept();
            return true;
        }
    }

    if (keybindingMatched(ke)) {
        return true;
    }

    processNormalKey(ke);
    return false;
}

void TCommandLine::recheckWholeLine()
{
    if (!mpHost || !mpHost->mEnableSpellCheck) {
        return;
    }

    // Save the current position
    QTextCursor oldCursor = textCursor();

    QTextCharFormat f;
    QTextCursor c = textCursor();
    // Move Cursor AND selection anchor to start:
    c.movePosition(QTextCursor::Start);
    // In case the first character is something other than the beginning of a
    // word
    c.movePosition(QTextCursor::NextWord);
    c.movePosition(QTextCursor::PreviousWord);
    // Now select the word
    c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    while (c.hasSelection()) {
        spellCheckWord(c);
        c.movePosition(QTextCursor::NextWord);
        c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    }
    // Jump back to where we started
    setTextCursor(oldCursor);
}

void TCommandLine::clearMarksOnWholeLine()
{
    QTextCursor oldCursor = textCursor();
    QTextCharFormat f;
    QTextCursor c = textCursor();
    c.select(QTextCursor::Document);
    c.setCharFormat(f);
    setTextCursor(c);
    f.setFontUnderline(false);
    oldCursor.setCharFormat(f);
    setTextCursor(oldCursor);
}

void TCommandLine::setAction(const int func){
    releaseFunc(mActionFunction, func);
    mActionFunction = func;
}

void TCommandLine::resetAction()
{
    if (mActionFunction) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(mActionFunction);
        mActionFunction = 0;
    }
}

// This function deferences previous functions in the Lua registry.
// This allows the functions to be safely overwritten.
void TCommandLine::releaseFunc(const int existingFunction, const int newFunction)
{
    if (newFunction != existingFunction) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(existingFunction);
    }
}

//This method adds word to internal suggestion list for command line tab auto completion
void TCommandLine::addSuggestion(const QString& suggestion)
{
    commandLineSuggestions += suggestion;
}

//This method removes word from internal suggestion list for command line tab auto completion
void TCommandLine::removeSuggestion(const QString& suggestion)
{
    commandLineSuggestions.remove(suggestion);
}

//This method clears all suggestions added for command line
void TCommandLine::clearSuggestions()
{
    commandLineSuggestions.clear();
}

void TCommandLine::slot_adjustAccessibleNames()
{
    bool multipleProfilesActive = (mudlet::self()->getHostManager().getHostCount() > 1);
    const QString hostName{mpHost ? mpHost->getName() : QString()};
    switch (mType) {
    case MainCommandLine:
        if (multipleProfilesActive) {
            setAccessibleName(tr("Input line for \"%1\" profile.",
                                 // Intentional comment to separate arguments
                                 "Accessibility-friendly name to describe the main command line for a "
                                 "Mudlet profile when more than one profile is loaded, %1 is the "
                                 "profile name. Because this is likely to be used often it should be "
                                 "kept as short as possible.").arg(hostName));
            setAccessibleDescription(tr("Type in text to send to the game server for the \"%1\" profile, or enter an alias "
                                        "to run commands locally.",
                                        // Intentional comment to separate arguments
                                        "Accessibility-friendly description for the main command line for "
                                        "a Mudlet profile when more than one profile is loaded, %1 is the "
                                        "profile name. Because this is likely to be used often it should be "
                                        "kept as short as possible.").arg(hostName));
        } else {
            setAccessibleName(tr("Input line.",
                                 // Intentional comment to separate arguments
                                 "Accessibility-friendly name to describe the main command line for a "
                                 "Mudlet profile when only one profile is loaded. Because this is "
                                 "likely to be used often it should be kept as short as possible."));
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally.",
                                        // Intentional comment to separate arguments
                                        "Accessibility-friendly description for the main command line for "
                                        "a Mudlet profile when only one profile is loaded. Because this is "
                                        "likely to be used often it should be kept as short as possible."));
        }
        break;
    case SubCommandLine:
        if (multipleProfilesActive) {
            setAccessibleName(tr("Additional input line \"%1\" on \"%2\" window of \"%3\"profile.",
                                 // Intentional comment to separate arguments
                                 "Accessibility-friendly name to describe an extra command line on "
                                 "top of console/window when more than one profile is loaded, %1 is "
                                 "the command line name, %2 is the name of the window/console that "
                                 "it is on and %3 is the name of the profile.")
                                      .arg(mCommandLineName, mpConsole->mConsoleName, hostName));
            setAccessibleDescription(tr("Type in text to send to the game server for the \"%1\" profile, or enter an alias "
                                        "to run commands locally.",
                                        // Intentional comment to separate arguments
                                        "Accessibility-friendly description for an extra command line on top of a "
                                        "console/window when more than one profile is loaded, %1 is the profile "
                                        "name.").arg(hostName));
        } else {
            setAccessibleName(tr("Additional input line \"%1\" on \"%2\" window.",
                                 // Intentional comment to separate arguments
                                 "Accessibility-friendly name to describe an extra command line on "
                                 "top of console/window when only one profile is loaded, %1 is the "
                                 "command line name and %2 is the name of the window/console that "
                                 "it is on.")
                                      .arg(mCommandLineName, mpConsole->mConsoleName));
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally.",
                                        // Intentional comment to separate arguments
                                        "Accessibility-friendly description for an extra command line on "
                                        "top of a console/window when only one profile is loaded."));
        }
        break;
    case ConsoleCommandLine:
        // The mCommandLine for this type is the same as the parent TConsole
        if (multipleProfilesActive) {
            setAccessibleName(tr("Input line of \"%1\" window of \"%2\" profile.",
                                 // Intentional comment to separate arguments
                                 "Accessibility-friendly name to describe the built-in command line of a "
                                 "console/window other than the main one, when more than one profile is "
                                 "loaded, %1 is the name of the window/console and %2 is the name of the "
                                 "profile.").arg(mCommandLineName, hostName));
            setAccessibleDescription(tr("Type in text to send to the game server for the \"%1\" profile, or enter an alias "
                                        "to run commands locally.",
                                        // Intentional comment to separate arguments
                                        "Accessibility-friendly description for the built-in command line of a "
                                        "console/window other than the main window's one when more than one profile is "
                                        "loaded, %1 is the profile name.").arg(hostName));
        } else {
            setAccessibleName(tr("Input line of \"%1\" window.",
                                 // Intentional comment to separate arguments
                                 "Accessibility-friendly name to describe the built-in command line "
                                 "of a console/window other than the main one, when only one "
                                 "profile is loaded, %1 is the name of the window/console.")
                                      .arg(mCommandLineName));
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally.",
                                        // Intentional comment to separate arguments
                                        "Accessibility-friendly description for the built-in command line of a "
                                        "console/window other than the main window's one when only one profile is "
                                        "loaded."));
        }
        break;
    case UnknownType:
        Q_UNREACHABLE();
    }

}
