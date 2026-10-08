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
 * MudletApp::writeProfileData() saves one value of a profile - its description,
 * its server, an IRC password - through a QSaveFile. A write that cannot land,
 * as on a full disk, must be reported as a failure and must leave the earlier
 * value in place. A spec cannot fill a disk, so this case stands in for one
 * with a file size limit on this process: past it write() fails with EFBIG,
 * just as it fails with ENOSPC on a full disk.
 *
 * Run with: ctest -R ProfileDataWriteTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QTemporaryDir>

#include "MudletApp.h"
#include "utils.h"

#if defined(Q_OS_UNIX)
#include <algorithm>
#include <csignal>
#include <sys/resource.h>
#endif

#include "GroupedTest.h"
#include "PortableModeTestHelper.h"

class ProfileDataWriteTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfile = qsl("ProfileDataWriteProfile");
    const QString mItem = qsl("description");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;
    bool mInitialised = false;

    QStringList filesForTheItem() const { return QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfile)).entryList({qsl("%1*").arg(mItem)}, QDir::Files | QDir::Hidden); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config root cannot be redirected away from the real one");
        }

        QVERIFY(mConfigDir.isValid());
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        mInitialised = true;
    }

    void cleanupTestCase()
    {
        if (mInitialised) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mProfile)).removeRecursively();
        }
        mSavedXdgConfigHome.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
    }

    void test_aWriteThatCannotLandIsReportedAndKeepsTheOldValue()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to make a write fail");
#else
        QVERIFY(MudletApp::writeProfileData(mProfile, mItem, qsl("kept")).first);

        rlimit saved{};
        QCOMPARE(::getrlimit(RLIMIT_FSIZE, &saved), 0);
        rlimit limited = saved;
        limited.rlim_cur = std::min<rlim_t>(64 * 1024, saved.rlim_max);
        // 2 MiB once written as UTF-16, well past the limit
        const QString tooBig(1024 * 1024, QLatin1Char('x'));

        // Past the limit the kernel also raises SIGXFSZ, whose default action ends the process
        const auto previousHandler = std::signal(SIGXFSZ, SIG_IGN);
        QCOMPARE(::setrlimit(RLIMIT_FSIZE, &limited), 0);
        const auto [ok, error] = MudletApp::writeProfileData(mProfile, mItem, tooBig);
        ::setrlimit(RLIMIT_FSIZE, &saved);
        std::signal(SIGXFSZ, previousHandler);

        QVERIFY2(!ok, "a write that could not land was reported as a success");
        QVERIFY2(!error.isEmpty(), "a failed write gave no reason");
        QCOMPARE(MudletApp::readProfileData(mProfile, mItem), qsl("kept"));
        // The QSaveFile's own temporary file goes with the failure
        QCOMPARE(filesForTheItem(), QStringList{mItem});
#endif
    }
};

#include "ProfileDataWriteTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileDataWriteTest)
