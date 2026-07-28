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
 * Locks in the MUDLET_CONFIG_DIR contract used to isolate the Lua busted suite
 * (and any parallel run) from the real ~/.config/mudlet. The failure mode this
 * guards against is a future setupConfig() refactor silently dropping the
 * override, which resurfaces as hard-to-diagnose parallel-run sqlite flakiness.
 *
 * Only setupConfig()'s path resolution is exercised, so the mudlet singleton is
 * created but never init()'d.
 *
 * Run with: ctest -R ConfigDirOverrideTest -V
 */

#include <QtTest/QtTest>

#include "mudlet.h"

class ConfigDirOverrideTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { mudlet::start(); }

    void cleanup() { qunsetenv("MUDLET_CONFIG_DIR"); }

    // The singleton is intentionally not deleted: it was never init()'d, so its
    // destructor would touch members only set up by init(). The process exits
    // right after the suite, so leaking it is harmless.

    void test_overrideRelocatesAndCreatesConfigRoot()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        // A not-yet-existing subdirectory, so we also prove mkpath() runs.
        const QString target = QDir::cleanPath(tmp.path() + qsl("/isolated/config"));
        QVERIFY(!QDir(target).exists());

        qputenv("MUDLET_CONFIG_DIR", target.toUtf8());
        mudlet::self()->setupConfig();

        QCOMPARE(mudlet::getMudletPath(enums::mainPath), target);
        QCOMPARE(mudlet::getMudletPath(enums::profilesPath), target + qsl("/profiles"));
        QVERIFY2(QDir(target).exists(), "MUDLET_CONFIG_DIR should be created when missing");
    }

    // An empty value must be treated as unset, matching setupConfig()'s isEmpty()
    // gate, so a blank env var never silently redirects the config root.
    void test_emptyValueBehavesLikeUnset()
    {
        qputenv("MUDLET_CONFIG_DIR", QByteArray());
        mudlet::self()->setupConfig();
        const QString withEmpty = mudlet::getMudletPath(enums::mainPath);

        qunsetenv("MUDLET_CONFIG_DIR");
        mudlet::self()->setupConfig();
        const QString withUnset = mudlet::getMudletPath(enums::mainPath);

        QCOMPARE(withEmpty, withUnset);
    }

    // With the override unset (and no portable install), the config root is the
    // usual ~/.config/mudlet - i.e. normal users are unaffected.
    void test_unsetFallsBackToHomeConfigDir()
    {
        qunsetenv("MUDLET_CONFIG_DIR");
        mudlet::self()->setupConfig();

        const QString homeDefault = qsl("%1/.config/mudlet").arg(QDir::homePath());
        if (QFileInfo::exists(qsl("%1/portable.txt").arg(homeDefault))) {
            QSKIP("portable.txt present - config root is deliberately relocated");
        }
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), homeDefault);
    }
};

#include "ConfigDirOverrideTest.moc"
QTEST_MAIN(ConfigDirOverrideTest)
