/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * Tests for the off-by-one write in cTelnet::processSocketData() -
 * https://github.com/Mudlet/Mudlet/issues/1065
 *
 * processSocketData() used to terminate its input with
 * "in_buffer[amount + 1] = '\0'", one byte further along than the data it was
 * given. The socket path survived it because slot_socketReadyToBeRead() over-
 * allocates its stack buffer, but the same function is also reached from Lua's
 * feedTelnet() via cTelnet::loopbackTest(), which hands it a QByteArray sized
 * exactly to its contents - so the stray NUL landed one byte past the end of a
 * heap allocation.
 *
 * The discriminating test is nulTerminatorLandsAtTheDataEnd(): a sentinel is
 * planted at [amount + 1] and must still be there afterwards. It fails on the
 * unfixed code without needing a sanitizer.
 *
 * Run with: ctest -R cTelnetBufferTest -V
 */

#include <QtTest/QtTest>
#include <chrono>
#include <cstring>
#include <memory>

#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForBufferTest();

class cTelnetBufferTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("BufferTest-Host");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // The byte processSocketData() is entitled to overwrite with its NUL, and
    // the one immediately after it that it must leave alone.
    static constexpr char scmTerminatorSlot = '\x7b';
    static constexpr char scmPastTheEnd = '\x7c';

    // True if any line in the main console buffer contains the given substring
    bool bufferContains(const QString& text) const
    {
        TMainConsole* console = mpHost->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForBufferTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        QTimer::singleShot(0ms, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        mpHost->mpConsole->buffer.clear();
    }

    // The regression test for #1065. processSocketData() is handed `payloadSize`
    // bytes inside a buffer that has two spare bytes after them. It may write
    // its NUL over the first spare byte; the second must come back untouched.
    void nulTerminatorLandsAtTheDataEnd()
    {
        constexpr int payloadSize = 8;
        QByteArray backing(payloadSize + 2, '\0');
        std::memset(backing.data(), 'A', payloadSize);
        backing[payloadSize] = scmTerminatorSlot;
        backing[payloadSize + 1] = scmPastTheEnd;

        mpHost->mTelnet.processSocketData(backing.data(), payloadSize, true);

        QCOMPARE(backing.at(payloadSize), '\0');
        QVERIFY2(backing.at(payloadSize + 1) == scmPastTheEnd,
                 "processSocketData() wrote its NUL terminator one byte past the data it was given "
                 "- the off-by-one of issue #1065 is back.");
    }

    // The same off-by-one across the sizes a read can plausibly return, so a
    // future rewrite cannot reintroduce it for only some lengths.
    void nulTerminatorLandsAtTheDataEndAtEverySize()
    {
        for (const int payloadSize : {1, 2, 4, 8, 15, 16, 31, 32, 33, 63, 64, 1024}) {
            QByteArray backing(payloadSize + 2, '\0');
            std::memset(backing.data(), 'A', payloadSize);
            backing[payloadSize] = scmTerminatorSlot;
            backing[payloadSize + 1] = scmPastTheEnd;

            mpHost->mTelnet.processSocketData(backing.data(), payloadSize, true);

            QVERIFY2(backing.at(payloadSize + 1) == scmPastTheEnd, qPrintable(qsl("processSocketData() wrote past the end of a %1 byte payload.").arg(payloadSize)));
        }
    }

    // The heap shape that Lua's feedTelnet() actually produces: an allocation
    // sized exactly to the data plus its terminator. Writing at [size + 1] runs
    // off the end of it, which AddressSanitizer reports as a heap-buffer-overflow.
    void exactlySizedHeapAllocationIsNotOverrun()
    {
        const QByteArray payload = QByteArrayLiteral("heap probe\r\n");
        const auto size = static_cast<int>(payload.size());
        auto buffer = std::make_unique<char[]>(size + 1); // +1 for the terminator, exactly as QByteArray allocates
        std::memcpy(buffer.get(), payload.constData(), size);
        buffer[size] = scmTerminatorSlot;

        mpHost->mTelnet.processSocketData(buffer.get(), size, true);

        QCOMPARE(buffer[size], '\0');
    }

    // The production route from Lua: feedTelnet() -> loopbackTest() ->
    // processSocketData(), with a QByteArray squeezed down to its contents so
    // there is no slack to absorb a stray write.
    void feedTelnetPathDisplaysItsDataIntact()
    {
        QByteArray payload = QByteArrayLiteral("BUFFER_TEST_MARKER\r\n");
        payload.squeeze();

        mpHost->mTelnet.loopbackTest(payload);
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return bufferContains(qsl("BUFFER_TEST_MARKER"));
                         },
                         QDeadlineTimer(5s)),
                 "Text fed through loopbackTest() did not reach the console.");
    }

    // A closed or errored socket reports -1 and an empty read reports 0. Neither
    // may touch the caller's buffer, which for amount == 0 can legitimately have
    // no writable byte at all.
    void emptyAndErroredReadsLeaveTheBufferAlone()
    {
        QByteArray backing(2, '\0');
        backing[0] = scmTerminatorSlot;
        backing[1] = scmPastTheEnd;

        mpHost->mTelnet.processSocketData(backing.data(), 0, true);
        QCOMPARE(backing.at(0), scmTerminatorSlot);
        QCOMPARE(backing.at(1), scmPastTheEnd);

        mpHost->mTelnet.processSocketData(backing.data(), -1, true);
        QCOMPARE(backing.at(0), scmTerminatorSlot);
        QCOMPARE(backing.at(1), scmPastTheEnd);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();
        delete mudlet::self();
    }
};

void initializeQRCResourcesForBufferTest()
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

#include "cTelnetBufferTest.moc"
QTEST_MAIN(cTelnetBufferTest)
