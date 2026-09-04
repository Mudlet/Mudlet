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
 * Someone who used Mudlet before and has no profiles left still counts as an
 * experienced player, so the connection dialog owes them the games list rather
 * than a beginner's tutorial invitation - the same answer the interface tour
 * gives.
 *
 * The gate memoises for the life of the process, so the fresh-install half of
 * this lives in TutorialInvitationLayoutTest.
 *
 * Run with: ctest -R TutorialInvitationGateTest -V
 */

#include <QtTest/QtTest>

#include <QPushButton>

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class TutorialInvitationGateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));

        // settings on file and no profiles: what Mudlet looks like to someone
        // who used a release from before the first launch date was recorded and
        // has since removed their profiles. Written before start() so that
        // init() finds the same traces of earlier use a returning user leaves.
        {
            QSettings settings(qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()), QSettings::IniFormat);
            settings.setValue(qsl("telnetHandlerAsked"), true);
            settings.sync();
            QCOMPARE(settings.status(), QSettings::NoError);
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "settings on file with no recorded first launch have to read as earlier use");
        QVERIFY2(QDir(mudlet::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty(), "this case is about having no profiles left");
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_returningPlayerGetsTheGamesList()
    {
        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        dlg->fillout_form();

        auto* skipButton = dlg->findChild<QPushButton*>(qsl("skipToGamesButton"));
        QVERIFY(skipButton);
        QVERIFY2(!skipButton->isVisibleTo(dlg), "the invitation's skip button means the games list was taken away from a returning player");
        // the invitation hides the list by hiding its parent, which leaves
        // isHidden() false on the list itself - so ask about visibility
        QVERIFY2(dlg->listWidget_profiles->isVisibleTo(dlg), "a returning player has to be given the games list");
        QVERIFY2(dlg->listWidget_profiles->count() > 0, "the games list has to be filled");
        // having no saved profiles puts the welcome message up on its way
        // through fillout_form(); picking a game takes it back down again
        QVERIFY2(!dlg->welcome_message->isVisibleTo(dlg), "the invitation must not be left over next to the games list");
        QVERIFY2(dlg->tabWidget_connectionInfo->isVisibleTo(dlg), "the connection details belong on screen for the selected game");

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};

#include "TutorialInvitationGateTest.moc"
MUDLET_GROUPED_TEST_MAIN(TutorialInvitationGateTest)
