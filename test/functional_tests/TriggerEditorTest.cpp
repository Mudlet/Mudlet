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

#include <QtTest/QtTest>

#include <QClipboard>

#include "SingleLineTextEdit.h"
#include "TelnetServerStub.h"
#include "TrailingWhitespaceMarker.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForTriggerEditorTest();

class TriggerEditorTest : public QObject {
  Q_OBJECT

private:
  TelnetServerStub *mpServer = nullptr;
  Host *mpHost = nullptr;
  const QString mHostname = "TriggerEditor-Test";
  const QString mPort = "4004";
  const QString mLocalhost = "localhost";

  void startProfile(const QString &hostname, const QString &address,
                    const QString &port) {
    qDebug() << "[CI-TRACE] TriggerEditorTest::startProfile ENTER" << hostname
             << address << port;
    // Dismiss the first-launch tutorial as soon as the dialog is shown so
    // that new_profile_button is visible/clickable below.
    QObject::connect(
        mudlet::self(), &mudlet::signal_connectionDialogShown, qApp,
        [] {
          qDebug() << "[CI-TRACE] TriggerEditorTest signal_connectionDialogShown handler invoked";
          auto *dialog = mudlet::self()->mpConnectionDialog.data();
          qDebug() << "[CI-TRACE] dialog ptr=" << static_cast<void*>(dialog)
                   << "showingTutorial=" << (dialog ? dialog->showingTutorialInvitation() : false);
          if (dialog && dialog->showingTutorialInvitation()) {
            dialog->dismissTutorialInvitation();
          }
        },
        Qt::SingleShotConnection);

    QTimer::singleShot(0, qApp, [hostname, address, port]() {
      qDebug() << "[CI-TRACE] TriggerEditorTest QTimer fired -> startAutoLogin";
      mudlet::self()->startAutoLogin({});
      QTest::qWait(100);
      auto *dialog = mudlet::self()->mpConnectionDialog.data();
      qDebug() << "[CI-TRACE] before mouseClick: dialog=" << static_cast<void*>(dialog)
               << "new_profile_button visible=" << (dialog ? dialog->new_profile_button->isVisible() : false)
               << "enabled=" << (dialog ? dialog->new_profile_button->isEnabled() : false);
      QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button,
                        Qt::LeftButton);
      QTest::qWait(100);
      qDebug() << "[CI-TRACE] after mouseClick new_profile_button: focusWidget=" << QApplication::focusWidget()
               << "objName=" << (QApplication::focusWidget() ? QApplication::focusWidget()->objectName() : QString());
      QTest::keyClicks(QApplication::focusWidget(), hostname);
      QTest::qWait(100);
      qDebug() << "[CI-TRACE] after keyClicks hostname: focusWidget=" << QApplication::focusWidget()
               << "profile_name_entry text=" << (dialog ? dialog->profile_name_entry->text() : QString());
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      qDebug() << "[CI-TRACE] after Tab: focusWidget=" << QApplication::focusWidget()
               << "objName=" << (QApplication::focusWidget() ? QApplication::focusWidget()->objectName() : QString());
      QTest::keyClicks(QApplication::focusWidget(), address);
      QTest::qWait(100);
      qDebug() << "[CI-TRACE] after keyClicks address: host_name_entry text="
               << (dialog ? dialog->host_name_entry->text() : QString());
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      qDebug() << "[CI-TRACE] after Tab: focusWidget=" << QApplication::focusWidget()
               << "objName=" << (QApplication::focusWidget() ? QApplication::focusWidget()->objectName() : QString());
      QTest::keyClicks(QApplication::focusWidget(), port);
      QTest::qWait(100);
      qDebug() << "[CI-TRACE] after keyClicks port: port_entry text="
               << (dialog ? dialog->port_entry->text() : QString());
      qDebug() << "[CI-TRACE] sending Enter key";
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
      qDebug() << "[CI-TRACE] Enter sent; QTimer lambda finished";
    });

    qDebug() << "[CI-TRACE] TriggerEditorTest waiting for signal_profileLoaded (timeout 5000ms)";
    QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
    if (!spy.wait(5000)) {
      qDebug() << "[CI-TRACE] TriggerEditorTest spy.wait TIMED OUT";
      auto *dialog = mudlet::self()->mpConnectionDialog.data();
      qDebug() << "[CI-TRACE] post-timeout dialog=" << static_cast<void*>(dialog)
               << "visible=" << (dialog ? dialog->isVisible() : false)
               << "profile_name_entry text=" << (dialog ? dialog->profile_name_entry->text() : QString())
               << "host_name_entry text=" << (dialog ? dialog->host_name_entry->text() : QString())
               << "port_entry text=" << (dialog ? dialog->port_entry->text() : QString());
      QFAIL("Profile took too long to load.");
    }
    qDebug() << "[CI-TRACE] TriggerEditorTest signal_profileLoaded received";
    mpHost = mudlet::self()->getActiveHost();
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
    initializeQRCResourcesForTriggerEditorTest();

    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, mPort.toUShort());
    mudlet::start();
    mudlet::self()->setupConfig();
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
    deleteProfileDirectory(mHostname);
    delete mudlet::self();
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
};

void initializeQRCResourcesForTriggerEditorTest() {
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
  qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
  qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  qInitResources_mudlet_fonts_posix();
#endif
#endif
  qInitResources_mudlet();
  qInitResources_qm();
}

#include "TriggerEditorTest.moc"
QTEST_MAIN(TriggerEditorTest)
