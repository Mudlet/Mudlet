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
 * The mute switches live in MudletMedia and the main window only mirrors them,
 * so these check both directions of that: a switch thrown anywhere (Lua's
 * setConfig lands on the same setters) shows on the toolbar and the menu, and
 * the toolbar and menu throw the switch rather than keeping state of their own.
 * No profile is opened, as none of this needs one.
 *
 * Run with: ctest -R MediaMuteStateTest -V
 */

#include <QAction>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest/QtTest>

#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "MudletMedia.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MediaMuteStateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static QAction* action(const QString& objectName) { return mudlet::self()->findChild<QAction*>(objectName); }

    static void unmuteBoth()
    {
        MudletMedia::self()->setApiMuted(false);
        MudletMedia::self()->setGameMuted(false);
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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY(MudletMedia::self());
    }

    void init() { unmuteBoth(); }

    void cleanupTestCase()
    {
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_eachSwitchShowsOnItsOwnToolbarAndMenuEntry()
    {
        QAction* toolbarApi = action(qsl("muteAPI"));
        QAction* menuApi = action(qsl("dactionMuteAPI"));
        QAction* toolbarGame = action(qsl("muteGame"));
        QAction* menuGame = action(qsl("dactionMuteGame"));
        QVERIFY(toolbarApi && menuApi && toolbarGame && menuGame);

        MudletMedia::self()->setApiMuted(true);

        QVERIFY(toolbarApi->isChecked());
        QVERIFY(menuApi->isChecked());
        QCOMPARE(toolbarApi->text(), qsl("Unmute sounds from Mudlet (Triggers, Scripts, etc.)"));
        QVERIFY2(!toolbarGame->isChecked() && !menuGame->isChecked(), "muting scripts' sounds showed the game's as muted too");
        QCOMPARE(toolbarGame->text(), qsl("Mute sounds from the game (MCMP, MSP)"));

        MudletMedia::self()->setGameMuted(true);

        QVERIFY(toolbarGame->isChecked());
        QVERIFY(menuGame->isChecked());
        QCOMPARE(toolbarGame->text(), qsl("Unmute sounds from the game (MCMP, MSP)"));

        MudletMedia::self()->setApiMuted(false);

        QVERIFY(!toolbarApi->isChecked());
        QVERIFY(!menuApi->isChecked());
        QCOMPARE(toolbarApi->text(), qsl("Mute sounds from Mudlet (triggers, scripts, etc.)"));
        QVERIFY(toolbarGame->isChecked());
    }

    // "Unmute all media" is only on offer once both switches are thrown
    void test_theMuteAllButtonShowsWhetherBothAreMuted()
    {
        QAction* toolbarAll = action(qsl("muteMedia"));
        QAction* menuAll = action(qsl("dactionMuteMedia"));
        auto* button = mudlet::self()->findChild<QToolButton*>(qsl("mute"));
        QVERIFY(toolbarAll && menuAll && button);

        MudletMedia::self()->setApiMuted(true);
        QVERIFY(!toolbarAll->isChecked());
        QVERIFY(!menuAll->isChecked());
        QCOMPARE(button->text(), qsl("Mute all media"));

        MudletMedia::self()->setGameMuted(true);
        QVERIFY(toolbarAll->isChecked());
        QVERIFY(menuAll->isChecked());
        QVERIFY(button->isChecked());
        QCOMPARE(toolbarAll->text(), qsl("Unmute all media"));
        QCOMPARE(button->text(), qsl("Unmute all media"));
    }

    void test_theToolbarAndMenuThrowTheSwitches()
    {
        QAction* toolbarApi = action(qsl("muteAPI"));
        QAction* menuGame = action(qsl("dactionMuteGame"));
        QVERIFY(toolbarApi && menuGame);

        toolbarApi->trigger();
        QVERIFY(MudletMedia::self()->apiMuted());
        QVERIFY(!MudletMedia::self()->gameMuted());

        menuGame->trigger();
        QVERIFY(MudletMedia::self()->gameMuted());

        toolbarApi->trigger();
        QVERIFY(!MudletMedia::self()->apiMuted());
        QVERIFY(MudletMedia::self()->gameMuted());
    }

    // Mute all mutes whichever is not muted yet, and only unmutes once both are
    void test_muteAllMutesTheRestThenUnmutesBoth()
    {
        QAction* toolbarAll = action(qsl("muteMedia"));
        QVERIFY(toolbarAll);

        MudletMedia::self()->setGameMuted(true);
        toolbarAll->trigger();
        QVERIFY(MudletMedia::self()->apiMuted());
        QVERIFY(MudletMedia::self()->gameMuted());

        toolbarAll->trigger();
        QVERIFY(!MudletMedia::self()->apiMuted());
        QVERIFY(!MudletMedia::self()->gameMuted());
    }

    // Setting the value already held still puts a control that drifted from
    // it back in step, as a checkable action flips itself before it asks
    void test_settingTheHeldValueResyncsTheControls()
    {
        QAction* toolbarApi = action(qsl("muteAPI"));
        QAction* menuApi = action(qsl("dactionMuteAPI"));
        QVERIFY(toolbarApi && menuApi);

        MudletMedia::self()->setApiMuted(true);
        toolbarApi->setChecked(false);
        menuApi->setChecked(false);

        MudletMedia::self()->setApiMuted(true);

        QVERIFY(toolbarApi->isChecked());
        QVERIFY(menuApi->isChecked());
    }
};

#include "MediaMuteStateTest.moc"
MUDLET_GROUPED_TEST_MAIN(MediaMuteStateTest)
