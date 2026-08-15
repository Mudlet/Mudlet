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

#include <cmath>

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
// hard-wrapped itself, so that triggers see whole logical lines. The console
// this sets up also serves the case covering the colour links are echoed in
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
    // Short, and not ending at the wrap column, so draining it cannot itself be
    // held back as a wrapped segment:
    const QString mWelcome = qsl("Welcome to the test game.");

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->setWelcomeMessage(mWelcome);
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
        connectAndDrainWelcome();

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(mSegment1), "wrapped segment was not committed as its own line with the option off");
        QVERIFY2(waitForLineInBuffer(mSegment2), "continuation was not committed as its own line with the option off");
        QVERIFY2(!bufferHasLine(mSegment1 + QChar::Space + mSegment2), "lines were joined although the option is off");
    }

    void test_wrappedLinesAreJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        connectAndDrainWelcome();

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(mSegment1 + QChar::Space + mSegment2), "wrapped segment and its continuation were not joined into one logical line");
    }

    void test_promptIsNotJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        connectAndDrainWelcome();

        // The prompt is terminated by IAC GA rather than a newline; the
        // full-width line before it must not swallow it:
        mpServer->sendRaw(mSegment1.toUtf8() + "\r\nHP:100> \xff\xf9");

        QVERIFY2(waitForLineInBuffer(mSegment1), "full-width final line was not committed on its own when followed by a prompt");
        QVERIFY2(waitForLineInBuffer(qsl("HP:100> ")), "prompt was not committed on its own");
    }

    void test_loneFullWidthLineIsFlushed()
    {
        const QString welcome = qsl("Welcome to the test game.");
        mpServer->setWelcomeMessage(welcome);
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        connectAndDrainWelcome();

        // The stub sends its welcome message 100ms after connecting. Letting it
        // land first is what makes this test exercise the flush timer at all:
        // otherwise it arrives as a following line and commits the held segment
        // through the ordinary painted path before the timer ever fires.
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return bufferHasLine(welcome);
                },
                2000));

        // Nothing follows, so the held line has to be committed by the
        // flush timer once the game goes quiet:
        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n");

        // The timer is created when the line is first held back:
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QTimer* flushTimer = nullptr;
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             flushTimer = console->findChild<QTimer*>(qsl("serverWrapFlushTimer"));
                             return flushTimer && flushTimer->isActive();
                         },
                         2000),
                 "the full-width line was not held back for a continuation");

        // Only showNewLines() advances buffer.mCursorY, so it having caught up
        // with the buffer is what distinguishes a painted line from an appended
        // one. It has to be read the moment the flush returns rather than
        // polled for: cTelnet's posting timer calls finalize() as well and
        // repaints within a tick, so any wait long enough to see the line
        // appear is also long enough to lose the evidence.
        int sizeAtFlush = -1;
        int cursorAtFlush = -1;
        connect(flushTimer, &QTimer::timeout, this, [&]() {
            sizeAtFlush = console->buffer.size();
            cursorAtFlush = console->buffer.mCursorY;
        });

        QVERIFY2(waitForLineInBuffer(mSegment1), "held full-width line was not flushed after the game went quiet");
        QVERIFY2(sizeAtFlush > 0, "the flush timer never fired, so the line was committed by some other path");
        QCOMPARE(cursorAtFlush, sizeAtFlush);
    }

    void test_blankLineEndsParagraph()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        connectAndDrainWelcome();

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\n\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(mSegment1), "full-width line before a blank line was not committed on its own");
        QVERIFY2(waitForLineInBuffer(mSegment2), "line after a blank line was not committed on its own");
        QVERIFY2(!bufferHasLine(mSegment1 + QChar::Space + mSegment2), "lines were joined across a blank line");
    }

    void test_artAndIndentedLinesAreNotJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        connectAndDrainWelcome();

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
        connectAndDrainWelcome();

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
        connectAndDrainWelcome();

        // A line space-padded out to the wrap column is a table row or a
        // colour fill, not a wrapped segment - word wrap never produces a
        // run of trailing spaces:
        const QString padded = qsl("2 - visit the game") + QString(60, QChar::Space);
        mpServer->sendRaw(padded.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(padded), "padded line was not committed on its own");
        QVERIFY2(waitForLineInBuffer(mSegment2), "line after a padded line was not committed on its own");
    }

    void test_sentenceGapWrapJoins()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        connectAndDrainWelcome();

        // Games that put two spaces after a full stop keep both when the
        // wrap point lands right after a sentence - neither a held line
        // nor a continuation ending in ".  " is padding:
        const QString sentenceGap = QString(62, QChar('x')) + qsl(" alpha.  ");
        mpServer->sendRaw(sentenceGap.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");
        QVERIFY2(waitForLineInBuffer(sentenceGap + mSegment2), "line ending in a sentence gap was mistaken for padding and not joined");

        mpServer->sendRaw(mSegment1.toUtf8() + "\r\nbeta done.  \r\n");
        QVERIFY2(waitForLineInBuffer(mSegment1 + qsl(" beta done.  ")), "continuation ending in a sentence gap was not joined onto the held line");

        // Three or more trailing spaces are still padding, sentence or not:
        const QString sentencePadded = QString(62, QChar('x')) + qsl(" alpha.") + QString(5, QChar::Space);
        mpServer->sendRaw(sentencePadded.toUtf8() + "\r\n" + mSegment2.toUtf8() + "\r\n");
        QVERIFY2(waitForLineInBuffer(sentencePadded), "sentence-final line padded with several spaces was not committed on its own");
    }

    void test_listEntriesAreNotJoined()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // Only the marker tells these entries from a wrapped paragraph; the
        // last carries the single leading space a game may indent an index by:
        const QString entry1 = heldLine(qsl("[581] Stat Fury - a viking only stat that grants bonuses"));
        const QString entry2 = heldLine(qsl("(3) Viking Default: what you get if you do not customise"));
        const QString entry3 = heldLine(qsl("1. Viking Specializations lists the class specialisations"));
        const QString entry4 = qsl(" [1366] Vikings: a barbaric fighter class.");
        verifyHeldLines({entry1, entry2, entry3});
        mpServer->sendRaw(entry1.toUtf8() + "\r\n" + entry2.toUtf8() + "\r\n" + entry3.toUtf8() + "\r\n" + entry4.toUtf8() + "\r\n");

        QVERIFY2(waitForLineInBuffer(entry1), "bracketed list entry was joined onto the entry below it");
        QVERIFY2(waitForLineInBuffer(entry2), "parenthesised list entry was joined onto the entry below it");
        QVERIFY2(waitForLineInBuffer(entry3), "numbered list entry was joined onto the entry below it");
        QVERIFY2(waitForLineInBuffer(entry4), "indented list entry was joined onto the entry above it");
    }

    void test_wrappedListEntryStillJoins()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // The marker is looked for on the continuation only, so an entry too
        // long for one line still wraps like any other prose:
        const QString entry = heldLine(qsl("[1364] Viking Default: if you chose not to customise your"));
        verifyHeldLines({entry});
        mpServer->sendRaw(entry.toUtf8() + "\r\nviking this is what is included.\r\n");

        QVERIFY2(waitForLineInBuffer(entry + qsl(" viking this is what is included.")), "a wrapped list entry was no longer joined back together");
    }

    void test_proseIsNotMistakenForAList()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        // Every one of these continuations opens with something a list
        // marker could be mistaken for:
        const QString dash = heldLine(qsl("The Grand Bazaar sells everything you could want in this"));
        const QString aside = heldLine(qsl("You gain a large amount of experience for your daring"));
        const QString price = heldLine(qsl("The merchant paid for the whole shipment in advance, all"));
        const QString reference = heldLine(qsl("More detail about the viking class can be found over"));
        verifyHeldLines({dash, aside, price, reference});

        mpServer->sendRaw(dash.toUtf8() + "\r\n- weapons, armour and rope - at a very fair price.\r\n");
        QVERIFY2(waitForLineInBuffer(dash + qsl(" - weapons, armour and rope - at a very fair price.")), "a spaced dash opening a continuation was mistaken for a bullet");

        mpServer->sendRaw(aside.toUtf8() + "\r\n(2500) and the whole town cheers for you.\r\n");
        QVERIFY2(waitForLineInBuffer(aside + qsl(" (2500) and the whole town cheers for you.")), "a parenthesised number too long to be a label was mistaken for one");

        mpServer->sendRaw(price.toUtf8() + "\r\n1364. gold was a fair price for it.\r\n");
        QVERIFY2(waitForLineInBuffer(price + qsl(" 1364. gold was a fair price for it.")), "a number too long to be a list label was mistaken for one");

        mpServer->sendRaw(reference.toUtf8() + "\r\n(see help vikings) for the full list.\r\n");
        QVERIFY2(waitForLineInBuffer(reference + qsl(" (see help vikings) for the full list.")), "a parenthesised phrase carrying no number was mistaken for a list marker");
    }

    void test_linkColourFollowsTheConsoleBackground()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mpConsole->buffer.mWrapAt = 500;
        connectAndDrainWelcome();

        QStringList func(qsl("noop()"));
        QStringList hint(qsl("a link"));

        const QString onDark = qsl("a link against a dark background");
        host->mBgColor = QColor(Qt::black);
        host->mpConsole->echoLink(onDark, func, hint, false);
        host->mpConsole->print("\n");
        QVERIFY2(waitForLineInBuffer(onDark), "the link was not printed against the dark background");
        const TChar dark = firstCharacterOf(onDark);

        const QString onLight = qsl("a link against a light background");
        host->mBgColor = QColor(Qt::white);
        host->mpConsole->echoLink(onLight, func, hint, false);
        host->mpConsole->print("\n");
        QVERIFY2(waitForLineInBuffer(onLight), "the link was not printed against the light background");
        const TChar light = firstCharacterOf(onLight);

        QVERIFY2(dark.foreground() != light.foreground(), "the link colour did not follow the console background at all");
        QVERIFY2(light.foreground() == QColor(Qt::blue), "a light background did not keep the darker blue that reads best on it");
        QVERIFY2(contrastRatio(dark.foreground(), dark.background()) > 4.5, "the link does not contrast enough with a dark console background");
        QVERIFY2(contrastRatio(light.foreground(), light.background()) > 4.5, "the link does not contrast enough with a light console background");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

