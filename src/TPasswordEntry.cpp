/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include "TPasswordEntry.h"

#include "Host.h"
#include "HostManager.h"
#include "KeyUnit.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "mudlet.h"

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QTimer>

using namespace std::chrono_literals;

TPasswordEntry::TPasswordEntry(Host* pHost, TCommandLine* pCommandLine, QWidget* parent)
: QLineEdit(parent)
, mpHost(pHost)
, mpCommandLine(pCommandLine)
{
    setObjectName(qsl("passwordEntry_%1").arg(pHost->getName()));
    setFont(pCommandLine->font());
    // The command line's palette sets its text colour after construction, so
    // its placeholder colour is still derived from the default text colour -
    // black on a black command line. The placeholder carries the Esc hint, so
    // it is derived from the text colour that is actually in use.
    QPalette palette = pCommandLine->mRegularPalette;
    QColor placeholderColor = palette.color(QPalette::Text);
    placeholderColor.setAlpha(128);
    palette.setColor(QPalette::PlaceholderText, placeholderColor);
    setPalette(palette);

    mpRevealAction = addAction(QIcon(qsl(":/icons/password-show-on.png")), QLineEdit::TrailingPosition);
    connect(mpRevealAction, &QAction::triggered, this, [this]() {
        setRevealed(echoMode() != QLineEdit::Normal);
    });
    setRevealed(false);

    //: Placeholder text of the box the game's request for hidden input is answered with
    setPlaceholderText(tr("Hidden input - Esc to answer in the command line instead"));

    // So that a keychain password arriving late cannot be typed over a player
    // who has started answering
    connect(this, &QLineEdit::textEdited, pHost, &Host::passwordEntryEdited);

    // A revealed password is still not for the selection clipboard: QLineEdit
    // copies a Normal-mode selection there on every mouse release and keyboard
    // selection, and a middle click would then paste it into a command line
    if (QClipboard* pClipboard = QGuiApplication::clipboard(); pClipboard->supportsSelection()) {
        connect(pClipboard, &QClipboard::selectionChanged, this, &TPasswordEntry::slot_selectionClipboardChanged);
    }

    connect(mudlet::self(), &mudlet::signal_adjustAccessibleNames, this, &TPasswordEntry::slot_adjustAccessibleNames);
    slot_adjustAccessibleNames();
}

void TPasswordEntry::slot_selectionClipboardChanged()
{
    QClipboard* pClipboard = QGuiApplication::clipboard();
    if (echoMode() != QLineEdit::Normal || !pClipboard->ownsSelection()) {
        return;
    }
    // Only what came from this box: a selection made in an output pane
    // meanwhile is left alone. Cleared before any other application can ask
    // for it, since requests are served by this event loop.
    if (pClipboard->text(QClipboard::Selection) == selectedText()) {
        pClipboard->clear(QClipboard::Selection);
    }
}

void TPasswordEntry::setRevealed(const bool revealed)
{
    setEchoMode(revealed ? QLineEdit::Normal : QLineEdit::Password);
    // setEchoMode(Normal) clears these, so they are put back after every change
    setInputMethodHints(inputMethodHints() | Qt::ImhHiddenText | Qt::ImhSensitiveData | Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase);
    if (revealed) {
        mpRevealAction->setIcon(QIcon(qsl(":/icons/password-show-off.png")));
        //: Name and tooltip of the button that hides the text typed into the hidden-input box again
        mpRevealAction->setText(tr("Hide password"));
    } else {
        mpRevealAction->setIcon(QIcon(qsl(":/icons/password-show-on.png")));
        //: Name and tooltip of the button that shows the text typed into the hidden-input box
        mpRevealAction->setText(tr("Show password"));
    }
    mpRevealAction->setToolTip(mpRevealAction->text());
}

void TPasswordEntry::setReopened()
{
    mReopened = true;
    //: Placeholder text of the hidden-input box when it opens again after the player pressed Esc on an empty one and their next line did not make the game stop hiding input
    setPlaceholderText(tr("Still hidden - Esc again to stop hiding input until the game says otherwise"));
    slot_adjustAccessibleNames();
}

