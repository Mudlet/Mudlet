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
 * Qt never says no to a font family that is not installed: it quietly
 * substitutes an arbitrary one, so a profile saved on a machine that had
 * "JetBrains Mono NL" opens on a machine that does not with its console drawn
 * in whatever the font database picked (issue #4159).
 *
 * The profile load path takes the family straight from the saved XML through
 * QFont::fromString() into Host::setDisplayFont(), which only rejects a font
 * whose glyphs have zero width - so the check has to happen afterwards, once
 * the fonts bundled inside installed packages are registered too. That is
 * Host::substituteMissingDisplayFont(), and this covers it and the shared
 * Host::resolveFontFamily() it is built on. There is no Lua entry point for
 * either, which is why this is a functional test rather than a spec.
 *
 * Run with: ctest -R MissingDisplayFontTest -V
 */

#include <QtTest/QtTest>

#include <QFont>
#include <QFontDatabase>
#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MissingDisplayFontTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("MissingDisplayFont-Test");
    // No font database anywhere lists this, so it is what an uninstalled family
    // looks like to Mudlet:
    const QString mMissingFamily = qsl("No Such Font At All");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    QList<int> mApplicationFontIds;

    // A real Mudlet run copies the bundled fonts out of the Qt resources into
    // the config directory on the way up (see main.cpp) and FontManager picks
    // them up from there; a test binary never runs that step, so register them
    // straight from the resources instead.
    void registerBundledFont(const QString& resourcePath)
    {
        const int id = QFontDatabase::addApplicationFont(resourcePath);
        QVERIFY2(id != -1, qPrintable(qsl("could not register the bundled font \"%1\"").arg(resourcePath)));
        mApplicationFontIds.append(id);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        registerBundledFont(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"));
        registerBundledFont(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf"));

        if (portableMarkerPresent()) {
            QSKIP("portable.txt marker present - config dir cannot be redirected for this test");
        }
        QVERIFY2(mudlet::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "test config dir redirection did not take effect");

        QVERIFY(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()));
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);

        QVERIFY2(mudlet::self()->getAvailableFonts().contains(Host::scmDefaultFontFamily, Qt::CaseInsensitive),
                 "the bundled default font is not registered, so this test cannot tell a fallback from a substitution");
        QVERIFY2(!mudlet::self()->getAvailableFonts().contains(mMissingFamily, Qt::CaseInsensitive), "the stand-in for an uninstalled font turns out to be installed");
    }

    void cleanupTestCase()
    {
        for (const int id : mApplicationFontIds) {
            QFontDatabase::removeApplicationFont(id);
        }
        mApplicationFontIds.clear();
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_resolveReportsAnInstalledFamilyAsItself()
    {
        const auto resolved = mpHost->resolveFontFamily(Host::scmDefaultFontFamily);
        QVERIFY(resolved.available);
        QCOMPARE(resolved.family, Host::scmDefaultFontFamily);
        QCOMPARE(resolved.weight, QFont::Normal);
    }

    void test_resolveReportsAnUninstalledFamilyAsUnavailable()
    {
        const auto resolved = mpHost->resolveFontFamily(mMissingFamily);
        QVERIFY(!resolved.available);
        // the name comes back untouched, so a caller can name it in its message
        QCOMPARE(resolved.family, mMissingFamily);
    }

    void test_resolveSplitsAStyleOffAnInstalledFamily()
    {
        const auto resolved = mpHost->resolveFontFamily(qsl("%1 Bold").arg(Host::scmDefaultFontFamily));
        QVERIFY(resolved.available);
        QCOMPARE(resolved.family, Host::scmDefaultFontFamily);
        QCOMPARE(resolved.weight, QFont::Bold);
    }

    void test_installedDisplayFontIsLeftAlone()
    {
        // setDisplayFont() takes the family unchecked, which is exactly what the
        // XML import does with what QFont::fromString() gave it
        QVERIFY(mpHost->setDisplayFont(QFont(Host::scmDefaultFontFamily, 13, QFont::Normal)).first);

        QVERIFY(!mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(mpHost->getDisplayFont().pointSize(), 13);
    }

    void test_missingDisplayFontFallsBackToTheBundledDefault()
    {
        QVERIFY(mpHost->setDisplayFont(QFont(mMissingFamily, 14, QFont::Normal)).first);

        QVERIFY(mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        // the rest of the saved font has nothing wrong with it, so it survives
        QCOMPARE(mpHost->getDisplayFont().pointSize(), 14);
    }

    void test_styleSuffixedDisplayFontKeepsItsBaseFamilyAndWeight()
    {
        QVERIFY(mpHost->setDisplayFont(QFont(qsl("%1 Bold").arg(Host::scmDefaultFontFamily), 12)).first);

        QVERIFY(mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(mpHost->getDisplayFont().weight(), QFont::Bold);
        QCOMPARE(mpHost->getDisplayFont().pointSize(), 12);
    }
};

#include "MissingDisplayFontTest.moc"
MUDLET_GROUPED_TEST_MAIN(MissingDisplayFontTest)
