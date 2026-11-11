/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2020, 2022-2026 by Stephen Lyons                   *
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

#include "MudletApp.h"
#include "TCommandHistory.h"
#include "TEncodingHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "TConsole.h"
#include "TMainConsole.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TEvent.h"
#include "mudlet.h"

#include <hunspell/hunspell.h>

#include <QAbstractTextDocumentLayout>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMimeData>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStyleOptionFrame>
#include <QtMath>
#include <QTextBlock>
#include <algorithm>
#include <chrono>

using namespace std::chrono_literals;

namespace {
// spellCheckWord() takes the word under the cursor it is given, and for a
// cursor at the end of a word that is whatever follows it - so hand it the
// start of the word that ends at, or runs across, the position instead
QTextCursor cursorAtWordStart(QTextDocument* pDocument, const int position)
{
    QTextCursor cursor(pDocument);
    const bool onALetter = pDocument->characterAt(position).isLetterOrNumber();
    const bool afterALetter = position > 0 && pDocument->characterAt(position - 1).isLetterOrNumber();
    cursor.setPosition(afterALetter && !onALetter ? position - 1 : position);
    cursor.select(QTextCursor::WordUnderCursor);
    cursor.setPosition(cursor.selectionStart());
    return cursor;
}
} // namespace

TCommandLine::TCommandLine(Host* pHost, const QString& name, enums::CommandLineType type, TConsole* pConsole, QWidget* parent)
: QPlainTextEdit(parent)
, mCommandLineName(name)
, mpHost(pHost)
, mType(type)
, mpKeyUnit(pHost->getKeyUnit())
, mpConsole(pConsole)
{
    setObjectName(qsl("commandLine_%1_%2").arg(mpHost->getName(), name));

    setAutoFillBackground(true);
    // The strips centerRows() leaves above and below the viewport fill like the viewport
    setBackgroundRole(QPalette::Base);
    setFocusPolicy(Qt::StrongFocus);

    setFont(mpConsole->font());
    document()->setDocumentMargin(2);
    connect(document(), &QTextDocument::contentsChange, this, &TCommandLine::slot_contentsChange);

    if (mType & (enums::MainCommandLine | enums::ConsoleCommandLine)) {
        // The seam along the top of the console's foot parts it from the text above
        setFrameShape(QFrame::NoFrame);
    }

    mRegularPalette.setColor(QPalette::Text, mpHost->mCommandLineFgColor);
    mRegularPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    mRegularPalette.setColor(QPalette::Base, mpHost->mCommandLineBgColor);

    setPalette(mRegularPalette);
    //style subCommandLines by stylesheet
    if (mType != enums::MainCommandLine) {
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

    // Forward textChanged signal for hyperlink visibility triggers
    connect(this, &QPlainTextEdit::textChanged, this, &TCommandLine::commandLineTextChanged);
}

TCommandLine::UserEditScope::UserEditScope(TCommandLine& line)
: mLine(line)
{
    const QTextCursor cursor = mLine.textCursor();
    const QString text = mLine.toPlainText();
    mLine.mEditStartedOnBlankLine = text.isEmpty() || (cursor.hasSelection() && cursor.selectionStart() == 0 && cursor.selectionEnd() == text.length());
    mLine.mUserEditInProgress = true;
}

TCommandLine::UserEditScope::~UserEditScope()
{
    mLine.mUserEditInProgress = false;
}

void TCommandLine::restorePlayerTypedLine(const QString& text)
{
    {
        const UserEditScope userEdit(*this);
        setPlainText(text);
    }
    moveCursor(QTextCursor::End);
    adjustHeight();
}

void TCommandLine::slot_contentsChange()
{
    const QString text = toPlainText();
    // A spell-check underline or a recheck changes formats only
    if (text == mLastPlainText) {
        return;
    }
    mLastPlainText = text;
    if (!mUserEditInProgress || text.isEmpty()) {
        // A script, clear(), history recall or Tab completion
        mPlayerTypedLine = false;
    } else if (mEditStartedOnBlankLine) {
        mPlayerTypedLine = true;
    }
    // Typing more into a line keeps whatever it was
}

void TCommandLine::insertFromMimeData(const QMimeData* source)
{
    // Paste, drop and middle-click all land here
    const UserEditScope userEdit(*this);
    QPlainTextEdit::insertFromMimeData(source);
}

bool TCommandLine::claimsShortcutOverride(const QKeyEvent* ke) const
{
    if (mpHost->isClosingDown()) {
        return false;
    }
    // The caret-mode shortcut must beat application-wide QShortcuts, such as
    // the profile-switching ones on Ctrl+Tab
    if (mpHost->caretShortcutMatches(ke)) {
        return true;
    }
    // QShortcutMap consumes a profile-switching shortcut before the KeyPress
    // arrives, so a user binding on one is claimed here and runs off that KeyPress
    return keybindingWouldMatchProfileSwitchShortcut(ke);
}

void TCommandLine::processNormalKey(QEvent* event)
{
    {
        const UserEditScope userEdit(*this);
        QPlainTextEdit::event(event);
    }
    adjustHeight();

    mHistoryBuffer = 0;
    if (mCompletion.lineEdited(toPlainText())) {
        mCompletion.restartHistoryWalk();
    }

    spellCheck();
}

bool TCommandLine::keybindingMatched(QKeyEvent* keyEvent)
{
    // Don't process keybindings if the host is closing down to prevent crashes
    // when multiple profiles are closed quickly
    if (mpHost->isClosingDown()) {
        return false;
    }

    if (mpKeyUnit->processDataStream(static_cast<Qt::Key>(keyEvent->key()), static_cast<Qt::KeyboardModifiers>(keyEvent->modifiers()))) {
        keyEvent->accept();
        return true;
    }

    return false;
}

bool TCommandLine::takeKeyPress(QKeyEvent* event)
{
    if (!mpHost || mpHost->isClosingDown()) {
        return false;
    }

    switch (event->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
    case Qt::Key_Meta:
        // Taken alone, it would move focus here ahead of the key it goes with, such as the Tab of Shift+Tab
        return false;
    default:
        break;
    }

    // Focus in an inactive window waits for it to be activated
    activateWindow();
    setFocus(Qt::OtherFocusReason);
    QCoreApplication::sendEvent(this, event);
    event->accept();
    return true;
}

bool TCommandLine::keybindingWouldMatchProfileSwitchShortcut(const QKeyEvent* keyEvent) const
{
    if (!mpKeyUnit || mpHost->isClosingDown()) {
        return false;
    }

    auto* pMudlet = mudlet::self();
    if (!pMudlet || !pMudlet->profileSwitchShortcutMatches(keyEvent)) {
        return false;
    }

    return mpKeyUnit->wouldMatch(static_cast<Qt::Key>(keyEvent->key()), keyEvent->modifiers());
}

// This function overrides the QWidget::event() and should return true if the
// event was recognized, otherwise it should return false. If the recognized
// event was accepted (see QEvent::accepted), any further processing such as
// event propagation to the parent widget stops.
bool TCommandLine::event(QEvent* event)
{
    // Qt routes real key presses to the focus proxy but not synthetic ones, such
    // as TTextEdit's caret-mode forwarder, which would put the first character
    // of a password here in the clear
    if (QWidget* proxy = focusProxy(); proxy && event->type() == QEvent::KeyPress) {
        return QApplication::sendEvent(proxy, event);
    }

    // Don't process events if the host is closing down to prevent crashes
    // when multiple profiles are closed quickly
    if (mpHost->isClosingDown()) {
        return QPlainTextEdit::event(event);
    }

    if (event->type() == QEvent::ShortcutOverride && claimsShortcutOverride(static_cast<QKeyEvent*>(event))) {
        event->accept();
        return true;
    }

    const Qt::KeyboardModifiers allModifiers = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier;
    if (event->type() == QEvent::KeyPress) {
        auto* ke = dynamic_cast<QKeyEvent*>(event);
        if (!ke) {
            qCritical().noquote() << "TCommandLine::event(QEvent*) CRITICAL - a QEvent that is supposed to be a QKeyEvent is not dynamically castable to the latter - so the processing of this event "
                                     "has been aborted - please report this to Mudlet Makers.";
            return false;
        }

        if (ke->matches(QKeySequence::Copy)) { // Copy is Ctrl+C and possibly Ctrl+Ins, F16
            // Check for console selections in both upper and lower panes
            // Prioritize console selection over command line text
            const bool hasUpperPaneSelection = !mpConsole->mUpperPane->mSelectedRegion.isEmpty();
            const bool hasLowerPaneSelection = !mpConsole->mLowerPane->mSelectedRegion.isEmpty();

            if (hasUpperPaneSelection) {
                // Copy from upper pane if it has a selection
                mpConsole->mUpperPane->slot_copySelectionToClipboard();
                ke->accept();
                return true;
            }
            if (hasLowerPaneSelection) {
                // Copy from lower pane if it has a selection
                mpConsole->mLowerPane->slot_copySelectionToClipboard();
                ke->accept();
                return true;
            }
            // If no console selection, fall through to default command line copy behavior
        }

        if (ke->matches(QKeySequence::Find)) { // Find is Ctrl+F
            if (keybindingMatched(ke)) {       // If user has set up a keybind then do that instead.
                return true;
            }

            if (mudlet::self()->dactionInputLine->isChecked()) {
                // If hidden then reveal as if pressed Alt-L
                mudlet::self()->dactionInputLine->setChecked(false);
                mudlet::self()->mpCurrentActiveHost->setCompactInputLine(false);
            }

            mpConsole->mUpperPane->slot_copySelectionToSearchBar();
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
                mCompletion.reset();
                mCompletion.resetHistoryWalk();
                mHistoryBuffer = 0;
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
            if ((ke->modifiers() & (allModifiers & ~(Qt::ShiftModifier))) == Qt::NoModifier) {
                // Process as plain <BACKTAB> - (ignoring implicit <SHIFT>)
                handleTabCompletion(false);
                adjustHeight();
                ke->accept();
                return true;
            }

            if (keybindingMatched(ke)) {
                // Process as a possible key binding if there are ANY modifiers
                // other than just the ignored <SHIFT>:
                return true;
            }
            break;

        case Qt::Key_Tab:
            if (mpHost->caretShortcutMatches(ke)) {
                mpHost->setCaretEnabled(true);
                ke->accept();
                return true;
            }

            if ((ke->modifiers() & allModifiers) == Qt::NoModifier) {
                handleTabCompletion(true);
                ke->accept();
                return true;
            }

            // Process as a possible key binding if there are ANY modifiers
            // CHECKME: What about system foreground application switching?
            if (keybindingMatched(ke)) {
                return true;
            }
            break;

        case Qt::Key_F6:
            if (mpHost->caretShortcutMatches(ke)) {
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
            if ((ke->modifiers() & (allModifiers & ~(Qt::ControlModifier | Qt::ShiftModifier))) == Qt::NoModifier) {
                // Ignore state of <CTRL> and <SHIFT> keys
                mHistoryBuffer = 0;
                mCompletion.reset();
                mCompletion.resetHistoryWalk();
                {
                    // This does the actual deletion of the character:
                    const UserEditScope userEdit(*this);
                    QPlainTextEdit::event(event);
                }
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
                mCompletion.reset();
                mCompletion.resetHistoryWalk();
                {
                    // This does the actual deletion of the character:
                    const UserEditScope userEdit(*this);
                    QPlainTextEdit::event(event);
                }
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
                {
                    const UserEditScope userEdit(*this);
                    textCursor().insertBlock();
                }
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
            if ((ke->modifiers() & allModifiers) == (Qt::ControlModifier | Qt::KeypadModifier)) {
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
            if ((ke->modifiers() & allModifiers) == (Qt::ControlModifier | Qt::KeypadModifier)) {
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
                mCompletion.reset();
                mCompletion.restartHistoryWalk();
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
                QTimer::singleShot(0ms, this, [this]() {
                    mpConsole->scrollUp(mpConsole->mUpperPane->getScreenHeight());
                });
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

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::InputMethod) {
        // Keys that fell through the switch, Space among them, and dead-key and IME composition
        const UserEditScope userEdit(*this);
        return QPlainTextEdit::event(event);
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
    if (event->reason() != Qt::ActiveWindowFocusReason && mpHost->mainConsoleView()) {
        mpHost->mainConsoleView()->recordActiveCommandLine(this);
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
    // Redirect focus to main commandline when hiding a SubCommandLine to prevent keyboard input being trapped
    if (mType == enums::SubCommandLine && hasFocus() && mpHost->mainConsoleView() && mpHost->mainConsoleView()->mpCommandLine) {
        mpHost->mainConsoleView()->mpCommandLine->setFocus();
    }

    QPlainTextEdit::hideEvent(event);
}

// Measured from the layout, not font metrics: a row lays out a pixel or two taller than
// QFontMetrics::height(), and falling short leaves a scroll range. All blocks share the first's font.
int TCommandLine::heightForRows(const int rows) const
{
    const QTextBlock firstBlock = document()->firstBlock();
    const qreal documentMargin = document()->documentMargin();
    // blockBoundingRect() lays the block out if needed, but folds the document's bottom margin into the last block.
    qreal blockHeight = document()->documentLayout()->blockBoundingRect(firstBlock).height();
    if (!firstBlock.next().isValid()) {
        blockHeight -= documentMargin;
    }
    const QTextLayout* layout = firstBlock.layout();
    const int rowsInFirstBlock = (layout && layout->lineCount() > 0) ? layout->lineCount() : 1;
    // a viewport that merely matches the text still keeps a row of range, so clear it by one
    const QMargins frame = frameSides();
    return qCeil(blockHeight / rowsInFirstBlock * rows + 2 * documentMargin) + 1 + frame.top() + frame.bottom();
}

// What QFrame derives its frame from, asked of the style: contentsMargins() reads zero until the widget is first shown.
QMargins TCommandLine::frameSides() const
{
    QStyleOptionFrame option;
    initStyleOption(&option);
    const QRect contents = style()->subElementRect(QStyle::SE_ShapedFrameContents, &option, this);
    return {contents.left() - option.rect.left(), contents.top() - option.rect.top(), option.rect.right() - contents.right(), option.rect.bottom() - contents.bottom()};
}

// QAbstractScrollArea adds twice frameWidth(), its widest side, to both axes, and a style sheet's
// padding counts as frame - so padding-left: 1000px alone would demand over 2000px each way.
QSize TCommandLine::minimumSizeHint() const
{
    const QSize hint = QPlainTextEdit::minimumSizeHint();
    const QMargins frame = frameSides();
    const int assumedFrame = 2 * frameWidth();
    return {hint.width() - assumedFrame + frame.left() + frame.right(), hint.height() - assumedFrame + frame.top() + frame.bottom()};
}

void TCommandLine::adjustHeight()
{
    // Make sure adjustHeight won't crash if it's used before mpConsole->layerCommandLine has a value
    if (!mpConsole->layerCommandLine) {
        qWarning() << "TCommandLine::adjustHeight() ERROR: mpConsole->layerCommandLine is NULL!";
        return;
    }
    int lines = static_cast<int>(document()->size().height());
    // Workaround for SubCommandLines textCursor not visible in some situations
    // SubCommandLines cannot autoresize
    if (mType == enums::SubCommandLine) {
        centerRows();
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
    const int marginH = lines > 1 ? 10 : 5;
    // Falling short leaves the vertical scroll bar a range that a click-drag scrolls into,
    // invisibly and irreversibly with the scroll bar switched off.
    int _height = std::max((fontH + 1) * lines + marginH, heightForRows(lines));
    if (_height < mpHost->commandLineMinimumHeight) {
        _height = mpHost->commandLineMinimumHeight;
    }
    if (_height > height() || _height < height()) {
        const int layerHeight = mpConsole->commandLineLayerHeight(_height);
        mpConsole->layerCommandLine->setMinimumHeight(layerHeight);
        mpConsole->layerCommandLine->setMaximumHeight(layerHeight);
        const int x = mpConsole->width();
        const int y = mpConsole->height();
        const QSize s = QSize(x, y);
        QResizeEvent event(s, s);
        QApplication::sendEvent(mpConsole, &event);
    }
    centerRows();
}

// The slack goes one pixel more above the rows than below: heightForRows() leaves its spare pixel under them
void TCommandLine::centerRows()
{
    const int rows = std::max(1, static_cast<int>(document()->size().height()));
    const int slack = std::max(0, height() - heightForRows(rows));
    const int top = std::min(slack, slack / 2 + 1);
    const QMargins margins(0, top, 0, slack - top);
    if (viewportMargins() != margins) {
        setViewportMargins(margins);
        emit rowsMoved();
    }
}

void TCommandLine::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    centerRows();
}

void TCommandLine::changeEvent(QEvent* event)
{
    QPlainTextEdit::changeEvent(event);
    // setCmdLineStyleSheet() can bring a font or a frame with no adjustHeight() to follow
    if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) {
        centerRows();
    }
}

void TCommandLine::spellCheck()
{
    if (!mpHost->getEnableSpellCheck()) {
        return;
    }

    QTextCursor oldCursor = textCursor();
    QTextCursor c = cursorAtWordStart(document(), oldCursor.position());
    spellCheckWord(c);
    QTextCharFormat f;
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
    const QString t = pA->text();
#endif
    QTextCursor c = cursorForPosition(mPopupPosition);
    c.select(QTextCursor::WordUnderCursor);

    c.removeSelectedText();
    c.insertText(t);
    c.clearSelection();

    // Recheck the replacement, which the caret need not be on
    QTextCursor replacement = cursorAtWordStart(document(), c.position());
    spellCheckWord(replacement);
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

    auto codecName = mpHost->spellChecker().systemCodecName();
    auto handle_system = mpHost->spellChecker().systemHandle();
    auto handle_profile = mpHost->spellChecker().userHandle();
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
        action_addWord = new QAction(tr("Add to user dictionary"), popup);
        action_addWord->setEnabled(false);
        action_removeWord = new QAction(tr("Remove from user dictionary"), popup);
        action_removeWord->setEnabled(false);
        // }
        if (MudletApp::usingMudletDictionaries()) {
            /*:
            This line is shown in the list of spelling suggestions on the profile's command
            line context menu to clearly divide up where the suggestions for correct
            spellings are coming from.  The precise format might be modified as long as it
            is clear that the entries below this line in the menu come from the spelling
            dictionary that the user has chosen in the profile setting which we have
            bundled with Mudlet; the entries about this line are the ones that the user
            has personally added.
            */
            action_dictionarySeparatorLine = new QAction(tr("▼Mudlet▼ │ dictionary suggestions │ ▲User▲"), popup);
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
            action_dictionarySeparatorLine = new QAction(tr("▼System▼ │ dictionary suggestions │ ▲User▲"), popup);
        }
        action_dictionarySeparatorLine->setEnabled(false);
    }

    QList<QAction*> spellings_system;
    QList<QAction*> spellings_profile;
    // We always use UTF-8 for the per profile/shared dictionary so we do not
    // need to have a codec prepared for it and can use QString::toUtf8()
    // directly:
    const QByteArray utf8Text = mSpellCheckedWord.toUtf8();
    int systemSuggestionsCount = 0;
    int userSuggestionsCount = 0;
    char** pSystemSuggestionsList = nullptr;
    char** pUserSuggestionsList = nullptr;
    if (handle_system && !codecName.isEmpty()) {
        // The dictionary used from "the system" may not be UTF-8 encoded so we
        // will need to transform the UTF-16BE "QString" to the appropriate encoding:
        const QByteArray encodedText = TEncodingHelper::encode(mSpellCheckedWord, codecName);
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

        systemSuggestionsCount = Hunspell_suggest(handle_system, &pSystemSuggestionsList, encodedText.constData());
    }

    if (handle_profile) {
        userSuggestionsCount = Hunspell_suggest(handle_profile, &pUserSuggestionsList, utf8Text.constData());
    }

    if (systemSuggestionsCount) {
        for (int i = 0; i < systemSuggestionsCount; ++i) {
            auto pA = new QAction(TEncodingHelper::decode(pSystemSuggestionsList[i], codecName), popup);
#if defined(Q_OS_FREEBSD)
            // Adding the text afterwards as user data as well as in the
            // constructor is to fix a bug(?) in FreeBSD that
            // automagically adds a '&' somewhere in the text to be a
            // shortcut - but doesn't show it and forgets to remove
            // it when asked for the text later:
            pA->setData(TEncodingHelper::decode(pSystemSuggestionsList[i], codecName));
#endif
            connect(pA, &QAction::triggered, this, &TCommandLine::slot_popupMenu);
            spellings_system << pA;
        }

    } else {
        /*:
        Used when the command spelling checker using the selected system dictionary has
        no words to suggest.
        */
        auto pA = new QAction(tr("no suggestions (system)"), popup);
        pA->setEnabled(false);
        spellings_system << pA;
    }

    if (handle_profile) {
        if (userSuggestionsCount) {
            for (int i = 0; i < userSuggestionsCount; ++i) {
                auto pA = new QAction(QString::fromUtf8(pUserSuggestionsList[i]), popup);
#if defined(Q_OS_FREEBSD)
                // Adding the text afterwards as user data as well as in the
                // constructor is to fix a bug(?) in FreeBSD that
                // automagically adds a '&' somewhere in the text to be a
                // shortcut - but doesn't show it and forgets to remove
                // it when asked for the text later:
                pA->setData(QString::fromUtf8(pUserSuggestionsList[i]));
#endif
                connect(pA, &QAction::triggered, this, &TCommandLine::slot_popupMenu);
                spellings_profile << pA;
            }

        } else {
            QAction* pA = nullptr;
            if (mpHost->spellChecker().usingSharedDictionary()) {
                /*:
                Used when the command spelling checker using the dictionary shared between
                profile has no words to suggest.
                */
                pA = new QAction(tr("no suggestions (shared)"), popup);
            } else {
                /*:
                Used when the command spelling checker using the profile's own dictionary has
                no words to suggest.
                */
                pA = new QAction(tr("no suggestions (profile)"), popup);
            }
            pA->setEnabled(false);
            spellings_profile << pA;
        }
    }

    // The actions hold copies of the suggestions, so the lists can go now
    if (pSystemSuggestionsList) {
        Hunspell_free_list(handle_system, &pSystemSuggestionsList, systemSuggestionsCount);
    }
    if (pUserSuggestionsList) {
        Hunspell_free_list(handle_profile, &pUserSuggestionsList, userSuggestionsCount);
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

    auto separator_aboveStandardMenu = popup->insertSeparator(popup->actions().constFirst());
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
    if (event->button() == Qt::RightButton) {
        auto popup = createStandardContextMenu(event->globalPosition().toPoint());
        popup->setAttribute(Qt::WA_DeleteOnClose);
        if (mpHost->getEnableSpellCheck()) {
            fillSpellCheckList(event, popup);
            // else the word is in the dictionary - in either case show the context
            // menu - either the one with the prefixed spellings, or the standard one
        }

        popup->addSeparator();
        for (const auto& [label, eventName] : contextMenuItems.asKeyValueRange()) {
            auto action = new QAction(label, popup);
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
    // The aliases and event handlers run below may write to the line
    mPlayerTypedLine = false;
    mCompletion.reset();
    mCompletion.resetHistoryWalk();

    // Emit signal for hyperlink visibility triggers before processing the command
    emit commandSubmitted();

    QStringList commandList = toPlainText().split(QChar::LineFeed);

    for (QString& command : commandList) {
        if (mType != enums::MainCommandLine && mActionFunction) {
            mpHost->getLuaInterpreter()->callCmdLineAction(mActionFunction, command);
        } else {
            mpHost->send(command);
            // A line handed to a Lua action is not the game's to answer
            mpHost->playerSentLineFromCommandLine();
        }
        // send command to your MiniConsole
        if (mType == enums::ConsoleCommandLine && !mActionFunction && mpHost->mCommandEchoMode != Host::CommandEchoMode::Never) {
            // This usage of commandList modifies the content!!!
            mpConsole->printCommand(command);
        }
    }

    // A line typed past the hidden-input box stays out of the on-disk history
    if (!toPlainText().isEmpty() && (!mpHost->isRemoteEchoingActive() || mpHost->disablePasswordMasking())) {
        if (mpHost->mAutoClearCommandLineAfterSend) {
            mHistoryBuffer = 0;
        } else {
            mHistoryBuffer = 1;
        }

        TCommandHistory::add(mHistoryList, toPlainText());
    }

    if (mpHost->mAutoClearCommandLineAfterSend) {
#if defined(Q_OS_MACOS)
        // clearing the input line on macOS 11.6 makes VoiceOver announce the removed text,
        // essentially re-announcing everything we've typed. This workaround fixes this behaviour
        // and does not seem to negatively affect other platforms
        hide();
#endif
        clear();
#if defined(Q_OS_MACOS)
        show();
#endif
    } else {
        selectAll();
    }
    adjustHeight();
}

void TCommandLine::handleTabCompletion(bool direction)
{
    const auto completion = mCompletion.tabComplete(toPlainText(), direction, mpHost->mainConsoleView()->buffer, commandLineSuggestions, tabCompleteBlacklist);
    if (!completion) {
        return;
    }
    setPlainText(completion->line);
    mudlet::self()->announce(completion->word);
    moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    mCompletion.setShownLine(toPlainText());
}

// Hitting the cursor up key gets you in autocompletion mode.
// in this mode mudlet tries to find matching substrings in the user's
// command history. Repeated usage of the cursor up key cycles through
// all possible matches. If the user keeps on typing more letters the
// the set of possible matches is getting smaller. The ESC key brings you back to regular mode

void TCommandLine::handleAutoCompletion(const bool forward)
{
    const QString line = toPlainText();
    const QTextCursor playerCursor = textCursor();
    const bool tailSelected = playerCursor.selectionStart() == mCompletion.shownLine().size() && playerCursor.selectionEnd() == line.size();
    const auto completion = mCompletion.historyComplete(line, tailSelected, mHistoryList, forward);
    if (completion.tailCut) {
        setPlainText(completion.typed);
    }
    mudlet::self()->announce(completion.typed);
    if (completion.line) {
        setPlainText(*completion.line);
        // As the document gives it back, which turns a no-break space into a plain one
        mCompletion.setHistoryCompletion(toPlainText());
        mudlet::self()->announce(*completion.line);
        // By position, not by moving Right: that steps whole graphemes, which an emoji or accent spans several of
        QTextCursor shown = textCursor();
        shown.setPosition(completion.typed.size());
        shown.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        setTextCursor(shown);
        return;
    }
    if (completion.tailCut) {
        moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    } else {
        setTextCursor(playerCursor);
    }
}

// cursor up/down: turns on autocompletion mode and cycles through all possible matches
// In case nothing has been typed it cycles through the command history in
// reverse order compared to cursor down.

void TCommandLine::historyMove(MoveDirection direction)
{
    // DOWN at position 0 with text: save to history and clear input
    if (direction == MOVE_DOWN && mHistoryBuffer == 0 && !toPlainText().isEmpty() && (!mpHost->isRemoteEchoingActive() || mpHost->disablePasswordMasking())) {
        TCommandHistory::add(mHistoryList, toPlainText());

        clear();
        adjustHeight();
        return;
    }

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
        handleAutoCompletion(direction == MOVE_UP);
    }
    adjustHeight();
}

void TCommandLine::slot_clearSelection(bool yes)
{
    if (yes) {
        mpConsole->clearSelection();
    }
}

void TCommandLine::slot_removeWord()
{
    if (mSpellCheckedWord.isEmpty()) {
        return;
    }

    mpHost->spellChecker().removeWord(mSpellCheckedWord);
    // The word right-clicked need not be the one the caret is on, and it
    // may be in the line more than once
    recheckWholeLine();
}

void TCommandLine::slot_addWord()
{
    if (mSpellCheckedWord.isEmpty()) {
        return;
    }

    mpHost->spellChecker().addWord(mSpellCheckedWord);
    // The word right-clicked need not be the one the caret is on, and it
    // may be in the line more than once
    recheckWholeLine();
}

void TCommandLine::spellCheckWord(QTextCursor& c)
{
    if (!mpHost->getEnableSpellCheck()) {
        return;
    }

    Hunhandle* systemDictionaryHandle = mpHost->spellChecker().systemHandle();

    QTextCharFormat f;
    c.select(QTextCursor::WordUnderCursor);
    const QString spellCheckedWord = c.selectedText();
    const bool wantSpellCheck = systemDictionaryHandle && TBuffer::lengthInGraphemes(spellCheckedWord) >= mudlet::self()->mMinLengthForSpellCheck;
    if (!wantSpellCheck) {
        // We don't check when there is no dictionary or the word is too short,
        // but may need to undo any prior underline:
        f.setFontUnderline(false);
        c.setCharFormat(f);
        return;
    }

    // The dictionary used from "the system" may not be UTF-8 encoded so we
    // will need to transform the UTF-16BE "QString" to the appropriate encoding:
    const QByteArray codecName = mpHost->spellChecker().systemCodecName();
    if (codecName.isEmpty()) {
        // If we don't know the encoding, we can't safely spell-check
        f.setFontUnderline(false);
        c.setCharFormat(f);
        return;
    }

    const QByteArray encodedText = TEncodingHelper::encode(spellCheckedWord, codecName);
    if (!Hunspell_spell(systemDictionaryHandle, encodedText.constData())) {
        // Word is not in selected system dictionary
        Hunhandle* userDictionaryhandle = mpHost->spellChecker().userHandle();
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
    if (!mpHost->getEnableSpellCheck()) {
        return;
    }

    QTextCursor c(document());
    const int length = document()->characterCount() - 1;
    int position = 0;
    while (position < length) {
        c.setPosition(position);
        c.select(QTextCursor::WordUnderCursor);
        if (!c.hasSelection()) {
            ++position;
            continue;
        }
        const int wordEnd = c.selectionEnd();
        QTextCursor word = cursorAtWordStart(document(), c.selectionStart());
        spellCheckWord(word);
        position = std::max(wordEnd, position + 1);
    }
}

void TCommandLine::clearMarksOnWholeLine()
{
    QTextCursor oldCursor = textCursor();
    QTextCharFormat f;
    QTextCursor c(document());
    c.select(QTextCursor::Document);
    c.setCharFormat(f);
    f.setFontUnderline(false);
    oldCursor.setCharFormat(f);
    setTextCursor(oldCursor);
}

void TCommandLine::setAction(const int func)
{
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
    const bool multipleProfilesActive = (HostManager::self()->getHostCount() > 1);
    const QString hostName{mpHost ? mpHost->getName() : QString()};
    switch (mType) {
    case enums::MainCommandLine:
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
                                        "to run commands locally.")
                                             .arg(hostName));
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
    case enums::SubCommandLine:
        if (multipleProfilesActive) {
            /*:
            Accessibility-friendly name to describe an extra command line on
            top of console/window when more than one profile is loaded, %1 is
            the command line name, %2 is the name of the window/console that
            it is on and %3 is the name of the profile.
            */
            setAccessibleName(tr("Additional input line \"%1\" on \"%2\" window of \"%3\"profile.").arg(mCommandLineName, mpConsole->mConsoleName, hostName));
            /*:
            Accessibility-friendly description for an extra command line on top of a
            console/window when more than one profile is loaded, %1 is the profile
            name.
            */
            setAccessibleDescription(tr("Type in text to send to the game server for the \"%1\" profile, or enter an alias "
                                        "to run commands locally.")
                                             .arg(hostName));
        } else {
            /*:
            Accessibility-friendly name to describe an extra command line on
            top of console/window when only one profile is loaded, %1 is the
            command line name and %2 is the name of the window/console that
            it is on.
            */
            setAccessibleName(tr("Additional input line \"%1\" on \"%2\" window.").arg(mCommandLineName, mpConsole->mConsoleName));
            /*:
            Accessibility-friendly description for an extra command line on
            top of a console/window when only one profile is loaded.
            */
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally."));
        }
        break;
    case enums::ConsoleCommandLine:
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
                                        "to run commands locally.")
                                             .arg(hostName));
        } else {
            /*:
            Accessibility-friendly name to describe the built-in command line
            of a console/window other than the main one, when only one
            profile is loaded, %1 is the name of the window/console.
            */
            setAccessibleName(tr("Input line of \"%1\" window.").arg(mCommandLineName));
            /*:
            Accessibility-friendly description for the built-in command line of a
            console/window other than the main window's one when only one profile is
            loaded.
            */
            setAccessibleDescription(tr("Type in text to send to the game server, or enter an alias to run commands "
                                        "locally."));
        }
        break;
    case enums::UnknownCommandLine:
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

    QString pathFileName{MudletApp::getMudletPath(enums::profileDataItemPath, pHost->getName(), mBackingFileName)};
    if (!QFileInfo::exists(pathFileName)) {
        // which will be the case for the first time the command line is
        // created - so it might not be an error:
        if (mCommandLineName != qsl("main")) {
            qDebug() << "TCommandLine::restoreHistory() ALERT - unable to open command history for the command line called: " << mCommandLineName << " of type: " << mType
                     << " because the file: " << mBackingFileName << " does not exist in the profile's home directory, unless this is a new command line then this is an unexpected error.";
        }
        return;
    }
    QString error;
    if (!TCommandHistory::read(pathFileName, mHistoryList, error)) {
        qWarning() << "TCommandLine::restoreHistory() ERROR - unable to read command history from file for the command line called: " << mCommandLineName << " of type: " << mType
                   << " reason: " << error;
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

    QString pathFileName{MudletApp::getMudletPath(enums::profileDataItemPath, pHost->getName(), mBackingFileName)};
    QString error;
    if (!TCommandHistory::write(pathFileName, mHistoryList, saveSize, error)) {
        qDebug().nospace().noquote() << "TCommandLine::slot_saveHistory() ERROR - unable to save command history for the command line called: " << mCommandLineName << " of type: " << mType
                                     << " reason: " << error;
    }
}

/* We need to have this method defined in this file as the TConsole class is
 * forward declared in the header file and it is problematic to define any
 * dereferencing of it there:*/
TConsole* TCommandLine::console() const
{
    return mpConsole;
}
