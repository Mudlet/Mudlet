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
 * A package's fonts are registered before its XML is imported, so that the
 * scripts in it - which run as they are read in - can already use a font the
 * package brought with it. That leaves one way out of installPackage() with
 * fonts registered and nothing installed to own them: an archive that carries
 * fonts but no package XML or .trigger that could be read is refused, its
 * unpacked folder is taken away again, and it is never entered into
 * mInstalledPackages - so uninstallPackage(), the only thing that calls
 * FontManager::unloadFonts(), can never be asked to take those fonts back out.
 * They would stay registered with the process for as long as it runs.
 *
 * The refusal path has to unload them itself, and that is what the first test
 * here pins. The second is the positive control: registering the fonts up front
 * still has to work for an archive that does install, and they still have to go
 * away when it is uninstalled. The third pins the reason the fonts go in first
 * at all - a script inside the package, running as the XML is read, can already
 * use the font its own archive carries. ErionMud-UI in the package repository
 * does exactly that with the Copperplate Gothic Light it ships.
 *
 * A test binary never runs the step in main.cpp that copies the bundled fonts
 * into the config directory, so nothing has registered "Bitstream Vera Sans
 * Mono" when this starts - which is what lets the assertions below read the
 * global font database directly.
 *
 * Run with: ctest -R PackageFontsOnRefusedArchiveTest -V
 */

#include <QtTest/QtTest>

#include <QFontDatabase>
#include <QTemporaryDir>
#include <zip.h>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TLuaInterpreter.h"
#include "mudlet.h"

#include "GroupedTest.h"

class PackageFontsOnRefusedArchiveTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("PackageFontsOnRefusedArchive-Test");
    // The family the bundled VeraMono.ttf declares - what the test looks for in
    // the process-wide font database.
    const QString mFontFamily = qsl("Bitstream Vera Sans Mono");
    QTemporaryDir mConfigDir;
    QTemporaryDir mArchiveDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

    // GroupedTestMain.cpp registers the Qt font resources, so the bundled font
    // is readable here - and reading it out is how the test archives get a real
    // font file to carry without one having to live in the repository.
    static QByteArray bundledFontBytes()
    {
        QFile font(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"));
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

    // A package holding one script that answers, as it is read in, whether the
    // family the archive carries is already usable.
    static QByteArray fontProbePackageXml(const QString& packageName, const QString& fontFamily)
    {
        return qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<!DOCTYPE MudletPackage>\n"
                   "<MudletPackage version=\"1.001\">\n"
                   "<ScriptPackage>\n"
                   "<Script isActive=\"yes\" isFolder=\"no\">\n"
                   "<name>%1 font probe</name>\n"
                   "<packageName></packageName>\n"
                   "<script>packageFontSeenAtInstall = (getAvailableFonts()[\"%2\"] == true)</script>\n"
                   "<eventHandlerList />\n"
                   "</Script>\n"
                   "</ScriptPackage>\n"
                   "</MudletPackage>\n")
                .arg(packageName, fontFamily)
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

    QString packageFolder(const QString& packageName) const { return MudletApp::getMudletPath(enums::profilePackagePath, mProfileName, packageName); }

private slots:
    void initTestCase()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so there is no bundled font in the resources to build the test packages out of");
#else
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // Keep the test hermetic: point the config dir resolution at a temporary
        // directory instead of the user's real profiles - the packages below are
        // unpacked into, and deleted from, whichever one this resolves to.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mArchiveDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(MudletApp::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "test config dir redirection did not take effect");

        QVERIFY2(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()), "failed to create the Host");
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);

        QVERIFY2(!bundledFontBytes().isEmpty(), "the bundled font could not be read out of the Qt resources");
        // Without this the tests below could pass on a font that was already
        // there rather than on one a package brought:
        if (QFontDatabase::families().contains(mFontFamily)) {
            QSKIP("the font this test installs is already registered on this machine, so a package bringing it cannot be told apart");
        }
