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

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
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

// Tests Host::mUndoServerWrap: rejoining of lines that the game server
// hard-wrapped itself, so that triggers see whole logical lines
class UndoServerWrapTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-UndoServerWrap";
    const QString mpPort = "4001";
    const QString mpLocalhost = "localhost";

    // 70 characters, inside the join band for a wrap column of 80:
    const QString mSegment1 = QString(64, QChar('x')) + qsl(" alpha");
    const QString mSegment2 = qsl("beta tail.");

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, mpPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_wrappedLinesStaySplitByDefault()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(mSegment1), "wrapped segment was not committed as its own line with the option off");
        QVERIFY2(waitForLineInBuffer(mSegment2), "continuation was not committed as its own line with the option off");
        QVERIFY2(!bufferHasLine(mSegment1 + QChar::Space + mSegment2), "lines were joined although the option is off");
    }

    void test_wrappedLinesAreJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(mSegment1 + QChar::Space + mSegment2), "wrapped segment and its continuation were not joined into one logical line");
    }

    void test_promptIsNotJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // The prompt is terminated by IAC GA rather than a newline; the
        // full-width line before it must not swallow it:
        mpServer->sendRaw(mSegment1.toUtf8() + "\r\nHP:100> \xff\xf9");

        QVERIFY2(waitForLineInBuffer(mSegment1), "full-width final line was not committed on its own when followed by a prompt");
        QVERIFY2(waitForLineInBuffer(qsl("HP:100> ")), "prompt was not committed on its own");
    }

    void test_loneFullWidthLineIsFlushed()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // Nothing follows, so the held line has to be committed by the
        // flush timer once the game goes quiet:
        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(mSegment1), "held full-width line was not flushed after the game went quiet");
    }

    void test_blankLineEndsParagraph()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(mSegment1), "full-width line before a blank line was not committed on its own");
        QVERIFY2(waitForLineInBuffer(mSegment2), "line after a blank line was not committed on its own");
        QVERIFY2(!bufferHasLine(mSegment1 + QChar::Space + mSegment2), "lines were joined across a blank line");
    }

    void test_artAndIndentedLinesAreNotJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // A full-width divider, a full-width prose line followed by an
        // indented line (menu/centered art), and only then real wrapped
        // prose - only the last pair may be joined:
        const QString divider = QString(70, QChar('-'));
        const QString indented = qsl("   [1] Enter the game");
        mpServer->sendRaw(divider.toUtf8() + "\r\n" + mSegment1.toUtf8() + "\r\n" + indented.toUtf8() + "\r\n" + mSegment1.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(divider), "full-width divider was not committed on its own");
        QVERIFY2(waitForLineInBuffer(indented), "indented line was joined although it cannot be a wrap continuation");
        QVERIFY2(waitForLineInBuffer(mSegment1), "full-width line before an indented line was not committed on its own");
        QVERIFY2(waitForLineInBuffer(mSegment1 + QChar::Space + mSegment2), "genuine wrapped prose was no longer joined");
    }

    void test_keptBreakSpaceStylesJoin()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // Some games keep the space they broke the line at - either at the
        // end of the wrapped line or at the start of the continuation.
        // Either way the rejoined line carries exactly one space:
        mpServer->sendRaw(mSegment1.toUtf8() + " \r\nbeta trailing.\r\n");
        QVERIFY2(waitForLineInBuffer(mSegment1 + qsl(" beta trailing.")), "trailing-space wrap style was not joined into one line");

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n gamma leading.\r\n");
        QVERIFY2(waitForLineInBuffer(mSegment1 + qsl(" gamma leading.")), "leading-space wrap style was not joined into one line");
    }

    void test_paddedLinesAreNotJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // A line space-padded out to the wrap column is a table row or a
        // colour fill, not a wrapped segment - word wrap never produces a
        // run of trailing spaces:
        const QString padded = qsl("2 - visit the game") + QString(60, QChar::Space);
        mpServer->sendRaw(padded.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(padded), "padded line was not committed on its own");
        QVERIFY2(waitForLineInBuffer(mSegment2), "line after a padded line was not committed on its own");
    }

    void test_wrapDetectionRaisesHint()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        QVERIFY(!host->mServerWrapHintShown);
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // 100 lines all ending hard against a 78 column ceiling:
        QByteArray data;
        const QByteArray line = QString(QString(72, QChar('y')) + qsl(" hello")).toUtf8();
        for (int i = 0; i < 100; ++i) {
            data += line + "\r\n";
        }
        mpServer->sendRaw(data);

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return host->mServerWrapHintShown;
                         },
                         5000),
                 "wrap detection did not fire on 100 lines against a stable ceiling");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

private:
    void enableUndoServerWrap()
    {
        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mUndoServerWrap = true;
        host->mUndoServerWrapWidth = 80;
        // Keep Mudlet's own display wrap out of the way so that logical
        // lines can be compared with buffer lines verbatim:
        host->mpConsole->buffer.mWrapAt = 500;
    }

    // Utility function to manually start a profile like a user would do via the
    // GUI
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

    bool bufferHasLine(const QString& text)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i) == text) {
                return true;
            }
        }
        return false;
    }

    // Polls the console buffer until a line exactly matching the expected text
    // appears, with a timeout
    bool waitForLineInBuffer(const QString& text, int timeoutMs = 5000)
    {
        return QTest::qWaitFor(
                [&]() {
                    return bufferHasLine(text);
                },
                timeoutMs);
    }

    // Utility function
    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
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

#include "UndoServerWrapTest.moc"
QTEST_MAIN(UndoServerWrapTest)
