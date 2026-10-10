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
 * tempComplexRegexTrigger()'s eleventh argument names a sound file to play on
 * every fire. Whether a trigger is a sound trigger leaves nothing a script can
 * read back - a fire with no file only logs a warning - so it is checked here
 * on the trigger itself (#10654).
 *
 * Run with: ctest -R TriggerSoundArgumentTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TriggerSoundArgumentTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("TriggerSoundArgument-Test");
    const QString mLocalhost = qsl("localhost");

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so a concurrent copy of this test
        // does not share a profile list with it - see UnitDeferredDeleteTest
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mHostname);

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            TestProfile::removeProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_emptySoundFileMakesNoSoundTrigger()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(R"(tempComplexRegexTrigger("soundArgumentEmpty", "^sound argument empty$", function() end, 0, 0, 0, 0, 0, 0, 0, "", 0, 0))")));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("soundArgumentEmpty"));
        QVERIFY(pTrigger);
        QVERIFY2(!pTrigger->mSoundTrigger, "an empty sound file name should mean no sound, as a number or nil does");
    }

    // The control: a real name still makes a sound trigger
    void test_namedSoundFileMakesASoundTrigger()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(
                qsl(R"(tempComplexRegexTrigger("soundArgumentNamed", "^sound argument named$", function() end, 0, 0, 0, 0, 0, 0, 0, "ding.wav", 0, 0))")));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("soundArgumentNamed"));
        QVERIFY(pTrigger);
        QVERIFY(pTrigger->mSoundTrigger);
    }
};

#include "TriggerSoundArgumentTest.moc"
MUDLET_GROUPED_TEST_MAIN(TriggerSoundArgumentTest)
