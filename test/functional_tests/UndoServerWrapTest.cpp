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

    void test_sentenceGapWrapJoins()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        enableUndoServerWrap();
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

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

    void test_hintLinkReadsAndReversesTheChange()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        // so that the messages under test can be compared with buffer lines
        // verbatim rather than as Mudlet chose to wrap them
        host->mpConsole->buffer.mWrapAt = 500;
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    return mpServer->clientConnected();
                },
                2000));

        QByteArray data;
        const QByteArray line = QString(QString(72, QChar('y')) + qsl(" hello")).toUtf8();
        for (int i = 0; i < 100; ++i) {
            data += line + "\r\n";
        }
        mpServer->sendRaw(data);

        const QString hintLinkText = qsl("  ➜ Click here to turn that on now");
        QVERIFY2(waitForLineInBuffer(hintLinkText), "the link offering to undo the game's wrapping was not printed");

        const TChar hintLink = firstCharacterOf(hintLinkText);
        QVERIFY2(hintLink.isUnderlined(), "the hint link is not underlined, so it does not read as clickable");
        QVERIFY2(hintLink.foreground() != QColor(Qt::blue), "the hint link is still in the blue that is unreadable on a dark background");
        QVERIFY2(contrastRatio(hintLink.foreground(), hintLink.background()) > 4.5, "the hint link does not contrast enough with the console background to be read");

        clickLink(hintLinkText);
        QVERIFY2(host->mUndoServerWrap, "clicking the hint link did not turn undoing the game's wrapping on");
        QCOMPARE(host->mUndoServerWrapWidth, 78);

        const QString confirmation = qsl("[ INFO ]  - Mudlet now undoes the game's wrapping, so triggers see whole lines:");
        QVERIFY2(waitForLineInBuffer(confirmation), "turning the option on was not confirmed as a Mudlet info message");

        const QString undoLinkText = qsl("  ➜ Undo, leave the lines as they come in");
        QVERIFY2(waitForLineInBuffer(undoLinkText), "the confirmation did not offer a link to undo the change");
        const TChar undoLink = firstCharacterOf(undoLinkText);
        QCOMPARE(undoLink.foreground(), hintLink.foreground());

        clickLink(undoLinkText);
        QVERIFY2(!host->mUndoServerWrap, "clicking the undo link did not turn undoing the game's wrapping back off");
        QVERIFY2(waitForLineInBuffer(qsl("[ INFO ]  - Mudlet no longer undoes the game's wrapping, so triggers see the")), "turning the option back off was not confirmed as a Mudlet info message");

        // Every click that changes something reports; clicking a link for the
        // setting it already has is the one case that stays quiet
        const QString offMessage = qsl("[ INFO ]  - Mudlet no longer undoes the game's wrapping, so triggers see the");
        QCOMPARE(bufferLineCount(offMessage), 1);
        clickLink(undoLinkText);
        QTest::qWait(100);
        QCOMPARE(bufferLineCount(offMessage), 1);
        QVERIFY2(!host->mUndoServerWrap, "a redundant click changed the setting");
    }

    void test_settingIsNotAnnouncedWithoutTheHint()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        QVERIFY(!host->mServerWrapHintShown);

        host->mLuaInterpreter.compileAndExecuteScript(qsl("setConfig(\"undoServerWrap\", true)"));
        QVERIFY(host->mUndoServerWrap);
        QTest::qWait(100);

        QVERIFY2(!bufferHasLine(qsl("[ INFO ]  - Mudlet now undoes the game's wrapping, so triggers see whole lines:")),
                 "a script changing the setting was reported on although no hint had offered it");
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

    // Runs the link's command the way a click would. A real click does more -
    // spoilers, disabled links, visited state, Lua-reference links - none of
    // which these hint links use
    void clickLink(const QString& text)
    {
        auto host = mudlet::self()->getActiveHost();
        const QStringList commands = host->mpConsole->buffer.mLinkStore.getLinksConst(firstCharacterOf(text).linkIndex());
        QVERIFY2(commands.size() == 1, qPrintable(qsl("\"%1\" is not a link carrying exactly one command").arg(text)));
        host->mLuaInterpreter.compileAndExecuteScript(commands.first());
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

    int bufferLineCount(const QString& text)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        int found = 0;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i) == text) {
                ++found;
            }
        }
        return found;
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
