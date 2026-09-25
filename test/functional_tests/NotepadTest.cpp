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
#include <QAction>
#include <QIcon>
#include <QLineEdit>
#include <QPalette>
#include <QPlainTextEdit>
#include <QScopeGuard>
#include <QTabWidget>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "dlgNotepad.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// The notepad keeps a profile's notes in tabs, sends lines from them to the
// game, and searches them - none of which has a Lua binding, so all of it has
// gone untested. What the tabs hold has to survive the file they are written
// to, what is sent has to arrive at the game, and a search has to mark what it
// found; those are what these cover.
class NotepadTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    const QString mHostname = "Test-Notepad";
    QString mPort; // assigned the server's actual ephemeral port in init()
    const QString mLocalhost = "localhost";

    // The names the notepad writes and the ones it upgrades from, which the
    // tests below have to put in place or clear out before it starts up.
    QString notesPath(const QString& fileName) const { return MudletApp::getMudletPath(enums::profileDataItemPath, mHostname, fileName); }

    static QPlainTextEdit* editAt(dlgNotepad* notepad, const int index) { return qobject_cast<QPlainTextEdit*>(notepad->tabWidget->widget(index)); }

    static QLineEdit* findBox(dlgNotepad* notepad) { return notepad->findChild<QLineEdit*>(qsl("notepadFindBox")); }

    static QToolButton* findBarButton(dlgNotepad* notepad, const QString& name) { return notepad->findChild<QToolButton*>(name); }

    // A QIcon holds no record of which file it was built from, so the only way
    // to name the arrow on screen is to render it and compare the pixels. The
    // ratio is pinned to 1 because the resource it is compared against is not
    // scaled, and a pixmap asked for without one comes back at the display's.
    static bool iconIs(const QIcon& icon, const QString& resourcePath, const QIcon::State state = QIcon::Off)
    {
        const QPixmap expected(resourcePath);
        if (expected.isNull()) {
            return false;
        }
        return icon.pixmap(expected.size(), 1.0, QIcon::Normal, state).toImage() == expected.toImage();
    }

    bool waitForServerToReceive(const QByteArray& text) const
    {
        return QTest::qWaitFor(
                [this, &text]() {
                    return mpServer->received().contains(text);
                },
                5000);
    }

    void writeNotesFile(const QString& fileName, const QByteArray& content) const
    {
        QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mHostname));
        QFile file(notesPath(fileName));
        QVERIFY2(file.open(QIODevice::WriteOnly), "could not write the notes file the test needs in place");
        file.write(content);
        file.close();
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
        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

    // Notes are the one thing in a profile a player types by hand, so what the
    // tabs hold - and which one was open - has to come back exactly.
    void test_theTabsAndWhatTheyHoldComeBackFromTheFile()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto* profile = mudlet::self()->getActiveHost();

        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(profile));
        editAt(notepad.data(), 0)->setPlainText(qsl("first note"));
        notepad->addTab(qsl("Herbs"), qsl("valerian\nbloodroot"));
        notepad->tabWidget->setCurrentIndex(1);
        notepad->save();
        notepad.reset();

        QScopedPointer<dlgNotepad> reopened(new dlgNotepad(profile));
        QCOMPARE(reopened->tabWidget->count(), 2);
        QCOMPARE(reopened->tabWidget->tabText(1), qsl("Herbs"));
        QCOMPARE(editAt(reopened.data(), 0)->toPlainText(), qsl("first note"));
        QCOMPARE(editAt(reopened.data(), 1)->toPlainText(), qsl("valerian\nbloodroot"));
        QCOMPARE(reopened->tabWidget->currentIndex(), 1);
    }

    // Closing the only tab would leave the notepad with nowhere to type.
    void test_theLastTabCannotBeClosed()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));

        QCOMPARE(notepad->tabWidget->count(), 1);
        notepad->closeTab(0);
        QCOMPARE(notepad->tabWidget->count(), 1);
    }

    void test_closingATabLeavesTheOthersInPlace()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));

        notepad->addTab(qsl("Herbs"), qsl("valerian"));
        notepad->addTab(qsl("Enemies"), qsl("a goblin"));
        QCOMPARE(notepad->tabWidget->count(), 3);

        notepad->closeTab(1);

        QCOMPARE(notepad->tabWidget->count(), 2);
        QCOMPARE(notepad->tabWidget->tabText(1), qsl("Enemies"));
        QCOMPARE(editAt(notepad.data(), 1)->toPlainText(), qsl("a goblin"));
        QVERIFY2(editAt(notepad.data(), 0)->toPlainText().isEmpty(), "a tab other than the one asked for was closed");
    }

    // Notes written before the notepad had tabs are a plain text file, and the
    // upgrade to the tabbed one has to bring them across rather than lose them.
    void test_notesFromBeforeThereWereTabsAreBroughtAcross()
    {
        startProfile(mHostname, mLocalhost, mPort);
        writeNotesFile(qsl("notes_utf8.txt"), QStringLiteral("an old note about a dragon").toUtf8());

        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));

        QCOMPARE(notepad->tabWidget->count(), 1);
        QCOMPARE(editAt(notepad.data(), 0)->toPlainText(), qsl("an old note about a dragon"));
    }

    // A notes file that cannot be read is not a reason to come up with no tab
    // at all, which would leave the notepad unusable.
    void test_aNotesFileThatIsNotJsonStillOpensANote()
    {
        startProfile(mHostname, mLocalhost, mPort);
        writeNotesFile(qsl("notes.json"), QByteArrayLiteral("{ this is not json"));

        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));

        QCOMPARE(notepad->tabWidget->count(), 1);
        QVERIFY2(editAt(notepad.data(), 0)->toPlainText().isEmpty(), "a note that could not be read came back with something in it");
    }

    void test_sendingAllOfANoteSendsEveryLineOfItToTheGame()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        editAt(notepad.data(), 0)->setPlainText(qsl("north\nlook\nsouth"));

        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_sendAll"));

        QVERIFY2(waitForServerToReceive("north"), "the first line of the note never reached the game");
        QVERIFY2(waitForServerToReceive("look"), "the middle line of the note never reached the game");
        QVERIFY2(waitForServerToReceive("south"), "the last line of the note never reached the game");
    }

    // The prepend box is how a note of bare names is turned into commands.
    void test_thePrependedTextGoesInFrontOfEveryLineSent()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        editAt(notepad.data(), 0)->setPlainText(qsl("valerian\nbloodroot"));

        auto* prepend = notepad->findChild<QLineEdit*>(qsl("notepadPrependText"));
        QVERIFY(prepend);
        prepend->setText(qsl("outr "));

        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_sendAll"));

        QVERIFY2(waitForServerToReceive("outr valerian"), "the first line went without the text in front of it");
        QVERIFY2(waitForServerToReceive("outr bloodroot"), "the second line went without the text in front of it");
    }

    // Stopping is the way out of having sent a long note by mistake, so what is
    // left of it must not carry on arriving.
    void test_stoppingLeavesTheRestOfTheNoteUnsent()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        editAt(notepad.data(), 0)->setPlainText(qsl("north\nlook\nsouth\neast"));

        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_sendAll"));

        // Stop on the tick that sends the first line rather than on its arrival
        // at the game, so the stop lands at the start of the 300ms the notepad
        // leaves before the next line instead of some way into it.
        auto* sendTimer = notepad->findChild<QTimer*>();
        QVERIFY2(sendTimer, "the notepad has no timer pacing the lines it sends");
        QSignalSpy firstLineSent(sendTimer, &QTimer::timeout);
        QVERIFY2(firstLineSent.wait(5000), "the notepad never got round to sending the first line");
        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_stopSending"));

        QVERIFY2(waitForServerToReceive("north"), "the line already on its way when sending was stopped never reached the game");
        // Comfortably longer than the 300ms the notepad leaves between lines,
        // so the rest would have arrived by now had stopping not taken:
        QTest::qWait(1200ms);
        QVERIFY2(!mpServer->received().contains("look"), "the next line arrived after sending was stopped");
        QVERIFY2(!mpServer->received().contains("south"), "a line after the next one arrived after sending was stopped");
        QVERIFY2(!mpServer->received().contains("east"), "the last line arrived after sending was stopped");
    }

    // Every match is marked, and the one being looked at is marked differently
    // so it can be told apart from the rest.
    void test_searchingMarksEveryMatchAndTheOneBeingLookedAt()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        editAt(notepad.data(), 0)->setPlainText(qsl("a herb here\nanother herb there\nand a herb everywhere"));

        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_showFindBar"));
        findBox(notepad.data())->setText(qsl("herb"));
        QCOMPARE(editAt(notepad.data(), 0)->extraSelections().size(), 3);

        findBarButton(notepad.data(), qsl("notepadFindNext"))->click();

        const auto selections = editAt(notepad.data(), 0)->extraSelections();
        QCOMPARE(selections.size(), 3);
        int currentMatches = 0;
        for (const auto& selection : selections) {
            if (selection.format.background().color() == QColor(255, 165, 0, 150)) {
                ++currentMatches;
            }
        }
        QCOMPARE(currentMatches, 1);
    }

    // A search that has run out of note starts again at the top rather than
    // stopping at the last match.
    void test_searchingOnPastTheLastMatchComesBackToTheFirst()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        auto* textEdit = editAt(notepad.data(), 0);
        textEdit->setPlainText(qsl("one herb\ntwo herb"));

        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_showFindBar"));
        findBox(notepad.data())->setText(qsl("herb"));
        const int firstMatch = textEdit->textCursor().selectionStart();

        auto* nextButton = findBarButton(notepad.data(), qsl("notepadFindNext"));
        QVERIFY(nextButton);
        nextButton->click();
        const int secondMatch = textEdit->textCursor().selectionStart();
        QVERIFY2(secondMatch > firstMatch, "the search did not move on to the second match");

        nextButton->click();
        QCOMPARE(textEdit->textCursor().selectionStart(), firstMatch);
    }

    void test_closingTheFindBarTakesTheMarksWithIt()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        editAt(notepad.data(), 0)->setPlainText(qsl("a herb here\nanother herb there"));

        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_showFindBar"));
        findBox(notepad.data())->setText(qsl("herb"));
        QCOMPARE(editAt(notepad.data(), 0)->extraSelections().size(), 2);

        auto* closeButton = findBarButton(notepad.data(), qsl("notepadFindClose"));
        QVERIFY(closeButton);
        closeButton->click();

        QVERIFY2(notepad->findChild<QWidget*>(qsl("notepadFindBar"))->isHidden(), "the find bar stayed up");
        QCOMPARE(editAt(notepad.data(), 0)->extraSelections().size(), 0);
    }

    // Clearing the box is how a search is called off without closing the bar.
    void test_anEmptySearchMarksNothing()
    {
        startProfile(mHostname, mLocalhost, mPort);
        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        editAt(notepad.data(), 0)->setPlainText(qsl("a herb here\nanother herb there"));

        QVERIFY(QMetaObject::invokeMethod(notepad.data(), "slot_showFindBar"));
        findBox(notepad.data())->setText(qsl("herb"));
        QCOMPARE(editAt(notepad.data(), 0)->extraSelections().size(), 2);

        findBox(notepad.data())->setText(QString());
        QCOMPARE(editAt(notepad.data(), 0)->extraSelections().size(), 0);
    }

    // The send controls hide behind one small arrow, and the grey arrow the .ui
    // file carries is invisible on a dark background, so the icon has to be
    // picked from the palette - both as the notepad opens and again if the
    // appearance is switched while it is up (#9488).
    void test_theSendControlsArrowIsPickedForTheBackgroundItSitsOn()
    {
        startProfile(mHostname, mLocalhost, mPort);

        const QPalette savedPalette = QApplication::palette();
        const auto restorePalette = qScopeGuard([savedPalette]() {
            QApplication::setPalette(savedPalette);
        });

        // Dark first, because the .ui file's own icon is the grey one: an arrow
        // asserted to be grey says nothing about whether anything picked it.
        QPalette darkPalette(savedPalette);
        darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
        QApplication::setPalette(darkPalette);
        QApplication::processEvents();

        QScopedPointer<dlgNotepad> notepad(new dlgNotepad(mudlet::self()->getActiveHost()));
        QAction* pToggle = notepad->action_toggleSendControls;
        QVERIFY2(iconIs(pToggle->icon(), qsl(":/icons/arrow-right-16x.png")), "the notepad came up with the arrow that disappears into a dark background");
        QVERIFY2(iconIs(pToggle->icon(), qsl(":/icons/arrow-down-16x.png"), QIcon::On), "the notepad came up with an arrow that disappears into a dark background once the controls are open");

        QPalette lightPalette(savedPalette);
        lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
        QApplication::setPalette(lightPalette);
        QApplication::processEvents();

        QVERIFY2(iconIs(pToggle->icon(), qsl(":/icons/arrow-right_grey-16x.png")), "switching to a light appearance left the arrow the one meant for a dark one");
        QVERIFY2(iconIs(pToggle->icon(), qsl(":/icons/arrow-down_grey-16x.png"), QIcon::On), "switching to a light appearance left the opened-controls arrow the one meant for a dark one");
    }
};

#include "NotepadTest.moc"
MUDLET_GROUPED_TEST_MAIN(NotepadTest)