void TPasswordEntry::slot_adjustAccessibleNames()
{
    const bool multipleProfilesActive = (HostManager::self()->getHostCount() > 1);
    const QString hostName{mpHost ? mpHost->getName() : QString()};
    if (multipleProfilesActive) {
        //: Accessibility-friendly name of the hidden-input box when more than one profile is loaded, %1 is the profile name
        setAccessibleName(tr("Hidden input for \"%1\" profile.").arg(hostName));
    } else {
        //: Accessibility-friendly name of the hidden-input box when only one profile is loaded
        setAccessibleName(tr("Hidden input."));
    }
    if (mReopened) {
        //: Accessibility-friendly description of the hidden-input box after the player stepped past it once with Esc
        setAccessibleDescription(tr("The game is still hiding what you type. Enter sends the text straight to the game, without aliases or history. "
                                    "Esc empties the box; Esc on an empty box stops hiding input until the game says otherwise."));
    } else {
        //: Accessibility-friendly description of the hidden-input box
        setAccessibleDescription(tr("The game asks for hidden input. Enter sends the text straight to the game, without aliases or history. "
                                    "Esc empties the box; Esc on an empty box closes it so you can use the command line instead."));
    }
}

void TPasswordEntry::submit()
{
    // Hidden again before the text goes, so that VoiceOver does not read the
    // removed text aloud and a later destruction zero-fills the buffer - and
    // before the text is read, since changing the echo mode makes Qt copy it
    setRevealed(false);
    // The one place the text leaves the box
    QString line = text();
    // A pasted line break must never make a second line: sendData() strips only
    // the line feed
    line.remove(QChar::CarriageReturn);
    line.remove(QChar::LineFeed);
    // Also clears the undo history, and leaves `line` the last holder of the
    // buffer the keystrokes went into, for the send path to zero
    setText(QString());
    if (mpHost->sendPasswordEntry(std::move(line))) {
        //: Placeholder text of the hidden-input box after Enter, while the game still hides input
        setPlaceholderText(tr("Sent - waiting for the game"));
    } else {
        // Nothing to write to - the connection is gone, or this is a replay -
        // and the text is already dropped, so say so rather than claim a send
        //: Placeholder text of the hidden-input box after Enter when the line could not be sent because Mudlet is not connected to the game
        setPlaceholderText(tr("Not sent - not connected to the game"));
        //: Shown in the game window when Enter in the hidden-input box could not send the line because Mudlet is not connected to the game
        mpHost->postMessage(tr("[ WARN ]  - The line typed into the hidden-input box was not sent: Mudlet is not connected to the game."));
    }
    emit submitted();
}

// What Ctrl+C means from the command line when an output pane holds a
// selection: that selection, never the command line's own text.
bool TPasswordEntry::copyConsoleSelection()
{
    TConsole* pConsole = mpCommandLine->console();
    if (!pConsole) {
        return false;
    }
    for (TTextEdit* pPane : {pConsole->mUpperPane, pConsole->mLowerPane}) {
        if (pPane && !pPane->mSelectedRegion.isEmpty()) {
            pPane->slot_copySelectionToClipboard();
            return true;
        }
    }
    return false;
}

bool TPasswordEntry::event(QEvent* event)
{
    if (!mpHost || mpHost->isClosingDown() || !mpCommandLine) {
        return QLineEdit::event(event);
    }

    if (event->type() == QEvent::ShortcutOverride) {
        // The two claims the command line makes, so that the caret-mode shortcut
        // and a user binding on a profile-switch shortcut arrive here as key
        // presses. Every other ShortcutOverride is QLineEdit's to claim or not:
        // accepting them wholesale would kill every application shortcut.
        if (mpCommandLine->claimsShortcutOverride(static_cast<QKeyEvent*>(event))) {
            event->accept();
            return true;
        }
        return QLineEdit::event(event);
    }

    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        handleKeyPress(ke);
        // Handled or not: a key press that is not both accepted and reported
        // handled propagates up the parent chain, and while no ancestor handles
        // keys, this is the belt to that brace.
        ke->accept();
        return true;
    }

    return QLineEdit::event(event);
}

