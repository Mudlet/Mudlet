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

#include <QDir>
#include <QLineEdit>
#include <QScrollBar>
#include <QSplitter>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// The search bar over the console is reachable only from its own widgets: there
// is no Lua binding for it, which is why none of it has ever been under test.
// What it does is entirely in what it leaves behind - which line the search is
// now on, and which characters are marked as found - so that is what these
// assert, rather than anything about how the result is drawn.
class ConsoleSearchBarTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-ConsoleSearch";
    const QString mOtherHostname = "Test-ConsoleSearch-Other";
    QString mPort; // assigned the stub's actual ephemeral port in init()
    const QString mLocalhost = "localhost";

    // One line carries the term once in lower case and a later one carries it
    // twice in upper case, so a single fixture answers where a search starts,
    // how far each step takes it, and what the case-sensitivity option changes.
    static QString searchableText()
    {
        return qsl("gorbash sleeps by the fire.\r\n"
                   "a dusty road leads north.\r\n"
                   "GORBASH stirs and GORBASH yawns.\r\n"
                   "nothing else moves.\r\n");
    }

    // The last line holding the text, since a search starts at the end of the
    // buffer and the profile's own startup lines sit above the fixture.
    static int lineHolding(TConsole* console, const QString& text)
    {
        for (int i = console->buffer.getLastLineNumber(); i >= 0; --i) {
            if (console->buffer.lineBuffer.at(i).contains(text)) {
                return i;
            }
        }
        return -1;
    }

    static int markedCharactersOn(TConsole* console, const int lineNumber)
    {
        int count = 0;
        for (const auto& character : console->buffer.buffer.at(lineNumber)) {
            if (character.isFound()) {
                ++count;
            }
        }
        return count;
    }

    static int markedLineCount(TConsole* console)
    {
        int count = 0;
        for (int i = console->buffer.getLastLineNumber(); i >= 0; --i) {
            if (markedCharactersOn(console, i) > 0) {
                ++count;
            }
        }
        return count;
    }

    // Deep enough that the scroll bar is off its minimum, with the buffer one line short of a trim
    static void fillToTheBrimAround(TConsole* console, const QString& text)
    {
        console->buffer.setBufferSize(100, 20);
        while (console->buffer.size() < 70) {
            console->print(qsl("filler line\n"));
        }
        console->print(text + qsl("\n"));
        while (console->buffer.size() < 100) {
            console->print(qsl("filler line\n"));
        }
    }

    static bool runLua(TConsole* console, const QString& script) { return console->mpHost->getLuaInterpreter()->compileAndExecuteScript(script); }

    bool waitForTextInBuffer(TMainConsole* console, const QString& text, const int timeoutMs = 5000)
    {
        return QTest::qWaitFor(
                [console, &text]() {
                    return lineHolding(console, text) > -1;
                },
                timeoutMs);
    }

    TMainConsole* startSearchableProfile()
    {
        Host* host = startProfile(mHostname, mLocalhost, mPort);
        if (!host) {
            return nullptr;
        }
        TMainConsole* console = host->mainConsoleView();

        // Room for the longest fixture line, so that what the assertions below
        // count as one line cannot be wrapped into two:
        console->setWrapAt(200);
        mpServer->sendRaw(searchableText().toUtf8());
        if (!waitForTextInBuffer(console, qsl("nothing else moves."))) {
            return nullptr;
        }
        return console;
    }

    // Reports what went wrong and hands back nothing, so the caller's own
    // assertion ends the test instead of the null host doing it.
    Host* startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0ms, qApp, [hostname, address, port]() {
            const auto dialog = []() {
                return mudlet::self()->mpConnectionDialog.data();
            };

            mudlet::self()->startAutoLogin({});

            if (!QTest::qWaitFor(
                        [&dialog]() {
                            return dialog() && dialog()->isVisible();
                        },
                        5s)) {
                qWarning() << "the connection dialog never appeared";
                return;
            }

            const auto waitForFocus = [](QWidget* field, const char* name) {
                if (QTest::qWaitFor(
                            [field]() {
                                return QApplication::focusWidget() == field;
                            },
                            5s)) {
                    return true;
                }
                qWarning() << "focus never reached the" << name << "field";
                return false;
            };

            QTest::mouseClick(dialog()->new_profile_button, Qt::LeftButton);
            if (!waitForFocus(dialog()->profile_name_entry, "profile name")) {
                return;
            }
            QTest::keyClicks(dialog()->profile_name_entry, hostname);
            QTest::keyClick(dialog()->profile_name_entry, Qt::Key_Tab);

            if (!waitForFocus(dialog()->host_name_entry, "server address")) {
                return;
            }
            QTest::keyClicks(dialog()->host_name_entry, address);
            QTest::keyClick(dialog()->host_name_entry, Qt::Key_Tab);

            if (!waitForFocus(dialog()->port_entry, "port")) {
                return;
            }
            QTest::keyClicks(dialog()->port_entry, port);
            QTest::keyClick(dialog()->port_entry, Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5s)) {
            qWarning() << "Profile took too long to load.";
            return nullptr;
        }
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            qWarning() << "No active host available for the test.";
            return nullptr;
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2s)) {
            qWarning() << "Could not connect with the host.";
            return nullptr;
        }
        return host;
    }

    // Picks an existing profile in the connection dialog and loads it, without connecting
    Host* reopenProfile(const QString& profileName)
    {
        if (!QTest::qWaitFor(
                    []() {
                        return mudlet::self()->mpConnectionDialog.isNull();
                    },
                    5s)) {
            qWarning() << "the connection dialog the profiles were opened with never went away";
            return nullptr;
        }
        mudlet::self()->slot_showConnectionDialog();
        dlgConnectionProfiles* dialog = mudlet::self()->mpConnectionDialog.data();
        if (!dialog) {
            qWarning() << "the connection dialog did not come back";
            return nullptr;
        }
        const auto items = dialog->findData(*dialog->listWidget_profiles, profileName, dlgConnectionProfiles::csmNameRole);
        if (items.isEmpty()) {
            qWarning() << "the connection dialog does not list" << profileName;
            return nullptr;
        }
        dialog->listWidget_profiles->setCurrentItem(items.first());
        dialog->slot_itemClicked(items.first());
        dialog->slot_load();
        if (!QTest::qWaitFor(
                    [&profileName]() {
                        Host* host = mudlet::self()->getActiveHost();
                        return host && host->getName() == profileName;
                    },
                    15s)) {
            qWarning() << profileName << "did not load again";
            return nullptr;
        }
        return mudlet::self()->getActiveHost();
    }

    // The console is never shown in a test, so nothing else lays the splitter out
    static QList<int> commandSplitterSizes(Host* host)
    {
        QSplitter* splitter = host->mainConsoleView()->commandSplitter;
        splitter->resize(1000, 30);
        splitter->refresh();
        return splitter->sizes();
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(MudletApp::getMudletPath(enums::profileHomePath, profileName)); }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
        deleteProfileDirectory(mOtherHostname);
        MudletApp::getQSettings()->remove(qsl("commandSearchSplitterState"));
    }

    void cleanup()
    {
        const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
        const QString otherProfilePath = MudletApp::getMudletPath(enums::profileHomePath, mOtherHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
        deleteDirectory(otherProfilePath);
    }

    // A search is for the most recent thing said, so it starts at the end of the
    // buffer and works back - and it marks the whole line it stops on, not just
    // the first hit on it.
    void test_aNewSearchStartsAtTheEndAndMarksEveryHitOnTheLineItStopsOn()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int earlierLine = lineHolding(console, qsl("gorbash sleeps"));
        QVERIFY(laterLine > earlierLine && earlierLine > -1);

        // Taken first, as the split-screen tutorial the search opens adds a line after it
        const int lastLine = console->buffer.getLastLineNumber();
        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();

        QCOMPARE(console->mCurrentSearchResult, laterLine);
        QCOMPARE(markedCharactersOn(console, laterLine), 14);
        QCOMPARE(markedCharactersOn(console, earlierLine), 0);
        QCOMPARE(console->buffer.mCursorY, std::min(laterLine + 3, lastLine));
    }

    // Searching for the same thing again is how the earlier times it was said
    // are got to, and what is already marked stays marked as that happens.
    void test_searchingAgainStepsToTheMatchAboveTheOneItIsOn()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int earlierLine = lineHolding(console, qsl("gorbash sleeps"));

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        console->slot_searchBufferUp();

        QCOMPARE(console->mCurrentSearchResult, earlierLine);
        QCOMPARE(markedCharactersOn(console, earlierLine), 7);
        QCOMPARE(markedCharactersOn(console, laterLine), 14);
        QCOMPARE(console->buffer.mCursorY, std::min(earlierLine + 3, console->buffer.getLastLineNumber()));
    }

    void test_searchingDownComesBackTowardsTheEndOfTheBuffer()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int earlierLine = lineHolding(console, qsl("gorbash sleeps"));

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        console->slot_searchBufferUp();
        QCOMPARE(console->mCurrentSearchResult, earlierLine);

        console->slot_searchBufferDown();
        QCOMPARE(console->mCurrentSearchResult, laterLine);
        QCOMPARE(console->buffer.mCursorY, std::min(laterLine + 3, console->buffer.getLastLineNumber()));
    }

    // The first search back from the end opens the split screen, and the lower
    // pane that opens must not push the match out of the upper one.
    void test_theSearchThatOpensTheSplitScreenStopsJustBelowTheMatch()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        // Deep enough in the buffer that the scroll bar is not held at its minimum
        mpServer->sendRaw(QByteArray("filler line\r\n").repeated(200) + "the marked line\r\n" + QByteArray("filler line\r\n").repeated(100));
        int markedLine = -1;
        QVERIFY(QTest::qWaitFor(
                [console, &markedLine]() {
                    markedLine = lineHolding(console, qsl("the marked line"));
                    return markedLine > -1 && console->buffer.getLastLineNumber() > markedLine + 99;
                },
                5000));
        QVERIFY(console->mLowerPane->isHidden());

        console->mpBufferSearchBox->setText(qsl("marked line"));
        console->slot_searchBufferUp();
        QCOMPARE(console->mCurrentSearchResult, markedLine);
        QCOMPARE(console->mpScrollBar->value(), markedLine + 3);
        QTest::qWait(100);

        QVERIFY(!console->mLowerPane->isHidden());
        // More rows than the split-screen tutorial prints, or the old padded scroll would land here too
        QVERIFY(console->mLowerPane->getRowCount() > 1);
        QCOMPARE(console->buffer.mCursorY, markedLine + 3);
        QVERIFY(markedLine + 3 > console->mpScrollBar->minimum());
        QCOMPARE(console->mpScrollBar->value(), markedLine + 3);
    }

    // The split-screen tutorial prints into this buffer, and in a full one that
    // trims the top, so it must not move the line the search is scrolling to.
    void test_theSplitScreenTutorialInAFullBufferDoesNotMoveTheSearchOffItsMatch()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");
        mudlet::self()->mScrollbackTutorialsShown = 0;
        QVERIFY2(mudlet::self()->showSplitscreenTutorial(), "the tutorial is off, so nothing here would trim the buffer");

        QVERIFY2(console->buffer.size() < 70, "the buffer is too full to fill to its limit around the marked line");
        fillToTheBrimAround(console, qsl("the marked line"));
        QCOMPARE(console->buffer.size(), 100);
        const int markedLine = lineHolding(console, qsl("the marked line"));
        QVERIFY(console->mLowerPane->isHidden());

        console->mpBufferSearchBox->setText(qsl("marked line"));
        console->slot_searchBufferUp();
        QTest::qWait(100);

        QVERIFY2(lineHolding(console, qsl("Split-screen scrollback activated")) > -1, "the tutorial was not printed");
        const int movedTo = lineHolding(console, qsl("the marked line"));
        QVERIFY2(movedTo >= 0 && movedTo < markedLine, "the tutorial did not trim the buffer, so nothing here was exercised");
        QCOMPARE(console->buffer.mCursorY, movedTo + 3);
        QCOMPARE(console->mpScrollBar->value(), movedTo + 3);
        QCOMPARE(console->mCurrentSearchResult, movedTo);
    }

    // As above, for a script's scrollTo(): the line it names is the line it lands on.
    void test_theSplitScreenTutorialInAFullBufferDoesNotMoveAScrollToOffItsLine()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");
        mudlet::self()->mScrollbackTutorialsShown = 0;
        QVERIFY2(mudlet::self()->showSplitscreenTutorial(), "the tutorial is off, so nothing here would trim the buffer");

        QVERIFY2(console->buffer.size() < 70, "the buffer is too full to fill to its limit around the marked line");
        fillToTheBrimAround(console, qsl("the marked line"));
        QCOMPARE(console->buffer.size(), 100);
        const int markedLine = lineHolding(console, qsl("the marked line"));

        QVERIFY(runLua(console, qsl("scrollTo('main', %1)").arg(markedLine)));
        QTest::qWait(100);

        QVERIFY2(lineHolding(console, qsl("Split-screen scrollback activated")) > -1, "the tutorial was not printed");
        const int movedTo = lineHolding(console, qsl("the marked line"));
        QVERIFY2(movedTo >= 0 && movedTo < markedLine, "the tutorial did not trim the buffer, so nothing here was exercised");
        QCOMPARE(console->buffer.mCursorY, movedTo);
    }

    // A search that has not moved off the end of the buffer yet has nothing
    // below it, and saying so would be worse than staying quiet.
    void test_searchingDownWithNothingBelowTheCurrentLineDoesNothing()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferDown();

        QCOMPARE(console->mCurrentSearchResult, static_cast<int>(console->buffer.lineBuffer.size()));
        QCOMPARE(markedLineCount(console), 0);
        QCOMPARE(lineHolding(console, qsl("No search results")), -1);
    }

    void test_theCaseSensitiveOptionSkipsALineThatOnlyMatchesWithoutIt()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int earlierLine = lineHolding(console, qsl("gorbash sleeps"));

        console->mpAction_searchCaseSensitive->trigger();
        QVERIFY2(console->mpAction_searchCaseSensitive->isChecked(), "triggering the menu entry did not turn case sensitivity on");

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();

        QCOMPARE(console->mCurrentSearchResult, earlierLine);
        QCOMPARE(markedCharactersOn(console, laterLine), 0);
        QCOMPARE(markedCharactersOn(console, earlierLine), 7);
    }

    // Typing a different term starts a search of its own: what the last one
    // marked goes, and the new one begins at the end of the buffer again rather
    // than carrying on from where the last one had got to.
    void test_aNewTermClearsWhatTheOldOneMarkedAndStartsOver()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int dustyLine = lineHolding(console, qsl("a dusty road"));
        QVERIFY(dustyLine < laterLine);

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        QCOMPARE(console->mCurrentSearchResult, laterLine);

        console->mpBufferSearchBox->setText(qsl("dusty"));
        console->slot_searchBufferUp();

        QCOMPARE(console->mCurrentSearchResult, dustyLine);
        QCOMPARE(markedCharactersOn(console, dustyLine), 5);
        QCOMPARE(markedCharactersOn(console, laterLine), 0);
    }

    // Trimming the oldest lines moves every line up, so the search has to move
    // with them or the next step starts from a line it has not searched yet.
    void test_aTrimMovesTheSearchWithTheLineItFound()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        QCOMPARE(console->mCurrentSearchResult, laterLine);

        QVERIFY2(console->buffer.size() < 100, "the buffer is already past the smallest limit, so the trim below would take the fixture with it");
        console->buffer.setBufferSize(100, 1);
        for (int i = 0; i < 200 && lineHolding(console, qsl("GORBASH stirs")) == laterLine; ++i) {
            console->print(qsl("filler\n"));
        }
        const int movedTo = lineHolding(console, qsl("GORBASH stirs"));
        QVERIFY2(movedTo >= 0 && movedTo < laterLine, "the buffer was never trimmed, so nothing here was exercised");

        QCOMPARE(console->mCurrentSearchResult, movedTo);

        console->mpBufferSearchBox->setText(qsl("nothing"));
        console->slot_searchBufferUp();
        QCOMPARE(markedCharactersOn(console, movedTo), 0);
    }

    // A new term clears only the lines the search has marked, so those have to
    // be followed wherever the lines go, and wherever their characters are copied.
    void test_aNewTermClearsMarksOnALineThatMovedUpWhenALineAboveWasDeleted()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        QCOMPARE(markedLineCount(console), 1);

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        QVERIFY(runLua(console, qsl("moveCursor('main', 0, %1) deleteLine()").arg(lineHolding(console, qsl("a dusty road")))));
        QCOMPARE(lineHolding(console, qsl("GORBASH stirs")), laterLine - 1);
        QCOMPARE(markedCharactersOn(console, laterLine - 1), 14);

        console->mpBufferSearchBox->setText(qsl("nothing"));
        console->slot_searchBufferUp();

        QCOMPARE(markedLineCount(console), 1);
    }

    // A mark is cleared only in the console that was searched, so a copy of the
    // text taken anywhere else must not take the mark with it.
    void test_aCopyOfAMarkedLineDoesNotCarryTheMark()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int dustyLine = lineHolding(console, qsl("a dusty road"));
        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        QCOMPARE(markedLineCount(console), 1);

        QVERIFY(runLua(console, qsl("moveCursor('main', 0, %1) selectCurrentLine() copy() moveCursor('main', 0, %2) paste() moveCursorEnd() appendBuffer()").arg(laterLine).arg(dustyLine)));
        QVERIFY2(console->buffer.lineBuffer.at(dustyLine).contains(qsl("GORBASH stirs")), "the paste never happened, so nothing here was exercised");
        QCOMPARE(lineHolding(console, qsl("GORBASH stirs")), console->buffer.getLastLineNumber() - 1);

        QCOMPARE(markedLineCount(console), 1);
        QCOMPARE(markedCharactersOn(console, laterLine), 14);
    }

    void test_aNewTermClearsMarksThatARewrapMovedOntoLaterLines()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();

        console->setWrapAt(12);
        QVERIFY(runLua(console, qsl("wrapLine('main', %1)").arg(laterLine)));
        QVERIFY2(markedCharactersOn(console, laterLine) < 14, "the rewrap left the whole match on one line, so nothing here was exercised");
        QCOMPARE(markedLineCount(console), 2);

        console->mpBufferSearchBox->setText(qsl("nothing"));
        console->slot_searchBufferUp();

        QCOMPARE(markedLineCount(console), 1);
    }

    void test_aTermThatIsNowhereInTheBufferSaysSo()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        console->mpBufferSearchBox->setText(qsl("dragonfruit"));
        console->slot_searchBufferUp();

        QVERIFY2(lineHolding(console, qsl("No search results, sorry!")) > -1, "a search that matched nothing did not say so");
        QCOMPARE(markedLineCount(console), 0);
    }

    // An empty box is the state the search bar opens in, so pressing Return in
    // it must not report that nothing was found.
    void test_anEmptyTermIsNotSearchedFor()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the profile never started, or the fixture text never reached its buffer - see the warning above");

        console->mpBufferSearchBox->setText(QString());
        console->slot_searchBufferUp();

        QCOMPARE(lineHolding(console, qsl("No search results")), -1);
        QCOMPARE(markedLineCount(console), 0);
    }

    // Only the Central Debug Console keeps its search widgets in a find bar of
    // their own. Putting that bar away has to take the marks with it, or they
    // are left on the text with nothing on screen to say why.
    void test_hidingTheFindBarClearsWhatTheSearchMarked()
    {
        Host* host = startProfile(mHostname, mLocalhost, mPort);
        QVERIFY2(host, "the profile never started");
        mudlet::self()->attachDebugArea(host->getName());

        auto* console = mudlet::smpDebugConsole.data();
        QVERIFY(console);
        console->clear();
        console->print(qsl("gorbash stirs\n"));

        console->showSearchBar();
        QVERIFY2(!console->mpFindBar.isNull(), "the Central Debug Console was built without a find bar");
        // Not isVisible(): the console's own window is never shown in a test,
        // and that would make every child of it invisible whatever the bar did.
        QVERIFY2(!console->mpFindBar->isHidden(), "the find bar did not come up");

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        QCOMPARE(markedLineCount(console), 1);

        QVERIFY(QMetaObject::invokeMethod(console, "hideSearchBar"));

        QVERIFY2(console->mpFindBar->isHidden(), "the find bar stayed up");
        QCOMPARE(markedLineCount(console), 0);
    }

    // Side by side in multi-view each profile has a split of its own, so
    // dragging one must neither decide where another opens nor where the
    // first comes back to.
    void test_eachProfileKeepsItsOwnSplitBetweenCommandLineAndSearchBar()
    {
        Host* first = startProfile(mHostname, mLocalhost, mPort);
        QVERIFY2(first, "the first profile never started - see the warning above");
        TMainConsole* firstConsole = first->mainConsoleView();
        QVERIFY(firstConsole->commandSplitter);
        firstConsole->commandSplitter->setSizes({100, 900});
        firstConsole->slot_saveCommandSearchSettings();

        Host* second = startProfile(mOtherHostname, mLocalhost, mPort);
        QVERIFY2(second && second != first, "the second profile never started - see the warning above");
        TMainConsole* secondConsole = second->mainConsoleView();
        QVERIFY(secondConsole->commandSplitter);
        QList<int> sizes = commandSplitterSizes(second);
        QCOMPARE(sizes.size(), 2);
        QVERIFY2(sizes.at(0) > sizes.at(1), qPrintable(qsl("the second profile opened with its command line %1 px wide and its search bar %2 px wide").arg(sizes.at(0)).arg(sizes.at(1))));
        secondConsole->commandSplitter->setSizes({900, 100});
        secondConsole->slot_saveCommandSearchSettings();

        first->forceClose();
        QVERIFY2(first->requestClose(), "closing the first profile was refused");
        HostManager::self()->deleteHost(mHostname);

        Host* reopened = reopenProfile(mHostname);
        QVERIFY2(reopened, "the first profile never came back - see the warning above");
        sizes = commandSplitterSizes(reopened);
        QCOMPARE(sizes.size(), 2);
        QVERIFY2(sizes.at(0) < sizes.at(1), qPrintable(qsl("the first profile came back with its command line %1 px wide and its search bar %2 px wide").arg(sizes.at(0)).arg(sizes.at(1))));
    }

    // Mudlet once kept a single split for every profile, so whoever dragged it
    // then should not lose it to a profile that has no split of its own yet.
    void test_aProfileWithoutASplitOfItsOwnStartsFromTheOneEveryProfileShared()
    {
        QSplitter shared;
        shared.addWidget(new QWidget(&shared));
        shared.addWidget(new QWidget(&shared));
        shared.setSizes({100, 900});
        MudletApp::getQSettings()->setValue(qsl("commandSearchSplitterState"), shared.saveState());

        Host* host = startProfile(mHostname, mLocalhost, mPort);
        QVERIFY2(host, "the profile never started - see the warning above");
        QVERIFY(host->mainConsoleView()->commandSplitter);
        const QList<int> sizes = commandSplitterSizes(host);
        QCOMPARE(sizes.size(), 2);
        QVERIFY2(sizes.at(0) < sizes.at(1), qPrintable(qsl("the profile opened with its command line %1 px wide and its search bar %2 px wide").arg(sizes.at(0)).arg(sizes.at(1))));
    }
};

#include "ConsoleSearchBarTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConsoleSearchBarTest)
