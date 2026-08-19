/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * Regression test for profile data-loss after a crash during save.
 *
 * Profile saves go through QSaveFile: the data is written to a randomly named
 * temporary next to the target ("<name>.xml.AbCdEf") which is renamed over the
 * real file on commit. If Mudlet dies mid-save, that temporary is left behind,
 * empty, as the NEWEST file in the profile's current/ directory.
 *
 * mudlet::loadProfile() used to load the newest file of ANY name from
 * current/, so after such a crash it would "load" the empty leftover instead
 * of the newest real save: the profile opened with its connection settings
 * (stored in separate files) intact but every trigger/alias/script seemingly
 * wiped out. This test crashes a save in effigy - by planting an empty
 * QSaveFile-style leftover newer than a real save - and verifies the loader
 * skips it and restores the real data.
 *
 * Run with: ctest -R ProfileLoadTempFileTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
// A minimal but complete profile save holding one trigger - the "guts" whose
// survival the test asserts:
const QString scmTriggerName = qsl("synthetic data-loss canary");
const QString scmProfileXml = qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<TriggerPackage>
<Trigger isActive="yes" isFolder="no" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="no" isColorTriggerFg="no" isColorTriggerBg="no">
<name>synthetic data-loss canary</name>
<script>-- intentionally empty</script>
<triggerType>0</triggerType>
<conditonLineDelta>0</conditonLineDelta>
<mStayOpen>0</mStayOpen>
<mCommand></mCommand>
<packageName></packageName>
<mFgColor>#ff0000</mFgColor>
<mBgColor>#ffff00</mBgColor>
<mSoundFile></mSoundFile>
<colorTriggerFgColor>#000000</colorTriggerFgColor>
<colorTriggerBgColor>#000000</colorTriggerBgColor>
<regexCodeList>
<string>^synthetic pattern$</string>
</regexCodeList>
<regexCodePropertyList>
<integer>1</integer>
</regexCodePropertyList>
</Trigger>
</TriggerPackage>
</MudletPackage>
)");
} // namespace

class ProfileLoadTempFileTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("ProfileLoadTempFile-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool setModificationTime(const QString& path, const QDateTime& when)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite)) {
            return false;
        }
        return file.setFileTime(when, QFileDevice::FileModificationTime);
    }

private slots:
    void initTestCase()
    {
        // Keep the test hermetic: point the config dir resolution at a
        // temporary directory instead of the user's real profiles.
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        if (portableMarkerPresent()) {
            QSKIP("portable.txt marker present - config dir cannot be redirected for this test");
        }
        QVERIFY2(mudlet::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "test config dir redirection did not take effect");
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_loaderSkipsLeftoverSaveTemporary()
    {
        // 1. A real save, holding one trigger:
        const QString folder = mudlet::getMudletPath(enums::profileXmlFilesPath, mProfileName);
        QVERIFY(QDir().mkpath(folder));
        const QString xmlPath = qsl("%1/2020-01-01#00-00-00.xml").arg(folder);
        {
            QFile xmlFile(xmlPath);
            QVERIFY(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text));
            QVERIFY(xmlFile.write(scmProfileXml.toUtf8()) > 0);
        }

        // 2. What a crash mid-save leaves behind: an empty QSaveFile temporary
        //    that is the newest file in current/:
        const QString leftoverPath = qsl("%1/2020-01-02#00-00-00.xml.AbCdEf").arg(folder);
        {
            QFile leftover(leftoverPath);
            QVERIFY(leftover.open(QIODevice::WriteOnly));
        }
        const QDateTime now = QDateTime::currentDateTime();
        QVERIFY(setModificationTime(xmlPath, now.addSecs(-3600)));
        QVERIFY(setModificationTime(leftoverPath, now));

        // 3. Load the profile through the production loader; it must pick the
        //    real save, not the newer empty leftover:
        Host* pHost = mudlet::self()->loadProfile(mProfileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "loader tried to load a leftover QSaveFile temporary instead of the newest real save");
        QVERIFY2(pHost->getTriggerUnit()->findTrigger(scmTriggerName), "trigger from the real save is missing - the profile lost its data");
    }
};

#include "ProfileLoadTempFileTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileLoadTempFileTest)