void TPasswordEntry::handleKeyPress(QKeyEvent* ke)
{
    // macOS delivers the arrow keys with the keypad modifier, so it does not
    // count as one - as TCommandLine treats them
    const Qt::KeyboardModifiers modifiers = ke->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::GroupSwitchModifier);

    // A screen-reader user must be able to leave for the output pane to re-read
    // the prompt without losing the box
    if (mpHost->caretShortcutMatches(ke)) {
        mpHost->setCaretEnabled(true);
        return;
    }

    switch (ke->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (modifiers == Qt::NoModifier) {
            submit();
            return;
        }
        break;
    case Qt::Key_Escape:
        if (modifiers == Qt::NoModifier) {
            if (text().isEmpty()) {
                emit dismissed();
            } else {
                // Start over; also clears the undo history
                setText(QString());
            }
            return;
        }
        break;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
    case Qt::Key_Up:
    case Qt::Key_Down:
        // No focus change, no history, no completion
        return;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        if (modifiers == Qt::NoModifier) {
            scrollConsole(ke->key() == Qt::Key_PageUp);
            return;
        }
        break;
    default:
        break;
    }

    if (ke->matches(QKeySequence::Copy)) {
        copyConsoleSelection();
        return;
    }
    // In both echo modes: a revealed password is still not for the clipboard.
    // setText() after a submit or an Esc already clears the undo history, so
    // the undo swallow is defence in depth.
    if (ke->matches(QKeySequence::Cut) || ke->matches(QKeySequence::Undo) || ke->matches(QKeySequence::Redo)) {
        return;
    }

    // A printable key with no modifier, or with the ones a keyboard layout uses
    // to reach characters - AltGr arrives as Ctrl+Alt on Windows, Option as Alt
    // on macOS - is typed. Everything else is offered to the key bindings first,
    // so an F-key bound to a login alias runs and a Ctrl+letter binding runs,
    // while a numpad digit bound to a direction still types a digit of the
    // password. Stricter than the command line, which offers every key.
    const QString text = ke->text();
    const bool printable = !text.isEmpty() && text.front().isPrint();
    const Qt::KeyboardModifiers modifierKeys = ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    const bool altGr = (modifierKeys & Qt::ControlModifier) && (modifierKeys & Qt::AltModifier);
#if defined(Q_OS_MACOS)
    const bool layoutModifier = altGr || modifierKeys == Qt::AltModifier;
#else
    const bool layoutModifier = altGr;
#endif
    if (printable && (modifierKeys == Qt::NoModifier || layoutModifier)) {
        QLineEdit::event(ke);
        return;
    }

    if (mpHost->getKeyUnit()->processDataStream(static_cast<Qt::Key>(ke->key()), ke->modifiers())) {
        return;
    }

    // Ctrl+digit switches profile tabs from the command line; a user binding on
    // the same key has already had its turn above, as it has there
    if ((ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier)) == Qt::ControlModifier && ke->key() >= Qt::Key_0
        && ke->key() <= Qt::Key_9) {
        const int tabNumber = ke->key() == Qt::Key_0 ? 10 : (ke->key() - Qt::Key_0);
        if (mudlet::self()->mpTabBar->count() >= tabNumber) {
            mudlet::self()->slot_tabChanged(tabNumber - 1);
            return;
        }
    }

    QLineEdit::event(ke);
}

void TPasswordEntry::scrollConsole(const bool up)
{
    TConsole* pConsole = mpCommandLine->console();
    if (!pConsole) {
        return;
    }
    if (up) {
        pConsole->scrollUp(0);
        QTimer::singleShot(0ms, this, [this]() {
            if (TConsole* pC = mpCommandLine ? mpCommandLine->console() : nullptr) {
                pC->scrollUp(pC->mUpperPane->getScreenHeight());
            }
        });
    } else {
        pConsole->scrollDown(pConsole->mUpperPane->getScreenHeight());
    }
}

void TPasswordEntry::focusInEvent(QFocusEvent* event)
{
    // Keeps Host::setFocusOnHostActiveCommandLine() and the caret-mode key
    // forwarder landing here through the command line's focus proxy - but not
    // for a switch away from and back to the application, which would mess up
    // the record just as it would for the command line itself
    if (event->reason() != Qt::ActiveWindowFocusReason && mpHost && mpCommandLine) {
        mpHost->recordActiveCommandLine(mpCommandLine);
    }
    QLineEdit::focusInEvent(event);
}

void TPasswordEntry::mousePressEvent(QMouseEvent* event)
{
    QLineEdit::mousePressEvent(event);
    if (mpHost) {
        mudlet::self()->activateProfile(mpHost);
    }
}

void TPasswordEntry::mouseReleaseEvent(QMouseEvent* event)
{
    QLineEdit::mouseReleaseEvent(event);
    if (mpHost) {
        mudlet::self()->activateProfile(mpHost);
    }
}

void TPasswordEntry::contextMenuEvent(QContextMenuEvent* event)
{
    // Paste only: the standard menu's copy, cut, select-all and undo would put
    // the text where it must not go
    auto* pMenu = new QMenu(this);
    pMenu->setAttribute(Qt::WA_DeleteOnClose);
    //: Context menu entry of the hidden-input box
    pMenu->addAction(tr("Paste"), this, &QLineEdit::paste);
    pMenu->popup(event->globalPos());
    event->accept();
}
