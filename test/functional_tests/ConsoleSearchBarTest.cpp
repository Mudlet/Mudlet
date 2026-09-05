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

    bool waitForTextInBuffer(const QString& text, const int timeoutMs = 5000)
    {
        TMainConsole* console = mudlet::self()->getActiveHost()->mpConsole;
        return QTest::qWaitFor(
                [console, &text]() {
                    return lineHolding(console, text) > -1;
                },
                timeoutMs);
    }

    TMainConsole* startSearchableProfile()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto* host = mudlet::self()->getActiveHost();
        TMainConsole* console = host->mpConsole;

        // Room for the longest fixture line, so that what the assertions below
        // count as one line cannot be wrapped into two:
        console->setWrapAt(200);
        mpServer->sendRaw(searchableText().toUtf8());
        if (!waitForTextInBuffer(qsl("nothing else moves."))) {
            return nullptr;
        }
        return console;
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            const auto dialog = []() {
                return mudlet::self()->mpConnectionDialog.data();
            };

            mudlet::self()->startAutoLogin({});

            if (!QTest::qWaitFor(
                        [&dialog]() {
                            return dialog() && dialog()->isVisible();
                        },
                        5000)) {
                qWarning() << "the connection dialog never appeared";
                return;
            }

            const auto waitForFocus = [](QWidget* field, const char* name) {
                if (QTest::qWaitFor(
                            [field]() {
                                return QApplication::focusWidget() == field;
                            },
                            5000)) {
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
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(mudlet::getMudletPath(enums::profileHomePath, profileName)); }

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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

    // A search is for the most recent thing said, so it starts at the end of the
    // buffer and works back - and it marks the whole line it stops on, not just
    // the first hit on it.
    void test_aNewSearchStartsAtTheEndAndMarksEveryHitOnTheLineItStopsOn()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the fixture text never reached the buffer");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int earlierLine = lineHolding(console, qsl("gorbash sleeps"));
        QVERIFY(laterLine > earlierLine && earlierLine > -1);

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();

        QCOMPARE(console->mCurrentSearchResult, laterLine);
        QCOMPARE(markedCharactersOn(console, laterLine), 14);
        QCOMPARE(markedCharactersOn(console, earlierLine), 0);
    }

    // Searching for the same thing again is how the earlier times it was said
    // are got to, and what is already marked stays marked as that happens.
    void test_searchingAgainStepsToTheMatchAboveTheOneItIsOn()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the fixture text never reached the buffer");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int earlierLine = lineHolding(console, qsl("gorbash sleeps"));

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        console->slot_searchBufferUp();

        QCOMPARE(console->mCurrentSearchResult, earlierLine);
        QCOMPARE(markedCharactersOn(console, earlierLine), 7);
        QCOMPARE(markedCharactersOn(console, laterLine), 14);
    }

    void test_searchingDownComesBackTowardsTheEndOfTheBuffer()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the fixture text never reached the buffer");

        const int laterLine = lineHolding(console, qsl("GORBASH stirs"));
        const int earlierLine = lineHolding(console, qsl("gorbash sleeps"));

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferUp();
        console->slot_searchBufferUp();
        QCOMPARE(console->mCurrentSearchResult, earlierLine);

        console->slot_searchBufferDown();
        QCOMPARE(console->mCurrentSearchResult, laterLine);
    }

    // A search that has not moved off the end of the buffer yet has nothing
    // below it, and saying so would be worse than staying quiet.
    void test_searchingDownWithNothingBelowTheCurrentLineDoesNothing()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the fixture text never reached the buffer");

        console->mpBufferSearchBox->setText(qsl("gorbash"));
        console->slot_searchBufferDown();

        QCOMPARE(console->mCurrentSearchResult, static_cast<int>(console->buffer.lineBuffer.size()));
        QCOMPARE(markedLineCount(console), 0);
        QCOMPARE(lineHolding(console, qsl("No search results")), -1);
    }

    void test_theCaseSensitiveOptionSkipsALineThatOnlyMatchesWithoutIt()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the fixture text never reached the buffer");

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
        QVERIFY2(console, "the fixture text never reached the buffer");

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

    void test_aTermThatIsNowhereInTheBufferSaysSo()
    {
        auto* console = startSearchableProfile();
        QVERIFY2(console, "the fixture text never reached the buffer");

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
        QVERIFY2(console, "the fixture text never reached the buffer");

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
        startProfile(mHostname, mLocalhost, mPort);
        auto* host = mudlet::self()->getActiveHost();
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
};

#include "ConsoleSearchBarTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConsoleSearchBarTest)
