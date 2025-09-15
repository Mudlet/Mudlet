/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2020, 2022-2025 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2023-2025 by Lecker Kebap - Leris@mudlet.org            *
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
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TEvent.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QKeyEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSaveFile>
#include <QToolButton>
#include <QIcon>
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

    setFont(mpConsole->font());
    document()->setDocumentMargin(2);

    // Create password toggle button for MainCommandLine only
    if (mType == MainCommandLine) {
        mpPasswordToggleButton = new QToolButton(this);
        mpPasswordToggleButton->setMinimumSize(QSize(20, 20));
        mpPasswordToggleButton->setMaximumSize(QSize(20, 20));
        mpPasswordToggleButton->setFocusPolicy(Qt::NoFocus);
        mpPasswordToggleButton->setCursor(Qt::PointingHandCursor);
        mpPasswordToggleButton->setIcon(QIcon(qsl(":/icons/password-show-on.png")));
        mpPasswordToggleButton->setToolTip(tr("Show password"));
        mpPasswordToggleButton->setVisible(false); // Hidden by default
        connect(mpPasswordToggleButton, &QToolButton::clicked, this, &TCommandLine::slot_togglePasswordVisibility);
    }

    if (mType & (MainCommandLine|ConsoleCommandLine)) {
        // put an outline around the command line when it is integrated into
        // bottom of a TConsole - so that it can be visually separated from
        // the text output area - particulary when "dark" mode is in effect
        // as that modified "Fusion" style suffers from the division being
        // invisible without this:
        setFrameShape(QFrame::Box);
    }

    mRegularPalette.setColor(QPalette::Text, mpHost->mCommandLineFgColor);
    mRegularPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    mRegularPalette.setColor(QPalette::Base, mpHost->mCommandLineBgColor);

    setPalette(mRegularPalette);
    //style subCommandLines by stylesheet
    if (mType != MainCommandLine) {
        const QColor c = mpHost->mCommandLineBgColor;
        const QString styleSheet{qsl("QPlainTextEdit{background-color: rgb(%1, %2, %3);}").arg(c.red()).arg(c.green()).arg(c.blue())};
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
    // Restore the history settings:
    std::tie(mBackingFileName, mSaveCommands) = mpHost->getCmdLineSettings(mType, name);

    // Restore any previous historic commands even if we are not going to save
    // them under current settings:
    restoreHistory();

    connect(pHost, &Host::signal_saveCommandLinesHistory, this, &TCommandLine::slot_saveHistory);

    if (mType == MainCommandLine) { // Limit to the main command line only
        connect(mpHost, &Host::signal_remoteEchoChanged, this, [this](bool isRemoteEcho) {
            this->setEchoSuppression(isRemoteEcho);
        });
    }
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
            if (keybindingMatched(ke)) { // If user has set up a keybind then do that instead.
                return true;
            }

            if (mudlet::self()->dactionInputLine->isChecked()) {
                // If hidden then reveal as if pressed Alt-L
                mudlet::self()->dactionInputLine->setChecked(false);
                mudlet::self()->mpCurrentActiveHost->setCompactInputLine(false);
            }
            mpConsole->mpBufferSearchBox->setFocus();
            mpConsole->mpBufferSearchBox->selectAll();
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
                const int currentIndex = mudlet::self()->mpTabBar->currentIndex();
                const int count = mudlet::self()->mpTabBar->count();
                const int newIndex = (currentIndex - 1 < 0) ? (count - 1) : (currentIndex - 1);
                mudlet::self()->slot_tabChanged(newIndex);
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
            if (ke->modifiers() & Qt::AltModifier) {
                // prevents ALT+TAB system switching auto refocusing to command line
                return false;
            }

            if ((mpHost->mCaretShortcut == Host::CaretShortcut::Tab && !(ke->modifiers() & Qt::ControlModifier)) ||
                (mpHost->mCaretShortcut == Host::CaretShortcut::CtrlTab && (ke->modifiers() & Qt::ControlModifier))) {
                mpHost->setCaretEnabled(true);
                ke->accept();
                return true;
            }

            if ((ke->modifiers() & allModifiers) == Qt::ControlModifier) {
                // Switch to NEXT profile tab
                const int currentIndex = mudlet::self()->mpTabBar->currentIndex();
                const int count = mudlet::self()->mpTabBar->count();
                const int newIndex = (currentIndex + 1 < count) ? (currentIndex + 1) : 0;
                mudlet::self()->slot_tabChanged(newIndex);
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
            if ((mpHost->mCaretShortcut == Host::CaretShortcut::F6) && ((ke->modifiers() & allModifiers) == Qt::NoModifier)) {
                mpHost->setCaretEnabled(true);
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
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
                // This does the actual deletion of the character:
                QPlainTextEdit::event(event);
                // Recheck spelling of shortened word:
                spellCheck();
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
                // This does the actual deletion of the character:
                QPlainTextEdit::event(event);
                // Recheck spelling of shortened word:
                spellCheck();
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
                mpConsole->clearSplit();
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
                QTimer::singleShot(0, this, [this]() {  mpConsole->scrollUp(mpConsole->mUpperPane->getScreenHeight()); });
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

    // Record that this is the CommandLine in use for this profile, but NOT
    // if it was Qt::ActiveWindowFocusReason as that gets used just by
    // switching away and back to the Mudlet application and it messes up
    // the record:
    if (event->reason() != Qt::ActiveWindowFocusReason) {
        mpHost->recordActiveCommandLine(this);
    }

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
    const int fontH = QFontMetrics(font()).height();
    // Adjust height margin based on font size and if it is more than one row
    int marginH = lines > 1 ? 10 : 5;
    int _height = (fontH + 1) * lines + marginH;
    if (_height < mpHost->commandLineMinimumHeight) {
        _height = mpHost->commandLineMinimumHeight;
    }
    if (_height > height() || _height < height()) {
        mpConsole->layerCommandLine->setMinimumHeight(_height);
        mpConsole->layerCommandLine->setMaximumHeight(_height);
        const int x = mpConsole->width();
        const int y = mpConsole->height();
        const QSize s = QSize(x, y);
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
    const QString t = pA->text();
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

void TCommandLine::fillSpellCheckList(QMouseEvent* event, QMenu* popup)
{
    QTextCursor c = cursorForPosition(event->pos());
    c.select(QTextCursor::WordUnderCursor);
    mSpellCheckedWord = c.selectedText();

    const bool wantSpellCheck = TBuffer::lengthInGraphemes(mSpellCheckedWord) >= mudlet::self()->mMinLengthForSpellCheck;
    if (!wantSpellCheck) {
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
            /*:
            This line is shown in the list of spelling suggestions on the profile's command
            line context menu to clearly divide up where the suggestions for correct
            spellings are coming from.  The precise format might be modified as long as it
            is clear that the entries below this line in the menu come from the spelling
            dictionary that the user has chosen in the profile setting which we have
            bundled with Mudlet; the entries about this line are the ones that the user
            has personally added.
            */
            action_dictionarySeparatorLine = new QAction(tr("▼Mudlet▼ │ dictionary suggestions │ ▲User▲"));
        } else {
            /*:
            This line is shown in the list of spelling suggestions on the profile's command
            line context menu to clearly divide up where the suggestions for correct
            spellings are coming from.  The precise format might be modified as long as it
            is clear that the entries below this line in the menu come from the spelling
            dictionary that the user has chosen in the profile setting which is provided
            as part of the OS; the entries about this line are the ones that the user has
            personally added.
            */
            action_dictionarySeparatorLine = new QAction(tr("▼System▼ │ dictionary suggestions │ ▲User▲"));
        }
        action_dictionarySeparatorLine->setEnabled(false);
    }

    QList<QAction*> spellings_system;
    QList<QAction*> spellings_profile;
    // We always use UTF-8 for the per profile/shared dictionary so we do not
    // need to have a codec prepared for it and can use QString::toUtf8()
    // directly:
    const QByteArray utf8Text = mSpellCheckedWord.toUtf8();
    if (!(handle_system && codec)) {
        mSystemDictionarySuggestionsCount = 0;
    } else {
        // The dictionary used from "the system" may not be UTF-8 encoded so we
        // will need to transform the UTF-16BE "QString" to the appropriate encoding
        // using "codec" declared previously in this method:
        const QByteArray encodedText = codec->fromUnicode(mSpellCheckedWord);
        if (!Hunspell_spell(handle_system, encodedText.constData())) {
            // The word is NOT in the main system dictionary:
            if (handle_profile) {
                // Have a user dictionary so check it:
                if (!Hunspell_spell(handle_profile, utf8Text.constData())) {
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
        mUserDictionarySuggestionsCount = Hunspell_suggest(handle_profile, &mpUserSuggestionsList, utf8Text.constData());
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
        /*:
        Used when the command spelling checker using the selected system dictionary has
        no words to suggest.
        */
        auto pA = new QAction(tr("no suggestions (system)"));
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
                /*:
                Used when the command spelling checker using the dictionary shared between
                profile has no words to suggest.
                */
                pA = new QAction(tr("no suggestions (shared)"));
            } else {
                /*:
                Used when the command spelling checker using the profile's own dictionary has
                no words to suggest.
                */
                pA = new QAction(tr("no suggestions (profile)"));
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
}

void TCommandLine::mousePressEvent(QMouseEvent* event)
{
    // Prevent selection, drag/drop of text in the command line when echo suppression is on
    // Allow right-click to show the context menu (enables Paste)
    if (mIsEchoSuppressed && mType == MainCommandLine && event->button() != Qt::RightButton) {
        event->ignore();
        return;
    }

    if (event->button() == Qt::RightButton) {
        auto popup = createStandardContextMenu(event->globalPosition().toPoint());
        if (mpHost->mEnableSpellCheck) {
            fillSpellCheckList(event, popup);
            // else the word is in the dictionary - in either case show the context
            // menu - either the one with the prefixed spellings, or the standard one
        }

        popup->addSeparator();
        foreach(auto label, contextMenuItems.keys()) {
            auto eventName = contextMenuItems.value(label);
            auto action = new QAction(label, this);
            connect(action, &QAction::triggered, this, [=, this]() {
                TEvent mudletEvent = {};
                mudletEvent.mArgumentList << eventName;
                mudletEvent.mArgumentTypeList << ARGUMENT_TYPE_STRING;
                mpHost->raiseEvent(mudletEvent);
            });
            popup->addAction(action);
        }

        mPopupPosition = event->pos();
        popup->popup(event->globalPosition().toPoint());
        // The use of accept here is supposed to prevents this event from
        // reaching any parent widget - like the TConsole containing this
        // TCommandLine...
        event->accept();
    }

    // Process any other possible mousePressEvent - which is default context
    // menu handling - and which accepts the event:
    QPlainTextEdit::mousePressEvent(event);
    mudlet::self()->activateProfile(mpHost);
}

void TCommandLine::mouseReleaseEvent(QMouseEvent* event)
{
    // Process any other possible mouseReleaseEvent - which is default context
    // menu handling - and which accepts the event:
    QPlainTextEdit::mouseReleaseEvent(event);
    mudlet::self()->activateProfile(mpHost);
}

void TCommandLine::enterCommand(QKeyEvent* event)
{
    Q_UNUSED(event)
    mTabCompletionCount = -1;
    mAutoCompletionCount = -1;
    mTabCompletionTyped.clear();
    mLastCompletion.clear();
    mUserKeptOnTyping = false;

    QStringList commandList = toPlainText().split(QChar::LineFeed);

    for (QString& command : commandList) {
        if (mType != MainCommandLine && mActionFunction) {
            mpHost->getLuaInterpreter()->callCmdLineAction(mActionFunction, command);
        } else {
            mpHost->send(command);
        }
        // send command to your MiniConsole
        if (mType == ConsoleCommandLine && !mActionFunction && mpHost->mCommandEchoMode != Host::CommandEchoMode::Never){
            // This usage of commandList modifies the content!!!
            mpConsole->printCommand(command);
        }
    }

    if (!toPlainText().isEmpty() && !mpHost->isRemoteEchoingActive()) {
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

    const QStringList bufferList = mpHost->mpConsole->buffer.getEndLines(amount);
    QString buffer = bufferList.join(QChar::Space);

    buffer.replace(QChar(0x21af), QChar::LineFeed);
    buffer.replace(QChar::LineFeed, QChar::Space);

    QStringList wordList = buffer.split(QRegularExpression(qsl(R"(\b)"), QRegularExpression::UseUnicodePropertiesOption), Qt::SkipEmptyParts);
    wordList.append(commandLineSuggestions.values()); // hindsight 20/20 I do not need to split this to a separate table, a check to not append buffer to this table and only append suggested list does same thing for far less overhead.
    const QStringList blacklist = tabCompleteBlacklist.values();
    QStringList toDelete;

    for (const QString& wstr : std::as_const(wordList)) {
        if (blacklist.contains(wstr, Qt::CaseInsensitive)) {
            toDelete += wstr;
        }
    }
    for (const QString& dstr : std::as_const(toDelete)) {
        wordList.removeAll(dstr);
    }

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
        const QRegularExpression reg = QRegularExpression(qsl(R"(\b(\w+)$)"), QRegularExpression::UseUnicodePropertiesOption);
        const QRegularExpressionMatch match = reg.match(mTabCompletionTyped);
        const int typePosition = match.capturedStart();
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
            const QString tmp = filterList.back();
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

            const QString proposal = filterList[mTabCompletionCount];
            const QString userWords = mTabCompletionTyped.left(typePosition);
            setPlainText(QString(userWords + proposal));
            mudlet::self()->announce(proposal);
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
    mudlet::self()->announce(neu);
    mTabCompletionOld = neu;
    const int oldLength = toPlainText().size();
    if (mAutoCompletionCount >= mHistoryList.size()) {
        mAutoCompletionCount = mHistoryList.size() - 1;
    }
    if (mAutoCompletionCount < 0) {
        mAutoCompletionCount = 0;
    }
    for (int i = mAutoCompletionCount; i < mHistoryList.size(); i++) {
        const QString h = mHistoryList[i].mid(0, neu.size());
        if (neu == h) {
            mAutoCompletionCount = i;
            mLastCompletion = mHistoryList[i];
            setPlainText(mHistoryList[i]);
            mudlet::self()->announce(mHistoryList[i]);
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
    const int shift = (direction == MOVE_UP ? 1 : -1);
    if ((textCursor().selectedText().size() == toPlainText().size()) || (toPlainText().isEmpty()) || !mpHost->mHighlightHistory) {
        mHistoryBuffer += shift;
        if (mHistoryBuffer >= mHistoryList.size()) {
            mHistoryBuffer = mHistoryList.size() - 1;
        }
        if (mHistoryBuffer < 0) {
            mHistoryBuffer = 0;
        }
        setPlainText(mHistoryList[mHistoryBuffer]);
        mudlet::self()->announce(mHistoryList[mHistoryBuffer]);
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
    const QString spellCheckedWord = c.selectedText();
    const bool wantSpellCheck = TBuffer::lengthInGraphemes(spellCheckedWord) >= mudlet::self()->mMinLengthForSpellCheck;
    if (!wantSpellCheck) {
        // We don't check when the word is too short, but may need to
        // undo any prior underline, and we need to also reset the flag:
        f.setFontUnderline(false);
        c.setCharFormat(f);
        setTextCursor(c);
        mSpellChecking = false;
        return;
    }

    // The dictionary used from "the system" may not be UTF-8 encoded so we
    // will need to transform the UTF-16BE "QString" to the appropriate encoding
    // using the correct "codec":
    const QByteArray encodedText = mpHost->mpConsole->getHunspellCodec_system()->fromUnicode(spellCheckedWord);
    if (!Hunspell_spell(systemDictionaryHandle, encodedText.constData())) {
        // Word is not in selected system dictionary
        Hunhandle* userDictionaryhandle = mpHost->mpConsole->getHunspellHandle_user();
        if (userDictionaryhandle) {
            // The per-profile/shared dictionary is always UTF-8 encoded - so
            // we can use QString::toUtf8() directly to get the bytes needed:
            if (Hunspell_spell(userDictionaryhandle, spellCheckedWord.toUtf8().constData())) {
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
    const Qt::KeyboardModifiers allExceptShiftModifiers = Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier;

    if ((ke->modifiers() & allExceptShiftModifiers) == Qt::ControlModifier) {
        // let user-defined Ctrl+# keys match first - and only if the user
        // hasn't created one then we fallback to tab switching - however
        // since some locales need the SHIFT modifier to enter numbers from the
        // top keyboard row (e.g. French AZERTY) we must ignore that one!
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
    const QTextCursor oldCursor = textCursor();

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

void TCommandLine::addBlacklist(const QString& word)
{
    tabCompleteBlacklist += word;
}

void TCommandLine::removeBlacklist(const QString& word)
{
    tabCompleteBlacklist.remove(word);
}

void TCommandLine::clearBlacklist()
{
    tabCompleteBlacklist.clear();
}

void TCommandLine::slot_adjustAccessibleNames()
{
    const bool multipleProfilesActive = (mudlet::self()->getHostManager().getHostCount() > 1);
    const QString hostName{mpHost ? mpHost->getName() : QString()};
    switch (mType) {
    case MainCommandLine:
        if (multipleProfilesActive) {
            /*:
            Accessibility-friendly name to describe the main command line for a
            Mudlet profile when more than one profile is loaded, %1 is the
            profile name. Because this is likely to be used often it should be
            kept as short as possible.
            */
            setAccessibleName(tr("Input line for \"%1\" profile.").arg(hostName));
            /*:
            Accessibility-friendly description for the main command line for
            a Mudlet profile when more than one profile is loaded, %1 is the
            profile name. Because this is likely to be used often it should be
            kept as short as possible.
            */
            setAccessibleDescription(tr("Type in text to send to the game server for the \"%1\" profile, or enter an alias "
                                        "to run commands locally.").arg(hostName));
        } else {
            /*:
            Accessibility-friendly name to describe the main command line for a
            Mudlet profile when only one profile is loaded. Because this is
            likely to be used often it should be kept as short as possible.
            */
            setAccessibleName(tr("Input line."));
            /*:
            Accessibility-friendly description for the main command line for
            a Mudlet profile when only one profile is loaded. Because this is
            likely to be used often it should be kept as short as possible.
            */
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally."));
        }
        break;
    case SubCommandLine:
        if (multipleProfilesActive) {
            /*:
            Accessibility-friendly name to describe an extra command line on
            top of console/window when more than one profile is loaded, %1 is
            the command line name, %2 is the name of the window/console that
            it is on and %3 is the name of the profile.
            */
            setAccessibleName(tr("Additional input line \"%1\" on \"%2\" window of \"%3\"profile.")
                                      .arg(mCommandLineName, mpConsole->mConsoleName, hostName));
            /*:
            Accessibility-friendly description for an extra command line on top of a
            console/window when more than one profile is loaded, %1 is the profile
            name.
            */
            setAccessibleDescription(tr("Type in text to send to the game server for the \"%1\" profile, or enter an alias "
                                        "to run commands locally.").arg(hostName));
        } else {
            /*:
            Accessibility-friendly name to describe an extra command line on
            top of console/window when only one profile is loaded, %1 is the
            command line name and %2 is the name of the window/console that
            it is on.
            */
            setAccessibleName(tr("Additional input line \"%1\" on \"%2\" window.")
                                      .arg(mCommandLineName, mpConsole->mConsoleName));
            /*:
            Accessibility-friendly description for an extra command line on
            top of a console/window when only one profile is loaded.
            */
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally."));
        }
        break;
    case ConsoleCommandLine:
        // The mCommandLine for this type is the same as the parent TConsole
        if (multipleProfilesActive) {
            /*:
            Accessibility-friendly name to describe the built-in command line of a
            console/window other than the main one, when more than one profile is
            loaded, %1 is the name of the window/console and %2 is the name of the
            profile.
            */
            setAccessibleName(tr("Input line of \"%1\" window of \"%2\" profile.").arg(mCommandLineName, hostName));
            /*:
            Accessibility-friendly description for the built-in command line of a
            console/window other than the main window's one when more than one profile is
            loaded, %1 is the profile name.
            */
            setAccessibleDescription(tr("Type in text to send to the game server for the \"%1\" profile, or enter an alias "
                                        "to run commands locally.").arg(hostName));
        } else {
            /*:
            Accessibility-friendly name to describe the built-in command line
            of a console/window other than the main one, when only one
            profile is loaded, %1 is the name of the window/console.
            */
            setAccessibleName(tr("Input line of \"%1\" window.")
                                      .arg(mCommandLineName));
            /*:
            Accessibility-friendly description for the built-in command line of a
            console/window other than the main window's one when only one profile is
            loaded.
            */
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally."));
        }
        break;
    case UnknownType:
        Q_UNREACHABLE();
    }
}

void TCommandLine::restoreHistory()
{
    auto pHost = mpHost;
    if (!pHost) {
        qWarning().nospace().noquote() << "TCommandLine::restoreHistory() ERROR - got a Host pointer that was null - unable to save command history for the command line called: " << mCommandLineName
                                       << " of type: " << mType;
        return;
    }

    QString pathFileName{mudlet::self()->mudlet::getMudletPath(enums::profileDataItemPath, pHost->getName(), mBackingFileName)};
    QFile historyFile(pathFileName, this);
    if (historyFile.exists()) {
        if (historyFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
            QTextStream ifs(&historyFile);
            QString buffer;
            while (!ifs.atEnd() && ifs.status() == QTextStream::Ok) {
                ifs.readLineInto(&buffer);
                mHistoryList.append(buffer);
            }

            if (historyFile.error() != QFileDevice::NoError) {
                qWarning() << "TCommandLine::restoreHistory() ERROR - unable to read command history from file for the command line called: " << mCommandLineName << " of type: " << mType
                           << " reason: " << historyFile.errorString();
                historyFile.close();
                return;
            }

            historyFile.close();
            // Success!
            return;
        }

        // else failed to open the file despite it existing
        if (historyFile.error() != QFileDevice::NoError) {
            qWarning() << "TCommandLine::restoreHistory() ERROR - unable to open command history for the command line called: "
                       << mCommandLineName << " of type: " << mType
                       << " reason: " << historyFile.errorString();
            return;
        }
    }
    // else no such file - which will be the case for the first time the
    // command line is created - so it might not be an error:
    if (mCommandLineName != qsl("main")) {
        qDebug() << "TCommandLine::restoreHistory() ALERT - unable to open command history for the command line called: "
                << mCommandLineName << " of type: " << mType
                << " because the file: " << mBackingFileName
                << " does not exist in the profile's home directory, unless this is a new command line then this is an unexpected error.";
    }
}

void TCommandLine::slot_saveHistory()
{
    auto pHost = mpHost;
    if (!pHost) {
        qWarning().nospace().noquote() << "TCommandLine::slot_saveHistory() ERROR - got a Host pointer that was null - unable to save command history for the command line called: " << mCommandLineName
                                       << " of type: " << mType;
        return;
    }

    pHost->setCmdLineSettings(mType, mSaveCommands, mCommandLineName);
    auto saveSize = pHost->getCommandLineHistorySaveSize();
    if (!saveSize || !mSaveCommands) {
        // Option has been disabled so do nothing (won't delete the previous one
        // though!):
        return;
    }

    QString pathFileName{mudlet::self()->mudlet::getMudletPath(enums::profileDataItemPath, pHost->getName(), mBackingFileName)};
    QSaveFile historyFile(pathFileName, this);
    if (historyFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QTextStream ofs(&historyFile);
        // We need to add one here because usually the first line in
        // mHistoryList is an empty one - maybe it might represent the current
        // line and will get captured/saved if the profile is closed with some
        // unsent text in the command line?
        ofs << mHistoryList.mid(0, saveSize + 1).join(QChar::LineFeed);
        if (!historyFile.commit()) {
            qDebug().nospace().noquote() << "TCommandLine::slot_saveHistory() ERROR - unable to save command history for the command line called: " << mCommandLineName
                                         << " of type: " << mType << " reason: " << historyFile.errorString();
        }
    }
}

/*
 * setEchoSuppression - Handle password input mode for the main command line
 * 
 * This function manages the transition between normal command input and secure password entry.
 * When a MUD server requests password input, it signals echo suppression which hides user typing.
 * 
 * Key challenges handled:
 * - Preserving user's command text during password prompts for restoration afterward
 * - Distinguishing between commands and partial password input when suppression activates
 * - Maintaining correct selection state (important for auto-clear OFF workflow)
 * - Supporting users who type password characters before echo suppression kicks in
 * 
 * Common workflows:
 * 1. Auto-clear ON: user types command -> sends -> password prompt -> restore command
 * 2. Auto-clear OFF: user types command -> sends -> command selected -> password prompt -> restore selected command  
 * 3. Rapid login: user types 'password' -> server enables echo suppression mid-typing -> continue hidden
 * 4. Empty command line: straightforward password entry with no restoration needed
 * 
 * @param suppress true to start password mode (hide input), false to end it (restore normal input)
 */
void TCommandLine::setEchoSuppression(bool suppress)
{
    // Echo suppression (password masking) only applies to the main command line
    // SubCommandLines and ConsoleCommandLines are not affected by server password prompts
    if (mType != MainCommandLine) {
        return;
    }

    // Early exit if suppression state hasn't changed - avoids unnecessary processing
    if (mIsEchoSuppressed == suppress) {
        return;
    }

    mIsEchoSuppressed = suppress;

    if (suppress) {
        // === STARTING PASSWORD INPUT MODE ===
        // When password prompting begins, we need to handle different scenarios:
        // 1. User has command typed and selected (auto-clear OFF) - preserve for restoration
        // 2. User has unselected command (auto-clear ON) - check if it's a command vs. partial password
        // 3. User was already typing password characters before echo suppression activated
        // 4. Command line is empty - simple case, just start password mode
        
        const QString currentText = toPlainText();
        QString textToRestoreAfterPassword;  // Command text to restore after password entry
        QString partialPasswordToKeep;       // Password chars user typed before suppression activated
        
        // Analyze what the user currently has in the command line
        if (!currentText.isEmpty()) {
            QTextCursor cursor = textCursor();

            if (cursor.hasSelection()) {
                // SCENARIO 1: Auto-clear is OFF, previous command is selected
                // User workflow: sent command -> server shows password prompt -> command gets selected
                // Action: Save selected text for restoration after password, remember it was selected
                textToRestoreAfterPassword = cursor.selectedText();
                mRestoredTextShouldBeSelected = true;
            } else {
                // SCENARIO 2 & 3: Text exists but isn't selected - determine what it is
                mRestoredTextShouldBeSelected = false;
                
                // Check if current text matches recent command history to distinguish between:
                // - A command that was just sent (preserve it)
                // - Password characters already being typed (continue with them)
                bool isExistingCommand = false;
                const int maxHistoryEntriesToCheck = qMin(500, mHistoryList.size());

                for (int i = 0; i < maxHistoryEntriesToCheck; ++i) {
                    const QString& historyEntry = mHistoryList[i];

                    if (!historyEntry.isEmpty()) {
                        if (currentText == historyEntry) {
                            isExistingCommand = true;
                        }
                        break;
                    }
                }
                
                if (!isExistingCommand && !mHistoryList.isEmpty()) {
                    // SCENARIO 3: Text doesn't match history - likely password chars already typed
                    // User workflow: types 'password' -> server enables echo suppression mid-typing
                    // Action: Continue with these characters as hidden password input
                    partialPasswordToKeep = currentText;
                } else {
                    // SCENARIO 2: Text matches history - it's a recently sent command
                    // User workflow: types command -> sends it -> server prompts for password
                    // Action: Preserve command for restoration after password entry
                    textToRestoreAfterPassword = currentText;
                }
            }
        } else {
            // SCENARIO 4: Command line is empty - straightforward password entry
            mRestoredTextShouldBeSelected = false;
        }
        
        // Store the command text for later restoration (empty if none to restore)
        mTextToRestoreAfterEchoSuppression = textToRestoreAfterPassword;
        clear();  // Clear command line for password input
        
        // Show password toggle button and reset visibility state
        if (mpPasswordToggleButton) {
            mPasswordVisible = false; // Start with password hidden
            updatePasswordToggleButton();
            mpPasswordToggleButton->setVisible(true);
            positionPasswordToggleButton();
        }
        
        // If user was already typing password characters, restore them and continue
        if (!partialPasswordToKeep.isEmpty()) {
            setPlainText(partialPasswordToKeep);
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::End);  // Position cursor for continued typing
            setTextCursor(cursor);
        }
    } else {
        // === ENDING PASSWORD INPUT MODE ===
        // Password entry is complete, restore the command line to its previous state
        // This handles the transition from hidden password input back to normal command entry
        
        clear();  // Clear the password field first for security
        
        // Hide password toggle button
        if (mpPasswordToggleButton) {
            mpPasswordToggleButton->setVisible(false);
        }
        
        // Restore any command text that was preserved when password mode started
        if (!mTextToRestoreAfterEchoSuppression.isEmpty()) {
            setPlainText(mTextToRestoreAfterEchoSuppression);
            
            // Restore the original selection state to maintain user workflow consistency
            QTextCursor cursor = textCursor();

            if (mRestoredTextShouldBeSelected) {
                // Original text was selected (auto-clear OFF) - restore selection
                // User workflow: command selected -> password entered -> command re-selected for editing/resending
                cursor.movePosition(QTextCursor::Start);
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            } else {
                // Original text was not selected - position cursor at end for continued typing
                cursor.movePosition(QTextCursor::End);
            }

            setTextCursor(cursor);
            
            // Clear saved state - restoration is complete
            mTextToRestoreAfterEchoSuppression.clear();
            mRestoredTextShouldBeSelected = false;
        }
    }

    viewport()->update(); // Force repaint to apply/remove password masking visual effect
}

void TCommandLine::paintEvent(QPaintEvent* event)
{
    // Only mask text for the main command line when echo is suppressed and password is not visible
    if (mIsEchoSuppressed && mType == MainCommandLine && !mPasswordVisible) {
        QPainter painter(viewport());
        QTextCursor cursor = textCursor();
        QTextBlock block = document()->firstBlock();
        QFontMetrics fm(font());

        // Paint each line with asterisks instead of actual text
        for (QTextBlock b = block; b.isValid(); b = b.next()) {
            QString text = b.text();
            QString mask = QString('*').repeated(text.length());
            QRect r = blockBoundingGeometry(b).translated(contentOffset()).toRect();
            painter.drawText(r.topLeft() + QPoint(0, fm.ascent()), mask);
        }
        return;
    }

    QPlainTextEdit::paintEvent(event);
}

void TCommandLine::slot_togglePasswordVisibility()
{
    if (!mIsEchoSuppressed || mType != MainCommandLine) {
        return;
    }

    mPasswordVisible = !mPasswordVisible;
    updatePasswordToggleButton();
    viewport()->update(); // triggers paintEvent to mask/unmask
}

void TCommandLine::updatePasswordToggleButton()
{
    if (!mpPasswordToggleButton) {
        return;
    }

    if (mPasswordVisible) {
        // Password is visible, show "hide" icon (eye with slash)
        mpPasswordToggleButton->setIcon(QIcon(qsl(":/icons/password-show-off.png")));
        mpPasswordToggleButton->setToolTip(tr("Hide password"));
    } else {
        // Password is hidden, show "show" icon (eye)
        mpPasswordToggleButton->setIcon(QIcon(qsl(":/icons/password-show-on.png")));
        mpPasswordToggleButton->setToolTip(tr("Show password"));
    }
}

void TCommandLine::positionPasswordToggleButton()
{
    if (!mpPasswordToggleButton) {
        return;
    }

    // Position the button at the right side of the text edit
    const QRect viewportRect = viewport()->geometry();
    const int buttonWidth = mpPasswordToggleButton->width();
    const int buttonHeight = mpPasswordToggleButton->height();
    const int margin = 5;

    // Position at the right edge, vertically centered
    const int x = viewportRect.width() - buttonWidth - margin;
    const int y = (viewportRect.height() - buttonHeight) / 2;

    mpPasswordToggleButton->move(x, y);
}

void TCommandLine::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    // Reposition the password toggle button when the widget is resized
    if (mpPasswordToggleButton && mpPasswordToggleButton->isVisible()) {
        positionPasswordToggleButton();
    }
}
