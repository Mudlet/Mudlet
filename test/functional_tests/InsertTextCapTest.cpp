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

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

// Regression test: a single insertText() into the middle of an existing line
// (the TBuffer::insertInLine() path) must apply the same per-echo character
// cap that the echo/append path enforces, so an oversized insert cannot grow a
// line without bound.
class InsertTextCapTest : public QObject
{
    Q_OBJECT

    // Reference the production constant directly to avoid drift.
    static constexpr int kMaxCharactersPerEcho = TBuffer::MAX_CHARACTERS_PER_ECHO;

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-InsertCap";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // Inserting an over-long string into the middle of a line must cap the
    // inserted run at kMaxCharactersPerEcho, matching the echo/append path.
    void test_oversizedInsertIsCapped()
    {
        mpServer->setWelcomeMessage(QStringLiteral("HELLO\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QStringLiteral("HELLO")), "Welcome text never reached the buffer");

        auto console = mudlet::self()->getActiveHost()->mpConsole;

        // Position the user cursor in the middle of the "HELLO" line so that
        // insertText() routes through insertInLine() rather than the (already
        // capped) append path used when the cursor sits at the buffer end.
        QVERIFY2(console->moveCursor(2, 0), "Could not position the user cursor mid-line");

        const int originalLength = console->buffer.line(0).size();
        QVERIFY2(originalLength > 2, "Unexpected welcome line contents");

        const int overshoot = 500;
        const QString oversized(kMaxCharactersPerEcho + overshoot, QLatin1Char('Z'));
        console->insertText(oversized);

        const int newLength = console->buffer.line(0).size();
        const int insertedLength = newLength - originalLength;

        // Without the cap the full oversized string is inserted, so the inserted
        // run would be kMaxCharactersPerEcho + overshoot. With the cap it is
        // exactly kMaxCharactersPerEcho.
        QCOMPARE(insertedLength, kMaxCharactersPerEcho);

        // The character (lineBuffer) and styling (TChar deque) containers are
        // filled by two separate inserts that must stay the same length, or the
        // renderer reads past the end of one of them.
        QCOMPARE(static_cast<int>(console->buffer.buffer.at(0).size()), newLength);
    }

    // A normally-sized insert must be inserted in full (guards against the cap
    // being applied too aggressively).
    void test_normalInsertIsUntouched()
    {
        mpServer->setWelcomeMessage(QStringLiteral("HELLO\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QStringLiteral("HELLO")), "Welcome text never reached the buffer");

        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QVERIFY2(console->moveCursor(2, 0), "Could not position the user cursor mid-line");

        const QString original = console->buffer.line(0);
        const QString payload = QStringLiteral("insertedText");
        console->insertText(payload);

        // The run must be spliced in at the cursor (x = 2) without disturbing the
        // surrounding characters - the batched insert must match the old
        // per-character insertion exactly, not just in length.
        const QString expected = original.left(2) + payload + original.mid(2);
        QCOMPARE(console->buffer.line(0), expected);
        QCOMPARE(static_cast<int>(console->buffer.buffer.at(0).size()), expected.size());
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mpHostname);

        // Tear down Mudlet (and with it the live cTelnet connection) before the
        // stub server it is talking to, so the socket is closed from the client
        // side rather than being yanked out from under an active connection.
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
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

void initializeQRCResources()
{
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

#include "InsertTextCapTest.moc"
QTEST_MAIN(InsertTextCapTest)
