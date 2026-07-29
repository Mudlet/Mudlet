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
 * Tests for SGR 39 (default foreground) / SGR 49 (default background)
 * handling - https://github.com/Mudlet/Mudlet/issues/9466
 *
 * After "CSI 31 m" (red) followed by "CSI 39 m" (default foreground), a
 * later "CSI 1 m" (bold) must not resurrect the previous color as its
 * bright variant; bold default-color text renders with a bold font weight
 * like other terminal emulators do.
 *
 * Uses loopbackTest() to inject data directly into the telnet processing
 * pipeline. Mudlet is started once in initTestCase and reused across all
 * tests.
 *
 * Run with: ctest -R TelnetSgrDefaultColorTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

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
void initializeQRCResourcesForSgrDefaultColorTest();

class TelnetSgrDefaultColorTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("SGR-Default-Color-Test-Host");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // Injects raw telnet data into the processing pipeline via loopback and
    // waits for the buffer to process it.
    void injectData(const QByteArray& data)
    {
        QByteArray terminated = data + QByteArrayLiteral("\r\n");
        mpHost->mTelnet.loopbackTest(terminated);
        QTest::qWait(50ms);
    }

    // Returns the TChar of the first character of the first occurrence of
    // segment in the buffer, or std::nullopt if not found.
    std::optional<TChar> charOfSegment(const QString& segment)
    {
        TMainConsole* console = mpHost->mpConsole;
        for (int line = 0; line <= console->buffer.getLastLineNumber(); ++line) {
            const int col = console->buffer.line(line).indexOf(segment);
            if (col >= 0) {
                return console->buffer.buffer.at(line).at(col);
            }
        }
        return std::nullopt;
    }

private slots:
    // Start mudlet and create a profile once for all tests.
    void initTestCase()
    {
        initializeQRCResourcesForSgrDefaultColorTest();

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

    // Clear the buffer and reset SGR state before each test for isolation.
    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        mpHost->mpConsole->buffer.clear();
        mpHost->mpConsole->buffer.resetCurrentTextFormat();
    }

    // The reported bug: red, then default foreground, then bold - the bold
    // text must be the default color in a bold font, not bright red.
    void boldAfterDefaultForegroundReset()
    {
        injectData(QByteArrayLiteral("\x1b[31mred \x1b[39mplain \x1b[1mbold"));
        auto boldChar = charOfSegment(qsl("bold"));
        QVERIFY(boldChar.has_value());
        QVERIFY(boldChar->foreground() != mpHost->mLightRed);
        QCOMPARE(boldChar->foreground(), mpHost->mFgColor);
        QVERIFY(boldChar->isBold());
    }

    // The working comparison from the bug report: a full SGR 0 reset instead
    // of SGR 39 behaves the same way.
    void boldAfterFullReset()
    {
        injectData(QByteArrayLiteral("\x1b[31mred \x1b[0mplain \x1b[1mbold"));
        auto boldChar = charOfSegment(qsl("bold"));
        QVERIFY(boldChar.has_value());
        QCOMPARE(boldChar->foreground(), mpHost->mFgColor);
        QVERIFY(boldChar->isBold());
    }

    // SGR 39 must not stop a subsequent color change from working, including
    // its brightening by a following bold.
    void newColorAfterDefaultForegroundReset()
    {
        injectData(QByteArrayLiteral("\x1b[31ma \x1b[39mb \x1b[31magain \x1b[1mbright"));
        auto againChar = charOfSegment(qsl("again"));
        QVERIFY(againChar.has_value());
        QCOMPARE(againChar->foreground(), mpHost->mRed);
        auto brightChar = charOfSegment(qsl("bright"));
        QVERIFY(brightChar.has_value());
        QCOMPARE(brightChar->foreground(), mpHost->mLightRed);
    }

    // Regression guard: bold of an explicit color still brightens it.
    void boldColorStillBrightens()
    {
        injectData(QByteArrayLiteral("\x1b[31;1mbright"));
        auto brightChar = charOfSegment(qsl("bright"));
        QVERIFY(brightChar.has_value());
        QCOMPARE(brightChar->foreground(), mpHost->mLightRed);
    }

    // SGR 39 arriving while bold is already active: the text switches to the
    // default color right away.
    void defaultForegroundResetWhileBold()
    {
        injectData(QByteArrayLiteral("\x1b[1m\x1b[31mbright \x1b[39mafter"));
        auto brightChar = charOfSegment(qsl("bright"));
        QVERIFY(brightChar.has_value());
        QCOMPARE(brightChar->foreground(), mpHost->mLightRed);
        auto afterChar = charOfSegment(qsl("after"));
        QVERIFY(afterChar.has_value());
        QCOMPARE(afterChar->foreground(), mpHost->mFgColor);
        QVERIFY(afterChar->isBold());
    }

    // SGR 39 and SGR 1 within a single sequence behave like the separate
    // sequences.
    void combinedDefaultForegroundResetAndBold()
    {
        injectData(QByteArrayLiteral("\x1b[31mred \x1b[39;1mbold"));
        auto boldChar = charOfSegment(qsl("bold"));
        QVERIFY(boldChar.has_value());
        QCOMPARE(boldChar->foreground(), mpHost->mFgColor);
        QVERIFY(boldChar->isBold());
    }

    // SGR 39 also clears a 256-color foreground set via SGR 38;5.
    void boldAfterDefaultForegroundResetFrom256Color()
    {
        injectData(QByteArrayLiteral("\x1b[38;5;196mext \x1b[39mplain \x1b[1mbold"));
        auto boldChar = charOfSegment(qsl("bold"));
        QVERIFY(boldChar.has_value());
        QCOMPARE(boldChar->foreground(), mpHost->mFgColor);
        QVERIFY(boldChar->isBold());
    }

    // The background analog: SGR 49 restores the default background and a
    // later bold does not disturb it.
    void boldAfterDefaultBackgroundReset()
    {
        injectData(QByteArrayLiteral("\x1b[41mredbg \x1b[49mplainbg \x1b[1mboldbg"));
        auto plainChar = charOfSegment(qsl("plainbg"));
        QVERIFY(plainChar.has_value());
        QCOMPARE(plainChar->background(), mpHost->mBgColor);
        auto boldChar = charOfSegment(qsl("boldbg"));
        QVERIFY(boldChar.has_value());
        QCOMPARE(boldChar->background(), mpHost->mBgColor);
        QCOMPARE(boldChar->foreground(), mpHost->mFgColor);
    }

    // Real bytes captured from eden-test.rpgframework.de:4000's "Check client
    // compatibility" screen, where the bug was reported: a red checkbox dash
    // reset with SGR 39, followed by a bold section header on the next line.
    void realServerCompatibilityExcerpt()
    {
        injectData(QByteArrayLiteral("\x1b[1m      16 colors\x1b[22m: [\x1b[31m-\x1b[39m]                 \r\n"
                                     "\x1b[4m\x1b[1mColor Capabilities                  \x1b[22m\x1b[24m"));
        auto headerChar = charOfSegment(qsl("Color Capabilities"));
        QVERIFY(headerChar.has_value());
        QVERIFY(headerChar->foreground() != mpHost->mLightRed);
        QCOMPARE(headerChar->foreground(), mpHost->mFgColor);
        QVERIFY(headerChar->isBold());
        QVERIFY(headerChar->isUnderlined());
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();
        delete mudlet::self();
    }
};

void initializeQRCResourcesForSgrDefaultColorTest()
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

#include "TelnetSgrDefaultColorTest.moc"
QTEST_MAIN(TelnetSgrDefaultColorTest)
