/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Regression test for #3922: left-clicking into the main console to give it
// focus must not leave a one-character selection behind. Such a stray
// selection hijacks Ctrl+C away from the command line (TCommandLine prioritises
// any console selection over the input box).
class MainConsoleSelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-Selection";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    // Send a screenful of text so a click in the middle of the upper pane lands
    // on a real, filled line rather than empty space.
    QString fillerText() const
    {
        const QString line = QString(100, QLatin1Char('X'));
        QString message;
        for (int i = 0; i < 80; ++i) {
            message.append(line);
            message.append(QStringLiteral("\r\n"));
        }
        return message;
    }

    TTextEdit* upperPane() const
    {
        auto host = mudlet::self()->getActiveHost();
        if (!host || !host->mpConsole) {
            return nullptr;
        }
        return host->mpConsole->mUpperPane;
    }

    void sendMouse(QWidget* w, QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons, const QPointF& localPos)
    {
        const QPointF globalPos = w->mapToGlobal(localPos.toPoint());
        QMouseEvent event(type, localPos, globalPos, button, buttons, Qt::NoModifier);
        QApplication::sendEvent(w, &event);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // A plain click (press + a move that stays in the same character cell +
    // release) must not create a selection.
    void test_clickWithoutDragLeavesNoSelection()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");
        QVERIFY2(pane->width() > 400 && pane->height() > 100, qPrintable(QStringLiteral("Upper pane too small: %1x%2").arg(pane->width()).arg(pane->height())));

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();
        QVERIFY(pane->mSelectedRegion.isEmpty());

        // Click in the middle of the pane (well past the timestamp gutter), then
        // a move event at the very same pixel - i.e. no movement to a different
        // character cell - then release.
        const QPointF clickPos = QRectF(pane->rect()).center();
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, clickPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, clickPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, clickPos);

        QVERIFY2(pane->mSelectedRegion.isEmpty(), "A click with no drag left a stray selection in the console (regression of #3922)");
    }

    // Control case: a genuine drag across cells must still produce a selection,
    // so the fix above does not over-correct.
    void test_dragStillSelects()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();

        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + QPointF(60, 0); // several character cells to the right
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, endPos);

        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "A real drag failed to create a selection");
    }

    // Regression case for the review fix: once a drag has genuinely selected
    // text, dragging back to the original press cell must collapse the extent
    // instead of leaving the previous selection frozen.
    void test_dragBackToOriginCollapsesSelectionExtent()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();

        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + QPointF(60, 0); // several character cells to the right
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);

        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "The initial drag failed to create a selection");
        const QRect expandedSelection = pane->mSelectedRegion.boundingRect();

        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, startPos);

        const QRect collapsedSelection = pane->mSelectedRegion.boundingRect();
        QVERIFY2(collapsedSelection.width() < expandedSelection.width(),
                 "Dragging back to the press cell left the earlier selection extent frozen");
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mpHostname);

        // Tear down Mudlet (and with it the live cTelnet connection) before the
        // stub server it is talking to, so the socket is closed from the client
        // side rather than being yanked out from under an active connection when
        // the server is destroyed - the latter ordering can flake or crash.
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    bool waitForTextInBuffer(const QString& text, int timeoutMs = 5000)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        return QTest::qWaitFor(
                [&]() {
                    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
                        if (console->buffer.line(i) == text) {
                            return true;
                        }
                    }
                    return false;
                },
                timeoutMs);
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        deleteDirectory(path);
    }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "MainConsoleSelectionTest.moc"
MUDLET_GROUPED_TEST_MAIN(MainConsoleSelectionTest)
