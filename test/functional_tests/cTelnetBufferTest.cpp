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
 * The discriminating tests are nulTerminatorLandsAtTheDataEnd(), its every-size
 * sibling, and emptyAndErroredReadsLeaveTheBufferAlone(): a sentinel is planted
 * at [amount + 1] and must still be there afterwards. Those fail on the unfixed
 * code without needing a sanitizer, which matters because Windows CI builds
 * without one. Note that the byte at [amount] is written by the later
 * "buffer[datalen] = '\0'" too, so asserting on it only proves the call ran -
 * the sentinel one byte further along is what catches the bug.
 *
 * Run with: ctest -R cTelnetBufferTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QScopeGuard>

#include <chrono>
#include <cstring>
#include <memory>

#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class cTelnetBufferTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
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

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
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
        // A leaked recursion level is permanent for the profile and eventually
        // turns processSocketData() into a silent no-op, which would make the
        // "nothing was written" assertions below pass for the wrong reason.
        QCOMPARE(mpHost->mTelnet.mDecompressionRecursionDepth, 0);
    }

    void cleanup() { QCOMPARE(mpHost->mTelnet.mDecompressionRecursionDepth, 0); }

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

            // The terminator check proves the call actually ran, so the
            // past-the-end check below cannot pass by the function bailing out.
            QVERIFY2(backing.at(payloadSize) == '\0', qPrintable(qsl("processSocketData() did not terminate a %1 byte payload at all.").arg(payloadSize)));
            QVERIFY2(backing.at(payloadSize + 1) == scmPastTheEnd, qPrintable(qsl("processSocketData() wrote past the end of a %1 byte payload.").arg(payloadSize)));
        }
    }

    // With "Force telnet GA signal interpretation off" the GA branch appended a
    // newline without clearing recvdGA, so every remaining byte of the read
    // re-entered it and got its own newline. One buffer is fed here because that
    // guarantees the GA and the text after it share a read, which is the condition
    // - a socket write could in principle be split. mFORCE_GA_OFF is set directly
    // because that is what the preference does: cTelnet copies it from the Host at
    // connect time.
    void forcedGaOffKeepsTheRestOfTheReadOnOneLine()
    {
        const bool savedForceGaOff = mpHost->mTelnet.mFORCE_GA_OFF;
        const bool savedGaDriver = mpHost->mTelnet.mGA_Driver;
        auto restoreFlags = qScopeGuard([this, savedForceGaOff, savedGaDriver]() {
            mpHost->mTelnet.mFORCE_GA_OFF = savedForceGaOff;
            mpHost->mTelnet.mGA_Driver = savedGaDriver;
        });
        mpHost->mTelnet.mFORCE_GA_OFF = true;

        const QString trailing = qsl("AFTER-THE-GA this must stay on one line");
        // A leading newline commits any partial line an earlier test left behind,
        // so the prompt below cannot be glued onto residue.
        QByteArray data("\r\nHP:100 MP:50 > ");
        data += TN_IAC;
        data += TN_GA;
        data += trailing.toUtf8();
        data += "\r\n";

        mpHost->mTelnet.processSocketData(data.data(), data.size(), true);

        // Pre-fix every byte after the GA became its own line, so no single line
        // could hold the whole string: this assertion is the regression guard.
        QVERIFY2(bufferContains(trailing),
                 "the text following a GA was split up - recvdGA was not cleared in the "
                 "mFORCE_GA_OFF branch, so every byte after the GA got its own newline");
    }

    // The production route from Lua: feedTelnet() -> loopbackTest() ->
    // processSocketData(). loopbackTest() takes a non-const QByteArray and calls
    // data(), which detaches, so the allocation shape is Qt's choice rather than
    // ours - this is a "the pipeline still works" check, not a bounds check.
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
    // no writable byte at all. -2 stands in for the qsizetype narrowing in
    // loopbackTest(), which can produce a negative that is not -1.
    void emptyAndErroredReadsLeaveTheBufferAlone()
    {
        for (const int amount : {0, -1, -2}) {
            QByteArray backing(2, '\0');
            backing[0] = scmTerminatorSlot;
            backing[1] = scmPastTheEnd;

            mpHost->mTelnet.processSocketData(backing.data(), amount, true);

            QVERIFY2(backing.at(0) == scmTerminatorSlot, qPrintable(qsl("processSocketData() wrote into the buffer for a read of %1.").arg(amount)));
            QVERIFY2(backing.at(1) == scmPastTheEnd, qPrintable(qsl("processSocketData() wrote past the buffer for a read of %1.").arg(amount)));
        }
    }

    // Every exit from processSocketData() has to hand back the recursion level it
    // took, including the one that refuses the read outright. That refusal is the
    // only exit no other test here reaches, and a level leaked there would be
    // permanent for the profile: once enough have piled up the connection stops
    // accepting data altogether. Seeding the counter reaches the refusal without
    // needing a real decompression bomb to recurse.
    //
    // Whether the refusal happened is read off the caller's buffer rather than
    // the warning text: the refusal returns before the NUL terminator is written,
    // so an untouched sentinel means the read was dropped. That also pins the
    // threshold exactly, and unlike the posted message it does not depend on the
    // interface language.
    void recursionDepthIsHandedBackOnEveryExit()
    {
        constexpr int payloadSize = 12;
        const int seededDepthLimit = cTelnet::scmMaxDecompressionRecursion + 3;
        // A failed QVERIFY2 below aborts the slot mid-sweep, so put the counter
        // back from here rather than at the end - otherwise the seeded value
        // survives into cleanup() and the next slot's init(), and one real
        // failure reports as three with two of them pointing at the wrong place.
        const auto depthRestoreGuard = qScopeGuard([this] {
            mpHost->mTelnet.mDecompressionRecursionDepth = 0;
        });

        for (int seededDepth = 0; seededDepth <= seededDepthLimit; ++seededDepth) {
            // This read takes the level to seededDepth + 1, which is the value
            // the cap is tested against.
            const bool expectRefusal = (seededDepth + 1) > cTelnet::scmMaxDecompressionRecursion;

            // A full payload takes the ordinary fall-through exit, 0 and -1 the
            // nothing-to-read one; past the cap all three take the refusal.
            for (const int amount : {payloadSize, 0, -1}) {
                QByteArray backing(payloadSize + 1, 'A');
                backing[payloadSize] = scmTerminatorSlot;
                mpHost->mTelnet.mDecompressionRecursionDepth = seededDepth;

                mpHost->mTelnet.processSocketData(backing.data(), amount, true);

                QVERIFY2(mpHost->mTelnet.mDecompressionRecursionDepth == seededDepth,
                         qPrintable(qsl("processSocketData() came back from a %1 byte read at depth %2 with the depth at %3 - a recursion level was leaked.")
                                            .arg(amount)
                                            .arg(seededDepth)
                                            .arg(mpHost->mTelnet.mDecompressionRecursionDepth)));

                if (amount != payloadSize) {
                    continue; // a non-positive read never terminates the buffer either way
                }
                const bool wasRefused = backing.at(payloadSize) == scmTerminatorSlot;
                QVERIFY2(wasRefused == expectRefusal,
                         qPrintable(qsl("at depth %1 of %2 the read was %3 - the over-limit cap moved.")
                                            .arg(seededDepth + 1)
                                            .arg(cTelnet::scmMaxDecompressionRecursion)
                                            .arg(wasRefused ? qsl("dropped") : qsl("processed"))));
            }
        }
    }

    // Declared last on purpose: on the unfixed code this trips AddressSanitizer,
    // which aborts the process, so anything after it would never report. The
    // sentinels give it teeth on Windows too, where CI builds without ASan.
    void exactlySizedHeapAllocationIsNotOverrun()
    {
        const QByteArray payload = QByteArrayLiteral("heap probe\r\n");
        const auto size = static_cast<int>(payload.size());
        // Exactly the shape QByteArray allocates: the data plus its terminator.
        auto buffer = std::make_unique<char[]>(size + 1);
        std::memcpy(buffer.get(), payload.constData(), size);
        buffer[size] = scmTerminatorSlot;

        mpHost->mTelnet.processSocketData(buffer.get(), size, true);

        QCOMPARE(buffer[size], '\0');
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }
};

#include "cTelnetBufferTest.moc"
MUDLET_GROUPED_TEST_MAIN(cTelnetBufferTest)