private:
    // The stub sends its welcome message 100ms after the client connects. Every
    // test here asserts how one line relates to its neighbours, so a line
    // arriving at an uncontrolled moment can commit a held segment through a
    // path the test did not intend - which is how test_loneFullWidthLineIsFlushed
    // came to pass without ever reaching the flush timer it is named after.
    // Draining it leaves every test starting from a known buffer.
    void connectAndDrainWelcome()
    {
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return bufferHasLine(mWelcome);
                         },
                         3000),
                 "the stub's welcome message never arrived, so no later line can be attributed to the test's own input");
    }

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

    // Only a line inside the join band - the wrap column of 80 less
    // csmServerWrapSlack - is ever held back for a continuation. Lines that
    // have to be held are padded to a fixed width inside it and checked,
    // because one that drifted out would never be held, leaving every
    // assertion after it passing without the code under test having run:
    static constexpr qsizetype smHeldLineLength = 70;

    static QString heldLine(const QString& text) { return text + QChar::Space + QString(smHeldLineLength - text.size() - 1, QChar('x')); }

    void verifyHeldLines(const QList<QString>& lines)
    {
        for (const QString& line : lines) {
            QVERIFY2(line.size() == smHeldLineLength, qPrintable(qsl("test line is %1 characters, not the %2 that put it inside the join band").arg(line.size()).arg(smHeldLineLength)));
        }
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

    int lineNumberOf(const QString& text)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i) == text) {
                return i;
            }
        }
        return -1;
    }

    TChar firstCharacterOf(const QString& text)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        const int y = lineNumberOf(text);
        if (y < 0 || console->buffer.buffer.at(y).empty()) {
            // the caller gets a TChar that fails its own assertions, so say
            // outright what actually went wrong
            QTest::qFail(qPrintable(qsl("no buffer line reading \"%1\"").arg(text)), __FILE__, __LINE__);
            return TChar(nullptr);
        }
        return console->buffer.buffer.at(y).front();
    }

    // WCAG relative luminance, so that "readable" is a measurement rather than
    // a preference about which blue looks nicer
    static double relativeLuminance(const QColor& color)
    {
        const auto channel = [](const double value) {
            return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF()) + 0.0722 * channel(color.blueF());
    }

    static double contrastRatio(const QColor& first, const QColor& second)
    {
        const double one = relativeLuminance(first);
        const double other = relativeLuminance(second);
        return (std::max(one, other) + 0.05) / (std::min(one, other) + 0.05);
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
