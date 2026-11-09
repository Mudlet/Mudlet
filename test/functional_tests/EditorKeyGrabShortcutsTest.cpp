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
 * Ending a key grab, by Escape or by a captured keystroke, must give back every
 * shortcut the grab suspended, not only those a label-keyed table listed: Ctrl+N
 * and the others set on their actions were lost for the session after one grab.
 *
 * Run with: ctest -R EditorKeyGrabShortcutsTest -V
 */

#include <QAction>
#include <QHash>
#include <QKeySequence>
#include <QToolBar>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "KeyUnit.h"
#include "ProfileSessionTestHelper.h"
#include "TKey.h"
#include "dlgKeysMainArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorKeyGrabShortcutsTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    QHash<QAction*, QKeySequence> mShortcutsBeforeTheGrab;
    const QString mProfileName = qsl("EditorKeyGrabShortcuts-Test-Profile");

    // Add Item and its like carry no text at all, only the icon and the tooltip
    QString actionLabel(const QAction* pAction) const { return pAction->text().isEmpty() ? pAction->toolTip() : pAction->text(); }

    void armTheGrab()
    {
        QTest::mouseClick(mpEditor->mpKeysMainArea->lineEdit_key_binding, Qt::LeftButton);
        QCoreApplication::processEvents();
    }

    QString shortcutsNotGivenBack() const
    {
        QStringList lines;
        for (auto it = mShortcutsBeforeTheGrab.cbegin(); it != mShortcutsBeforeTheGrab.cend(); ++it) {
            const QKeySequence now = it.key()->shortcut();
            if (now == it.value()) {
                continue;
            }
            lines << qsl("%1: had %2, now %3").arg(actionLabel(it.key()), it.value().toString(QKeySequence::PortableText), now.isEmpty() ? qsl("nothing") : now.toString(QKeySequence::PortableText));
        }
        return lines.join(qsl("\n"));
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
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1100, 900);

        // The same set the grab suspends
        for (QAction* pAction : mpEditor->toolbarActions()) {
            mShortcutsBeforeTheGrab.insert(pAction, pAction->shortcut());
        }

        QVERIFY2(mShortcutsBeforeTheGrab.contains(mpEditor->mAddItem), "Add Item is not on the editor's toolbar, so the shortcut this test watches is not among the recorded ones");
        QCOMPARE(mpEditor->mAddItem->shortcut(), QKeySequence(QKeySequence::New));
        // A label-keyed lookup never matched in a translated UI, so Save carries its own
        QCOMPARE(mpEditor->mSaveItem->shortcut(), QKeySequence(QKeySequence::Save));

        mpEditor->slot_showKeys();
        mpEditor->addKey(false);
        QTest::qWait(100ms);
        QVERIFY2(mpEditor->mpCurrentKeyItem != nullptr, "addKey() left no current key item");
    }

    void cleanupTestCase()
    {
        mShortcutsBeforeTheGrab.clear();
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_escapeGivesEveryShortcutBack()
    {
        armTheGrab();
        QVERIFY2(mpEditor->mIsGrabKey, "clicking Grab Key did not start the grab");

        for (auto it = mShortcutsBeforeTheGrab.cbegin(); it != mShortcutsBeforeTheGrab.cend(); ++it) {
            if (it.value().isEmpty()) {
                continue;
            }
            QVERIFY2(it.key()->shortcut().isEmpty(), qPrintable(qsl("%1 kept its shortcut while the editor listened for a keystroke to bind").arg(actionLabel(it.key()))));
        }

        QTest::keyClick(mpEditor, Qt::Key_Escape);
        QCoreApplication::processEvents();
        QVERIFY2(!mpEditor->mIsGrabKey, "Escape left the editor listening for a keystroke");

        const QString report = shortcutsNotGivenBack();
        QVERIFY2(report.isEmpty(), qPrintable(report));
    }

    void test_aCapturedKeystrokeGivesEveryShortcutBack()
    {
        armTheGrab();
        QVERIFY2(mpEditor->mIsGrabKey, "clicking Grab Key did not start the grab");

        QTest::keyClick(mpEditor, Qt::Key_F7, Qt::ControlModifier);
        QCoreApplication::processEvents();
        QVERIFY2(!mpEditor->mIsGrabKey, "the grab is still armed after a keystroke was taken");

        // Proves the capturing branch of event(), not the Escape one, ended the grab
        TKey* pKey = mpHost->getKeyUnit()->getKey(mpEditor->mpCurrentKeyItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pKey != nullptr, "the key is not in the key unit");
        QCOMPARE(pKey->getKeyCode(), Qt::Key_F7);

        const QString report = shortcutsNotGivenBack();
        QVERIFY2(report.isEmpty(), qPrintable(report));
    }
};

#include "EditorKeyGrabShortcutsTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorKeyGrabShortcutsTest)
