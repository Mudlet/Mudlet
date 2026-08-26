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
 * the fonts bundled inside installed packages and modules are registered too.
 * That is Host::substituteMissingDisplayFont(), and this covers it and the
 * shared Host::resolveFontFamily() it is built on. There is no Lua entry point
 * for either, which is why this is a functional test rather than a spec.
 *
 * The first cases exercise those two directly; the last two go through the
 * production load path instead - mudlet::loadProfile() followed by
 * slot_connectionDialogueFinished(), which is exactly what Lua's loadProfile()
 * does - because where the check is made from decides both whether the player
 * ever sees the warning and whether a font a module supplies is mistaken for a
 * missing one.
 *
 * Run with: ctest -R MissingDisplayFontTest -V
 */

#include <QtTest/QtTest>

#include <QFont>
#include <QFontDatabase>
#include <QTemporaryDir>
#include <zip.h>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TMainConsole.h"
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
    // The cases that stand for a machine without Mudlet's own fonts: one watches
    // a family arrive with a module and nothing else, the other an installation
    // where the bundled default failed to register. Every other case needs them.
    const QStringList mCasesWithoutBundledFonts{qsl("test_aDisplayFontAModuleSuppliesIsNotJudgedMissing"), qsl("test_theBundledDefaultItselfIsNeverSubstituted")};
    // A second family Mudlet bundles, so a case can pick another installed font -
    // or have a module bring one - without depending on what this machine has:
    const QString mOtherBundledFamily = qsl("Ubuntu Mono");
    QTemporaryDir mConfigDir;
    QTemporaryDir mSaveDir;
    QTemporaryDir mArchiveDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    QList<int> mApplicationFontIds;

    // A real Mudlet run copies the bundled fonts out of the Qt resources into
    // the config directory on the way up (see main.cpp) and FontManager picks
    // them up from there; a test binary never runs that step, so register them
    // straight from the resources instead.
    void registerBundledFonts()
    {
        for (const QString& resourcePath :
             {qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf"), qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf")}) {
            const int id = QFontDatabase::addApplicationFont(resourcePath);
            QVERIFY2(id != -1, qPrintable(qsl("could not register the bundled font \"%1\"").arg(resourcePath)));
            mApplicationFontIds.append(id);
        }
        QVERIFY2(mudlet::self()->getAvailableFonts().contains(Host::scmDefaultFontFamily, Qt::CaseInsensitive),
                 "the bundled default font is not registered, so this test cannot tell a fallback from a substitution");
        QVERIFY2(mudlet::self()->getAvailableFonts().contains(mOtherBundledFamily, Qt::CaseInsensitive), "the second bundled font is not registered");
    }

    void unregisterBundledFonts()
    {
        for (const int id : mApplicationFontIds) {
            QFontDatabase::removeApplicationFont(id);
        }
        mApplicationFontIds.clear();
    }

    static QByteArray bundledFontBytes(const QString& resourcePath)
    {
        QFile font(resourcePath);
        if (!font.open(QIODevice::ReadOnly)) {
            return QByteArray();
        }
        return font.readAll();
    }

    static QByteArray minimalPackageXml(const QString& packageName)
    {
        return qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<!DOCTYPE MudletPackage>\n"
                   "<MudletPackage version=\"1.001\">\n"
                   "<AliasPackage>\n"
                   "<Alias isActive=\"yes\" isFolder=\"no\">\n"
                   "<name>%1 alias</name>\n"
                   "<script>send(\"hello\")</script>\n"
                   "<command></command>\n"
                   "<packageName></packageName>\n"
                   "<regex>^%1$</regex>\n"
                   "</Alias>\n"
                   "</AliasPackage>\n"
                   "</MudletPackage>\n")
                .arg(packageName)
                .toUtf8();
    }

    // libzip reads each source buffer at zip_close() time rather than when it is
    // added, so the contents have to outlive the loop - which is why the entries
    // come in as a list the caller owns.
    static bool writeArchive(const QString& path, const QList<std::pair<QString, QByteArray>>& entries)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        for (const auto& [entryName, contents] : entries) {
            zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
            if (!source || zip_file_add(archive, entryName.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
                zip_source_free(source);
                zip_discard(archive);
                return false;
            }
        }
        return zip_close(archive) == 0;
    }

    // The smallest profile save that names a display font, and optionally a
    // module to install: XMLimport::readHost() defaults every attribute it does
    // not find and skips the children it does not know, so nothing else is
    // needed to reach Host::setDisplayFontFromString().
    bool writeProfileSave(const QString& profileName, const QString& fontFamily, const QString& moduleName = QString(), const QString& modulePath = QString())
    {
        const QString folder = mudlet::getMudletPath(enums::profileXmlFilesPath, profileName);
        if (!QDir().mkpath(folder)) {
            return false;
        }
        QString modules;
        if (!moduleName.isEmpty()) {
            // The shape XMLexport::writeHost() gives an archive-backed module,
            // which is what XMLimport::readModulesDetailsMap() expects to find
            modules = qsl("<mInstalledModules>\n"
                          "<key>%1</key>\n"
                          "<filepath>%2</filepath>\n"
                          "<zipSync>0</zipSync>\n"
                          "<globalSave>0</globalSave>\n"
                          "<priority>0</priority>\n"
                          "</mInstalledModules>\n")
                              .arg(moduleName, modulePath);
        }
        const QString xml = qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                "<!DOCTYPE MudletPackage>\n"
                                "<MudletPackage version=\"1.001\">\n"
                                "<HostPackage>\n"
                                "<Host>\n"
                                "%1"
                                "<mDisplayFont>%2</mDisplayFont>\n"
                                "</Host>\n"
                                "</HostPackage>\n"
                                "</MudletPackage>\n")
                                    .arg(modules, QFont(fontFamily, 14).toString());
        QFile file(qsl("%1/2020-01-01#00-00-00.xml").arg(folder));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        return file.write(xml.toUtf8()) > 0;
    }

    // What the player would see: the console wraps a long message over several
    // buffer lines, so the text is put back together before it is searched.
    static QString consoleText(Host* pHost) { return pHost->mpConsole->buffer.lineBuffer.join(QChar::Space).simplified(); }

    // The family the saved profile asks for, straight out of the written XML
    static QString savedDisplayFontFamily(const QString& xmlPath)
    {
        QFile file(xmlPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        const QString contents = QString::fromUtf8(file.readAll());
        const QString openingTag = qsl("<mDisplayFont>");
        const int start = contents.indexOf(openingTag);
        const int end = contents.indexOf(qsl("</mDisplayFont>"), start);
        if (start < 0 || end < 0) {
            return QString();
        }
        const int from = start + openingTag.size();
        // QFont::toString() puts the family first, then the size and the rest
        return contents.mid(from, end - from).section(QChar::fromLatin1(','), 0, 0);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mSaveDir.isValid());
        QVERIFY(mArchiveDir.isValid());
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

        QVERIFY(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()));
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);

        QVERIFY2(!mudlet::self()->getAvailableFonts().contains(mMissingFamily, Qt::CaseInsensitive), "the stand-in for an uninstalled font turns out to be installed");
    }

    // Registered per case rather than once for the class, because a couple of
    // cases have to start with the bundled fonts absent.
    void init()
    {
        if (!mCasesWithoutBundledFonts.contains(QString::fromUtf8(QTest::currentTestFunction()))) {
            registerBundledFonts();
        }
    }

    void cleanup()
    {
        // Cases share one Host: a stand-in left behind by one case must not
        // leak into the next, and only a user-choice change retires it now
        mpHost->setDisplayFont(mpHost->getDisplayFont(), Host::DisplayFontChange::UserChoice);
        unregisterBundledFonts();
    }

    void cleanupTestCase()
    {
        unregisterBundledFonts();
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

    // The bundled default is the one family with nothing to stand in for it: on
    // an installation where it failed to register, a profile asking for it is
    // left alone rather than told "missing, using itself instead", and nothing is
    // remembered as missing.
    void test_theBundledDefaultItselfIsNeverSubstituted()
    {
        if (mudlet::self()->getAvailableFonts().contains(Host::scmDefaultFontFamily, Qt::CaseInsensitive)) {
            QSKIP("the bundled default is installed on this machine, so a broken installation cannot be staged");
        }
        QVERIFY(mpHost->setDisplayFont(QFont(Host::scmDefaultFontFamily, 14, QFont::Normal)).first);

        QVERIFY(!mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), Host::scmDefaultFontFamily);
    }

    // The other half of keeping the player's choice: the family the profile asks
    // for has to be forgotten the moment they really do pick another one, or the
    // profile would go on saving a font nobody asks for any more.
    void test_theFamilyAProfileAsksForIsOnlyKeptUntilAnotherOneIsChosen()
    {
        QVERIFY(mpHost->setDisplayFont(QFont(mMissingFamily, 14, QFont::Normal)).first);
        QVERIFY(mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);

        // What the preferences dialog sends when only the size changed: the family
        // it is showing, which is the stand-in rather than what was asked for
        QFont resized = mpHost->getDisplayFont();
        resized.setPointSize(11);
        QVERIFY(mpHost->setDisplayFont(resized).first);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);
        QCOMPARE(mpHost->getDisplayFontForSaving().pointSize(), 11);

        // ...while another family really is a new choice
        QVERIFY(mpHost->setDisplayFont(QFont(mOtherBundledFamily, 11, QFont::Normal), Host::DisplayFontChange::UserChoice).first);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mOtherBundledFamily);
    }

    // The family the stand-in is drawn in is a choice like any other: picking it
    // has to retire the stand-in, or the profile would go on asking for the font
    // this machine lacks even after the player settled for what they can see.
    void test_choosingTheVeryFamilyTheStandInUsesRetiresIt()
    {
        QVERIFY(mpHost->setDisplayFont(QFont(mMissingFamily, 14, QFont::Normal)).first);
        QVERIFY(mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);

        QVERIFY(mpHost->setDisplayFont(QFont(Host::scmDefaultFontFamily, 14, QFont::Normal), Host::DisplayFontChange::UserChoice).first);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), Host::scmDefaultFontFamily);
    }

    // The whole point of the check, over the production load path: the console
    // is drawn in a font that is really there, the player is told why, and the
    // profile goes on asking for the font they chose - a stand-in for a font
    // this machine happens to lack must not be saved as their choice, or one
    // opening on a borrowed machine would lose it everywhere.
    void test_aProfileNamingAnUninstalledFontLoadsOnTheDefaultAndKeepsAskingForItsOwn()
    {
        const QString profileName = qsl("MissingDisplayFont-Load-Test");
        QVERIFY2(writeProfileSave(profileName, mMissingFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        // What Lua's loadProfile() does next, and what the connection dialog does
        // for a profile the player opens:
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mpConsole, "the profile came up without a main console");

        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);

        const QString shown = consoleText(pHost);
        QVERIFY2(shown.contains(qsl("[ WARN ]")), qPrintable(qsl("no warning reached the main console; it holds: %1").arg(shown)));
        QVERIFY2(shown.contains(mMissingFamily), qPrintable(qsl("the warning does not name the missing font; the console holds: %1").arg(shown)));

        auto [saved, xmlPath, saveError] = pHost->saveProfile(mSaveDir.path(), qsl("fontcheck"));
        QVERIFY2(saved, qPrintable(saveError));
        pHost->waitForProfileSave();
        QVERIFY2(QFileInfo::exists(xmlPath), qPrintable(qsl("profile XML was not written to %1").arg(xmlPath)));
        QCOMPARE(savedDisplayFontFamily(xmlPath), mMissingFamily);
    }

    // A module's fonts are only registered once the modules are installed, which
    // happens well after the profile XML is read - so a check made too early
    // calls a font the profile perfectly well has missing, and swaps it out.
    void test_aDisplayFontAModuleSuppliesIsNotJudgedMissing()
    {
        const QString moduleName = qsl("font-module");
        const QString modulePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(moduleName));
        // A family other than the fallback, so a check made before the module
        // installs shows up as the console landing on the fallback - not only as
        // a message
        const QByteArray fontBytes = bundledFontBytes(qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf"));
        QVERIFY2(!fontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources");
        const QList<std::pair<QString, QByteArray>> entries{{qsl("UbuntuMono-R.ttf"), fontBytes}, {qsl("%1.xml").arg(moduleName), minimalPackageXml(moduleName)}};
        QVERIFY2(writeArchive(modulePath, entries), "could not write the test module archive");
        if (mudlet::self()->getAvailableFonts().contains(mOtherBundledFamily, Qt::CaseInsensitive)) {
            QSKIP("the family the module supplies is already installed on this machine, so this cannot tell whether the module was waited for");
        }

        const QString profileName = qsl("MissingDisplayFont-Module-Test");
        QVERIFY2(writeProfileSave(profileName, mOtherBundledFamily, moduleName, modulePath), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mpConsole, "the profile came up without a main console");

        QVERIFY2(mudlet::self()->getAvailableFonts().contains(mOtherBundledFamily, Qt::CaseInsensitive), "the module's font was never registered, so the profile's font really was missing");
        QCOMPARE(pHost->getDisplayFont().family(), mOtherBundledFamily);
        const QString shown = consoleText(pHost);
        QVERIFY2(!shown.contains(qsl("is not installed on this computer")), qPrintable(qsl("a font the module supplies was reported missing; the console holds: %1").arg(shown)));
    }
};

#include "MissingDisplayFontTest.moc"
MUDLET_GROUPED_TEST_MAIN(MissingDisplayFontTest)
