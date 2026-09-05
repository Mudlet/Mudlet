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
 * The trigger editor's advanced options disclosure control.
 *
 * Mudlet 5.0 shipped this as a bare ">" glyph with no label and no frame, and
 * users read it as decoration rather than a control. These tests pin the
 * properties that make it read as a button: a text label, and a frame that is
 * there without having to hover first.
 *
 * Run with: ctest -R TriggerEditorDisclosureTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include <QLineEdit>
#include <QScopeGuard>
#include <QToolButton>

#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TriggerEditorDisclosureTest : public QObject {
  Q_OBJECT

private:
  QTemporaryDir mConfigDir;
  QByteArray mSavedXdg;
  TelnetServerStub *mpServer = nullptr;
  dlgTriggerEditor *mpEditor = nullptr;
  Host *mpHost = nullptr;
  const QString mHostname = qsl("TriggerEditorDisclosure-Test");
  QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
  const QString mLocalhost = qsl("localhost");

  void deleteProfileDirectory(const QString &profileName) {
    QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
    if (dir.exists()) {
      dir.removeRecursively();
    }
  }

  // setTabOrder threads focus through widgets that never take tab focus
  // themselves (containers, scroll-area viewports), so "the next widget in
  // the chain" is not the next tab stop
  QWidget *nextTabStop(QWidget *from) const {
    for (QWidget *w = from->nextInFocusChain(); w && w != from;
         w = w->nextInFocusChain()) {
      if ((w->focusPolicy() & Qt::TabFocus) && w->isVisibleTo(mpEditor)) {
        return w;
      }
    }
    return nullptr;
  }

  QToolButton *toggle() const {
    return mpEditor->mpTriggersMainArea->toolButton_toggleExtraControls;
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
    mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids
                                    // collisions across concurrent test runs
    QVERIFY2(mpServer->isListening(),
             qPrintable(qsl("TelnetServerStub failed to start: %1")
                            .arg(mpServer->errorString())));
    mPort = QString::number(mpServer->serverPort());
    mudlet::start();
    mudlet::self()->setupConfig();
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);
    deleteProfileDirectory(mHostname);

    mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
    QVERIFY2(mpHost, "No active host after profile creation");
    QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
    QVERIFY2(connected.wait(1000), "Could not connect with the host.");

    mudlet::self()->slot_showScriptDialog();
    mpEditor = mpHost->mpEditorDialog;
    QVERIFY2(mpEditor, "Editor dialog should be created");
    // wide enough that the main area lays out at its natural size rather than
    // squeezed, which is what the geometry assertions below measure
    mpEditor->resize(1200, 800);
    QVERIFY(QTest::qWaitForWindowExposed(mpEditor));

    mpEditor->slot_showTriggers();
    mpEditor->addTrigger(false);
    QCoreApplication::processEvents();
    QVERIFY2(mpEditor->mpTriggersMainArea->isVisible(),
             "the triggers main area should be showing after adding a trigger");
  }

  void cleanupTestCase() {
    mpEditor = nullptr;
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

  void test_toggleReadsAsAButton() {
    QVERIFY2(!toggle()->text().isEmpty(),
             "the disclosure control needs a text label - an unlabelled arrow "
             "reads as an ornament, which is the bug this replaced");
    QCOMPARE(toggle()->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
    QVERIFY2(!toggle()->icon().isNull(),
             "the label should be paired with a disclosure arrow");
    QVERIFY2(toggle()->isCheckable(),
             "the arrow direction tracks the checked state");
    QVERIFY2(!toggle()->autoRaise(),
             "the frame has to be there at rest - an affordance that only "
             "appears on hover is one nobody discovers");
  }

  // It shares the Command row rather than taking a row of its own, and the ID
  // number keeps the far right of that row
  void test_toggleSitsBetweenTheCommandFieldAndTheId() {
    auto *area = mpEditor->mpTriggersMainArea;
    auto *command = area->lineEdit_trigger_command;

    // the ID is opt-in, and it is only in the layout while it is shown
    const bool idWasShown = mpHost->showIdsInEditor();
    auto restoreId = qScopeGuard(
        [this, idWasShown]() { mpHost->setShowIdsInEditor(idWasShown); });
    mpHost->setShowIdsInEditor(true);
    QCoreApplication::processEvents();

    const QRect toggleRect(toggle()->mapTo(area, QPoint(0, 0)),
                           toggle()->size());
    const QRect commandRect(command->mapTo(area, QPoint(0, 0)),
                            command->size());
    const QRect idRect(area->frameId->mapTo(area, QPoint(0, 0)),
                       area->frameId->size());

    const QRect commandBand(0, commandRect.top(), area->width(),
                            commandRect.height());
    QVERIFY2(toggleRect.intersects(commandBand),
             qPrintable(qsl("the toggle should share the command row, but it "
                            "spans y %1..%2 and the command field spans %3..%4")
                            .arg(toggleRect.top())
                            .arg(toggleRect.bottom())
                            .arg(commandRect.top())
                            .arg(commandRect.bottom())));
    QVERIFY2(toggleRect.left() >= commandRect.right(),
             "the toggle follows the command field, so the command label and "
             "field stay adjacent");
    QVERIFY2(toggleRect.right() <= idRect.left(),
             "the ID number keeps the far right of the row");
  }

  void test_tabFromCommandFieldReachesTheToggle() {
    auto *command = mpEditor->mpTriggersMainArea->lineEdit_trigger_command;
    QCOMPARE(nextTabStop(command), static_cast<QWidget *>(toggle()));
  }
};

#include "TriggerEditorDisclosureTest.moc"
MUDLET_GROUPED_TEST_MAIN(TriggerEditorDisclosureTest)
