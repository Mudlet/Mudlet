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
 * Functional tests for the trigger editor and its widgets.
 *
 * Run with: ctest -R TriggerEditorTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QScopeGuard>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "dlgTriggerEditor.h"
#include "SingleLineTextEdit.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TriggerEditorTest : public QObject {
  Q_OBJECT

private:
  QTemporaryDir mConfigDir;
  QByteArray mSavedXdg;
  TelnetServerStub *mpServer = nullptr;
  Host *mpHost = nullptr;
  const QString mHostname = "TriggerEditor-Test";
  QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
  const QString mLocalhost = "localhost";

  void startProfile(const QString &hostname, const QString &address,
                    const QString &port) {
    mpHost = TestProfile::create(hostname, address, port);
    if (!mpHost) {
      QFAIL("No active host available.");
    }

    QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
    if (!spy2.wait(500)) {
      QFAIL("Could not connect with the host.");
    }
  }

  void deleteProfileDirectory(const QString &profileName) {
    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, profileName);
    QDir dir(path);
    if (dir.exists()) {
      dir.removeRecursively();
    }
  }

private slots:
  void initTestCase() {
    if (portableMarkerPresent()) {
      QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, "
            "so the config dir cannot be redirected");
    }

    // A config root of this process's own. Sharing the developer's
    // ~/.config/mudlet means sharing a profile list, so a second copy of this
    // test running at the same time is told the name it types is already in
    // use and never gets an enabled Connect button. Since #9712 the opt-in
    // that makes setupConfig() adopt a directory is
    // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
    QVERIFY(mConfigDir.isValid());
    QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
    mSavedXdg = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
    mPort = QString::number(mpServer->serverPort());
    mudlet::start();
    mudlet::self()->setupConfig();
    QCOMPARE(mudlet::getMudletPath(enums::mainPath),
             qsl("%1/mudlet").arg(mConfigDir.path()));
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);
    deleteProfileDirectory(mHostname);

    startProfile(mHostname, mLocalhost, mPort);
    mpHost = mudlet::self()->getActiveHost();
    QVERIFY2(mpHost, "No active host after profile creation");
  }

  void cleanupTestCase() {
    mpHost = nullptr;
    delete mpServer;
    mpServer = nullptr;
    // Null when initTestCase skipped or failed ahead of mudlet::start(), and
    // getMudletPath() dereferences the instance rather than checking it
    if (mudlet::self()) {
      deleteProfileDirectory(mHostname);
      delete mudlet::self();
    }
    mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME")
                       : qputenv("XDG_CONFIG_HOME", mSavedXdg);
  }

  // Verify that copying text from the pattern editor strips the middle dot
  // whitespace markers (U+00B7) that visualise leading/trailing spaces
  void test_copyFromPatternEditorStripsWhitespaceMarks() {
    const QChar middleDot(0x00B7);

    SingleLineTextEdit edit;

    // Simulate what markQString produces for "  ^pattern$  "
    edit.setPlainText(QString("%1%1^pattern$%1%1").arg(middleDot));

    // Select all and copy via keyboard shortcut
    edit.selectAll();
    QTest::keyClick(&edit, Qt::Key_C, Qt::ControlModifier);

    const QClipboard *clipboard = QGuiApplication::clipboard();
    QVERIFY(clipboard);

    const QString copied = clipboard->text();
    QVERIFY2(!copied.contains(middleDot),
             "Copied text should not contain middle dot formatting marks");
    QCOMPARE(copied, qsl("  ^pattern$  "));
  }

  // Opening the context menu sends the focused editor a FocusOut with
  // Qt::PopupFocusReason, so a focus-out that drops the selection leaves the
  // menu's own Copy entry with nothing to copy (#10330)
  void test_copyFromPatternEditorContextMenu() {
    SingleLineTextEdit edit;
    edit.setPlainText(qsl("^pattern$"));
    edit.show();
    edit.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&edit));
    edit.setFocus();
    QTRY_VERIFY(edit.hasFocus());
    edit.selectAll();

    QClipboard *clipboard = QGuiApplication::clipboard();
    QVERIFY(clipboard);
    clipboard->setText(qsl("previous clipboard contents"));

    // Qt only sends that FocusOut for the first popup, and a menu left open
    // past a failed assertion would outlive the test
    QVERIFY2(!QApplication::activePopupWidget(), "a popup was already open");
    const auto closePopup = qScopeGuard([] {
      if (auto *popup = QApplication::activePopupWidget()) {
        popup->close();
      }
    });

    const QPoint pos(5, 5);
    QContextMenuEvent contextMenuEvent(QContextMenuEvent::Mouse, pos,
                                       edit.viewport()->mapToGlobal(pos));
    QApplication::sendEvent(edit.viewport(), &contextMenuEvent);

    auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
    QVERIFY2(menu, "right-clicking the pattern editor did not open its context menu");
    auto *copyAction = menu->findChild<QAction *>(qsl("edit-copy"));
    QVERIFY2(copyAction, "the context menu has no Copy entry named edit-copy");
    copyAction->trigger();

    QCOMPARE(clipboard->text(), qsl("^pattern$"));
  }

  // The deselect on focus-out exists so a pattern line does not keep showing a
  // stale selection once another line is being edited, so it has to survive
  // only the reasons that give focus straight back
  void test_patternEditorDeselectsOnlyWhenFocusMovesOn() {
    SingleLineTextEdit edit;
    edit.setPlainText(qsl("^pattern$"));
    edit.selectAll();

    QFocusEvent popupFocusOut(QEvent::FocusOut, Qt::PopupFocusReason);
    QApplication::sendEvent(&edit, &popupFocusOut);
    QVERIFY2(edit.textCursor().hasSelection(), "a popup taking focus dropped the selection");

    QFocusEvent windowFocusOut(QEvent::FocusOut, Qt::ActiveWindowFocusReason);
    QApplication::sendEvent(&edit, &windowFocusOut);
    QVERIFY2(edit.textCursor().hasSelection(), "switching windows dropped the selection");

    QFocusEvent tabFocusOut(QEvent::FocusOut, Qt::TabFocusReason);
    QApplication::sendEvent(&edit, &tabFocusOut);
    QVERIFY2(!edit.textCursor().hasSelection(), "focus moving to another widget kept the selection");
  }
  // enableTrigger()/disableTrigger() from Lua used to leave the editor's tree
  // icon stale until something else rebuilt the tree. The repaint is deferred
  // to the next event-loop turn so a script toggling many triggers per line
  // pays one tree walk, not one per call - hence the QTRY_ waits.
  void test_luaToggleRepaintsTheTreeItem() {
    mudlet::self()->slot_showScriptDialog();
    QTest::qWait(100ms);
    dlgTriggerEditor *pEditor = mpHost->mpEditorDialog;
    QVERIFY2(pEditor, "the editor dialog was not created");
    TLuaInterpreter *pLua = mpHost->getLuaInterpreter();
    QVERIFY(pLua->compileAndExecuteScript(
        qsl("permGroup(\"qaToggleFolder\", \"trigger\")\n"
            "permRegexTrigger(\"qaToggleLeaf\", \"qaToggleFolder\", "
            "{\"^qa toggle$\"}, \"\")")));
    pEditor->doCleanReset();
    QTreeWidgetItem *pLeaf = nullptr;
    // The tree is a private Ui member; its objectName is what the .ui gives it
    auto *pTree = pEditor->findChild<QTreeWidget *>(qsl("treeWidget_triggers"));
    QVERIFY(pTree);
    QVERIFY2(QTest::qWaitFor([&]() {
      const auto found = pTree->findItems(
          qsl("qaToggleLeaf"),
          Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive,
          0);
      pLeaf = found.isEmpty() ? nullptr : found.first();
      return pLeaf != nullptr;
    }), "the editor never rebuilt its tree around the planted trigger");
    QTreeWidgetItem *pFolder = pLeaf->parent();
    QVERIFY(pFolder);
    // The accessible description is written alongside the icon from the same
    // computed state, so it stands in for the icon here
    auto description = [](QTreeWidgetItem *pItem) {
      return pItem->data(0, Qt::AccessibleDescriptionRole).toString();
    };
    const QString active = dlgTriggerEditor::tr("activated");
    const QString inactive = dlgTriggerEditor::tr("deactivated");
    const QString activeFolder = dlgTriggerEditor::tr("activated folder");
    const QString inactiveFolder = dlgTriggerEditor::tr("deactivated folder");
    const QString inactiveParent = dlgTriggerEditor::tr("%1 in a deactivated group").arg(active);
    QCOMPARE(description(pLeaf), active);

    QVERIFY(pLua->compileAndExecuteScript(qsl("disableTrigger(\"qaToggleLeaf\")")));
    QTRY_COMPARE(description(pLeaf), inactive);

    QVERIFY(pLua->compileAndExecuteScript(qsl("enableTrigger(\"qaToggleLeaf\")")));
    QTRY_COMPARE(description(pLeaf), active);

    // A folder's state greys out everything under it
    QVERIFY(pLua->compileAndExecuteScript(qsl("disableTrigger(\"qaToggleFolder\")")));
    QTRY_COMPARE(description(pFolder), inactiveFolder);
    QCOMPARE(description(pLeaf), inactiveParent);

    // Toggled off and back on within one turn: the tree ends where it started
    QVERIFY(pLua->compileAndExecuteScript(
        qsl("enableTrigger(\"qaToggleFolder\")\n"
            "disableTrigger(\"qaToggleLeaf\")\n"
            "enableTrigger(\"qaToggleLeaf\")")));
    QTRY_COMPARE(description(pFolder), activeFolder);
    QCOMPARE(description(pLeaf), active);
  }
};

#include "TriggerEditorTest.moc"
MUDLET_GROUPED_TEST_MAIN(TriggerEditorTest)
