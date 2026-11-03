/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

// The Mudlet.ini values the main window reads through HostManager::readSettings() as it starts - the
// pool's mutes and two process-wide statics - and writes back as it closes. HeadlessHostSmokeTest covers
// the same values for a pool with no main window.

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "MudletMedia.h"
#include "PortableModeTestHelper.h"
#include "TMap.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MudletIniLateSettingsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    QString iniPath() const { return qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()); }

private slots:
    void initTestCase()
    {
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        // Each unlike its default, and in place before the main window reads anything
        QFile ini(iniPath());
        QVERIFY(ini.open(QIODevice::WriteOnly | QIODevice::Text));
        ini.write("[General]\n"
                  "enableMuteAPI=true\n"
                  "enableMuteGame=true\n"
                  "reportMapIssuesToConsole=true\n"
                  "storePasswordsSecurely=false\n");
        ini.close();
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getQSettings()->fileName(), iniPath());
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_theMainWindowReadsMudletIniThroughItsPool()
    {
        QCOMPARE(MudletApp::storingPasswordsSecurely(), false);
        QCOMPARE(HostManager::self()->media().apiMuted(), true);
        QCOMPARE(HostManager::self()->media().gameMuted(), true);
        QCOMPARE(TMap::smShowMapAuditErrors, true);
    }

    // What writeSettings() saves as Mudlet closes is what was read, so a value the main window failed
    // to read would overwrite the player's choice with the default
    void test_writingSettingsKeepsThePlayersChoices()
    {
        const QList<QPair<QString, bool>> expected{{qsl("enableMuteAPI"), true}, {qsl("enableMuteGame"), true}, {qsl("reportMapIssuesToConsole"), true}, {qsl("storePasswordsSecurely"), false}};
        // Gone first, so what is read back below can only be what writeSettings() put there
        QSettings* settings = MudletApp::getQSettings();
        QVERIFY(settings);
        QCOMPARE(settings->group(), QString());
        for (const auto& [key, value] : expected) {
            settings->remove(key);
        }
        mudlet::self()->writeSettings();
        QCOMPARE(settings->status(), QSettings::NoError);
        const QSettings written(iniPath(), QSettings::IniFormat);
        for (const auto& [key, value] : expected) {
            QVERIFY2(written.contains(key), qPrintable(key));
            QCOMPARE(written.value(key).toBool(), value);
        }
    }
};

#include "MudletIniLateSettingsTest.moc"
MUDLET_GROUPED_TEST_MAIN(MudletIniLateSettingsTest)