#endif
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // An archive holding fonts but nothing installable is refused - and the
    // fonts it had registered on the way in have to go with it.
    void test_fontsFromARefusedArchiveAreUnloaded()
    {
        const QString packageName = qsl("font-only");
        const QString archivePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(packageName));
        const QList<std::pair<QString, QByteArray>> entries{{qsl("VeraMono.ttf"), bundledFontBytes()}, {qsl("readme.txt"), QByteArray("fonts, and nothing Mudlet can install")}};
        QVERIFY2(writeArchive(archivePath, entries), "could not write the test archive");

        auto [ok, message] = mpHost->installPackage(archivePath, enums::PackageModuleType::Package, true);
        QVERIFY2(!ok, "an archive holding no package was installed");
        QVERIFY2(message.contains(qsl("no package found in")), qPrintable(qsl("the archive was refused for another reason: %1").arg(message)));

        QVERIFY2(!QFontDatabase::families().contains(mFontFamily), "the refused package's fonts are still registered with no installed package that could unload them");
        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "the refused package was registered as installed");
        QVERIFY2(!QDir(packageFolder(packageName)).exists(), "the refused package's folder was left behind");
    }

    // ...while an archive that does install keeps its fonts for as long as it is
    // installed, which is what registering them up front is for.
    void test_fontsFromAnInstalledPackageStayRegisteredUntilUninstall()
    {
        const QString packageName = qsl("font-pkg");
        const QString archivePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(packageName));
        const QList<std::pair<QString, QByteArray>> entries{{qsl("VeraMono.ttf"), bundledFontBytes()}, {qsl("%1.xml").arg(packageName), minimalPackageXml(packageName)}};
        QVERIFY2(writeArchive(archivePath, entries), "could not write the test archive");

        auto [ok, message] = mpHost->installPackage(archivePath, enums::PackageModuleType::Package, true);
        QVERIFY2(ok, qPrintable(message));
        QVERIFY2(mpHost->mInstalledPackages.contains(packageName), "the package was not registered as installed");
        QVERIFY2(QFontDatabase::families().contains(mFontFamily), "the installed package's font was not registered");

        QVERIFY2(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package), "the package could not be uninstalled");
        QVERIFY2(!QFontDatabase::families().contains(mFontFamily), "uninstalling the package left its font registered");
    }

    // ...and the reason they are registered before the import rather than after
    // it: the package's own scripts run as the XML is read in, and one that asks
    // for the font its archive carries has to find it already there.
    void test_packageScriptsCanUseTheBundledFontWhileInstalling()
    {
        const QString packageName = qsl("font-probe-pkg");
        const QString archivePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(packageName));
        const QList<std::pair<QString, QByteArray>> entries{{qsl("VeraMono.ttf"), bundledFontBytes()}, {qsl("%1.xml").arg(packageName), fontProbePackageXml(packageName, mFontFamily)}};
        QVERIFY2(writeArchive(archivePath, entries), "could not write the test archive");

        // A Host starts with script compilation blocked and a real profile load
        // turns it off before anything is imported, so do the same here - without
        // it the script would be stored but never run
        mpHost->mBlockScriptCompile = false;
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        QVERIFY(L);
        lua_pushnil(L);
        lua_setglobal(L, "packageFontSeenAtInstall");

        auto [ok, message] = mpHost->installPackage(archivePath, enums::PackageModuleType::Package, true);
        QVERIFY2(ok, qPrintable(message));

        lua_getglobal(L, "packageFontSeenAtInstall");
        const bool probeRan = !lua_isnil(L, -1);
        const bool sawTheFont = lua_toboolean(L, -1);
        lua_pop(L, 1);
        QVERIFY2(probeRan, "the package's script never ran, so this says nothing about what it could see");
        QVERIFY2(sawTheFont, "a script in the package could not use the font its own archive carries while it was being installed");

        QVERIFY2(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package), "the package could not be uninstalled");
    }
};

#include "PackageFontsOnRefusedArchiveTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageFontsOnRefusedArchiveTest)
