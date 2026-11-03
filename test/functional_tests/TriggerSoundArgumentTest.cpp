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
 * Not a spec: a sound trigger's fire with no file to play shows only in Mudlet's
 * log, which these watch for TMedia's warning that it has no URL to fetch a
 * missing file from; and they set the trigger editor's state of a ticked sound
 * box with no file directly.
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

class TriggerSoundArgumentTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QByteArray mSavedXdgCache;
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
        // TMedia makes its download cache directory before it finds it has no URL
        mSavedXdgCache = qgetenv("XDG_CACHE_HOME");
        qputenv("XDG_CACHE_HOME", qsl("%1/cache").arg(mConfigDir.path()).toUtf8());

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
        mSavedXdgCache.isNull() ? qunsetenv("XDG_CACHE_HOME") : qputenv("XDG_CACHE_HOME", mSavedXdgCache);
    }

    void test_emptySoundFileMakesNoSoundTrigger()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(R"(tempComplexRegexTrigger("soundArgumentEmpty", "^sound argument empty$", function() end, 0, 0, 0, 0, 0, 0, 0, "", 0, 0))")));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("soundArgumentEmpty"));
        QVERIFY(pTrigger);
        QVERIFY2(!pTrigger->mSoundTrigger, "an empty sound file name should mean no sound, as a number or nil does");
        QTest::failOnWarning(scmMediaFetchWarning);
        feedLine(qsl("sound argument empty"));
    }

    void test_blankSoundFileMakesNoSoundTrigger()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(R"(tempComplexRegexTrigger("soundArgumentBlank", "^sound argument blank$", function() end, 0, 0, 0, 0, 0, 0, 0, "  ", 0, 0))")));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("soundArgumentBlank"));
        QVERIFY(pTrigger);
        QVERIFY2(!pTrigger->mSoundTrigger, "a sound file name of only spaces should mean no sound");
        QTest::failOnWarning(scmMediaFetchWarning);
        feedLine(qsl("sound argument blank"));
    }

    // The state a ticked sound box with an empty file field leaves, from the
    // trigger editor or a package
    void test_soundTriggerWithNoFilePlaysNothing()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(R"(tempComplexRegexTrigger("soundFlagNoFile", "^sound flag no file$", function() end, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))")));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("soundFlagNoFile"));
        QVERIFY(pTrigger);
        pTrigger->mSoundTrigger = true;
        pTrigger->setSound(QString());
        QTest::failOnWarning(scmMediaFetchWarning);
        feedLine(qsl("sound flag no file"));
    }

    // A fire whose captures reach Lua resets `matches` afterwards, so a later
    // trigger on the same line would find it emptied
    void test_soundTriggerWithNoFileLeavesMatchesAlone()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(R"(tempComplexRegexTrigger("soundFlagQuiet", "^sound flag quiet$", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))")));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("soundFlagQuiet"));
        QVERIFY(pTrigger);
        pTrigger->mSoundTrigger = true;
        pTrigger->setSound(QString());
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(R"(
            SoundQuietFires = {}
            tempLineTrigger(1, 3, function()
                matches.fires = (matches.fires or 0) + 1
                SoundQuietFires[#SoundQuietFires + 1] = matches.fires
            end)
            feedTriggers("\nsound quiet first\nsound flag quiet\nsound quiet third\n")
            SoundQuietResult = table.concat(SoundQuietFires, ",")
        )")));
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "SoundQuietResult");
        const QString result = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        QCOMPARE(result, qsl("1,2,3"));
    }

    void test_namedSoundFileMakesASoundTrigger()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(
                qsl(R"(tempComplexRegexTrigger("soundArgumentNamed", "^sound argument named$", function() end, 0, 0, 0, 0, 0, 0, 0, "ding.wav", 0, 0))")));
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger(qsl("soundArgumentNamed"));
        QVERIFY(pTrigger);
        QVERIFY(pTrigger->mSoundTrigger);
        QCOMPARE(pTrigger->mSoundFile, qsl("ding.wav"));
        // ding.wav is not in the profile's media directory and the trigger names
        // no URL for it, so only a fire that played it gets this warning
        QTest::ignoreMessage(QtWarningMsg, scmMediaFetchWarning);
        feedLine(qsl("sound argument named"));
    }

private:
    inline static const QRegularExpression scmMediaFetchWarning{qsl(R"(TMedia::isValidUrl\(\) WARNING)")};

    void feedLine(const QString& line) { QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(R"(feedTriggers("%1\n"))").arg(line))); }
};

#include "TriggerSoundArgumentTest.moc"
MUDLET_GROUPED_TEST_MAIN(TriggerSoundArgumentTest)
