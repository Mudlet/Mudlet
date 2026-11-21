/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

/*
 * The keystroke a key answers, set in a field that listens for it. The first
 * case also holds the "Grab New Key" button gone: a form still carrying it
 * would pass every other case while offering two ways to do one thing.
 *
 * Run with: ctest -R EditorKeyCaptureTest -V
 */

#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QToolButton>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "KeyUnit.h"
#include "ProfileSessionTestHelper.h"
#include "TKey.h"
#include "dlgKeysMainArea.h"
#include "dlgSourceEditorArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

class EditorKeyCaptureTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    QTreeWidgetItem* mpKeyItem = nullptr;
    // A grab takes the editor's shortcuts away; this one shows them leave and return
    QAction* mpShortcutAction = nullptr;
    QKeySequence mShortcut;
    const QString mProfileName = qsl("EditorKeyCapture-Test-Profile");

    QLineEdit* field() const { return mpEditor->mpKeysMainArea->lineEdit_key_binding; }
    QLabel* hint() const { return mpEditor->mpLabel_keyHint; }
    QToolButton* clearButton() const { return mpEditor->mpButton_keyClear; }
    TKey* key() const { return mpKeyItem ? mpHost->getKeyUnit()->getKey(mpKeyItem->data(0, Qt::UserRole).toInt()) : nullptr; }

    void armTheGrab()
    {
        QTest::mouseClick(field(), Qt::LeftButton);
        QCoreApplication::processEvents();
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QCoreApplication::processEvents();
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1100, 900);

        // Any action with a shortcut will do: restoreShortcuts() restores every one the grab took
        for (QAction* pAction : mpEditor->toolbarActions()) {
            if (!pAction->shortcut().isEmpty()) {
                mpShortcutAction = pAction;
                mShortcut = pAction->shortcut();
                break;
            }
        }
        QVERIFY2(mpShortcutAction != nullptr, "no action of the editor's carries a shortcut, so there is nothing to watch a grab take away");

        mpEditor->slot_showKeys();
        mpEditor->addKey(false);
        QCoreApplication::processEvents();
        mpKeyItem = mpEditor->mpCurrentKeyItem;
        QVERIFY2(mpKeyItem != nullptr, "addKey() left no current key item");
    }

    void cleanupTestCase()
    {
        mpKeyItem = nullptr;
        mpShortcutAction = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_clickingTheFieldStartsListening()
    {
        QVERIFY2(mpEditor->mpKeysMainArea->findChild<QWidget*>(qsl("pushButton_key_grabKey")) == nullptr, "the keys form still carries the button that grabbed a key");
        QVERIFY2(!mpEditor->mIsGrabKey, "the editor is listening for a keystroke before anything asked it to");

        armTheGrab();

        QVERIFY2(mpEditor->mIsGrabKey, "clicking the key binding field did not start the grab");
        QCOMPARE(field()->property("editorListening").toBool(), true);
        QCOMPARE(field()->placeholderText(), qsl("Press a key combination"));
        QVERIFY2(field()->text().isEmpty(), "the field still shows the old keystroke while it waits for a new one");
        QCOMPARE(hint()->text(), qsl("Escape keeps the current key"));
        QVERIFY2(mpShortcutAction->shortcut().isEmpty(), "the editor kept its own shortcuts while listening for a keystroke to bind");
    }

    // Numpad walking is bound through the keypad modifier, so narrowing the
    // modifiers would lose it
    void test_theKeystrokeIsStoredWithItsModifiers()
    {
        QVERIFY2(mpEditor->mIsGrabKey, "the grab is not armed, so there is nothing to capture");

        QTest::keyClick(mpEditor, Qt::Key_8, Qt::ControlModifier | Qt::KeypadModifier);
        QCoreApplication::processEvents();

        TKey* pKey = key();
        QVERIFY2(pKey != nullptr, "the key is not in the key unit");
        QCOMPARE(pKey->getKeyCode(), Qt::Key_8);
        QVERIFY2(pKey->getKeyModifiers().testFlag(Qt::KeypadModifier), "the keypad modifier was dropped from the captured keystroke");
        QVERIFY2(pKey->getKeyModifiers().testFlag(Qt::ControlModifier), "the control modifier was dropped from the captured keystroke");
        QVERIFY2(!mpEditor->mIsGrabKey, "the grab is still armed after a keystroke was taken");
        QCOMPARE(field()->text(), mpHost->getKeyUnit()->getKeyName(pKey->getKeyCode(), pKey->getKeyModifiers()));
        QCOMPARE(field()->property("editorListening").toBool(), false);
        QCOMPARE(hint()->text(), qsl("Click to change"));
        QVERIFY2(!clearButton()->isHidden(), "a key with a keystroke is not offered the cross that forgets it");
    }

    void test_escapeKeepsTheKeystroke()
    {
        const QString before = field()->text();
        QVERIFY2(!before.isEmpty(), "there is no keystroke to keep");

        armTheGrab();
        QVERIFY2(mpEditor->mIsGrabKey, "clicking the field did not start the grab");
        QTest::keyClick(mpEditor, Qt::Key_Escape);
        QCoreApplication::processEvents();

        QVERIFY2(!mpEditor->mIsGrabKey, "Escape left the editor listening");
        QCOMPARE(field()->text(), before);
        QCOMPARE(field()->property("editorListening").toBool(), false);
        QCOMPARE(mpShortcutAction->shortcut(), mShortcut);
    }

    void test_leavingTheFieldGivesUpOnTheGrab()
    {
        const QString before = field()->text();
        armTheGrab();
        QVERIFY2(mpEditor->mIsGrabKey, "clicking the field did not start the grab");

        mpEditor->mpKeysMainArea->lineEdit_key_name->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();

        QVERIFY2(!mpEditor->mIsGrabKey, "the grab is still armed after the field lost the keyboard");
        QCOMPARE(field()->text(), before);
        QCOMPARE(field()->property("editorListening").toBool(), false);
        QCOMPARE(mpShortcutAction->shortcut(), mShortcut);
    }

    void test_theCrossForgetsTheKeystroke()
    {
        QVERIFY2(!clearButton()->isHidden(), "there is no cross to forget the keystroke with");

        clearButton()->click();
        QCoreApplication::processEvents();

        TKey* pKey = key();
        QVERIFY2(pKey != nullptr, "the key is not in the key unit");
        QCOMPARE(pKey->getKeyCode(), Qt::Key_unknown);
        QVERIFY2(field()->text().isEmpty(), "the field still shows a keystroke the key no longer answers");
        QCOMPARE(field()->placeholderText(), qsl("No key chosen"));
        QCOMPARE(hint()->text(), qsl("Click to set"));
        QVERIFY2(clearButton()->isHidden(), "the cross is still offered for a key with nothing to forget");
    }

    // Nothing on the toolbar takes the keyboard, so the field's FocusOut never
    // fires; and an unnamed key's name is read off the field the grab emptied
    void test_savingWhileListeningEndsTheGrab()
    {
        mpEditor->mpKeysMainArea->lineEdit_key_name->setText(qsl("New key"));
        armTheGrab();
        QVERIFY2(mpEditor->mIsGrabKey, "clicking the field did not start the grab");
        QVERIFY2(field()->text().isEmpty(), "the field still shows a keystroke while it waits for one");

        mpEditor->slot_saveSelectedItem();
        QCoreApplication::processEvents();

        QVERIFY2(!mpEditor->mIsGrabKey, "saving the item left the editor listening for a keystroke");
        QCOMPARE(mpShortcutAction->shortcut(), mShortcut);

        // The grab's application-wide filter takes arrow keys anywhere, so one
        // pressed elsewhere shows whether the filter is still installed
        QLineEdit elsewhere;
        elsewhere.setText(qsl("abc"));
        elsewhere.setCursorPosition(0);
        QTest::keyClick(&elsewhere, Qt::Key_Right);
        QCOMPARE(elsewhere.cursorPosition(), 1);

        TKey* pKey = key();
        QVERIFY2(pKey != nullptr, "the key is not in the key unit");
        QVERIFY2(!pKey->getName().isEmpty(), "the key was saved with no name at all");
        QVERIFY2(!mpKeyItem->text(0).isEmpty(), "the key's row in the tree is blank");
    }

    // The sidebar takes no keyboard either, so no FocusOut ends the grab here
    void test_switchingViewsEndsTheGrab()
    {
        armTheGrab();
        QVERIFY2(mpEditor->mIsGrabKey, "clicking the field did not start the grab");

        mpEditor->slot_showTimers();
        QCoreApplication::processEvents();

        QVERIFY2(!mpEditor->mIsGrabKey, "switching views left the editor listening for a keystroke");
        QCOMPARE(mpShortcutAction->shortcut(), mShortcut);

        mpEditor->slot_showKeys();
        mpEditor->slot_keySelected(mpKeyItem);
        QCoreApplication::processEvents();
    }

    // TKey::match() never matches a folder. The row widget must hide with its
    // three children, or it holds a grid row open as dead height
    void test_aKeyGroupIsOfferedNoneOfTheRow()
    {
        QWidget* pRow = mpEditor->mpKeysMainArea->findChild<QWidget*>(qsl("editorKeyBindingRow"));
        QVERIFY2(pRow != nullptr, "the keys form has no key binding row");
        QVERIFY2(pRow->isVisible(), "the key binding row is not showing for a key that can hold a keystroke");
        // sizeHint(), not height(): the form readings below are hints, and a
        // row stretched to fill the column is not what it costs the hint
        const int rowHeight = pRow->sizeHint().height();
        const int withRow = mpEditor->mpKeysMainArea->sizeHint().height();

        mpEditor->addKey(true);
        QCoreApplication::processEvents();

        QVERIFY2(!field()->isVisible(), "a key group is still offered a keystroke it can never match");
        QVERIFY2(!hint()->isVisible(), "a key group is still told how to set a keystroke");
        QVERIFY2(!clearButton()->isVisible(), "a key group is still offered the cross that forgets a keystroke");
        QVERIFY2(!pRow->isVisible(), "the row those three sit on is still showing for a key group");

        const int withoutRow = mpEditor->mpKeysMainArea->sizeHint().height();
        qInfo().noquote()
                << qsl("  the keys form asks for %1 with the key row and %2 without it, the row asking for %3").arg(QString::number(withRow), QString::number(withoutRow), QString::number(rowHeight));
        QVERIFY2(withRow - withoutRow > rowHeight,
                 qPrintable(qsl("the form gave back only %1 for a key group while the row it stopped showing asks for %2 - the grid row it stood in is still open")
                                    .arg(QString::number(withRow - withoutRow), QString::number(rowHeight))));
    }

    // Grabbed through the field as a player does it: the armed grab has emptied the field, so the old binding is
    // not there to compare the name with
    void test_aGrabRenamesANewKeyNamedAfterItsBinding()
    {
        mpEditor->slot_showKeys();
        mpEditor->addKey(false);
        QCoreApplication::processEvents();
        QTreeWidgetItem* pItem = mpEditor->mpCurrentKeyItem;
        QVERIFY2(pItem != nullptr, "addKey() left no current key item");
        TKey* pKey = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pKey != nullptr, "the new key is not in the key unit");

        armTheGrab();
        QTest::keyClick(mpEditor, Qt::Key_F7);
        QCoreApplication::processEvents();
        QCOMPARE(pKey->getName(), qsl("F7"));

        armTheGrab();
        QTest::keyClick(mpEditor, Qt::Key_F8);
        QCoreApplication::processEvents();
        QCOMPARE(pKey->getName(), qsl("F8"));
        QCOMPARE(pItem->text(0), qsl("F8"));
    }

    void test_aGrabRenamesAKeySavedWithNoBinding()
    {
        mpEditor->slot_showKeys();
        mpEditor->addKey(false);
        QCoreApplication::processEvents();
        QTreeWidgetItem* pItem = mpEditor->mpCurrentKeyItem;
        QVERIFY2(pItem != nullptr, "addKey() left no current key item");
        TKey* pKey = mpHost->getKeyUnit()->getKey(pItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pKey != nullptr, "the new key is not in the key unit");
        mpEditor->mpKeysMainArea->lineEdit_key_name->clear();
        mpEditor->slot_saveSelectedItem();
        QCOMPARE(pKey->getName(), mpEditor->unboundKeyText());

        armTheGrab();
        QTest::keyClick(mpEditor, Qt::Key_F4);
        QCoreApplication::processEvents();
        QCOMPARE(pKey->getName(), qsl("F4"));
    }
};

#include "EditorKeyCaptureTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorKeyCaptureTest)
