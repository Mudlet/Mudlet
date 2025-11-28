/***************************************************************************
 *   Copyright (C) 2008-2010 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2022 by Stephen Lyons - slysven@virginmedia.com   *
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


#include "dlgComposer.h"


#include "Host.h"
#include "TBuffer.h"
#include "TEncodingHelper.h"
#include "TMainConsole.h"
#include "mudlet.h"

#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTextCursor>


dlgComposer::dlgComposer(Host* pH)
: mpHost(pH)
{
    setupUi(this);
    const QFont font = QFont(qsl("Bitstream Vera Sans Mono"), 10, QFont::Normal);
    edit->setFont(font);
    connect(saveButton, &QAbstractButton::clicked, this, &dlgComposer::slot_save);
    connect(cancelButton, &QAbstractButton::clicked, this, &dlgComposer::slot_cancel);

    // Set up spellcheck - use event filter for deferred checking
    edit->installEventFilter(this);
    edit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(edit, &QWidget::customContextMenuRequested, this, &dlgComposer::slot_contextMenu);

    setAttribute(Qt::WA_DeleteOnClose);
}

void dlgComposer::slot_cancel()
{
    mpHost->mTelnet.atcpComposerCancel();
    this->hide();
}

void dlgComposer::slot_save()
{
    mpHost->mTelnet.atcpComposerSave(edit->toPlainText());
    this->hide();
}

void dlgComposer::init(const QString &newTitle, const QString &newText)
{
    title->setText(newTitle);
    edit->setPlainText(newText);
    if (mpHost && mpHost->mEnableSpellCheck) {
        recheckWholeLine();
    }
}

bool dlgComposer::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == edit && event->type() == QEvent::KeyPress && mpHost && mpHost->mEnableSpellCheck) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        QTextCursor oldCursor = edit->textCursor();
        oldCursor.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        oldCursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        const int wordStart = oldCursor.selectionStart();
        const int wordEnd = oldCursor.selectionEnd();

        bool result = QMainWindow::eventFilter(obj, event);

        bool isDelimiter = false;
        const QString text = keyEvent->text();
        if (!text.isEmpty()) {
            const QChar ch = text.at(0);
            isDelimiter = ch.isSpace() || ch.isPunct();
        }

        bool leftWord = false;
        if (text.isEmpty() && !isDelimiter) {
            const int pos = edit->textCursor().position();
            leftWord = pos < wordStart || pos > wordEnd;
        }

        if (isDelimiter || leftWord) {
            slot_spellCheck();
        }

        return result;
    }

    return QMainWindow::eventFilter(obj, event);
}

void dlgComposer::slot_spellCheck()
{
    if (!mpHost || !mpHost->mEnableSpellCheck) {
        return;
    }

    if (mSpellChecking) {
        return;
    }

    mSpellChecking = true;
    QTextCursor cursor = edit->textCursor();
    int originalPosition = cursor.position();

    spellCheckWord(cursor);

    cursor.setPosition(originalPosition);
    QTextCharFormat clearFormat;
    clearFormat.setFontUnderline(false);
    cursor.setCharFormat(clearFormat);
    edit->setTextCursor(cursor);
    mSpellChecking = false;
}

void dlgComposer::spellCheckWord(QTextCursor& c)
{
    if (!mpHost || !mpHost->mEnableSpellCheck) {
        return;
    }

    Hunhandle* systemDictionaryHandle = mpHost->mpConsole->getHunspellHandle_system();
    if (!systemDictionaryHandle) {
        return;
    }

    QTextCharFormat f;
    // Use StartOfWord/EndOfWord for precise selection without whitespace
    c.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString spellCheckedWord = c.selectedText();

    const bool wantSpellCheck = TBuffer::lengthInGraphemes(spellCheckedWord) >= mudlet::self()->mMinLengthForSpellCheck;
    if (!wantSpellCheck) {
        f.setFontUnderline(false);
        c.setCharFormat(f);
        return;
    }

    // The dictionary used from "the system" may not be UTF-8 encoded so we
    // will need to transform the UTF-16BE "QString" to the appropriate encoding:
    const QByteArray codecName = mpHost->mpConsole->getHunspellCodecName_system();
    if (codecName.isEmpty()) {
        f.setFontUnderline(false);
        c.setCharFormat(f);
        return;
    }

    const QByteArray encodedText = TEncodingHelper::encode(spellCheckedWord, codecName);
    if (!Hunspell_spell(systemDictionaryHandle, encodedText.constData())) {
        Hunhandle* userDictionaryhandle = mpHost->mpConsole->getHunspellHandle_user();
        if (userDictionaryhandle) {
            // The per-profile/shared dictionary is always UTF-8 encoded - so
            // we can use QString::toUtf8() directly to get the bytes needed:
            if (Hunspell_spell(userDictionaryhandle, spellCheckedWord.toUtf8().constData())) {
                // Use dash underline for words in user dictionary
                f.setUnderlineStyle(QTextCharFormat::DashUnderline);
                f.setUnderlineColor(Qt::cyan);
            } else {
                f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
                f.setUnderlineColor(Qt::red);
            }
        } else {
            f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
            f.setUnderlineColor(Qt::red);
        }
    } else {
        f.setFontUnderline(false);
    }
    c.setCharFormat(f);
}

void dlgComposer::recheckWholeLine()
{
    if (!mpHost || !mpHost->mEnableSpellCheck) {
        return;
    }

    if (mSpellChecking) {
        return;
    }

    mSpellChecking = true;
    // Save the current position
    const QTextCursor oldCursor = edit->textCursor();

    QTextCursor c = edit->textCursor();
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
    edit->setTextCursor(oldCursor);
    mSpellChecking = false;
}

void dlgComposer::slot_contextMenu(const QPoint& pos)
{
    auto* popup = edit->createStandardContextMenu();
    if (mpHost && mpHost->mEnableSpellCheck) {
        // Convert from widget coordinates to viewport coordinates
        QPoint viewportPos = edit->viewport()->mapFromParent(pos);
        QMouseEvent mouseEvent(QEvent::MouseButtonPress, viewportPos, edit->mapToGlobal(pos),
                               Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        fillSpellCheckList(&mouseEvent, popup);
    }

    mPopupPosition = pos;
    popup->popup(edit->mapToGlobal(pos));
}

void dlgComposer::fillSpellCheckList(QMouseEvent* event, QMenu* popup)
{
    QTextCursor c = edit->cursorForPosition(event->pos());
    // Use StartOfWord/EndOfWord for precise selection without whitespace
    c.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    mSpellCheckedWord = c.selectedText();

    const bool wantSpellCheck = TBuffer::lengthInGraphemes(mSpellCheckedWord) >= mudlet::self()->mMinLengthForSpellCheck;
    if (!wantSpellCheck) {
        return;
    }

    auto codecName = mpHost->mpConsole->getHunspellCodecName_system();
    auto handle_system = mpHost->mpConsole->getHunspellHandle_system();
    auto handle_profile = mpHost->mpConsole->getHunspellHandle_user();
    bool haveAddOption = false;
    bool haveRemoveOption = false;
    bool wordIsMisspelled = false;
    QAction* action_addWord = nullptr;
    QAction* action_removeWord = nullptr;
    QAction* action_dictionarySeparatorLine = nullptr;

    // We always use UTF-8 for the per profile/shared dictionary so we do not
    // need to have a codec prepared for it and can use QString::toUtf8()
    // directly:
    const QByteArray utf8Text = mSpellCheckedWord.toUtf8();

    if (handle_system && !codecName.isEmpty()) {
        // The dictionary used from "the system" may not be UTF-8 encoded so we
        // will need to transform the UTF-16BE "QString" to the appropriate encoding:
        const QByteArray encodedText = TEncodingHelper::encode(mSpellCheckedWord, codecName);
        if (!Hunspell_spell(handle_system, encodedText.constData())) {
            if (handle_profile) {
                if (!Hunspell_spell(handle_profile, utf8Text.constData())) {
                    wordIsMisspelled = true;
                    haveAddOption = true;
                } else {
                    haveRemoveOption = true;
                }
            } else {
                wordIsMisspelled = true;
            }
        }
    }

    if (!wordIsMisspelled) {
        return;
    }

    if (handle_profile) {
        //: Context menu action to add a word to the user's personal dictionary
        action_addWord = new QAction(tr("Add to user dictionary"));
        action_addWord->setEnabled(false);
        //: Context menu action to remove a word from the user's personal dictionary
        action_removeWord = new QAction(tr("Remove from user dictionary"));
        action_removeWord->setEnabled(false);
        if (mudlet::self()->mUsingMudletDictionaries) {
            /*:
            This separator line in the spell-check context menu divides suggestions
            from the user's personal dictionary (above) and Mudlet's built-in dictionary (below).
            The symbols are decorative and help indicate the direction. This appears in the composer window.
            */
            action_dictionarySeparatorLine = new QAction(tr("▼Mudlet▼ │ dictionary suggestions │ ▲User▲"));
        } else {
            /*:
            This separator line in the spell-check context menu divides suggestions
            from the user's personal dictionary (above) and the system dictionary (below).
            The symbols are decorative and help indicate the direction. This appears in the composer window.
            */
            action_dictionarySeparatorLine = new QAction(tr("▼System▼ │ dictionary suggestions │ ▲User▲"));
        }
        action_dictionarySeparatorLine->setEnabled(false);

        if (haveAddOption) {
            action_addWord->setEnabled(true);
            connect(action_addWord, &QAction::triggered, this, &dlgComposer::slot_addWord);
        }
        if (haveRemoveOption) {
            action_removeWord->setEnabled(true);
            connect(action_removeWord, &QAction::triggered, this, &dlgComposer::slot_removeWord);
        }
    }

    QList<QAction*> spellings_system;
    QList<QAction*> spellings_profile;

    if (!(handle_system && !codecName.isEmpty())) {
        mSystemDictionarySuggestionsCount = 0;
    } else {
        const QByteArray encodedText = TEncodingHelper::encode(mSpellCheckedWord, codecName);
        mSystemDictionarySuggestionsCount = Hunspell_suggest(handle_system, &mpSystemSuggestionsList, encodedText.constData());
    }

    if (handle_profile) {
        mUserDictionarySuggestionsCount = Hunspell_suggest(handle_profile, &mpUserSuggestionsList, utf8Text.constData());
    } else {
        mUserDictionarySuggestionsCount = 0;
    }

    if (mSystemDictionarySuggestionsCount) {
        for (int i = 0; i < mSystemDictionarySuggestionsCount; ++i) {
            auto pA = new QAction(TEncodingHelper::decode(mpSystemSuggestionsList[i], codecName));
#if defined(Q_OS_FREEBSD)
            // Adding the text afterwards as user data as well as in the
            // constructor is to fix a bug(?) in FreeBSD that
            // automagically adds a '&' somewhere in the text to be a
            // shortcut - but doesn't show it and forgets to remove
            // it when asked for the text later:
            pA->setData(TEncodingHelper::decode(mpSystemSuggestionsList[i], codecName));
#endif
            connect(pA, &QAction::triggered, this, &dlgComposer::slot_popupMenu);
            spellings_system << pA;
        }

    } else {
        //: Shown when the spell-checker has no suggestions from the system dictionary for the misspelled word in the composer
        auto pA = new QAction(tr("no suggestions (system)"));
        pA->setEnabled(false);
        spellings_system << pA;
    }

    if (handle_profile) {
        if (mUserDictionarySuggestionsCount) {
            for (int i = 0; i < mUserDictionarySuggestionsCount; ++i) {
                auto pA = new QAction(QString::fromUtf8(mpUserSuggestionsList[i]));
#if defined(Q_OS_FREEBSD)
                // Adding the text afterwards as user data as well as in the
                // constructor is to fix a bug(?) in FreeBSD that
                // automagically adds a '&' somewhere in the text to be a
                // shortcut - but doesn't show it and forgets to remove
                // it when asked for the text later:
                pA->setData(QString::fromUtf8(mpUserSuggestionsList[i]));
#endif
                connect(pA, &QAction::triggered, this, &dlgComposer::slot_popupMenu);
                spellings_profile << pA;
            }

        } else {
            QAction* pA = nullptr;
            auto mainConsole = mpHost->mpConsole;
            if (mainConsole->isUsingSharedDictionary()) {
                //: Shown when the spell-checker has no suggestions from the shared user dictionary for the misspelled word in the composer
                pA = new QAction(tr("no suggestions (shared)"));
            } else {
                //: Shown when the spell-checker has no suggestions from the profile-specific user dictionary for the misspelled word in the composer
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

void dlgComposer::slot_addWord()
{
    if (mSpellCheckedWord.isEmpty()) {
        return;
    }

    mpHost->mpConsole->addWordToSet(mSpellCheckedWord);
    // Redo spell check to update underlining
    recheckWholeLine();
}

void dlgComposer::slot_removeWord()
{
    if (mSpellCheckedWord.isEmpty()) {
        return;
    }

    mpHost->mpConsole->removeWordFromSet(mSpellCheckedWord);
    // Redo spell check to update underlining
    recheckWholeLine();
}

void dlgComposer::slot_popupMenu()
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
    QTextCursor c = edit->cursorForPosition(mPopupPosition);
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
    slot_spellCheck();
}
