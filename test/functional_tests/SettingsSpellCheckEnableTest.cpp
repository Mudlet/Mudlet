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
 * The system spell dictionary is read once the profile has finished loading,
 * and only for a profile that has spell check on. Turning spell check on in the
 * preferences is therefore another way the dictionary goes from unused to
 * used mid-session, and it has to be read then as well - otherwise the first
 * word typed pays for it.
 *
 * Run with: ctest -R SettingsSpellCheckEnableTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QSignalSpy>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Not the starting dictionary, so that reading it can only be down to the
// change made below. Hunspell_create() hands back a handle whether or not the
// files exist, so the load is logged on every platform.
static const QString scmDictionary = qsl("en_GB");

// loadSystemSpellDictionary() logs every read it makes, which is the only way to
// see one without asking for the handle - and asking for it would make the read.
static QtMessageHandler previousMessageHandler = nullptr;
static int dictionaryReads = 0;

static void countDictionaryReads(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (message.contains(qsl("System Hunspell dictionary \"%1\" loaded").arg(scmDictionary))) {
        ++dictionaryReads;
    }
    if (previousMessageHandler) {
        previousMessageHandler(type, context, message);
    }
}

class SettingsSpellCheckEnableTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsSpellCheckEnable-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort; // the stub's actual ephemeral port

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - see the same block in
        // DialogTeardownTest for why sharing the developer's one does not work
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestSettings::deleteProfileDirectory(mProfileName);

        previousMessageHandler = qInstallMessageHandler(countDictionaryReads);
        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        qInstallMessageHandler(previousMessageHandler);
        previousMessageHandler = nullptr;
        delete mpPreferences;
        mpPreferences = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_tickingSpellCheckReadsTheDictionary()
    {
        // Picking a dictionary queues a read too; the flag makes the slot skip it
        mpHost->setEnableSpellCheck(false);
        mpHost->setSpellDic(scmDictionary);
        QTest::qWait(TestSettings::scmQuietWindow);
        QCOMPARE(dictionaryReads, 0);

        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        QVERIFY2(mpPreferences->checkBox_spellCheck->isEnabled(), "the spell check box cannot be ticked");
        QVERIFY2(!mpPreferences->checkBox_spellCheck->isChecked(), "the spell check box does not show the profile's setting");

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_spellCheck->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the tick never reached the Host");
        QVERIFY2(mpHost->mEnableSpellCheck, "the tick did not turn spell check on");

        // Nothing here has asked for the handle, so a read now can only be the
        // one the tick queued
        QTRY_VERIFY2_WITH_TIMEOUT(dictionaryReads == 1, "turning spell check on left the dictionary unread, for the first word typed to pay for", 5000);
    }
};

#include "SettingsSpellCheckEnableTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsSpellCheckEnableTest)
