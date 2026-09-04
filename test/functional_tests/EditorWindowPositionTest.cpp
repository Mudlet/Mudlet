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
 * Regression tests for where the script editor window opens: wherever the user
 * last left it, whichever entry point re-opens it, and after the window object
 * has been destroyed and built again, which is the path a restart takes. The
 * only time it is allowed to move on its own is when the position it was left
 * at would put it out of the user's reach.
 *
 * Run with: ctest -R EditorWindowPositionTest -V
 */

#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorWindowPositionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorWindowPosition-Test-Profile");
    const QString mSecondProfileName = qsl("EditorWindowPosition-Test-Profile-2");
    const QString mDetachedProfileName = qsl("EditorWindowPosition-Test-Profile-3");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists() && !dir.removeRecursively()) {
            qWarning() << "deleteProfileDirectory: could not remove" << path << "- later failures may stem from this stale state";
        }
    }

    // QFAIL would only return from here, so record the failure and let the
    // caller see it through QTest::currentTestFailed()
    Host* startProfile(const QString& profileName)
    {
        Host* pHost = TestProfile::create(profileName, mLocalhost, mPort);
        if (!pHost) {
            QTest::qFail(qPrintable(qsl("No host available for '%1'.").arg(profileName)), __FILE__, __LINE__);
            return nullptr;
        }

        QSignalSpy spy(&(pHost->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2000)) {
            QTest::qFail(qPrintable(qsl("Could not connect with the host for '%1'.").arg(profileName)), __FILE__, __LINE__);
            return nullptr;
        }
        return pHost;
    }

    // A point on the primary screen that is nowhere near where centring would
    // land the window, so a re-centring bug cannot pass by coincidence
    static QPoint offCentrePoint() { return QApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(17, 23); }

    static bool touchesAnyScreen(const QWidget* widget)
    {
        const QRect frame = widget->frameGeometry();
        const QList<QScreen*> screens = QApplication::screens();
        return std::any_of(screens.cbegin(), screens.cend(), [&frame](const QScreen* screen) {
            return screen->availableGeometry().intersects(frame);
        });
    }

    // The property a rescued window has to end up with, stated in terms of what
    // the user gets rather than of the test the rescue itself applies
    static bool fullyOnAScreen(const QWidget* widget)
    {
        const QRect frame = widget->frameGeometry();
        const QList<QScreen*> screens = QApplication::screens();
        return std::any_of(screens.cbegin(), screens.cend(), [&frame](const QScreen* screen) {
            return screen->availableGeometry().contains(frame);
        });
    }

    // Stand in for the user dragging the window somewhere and closing it.
    // close() is what persists the position, through closeEvent() ->
    // writeSettings(); each wait is here because a window manager applies
    // move() asynchronously and pos() otherwise reads back stale
    static void placeEditorAtAndClose(dlgTriggerEditor* pEditor, const QPoint& position)
    {
        pEditor->show();
        QTest::qWait(50ms);
        pEditor->move(position);
        QTest::qWait(50ms);
        pEditor->close();
        QTest::qWait(50ms);
    }

    dlgTriggerEditor* reopenEditor()
    {
        mudlet::self()->slot_showTriggerDialog();
        // A fixed wait rather than QTRY_: what this guards against is a
        // reposition that happens after the window is shown, and a QTRY_ would
        // stop looking the moment the position was still right
        QTest::qWait(50ms);
        return mpHost->mpEditorDialog;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
        deleteProfileDirectory(mSecondProfileName);
        deleteProfileDirectory(mDetachedProfileName);
        mpHost = startProfile(mProfileName);
        if (QTest::currentTestFailed()) {
            return;
        }

        mpEditor = mpHost->mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            deleteProfileDirectory(mSecondProfileName);
            deleteProfileDirectory(mDetachedProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // All eight reach the same singleton window, each by its own route, so a
    // stray reposition in any one of them moves a window the user placed
    void testEveryEntryPointReopensWhereItWasLeft_data()
    {
        QTest::addColumn<QByteArray>("slotName");

        QTest::newRow("triggers") << QByteArray("slot_showTriggerDialog");
        QTest::newRow("aliases") << QByteArray("slot_showAliasDialog");
        QTest::newRow("timers") << QByteArray("slot_showTimerDialog");
        QTest::newRow("buttons") << QByteArray("slot_showActionDialog");
        QTest::newRow("scripts") << QByteArray("slot_showScriptDialog");
        QTest::newRow("keys") << QByteArray("slot_showKeyDialog");
        QTest::newRow("variables") << QByteArray("slot_showVariableDialog");
        // the Toolbox > Script editor entry and its shortcut, Alt+E or Ctrl+E
        QTest::newRow("editor") << QByteArray("slot_showEditorDialog");
    }

    void testEveryEntryPointReopensWhereItWasLeft()
    {
        QFETCH(QByteArray, slotName);

        const QPoint left = offCentrePoint();
        placeEditorAtAndClose(mpEditor, left);

        QVERIFY(QMetaObject::invokeMethod(mudlet::self(), slotName.constData(), Qt::DirectConnection));
        // A fixed wait rather than QTRY_: what this guards against is a
        // reposition that happens after the window is shown, and a QTRY_ would
        // stop looking the moment the position was still right
        QTest::qWait(50ms);

        QCOMPARE(mpHost->mpEditorDialog.data(), mpEditor);
        QCOMPARE(mpEditor->pos(), left);
    }

    // A window left on a monitor that has since been unplugged would otherwise
    // open out of reach, so that one case is allowed to move it
    void testAWindowLeftOffEveryScreenIsRescued()
    {
        placeEditorAtAndClose(mpEditor, QPoint(30000, 30000));
        QVERIFY2(!touchesAnyScreen(mpEditor), "setup: the editor has to really be off every screen before it is reopened");

        QCOMPARE(reopenEditor(), mpEditor);
        QVERIFY2(fullyOnAScreen(mpEditor), "An editor left where no screen covers it should be brought back into view");
    }

    // Dragged off the top edge of a monitor mounted above the main one: still
    // overlapping a screen after that monitor goes away, but with no title bar
    // left to grab, which is just as stuck
    void testAWindowWithItsTitleBarOffScreenIsRescued()
    {
        const QRect screen = QApplication::primaryScreen()->availableGeometry();
        placeEditorAtAndClose(mpEditor, QPoint(screen.left() + 40, screen.top() - mpEditor->frameGeometry().height() + 20));
        QVERIFY2(touchesAnyScreen(mpEditor), "setup: this case is about a window that does still overlap a screen");
        QVERIFY2(mpEditor->frameGeometry().top() < screen.top(), "setup: the title bar has to be above the top of the screen");

        QCOMPARE(reopenEditor(), mpEditor);
        QVERIFY2(fullyOnAScreen(mpEditor), "An editor whose title bar cannot be reached should be brought back into view");
    }

    // Pushed off the side until only a few pixels of title bar are left: still
    // overlapping a screen, but with nothing wide enough to aim a pointer at
    void testAWindowLeftAsASliverAtTheScreenEdgeIsRescued()
    {
        const QRect screen = QApplication::primaryScreen()->availableGeometry();
        placeEditorAtAndClose(mpEditor, QPoint(screen.right() - 4, screen.top() + 40));
        QVERIFY2(touchesAnyScreen(mpEditor), "setup: this case is about a window that does still overlap a screen");

        QCOMPARE(reopenEditor(), mpEditor);
        QVERIFY2(fullyOnAScreen(mpEditor), "An editor left as a sliver at the screen edge should be brought back into view");
    }

    // The other side of that: hanging off an edge is a position people choose on
    // purpose, and as long as there is title bar to grab it is theirs to keep
    void testAWindowDeliberatelyHungOffAnEdgeIsLeftAlone()
    {
        const QRect screen = QApplication::primaryScreen()->availableGeometry();
        const QPoint hangingOff(screen.right() - 200, screen.top() + 40);
        placeEditorAtAndClose(mpEditor, hangingOff);
        QVERIFY2(!fullyOnAScreen(mpEditor), "setup: most of the window has to be off the screen for this to discriminate");

        QCOMPARE(reopenEditor(), mpEditor);
        QCOMPARE(mpEditor->pos(), hangingOff);
    }

    // First run, with nothing remembered yet: the editor is placed in the
    // middle of the screen rather than left wherever the window system drops it
    void testTheFirstEverEditorIsCentred()
    {
        mudlet::getQSettings()->remove("script_editor_pos");
        delete mpHost->mpEditorDialog.data();

        mpEditor = reopenEditor();
        QVERIFY2(mpEditor != nullptr, "The editor should have been built again");

        const QRect screen = QApplication::primaryScreen()->availableGeometry();
        const QSize editorSize = mpEditor->size();
        QCOMPARE(mpEditor->pos(), QPoint(screen.center().x() - editorSize.width() / 2, screen.center().y() - editorSize.height() / 2));
    }

    // The position is written on close and has to be read back when the editor
    // is built again, which is the path a restart takes
    void testPositionAndSizeSurviveAnEditorRebuild()
    {
        const QPoint left = offCentrePoint() + QPoint(29, 31);
        placeEditorAtAndClose(mpEditor, left);
        const QSize leftSize = mpEditor->size();
        QCOMPARE(mudlet::getQSettings()->value("script_editor_pos").toPoint(), left);

        delete mpHost->mpEditorDialog.data();
        QVERIFY(mpHost->mpEditorDialog.isNull());

        mpEditor = reopenEditor();
        QVERIFY2(mpEditor != nullptr, "The editor should have been built again");
        QCOMPARE(mpEditor->pos(), left);
        QCOMPARE(mpEditor->size(), leftSize);
    }

    // Every profile builds its own editor when it loads but they all share the
    // one remembered position, so an editor the user never opened must not
    // write over the position of one they did. Also the only test that closes
    // an editor the way quitting Mudlet does, through Host::requestClose()
    void testAnUnopenedEditorDoesNotOverwriteThePosition()
    {
        Host* pSecondHost = startProfile(mSecondProfileName);
        if (QTest::currentTestFailed()) {
            return;
        }
        dlgTriggerEditor* pSecondEditor = pSecondHost->mpEditorDialog;
        QVERIFY2(pSecondEditor != nullptr, "The second profile should have built an editor of its own");
        QVERIFY2(pSecondEditor != mpEditor, "The two profiles should not be sharing one editor");

        const QPoint left = offCentrePoint() + QPoint(43, 47);
        QVERIFY2(pSecondEditor->pos() != left, "setup: the unopened editor has to start somewhere else for this to discriminate");
        placeEditorAtAndClose(mpEditor, left);

        pSecondHost->forceClose();
        QVERIFY2(pSecondHost->requestClose(), "Closing the second profile was refused");
        HostManager::self()->deleteHost(mSecondProfileName);

        QCOMPARE(mudlet::getQSettings()->value("script_editor_pos").toPoint(), left);
    }

    // A detached profile window reaches the editor by a route of its own that
    // never goes through mudlet's slots. Runs last: it leaves a second profile
    // open, and reopenEditor() works on whichever one is active
    void testADetachedWindowReopensTheEditorWhereItWasLeft()
    {
        Host* pDetachedHost = startProfile(mDetachedProfileName);
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier test");

        TTabBar* pTabBar = mudlet::self()->mpTabBar;
        QStringList tabNames;
        for (int i = 0, total = pTabBar->count(); i < total; ++i) {
            tabNames << pTabBar->tabData(i).toString();
        }
        // detaching tab 0 is refused, so this profile has to be somewhere after it
        const int tabIndex = tabNames.indexOf(mDetachedProfileName);
        QVERIFY2(tabIndex >= 1, qPrintable(qsl("'%1' is not on a tab that can be detached, tabs are: %2").arg(mDetachedProfileName, tabNames.join(qsl(", ")))));

        mudlet::self()->slot_tabDetachRequested(tabIndex, QPoint(200, 200));
        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(mDetachedProfileName);
        QVERIFY2(pDetachedWindow != nullptr, qPrintable(qsl("detaching tab %1 produced no window for '%2'").arg(QString::number(tabIndex), mDetachedProfileName)));

        dlgTriggerEditor* pDetachedEditor = pDetachedHost->mpEditorDialog;
        QVERIFY2(pDetachedEditor != nullptr, "The detached profile should have built an editor of its own");
        const QPoint left = offCentrePoint() + QPoint(51, 53);
        placeEditorAtAndClose(pDetachedEditor, left);

        QVERIFY(QMetaObject::invokeMethod(pDetachedWindow, "slot_showTriggerDialog", Qt::DirectConnection));
        QTest::qWait(50ms);

        QCOMPARE(pDetachedHost->mpEditorDialog.data(), pDetachedEditor);
        QCOMPARE(pDetachedEditor->pos(), left);

        mudlet::self()->slot_tabReattachRequested(mDetachedProfileName);
    }
};

#include "EditorWindowPositionTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorWindowPositionTest)
