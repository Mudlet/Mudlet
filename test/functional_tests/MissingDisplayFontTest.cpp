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
 * QFont::fromString() into Host::setDisplayFont(), which turns away only a
 * description it cannot read and a font whose glyphs have zero width - so the
 * check on the family itself has to happen afterwards, once
 * the fonts bundled inside installed packages and modules are registered too.
 * That is Host::substituteMissingDisplayFont(), and this covers it and the
 * shared Host::resolveFontFamily() it is built on. There is no Lua entry point
 * for either, which is why this is a functional test rather than a spec.
 *
 * The first cases exercise those two directly; the closing ones go through the
 * production load path instead - mudlet::loadProfile() followed by
 * slot_connectionDialogueFinished(), which is exactly what Lua's loadProfile()
 * does - because where the check is made from decides whether the player ever
 * sees the warning, whether a font a module supplies is mistaken for a missing
 * one, and whether one that leaves with an uninstalled package is noticed at
 * all.
 *
 * Run with: ctest -R MissingDisplayFontTest -V
 */

#include <QtTest/QtTest>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QFont>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFontInfo>
#include <QPointer>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QUuid>
#include <memory>
#include <zip.h>

#include "FontManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAlias.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextBox.h"
#include "dlgProfilePreferences.h"
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
    // The name a bundled font is renamed to so a package can be the only place it
    // comes from: unlike a family Mudlet bundles, taking the package away really
    // does take this one off the machine.
    const QString mPackageSuppliedFamily = qsl("Zqxwvu Package Font Mono");
    // The same for a module, so neither case can be fed by what the other left registered
    const QString mModuleSuppliedFamily = qsl("Zqxwvu Modules Font Mono");
    // A font only a module's next version brings
    const QString mUpdatedModuleFamily = qsl("Zqxwvu Updated Font Mono");
    // A module's next version that ships another font in a file of the same name
    const QString mRewrittenModuleFamily = qsl("Zqxwvu Rewrite Font Mono");
    const QString mRenamedModuleFamily = qsl("Zqxwvu Renamed Font Mono");
    // A module synced in from another profile
    const QString mSyncedModuleFamily = qsl("Zqxwvu Synced1 Font Mono");
    // Stands for a name no font database lists but the platform resolves anyway, like
    // the fontconfig alias "Helvetica". Which names those are is up to the machine, so
    // the cases make one of their own out of Qt's substitution table, which every
    // platform consults.
    const QString mAliasFamily = qsl("Zqxwvu Alias Font Mono");
    QTemporaryDir mConfigDir;
    QTemporaryDir mSaveDir;
    QTemporaryDir mArchiveDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    QList<int> mApplicationFontIds;
    // The profile and package of every font a case had a package register, unloaded by cleanup()
    // so that a case failing part way does not leave the family installed for the cases after it
    QList<std::pair<QString, QString>> mFontsToUnload;
    // The profiles a case gave Lua event handlers, which cleanup() kills for the same reason
    QList<QPointer<Host>> mHostsWithHandlers;

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
        QVERIFY2(FontManager::availableFonts().contains(Host::scmDefaultFontFamily, Qt::CaseInsensitive),
                 "the bundled default font is not registered, so this test cannot tell a fallback from a substitution");
        QVERIFY2(FontManager::availableFonts().contains(mOtherBundledFamily, Qt::CaseInsensitive), "the second bundled font is not registered");
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

    // A copy of a bundled font under a family name no font database anywhere
    // lists. The name records are overwritten in place with a replacement of the
    // same length, so every offset in the name table stays valid - and neither
    // FreeType nor Qt checks the table checksums. Both the ASCII and the UTF-16BE
    // record have to be caught, since a TrueType name table carries the family
    // under each encoding.
    static QByteArray renamedFontBytes(const QString& resourcePath, const QString& fromFamily, const QString& toFamily)
    {
        QByteArray bytes = bundledFontBytes(resourcePath);
        if (bytes.isEmpty() || fromFamily.size() != toFamily.size()) {
            return QByteArray();
        }
        const auto asUtf16Be = [](const QString& text) {
            QByteArray encoded;
            encoded.reserve(text.size() * 2);
            for (const QChar character : text) {
                encoded.append(static_cast<char>(character.unicode() >> 8));
                encoded.append(static_cast<char>(character.unicode() & 0xFF));
            }
            return encoded;
        };
        // The PostScript name is the family with the spaces squeezed out, and it
        // has to change too: CoreText refuses a font carrying a PostScript name
        // it has already registered, which would leave this one unavailable on
        // macOS while the original it was copied from is loaded.
        const QString fromPostScriptName = QString(fromFamily).remove(QChar::Space);
        const QString toPostScriptName = QString(toFamily).remove(QChar::Space);
        if (fromPostScriptName.size() != toPostScriptName.size()) {
            return QByteArray();
        }
        bytes.replace(fromFamily.toLatin1(), toFamily.toLatin1());
        bytes.replace(asUtf16Be(fromFamily), asUtf16Be(toFamily));
        bytes.replace(fromPostScriptName.toLatin1(), toPostScriptName.toLatin1());
        bytes.replace(asUtf16Be(fromPostScriptName), asUtf16Be(toPostScriptName));
        return bytes;
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
    // <mDisplayFont> is written by XMLexport::writeHost(), and that runs only for
    // a profile's own save - no package or module Mudlet exports carries one. So
    // this is both what a profile save looks like and the only thing that can
    // bring a display font in through an install.
    bool writeHostPackageXml(const QString& path, const QFont& font)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        const QString xml = qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                "<!DOCTYPE MudletPackage>\n"
                                "<MudletPackage version=\"1.001\">\n"
                                "<HostPackage>\n"
                                "<Host>\n"
                                "<mDisplayFont>%1</mDisplayFont>\n"
                                "</Host>\n"
                                "</HostPackage>\n"
                                "</MudletPackage>\n")
                                    .arg(font.toString());
        return file.write(xml.toUtf8()) != -1;
    }

    bool writeProfileSave(const QString& profileName, const QString& fontFamily, const QString& moduleName = QString(), const QString& modulePath = QString(), const QString& packageName = QString(), const int pointSize = 14)
    {
        const QString folder = MudletApp::getMudletPath(enums::profileXmlFilesPath, profileName);
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
        if (!packageName.isEmpty()) {
            modules.append(qsl("<mInstalledPackages>\n"
                               "<string>%1</string>\n"
                               "</mInstalledPackages>\n")
                                   .arg(packageName));
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
                                    .arg(modules, QFont(fontFamily, pointSize).toString());
        QFile file(qsl("%1/2020-01-01#00-00-00.xml").arg(folder));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        return file.write(xml.toUtf8()) > 0;
    }

    // A package carrying mPackageSuppliedFamily, which no font database lists until it installs
    QString writeFontPackage(const QString& packageName, QByteArray fontBytes = QByteArray())
    {
        if (fontBytes.isEmpty()) {
            fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily);
        }
        if (fontBytes.isEmpty()) {
            return QString();
        }
        const QString packagePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(packageName));
        const QList<std::pair<QString, QByteArray>> entries{{qsl("%1.ttf").arg(packageName), fontBytes}, {qsl("%1.xml").arg(packageName), minimalPackageXml(packageName)}};
        return writeArchive(packagePath, entries) ? packagePath : QString();
    }

    // What Lua's loadProfile() does, and what the connection dialog does for a profile the player opens
    Host* openProfile(const QString& profileName, const QString& fontFamily, const bool writeSave = true)
    {
        if (writeSave && !writeProfileSave(profileName, fontFamily)) {
            return nullptr;
        }
        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        if (!pHost || !pHost->mLoadedOk) {
            return nullptr;
        }
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        return pHost->mainConsoleView() ? pHost : nullptr;
    }

    // A missing family is put back on a later pass of the event loop, behind the install events
    static void runQueuedEvents()
    {
        for (int pass = 0; pass < 3; ++pass) {
            QCoreApplication::processEvents();
        }
    }

    bool installFontPackage(Host* pHost, const QString& packagePath, const QString& packageName)
    {
        mFontsToUnload.append({pHost->getName(), packageName});
        const bool installed = pHost->installPackage(packagePath, enums::PackageModuleType::Package).first;
        // installPackage() defers an install for as long as a save is in flight
        pHost->waitForProfileSave();
        runQueuedEvents();
        return installed;
    }

    static bool uninstallFontPackage(Host* pHost, const QString& packageName)
    {
        // uninstallPackage() refuses while a save runs, and an install or an uninstall queues one
        pHost->waitForProfileSave();
        const bool uninstalled = pHost->uninstallPackage(packageName, enums::PackageModuleType::Package);
        pHost->waitForProfileSave();
        runQueuedEvents();
        return uninstalled;
    }

    // Handlers a case registers go in fontHandlerTestIds, for killLuaHandlers() to remove
    // Callers wrap it in QVERIFY: a QVERIFY in here would return from this helper alone and leave the case running
    [[nodiscard]] bool runLua(Host* pHost, const QString& code)
    {
        if (!mHostsWithHandlers.contains(pHost)) {
            mHostsWithHandlers.append(pHost);
            pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("fontHandlerTestIds = {}"));
        }
        if (!pHost->getLuaInterpreter()->compileAndExecuteScript(code)) {
            qWarning().noquote() << "the Lua did not run:" << code;
            return false;
        }
        runQueuedEvents();
        return true;
    }

    static QString luaString(Host* pHost, const QString& expression)
    {
        pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("fontTestResult = %1").arg(expression));
        return pHost->getLuaInterpreter()->getLuaString(qsl("fontTestResult"));
    }

    static void killLuaHandlers(Host* pHost)
    {
        pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("for _, id in ipairs(fontHandlerTestIds or {}) do killAnonymousEventHandler(id) end fontHandlerTestIds = {}"));
    }

    static QString restoredMessage(const QString& family) { return qsl("The font \"%1\" that this profile uses is now available").arg(family); }

    static quint16 bigEndian16(const QByteArray& bytes, const int at) { return static_cast<quint16>((static_cast<quint8>(bytes.at(at)) << 8) | static_cast<quint8>(bytes.at(at + 1))); }

    static quint32 bigEndian32(const QByteArray& bytes, const int at) { return (static_cast<quint32>(bigEndian16(bytes, at)) << 16) | bigEndian16(bytes, at + 2); }

    // A font Qt registers but Host::setDisplayFont() turns away, its letters having no width: the
    // average width in the OS/2 table and every advance in the horizontal metrics are zeroed
    static QByteArray zeroWidthFont(QByteArray bytes)
    {
        if (bytes.size() < 12) {
            return QByteArray();
        }
        int os2 = -1;
        int hmtx = -1;
        int hhea = -1;
        const int tableCount = bigEndian16(bytes, 4);
        for (int table = 0; table < tableCount; ++table) {
            const int record = 12 + 16 * table;
            const QByteArray tag = bytes.mid(record, 4);
            const int offset = static_cast<int>(bigEndian32(bytes, record + 8));
            if (tag == "OS/2") {
                os2 = offset;
            } else if (tag == "hmtx") {
                hmtx = offset;
            } else if (tag == "hhea") {
                hhea = offset;
            }
        }
        if (os2 < 0 || hmtx < 0 || hhea < 0) {
            return QByteArray();
        }
        bytes[os2 + 2] = 0;
        bytes[os2 + 3] = 0;
        // advanceWidthMax
        bytes[hhea + 10] = 0;
        bytes[hhea + 11] = 0;
        const int metricCount = bigEndian16(bytes, hhea + 34);
        for (int metric = 0; metric < metricCount; ++metric) {
            bytes[hmtx + 4 * metric] = 0;
            bytes[hmtx + 4 * metric + 1] = 0;
        }
        return bytes;
    }

    // What the player would see: the console wraps a long message over several
    // buffer lines, so the text is put back together before it is searched.
    static QString consoleText(Host* pHost) { return pHost->mainConsoleView()->buffer.lineBuffer.join(QChar::Space).simplified(); }

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

    // Makes mAliasFamily into a name this machine resolves for itself, and hands back the
    // family it was pointed at - empty on a platform that cannot show the difference at all.
    // That family is never this machine's own stand-in for a name nothing knows, which the
    // cases below have to be able to tell a resolved alias apart from; since the stand-in
    // can itself be the bundled default, which of the two families this test registers is
    // used has to be decided here rather than fixed.
    QString stageAliasFamily()
    {
        // Asked afresh of a name nothing can have, so that what comes back is this
        // machine's stand-in for an unknown family rather than a font it really has
        const QString unknownName = QUuid::createUuid().toString();
        const QString standIn = QFontInfo(QFont(unknownName)).family();

        if (standIn == unknownName) {
            return QString();
        }

        const QString target = (standIn == Host::scmDefaultFontFamily) ? mOtherBundledFamily : Host::scmDefaultFontFamily;
        QFont::insertSubstitution(mAliasFamily, target);
        return target;
    }

    // Picks a row of the combo box's popup list the way a mouse does
    static void clickPopupRow(QFontComboBox* pFamilies, const int row)
    {
        pFamilies->showPopup();
        QAbstractItemView* pList = pFamilies->view();
        const QModelIndex index = pList->model()->index(row, 0);
        pList->scrollTo(index);
        const QPoint at = pList->visualRect(index).center();
        QTest::mouseMove(pList->viewport(), at);
        QTest::mouseClick(pList->viewport(), Qt::LeftButton, {}, at);
    }

private slots:
    void initTestCase()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so the bundled fonts this registers - the family it falls back to, and the one it renames for a package to supply - are not in the resources");
#else
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
        QVERIFY2(MudletApp::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "test config dir redirection did not take effect");

        QVERIFY(HostManager::self()->addHost(mProfileName, QString(), QString(), QString()));
        mpHost = HostManager::self()->getHost(mProfileName);
        QVERIFY(mpHost);

        QVERIFY2(!FontManager::availableFonts().contains(mMissingFamily, Qt::CaseInsensitive), "the stand-in for an uninstalled font turns out to be installed");
#endif
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
        QFont::removeSubstitutions(mAliasFamily);
        for (const auto& pHost : std::as_const(mHostsWithHandlers)) {
            if (pHost) {
                killLuaHandlers(pHost);
            }
        }
        mHostsWithHandlers.clear();
        for (const auto& [profileName, packageName] : std::as_const(mFontsToUnload)) {
            FontManager::self()->unloadFonts(profileName, packageName);
        }
        mFontsToUnload.clear();
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

    // The font database lists installed families, not the names a platform resolves on
    // top of them, so deciding "missing" by that list alone calls a font the machine
    // draws perfectly well missing.
    void test_resolveAcceptsANameThePlatformResolvesForItself()
    {
        QVERIFY2(!mpHost->resolveFontFamily(mAliasFamily).available, "the alias name is resolvable before anything resolves it, so this case proves nothing");
        const QString standingInFor = stageAliasFamily();
        if (standingInFor.isEmpty()) {
            QSKIP("this platform hands an unrecognised font name back unchanged instead of naming the family it drew in its place, so no name it resolves can be told from one nothing knows");
        }
        // Qt consults the substitution table on every platform, so a failure here is the
        // case's own tool rather than the machine it runs on
        QCOMPARE(QFontInfo(QFont(mAliasFamily)).family(), standingInFor);

        const auto resolved = mpHost->resolveFontFamily(mAliasFamily);
        QVERIFY2(resolved.available, "a name the platform resolves was reported as an uninstalled font");
        QCOMPARE(resolved.family, mAliasFamily);
        QCOMPARE(resolved.weight, QFont::Normal);
    }

    void test_aDisplayFontThePlatformResolvesIsNotStoodInFor()
    {
        const QString standingInFor = stageAliasFamily();
        if (standingInFor.isEmpty()) {
            QSKIP("this platform hands an unrecognised font name back unchanged instead of naming the family it drew in its place, so no name it resolves can be told from one nothing knows");
        }
        QCOMPARE(QFontInfo(QFont(mAliasFamily)).family(), standingInFor);

        QVERIFY(mpHost->setDisplayFont(QFont(mAliasFamily, 13, QFont::Normal)).first);

        QVERIFY2(!mpHost->substituteMissingDisplayFont(), "a name the platform resolves was stood in for");
        QCOMPARE(mpHost->getDisplayFont().family(), mAliasFamily);
        QCOMPARE(mpHost->getDisplayFont().pointSize(), 13);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mAliasFamily);
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
        if (FontManager::availableFonts().contains(Host::scmDefaultFontFamily, Qt::CaseInsensitive)) {
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

    // The same as the install case above, driven directly, so the two halves of
    // the rule are pinned without a profile load either side of them.
    void test_aDisplayFontArrivingFromXmlBecomesTheFamilyTheProfileAsksFor()
    {
        QVERIFY(mpHost->setDisplayFont(QFont(mMissingFamily, 14, QFont::Normal)).first);
        QVERIFY(mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);

        mpHost->setDisplayFontFromString(QFont(mOtherBundledFamily, 12).toString());
        QCOMPARE(mpHost->getDisplayFont().family(), mOtherBundledFamily);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mOtherBundledFamily);
        QCOMPARE(mpHost->getDisplayFontForSaving().pointSize(), 12);
    }

    // A family this machine does not have cannot retire the stand-in: the check
    // that put it up runs once, at profile load, so nothing would notice the new
    // family is missing too and the one the profile asked for would be forgotten.
    void test_aDisplayFontFromXmlNamingAnUninstalledFamilyKeepsTheRecordedChoice()
    {
        QVERIFY(mpHost->setDisplayFont(QFont(mMissingFamily, 14, QFont::Normal)).first);
        QVERIFY(mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);

        mpHost->setDisplayFontFromString(QFont(qsl("Another Font That Is Not There"), 12).toString());
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);
    }

    // A truncated <mDisplayFont>, or one naming no family at all, would otherwise
    // leave a default-constructed proportional font whose letters do have a width,
    // so nothing downstream turns it away - and taking it would count as a choice
    // of family and throw away the record of the font the profile really asks for.
    // The ignoreMessage() is what pins this to the guard rather than to the
    // zero-width refusal, which produces the same outcome by another route.
    void test_aFontDescriptionThatCannotBeReadLeavesTheFontAlone()
    {
        QVERIFY(mpHost->setDisplayFont(QFont(mMissingFamily, 14, QFont::Normal)).first);
        QVERIFY(mpHost->substituteMissingDisplayFont());
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);

        QTest::ignoreMessage(QtWarningMsg, R"(Host::setDisplayFontFromString(...) WARNING - "" is not a font description, so the font in use is kept.)");
        mpHost->setDisplayFontFromString(QString());
        QCOMPARE(mpHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);

        // a description that is well formed apart from naming no family, which Qt
        // turns away as well - the font in use has to survive that one too
        const QString noFamily = QFont(QString(), 12).toString();
        QTest::ignoreMessage(QtWarningMsg, qPrintable(qsl(R"(Host::setDisplayFontFromString(...) WARNING - "%1" is not a font description, so the font in use is kept.)").arg(noFamily)));
        mpHost->setDisplayFontFromString(noFamily);
        QCOMPARE(mpHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(mpHost->getDisplayFontForSaving().family(), mMissingFamily);
    }

    // setTextEditFont() takes the weight out of a "Family Style" name, so it has
    // to put the weight back to normal for a name that carries none - on both the
    // listed and the unlisted family, or the bold of an earlier call is left
    // behind on whatever family is set next.
    void test_setTextEditFontDoesNotLeaveAnEarlierWeightBehind()
    {
        const QString profileName = qsl("MissingDisplayFont-TextEdit-Test");
        QVERIFY2(writeProfileSave(profileName, Host::scmDefaultFontFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");

        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createTextEdit('main', 'mdfTextEdit', 0, 0, 100, 50)")));
        auto* pTextEdit = pHost->mainConsoleView()->textBoxWidget(qsl("mdfTextEdit"));
        QVERIFY2(pTextEdit, "the test text edit was not created");

        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setTextEditFont('mdfTextEdit', '%1 Bold')").arg(mOtherBundledFamily)));
        QCOMPARE(pTextEdit->font().family(), mOtherBundledFamily);
        QCOMPARE(pTextEdit->font().weight(), QFont::Bold);

        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setTextEditFont('mdfTextEdit', '%1')").arg(mMissingFamily)));
        QCOMPARE(pTextEdit->font().family(), mMissingFamily);
        QCOMPARE(pTextEdit->font().weight(), QFont::Normal);

        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setTextEditFont('mdfTextEdit', '%1 Bold')").arg(mOtherBundledFamily)));
        QCOMPARE(pTextEdit->font().weight(), QFont::Bold);
        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setTextEditFont('mdfTextEdit', '%1')").arg(Host::scmDefaultFontFamily)));
        QCOMPARE(pTextEdit->font().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pTextEdit->font().weight(), QFont::Normal);
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
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");

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

    // The one production path that reaches setDisplayFontFromString() after the
    // check has already run: a profile save installed as a package. An installed
    // family has to retire the stand-in and become what is saved; an uninstalled
    // one must not, because the check never runs again to notice it is missing.
    void test_aProfileSaveInstalledAsAPackageRetiresTheStandInOnlyIfItsFontIsInstalled()
    {
        const QString profileName = qsl("MissingDisplayFont-Install-Test");
        QVERIFY2(writeProfileSave(profileName, mMissingFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);

        const QString absentFamily = qsl("Another Font That Is Not There");
        const QString absentPath = mArchiveDir.filePath(qsl("font-absent.xml"));
        QVERIFY2(writeHostPackageXml(absentPath, QFont(absentFamily, 12)), "could not write the package XML");
        QVERIFY2(pHost->installPackage(absentPath, enums::PackageModuleType::Package).first, "the package naming an uninstalled font did not install");
        // installPackage() defers an install for as long as a save is in flight -
        // so without this the next install could be only queued and the case would
        // pass on a font that never arrived.
        pHost->waitForProfileSave();
        QCOMPARE(pHost->getDisplayFont().family(), absentFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);

        const QString presentPath = mArchiveDir.filePath(qsl("font-present.xml"));
        QVERIFY2(writeHostPackageXml(presentPath, QFont(mOtherBundledFamily, 12)), "could not write the package XML");
        QVERIFY2(pHost->installPackage(presentPath, enums::PackageModuleType::Package).first, "the package naming an installed font did not install");
        pHost->waitForProfileSave();
        QCOMPARE(pHost->getDisplayFont().family(), mOtherBundledFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mOtherBundledFamily);
    }

    // A save can hold a size past Host::scmMaxFontSize, at which Qt drops the family
    void test_aSavedDisplayFontTooLargeToShowKeepsItsFamilyAtTheDefaultSize()
    {
        const QString profileName = qsl("MissingDisplayFont-TooLarge-Test");
        QVERIFY2(writeProfileSave(profileName, mOtherBundledFamily, QString(), QString(), QString(), 999999), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        // QFontInfo, as getFont() reads it: QFont::family() only echoes the request
        QCOMPARE(QFontInfo(pHost->getDisplayFont()).family(), mOtherBundledFamily);
        QCOMPARE(pHost->getDisplayFont().pointSize(), 14);
    }

    // A package can bring one in while the profile is at any size, and in pixels too
    void test_aPackageDisplayFontTooLargeToShowKeepsItsFamilyAtTheSizeInUse()
    {
        const QString profileName = qsl("MissingDisplayFont-TooLargePackage-Test");
        QVERIFY2(writeProfileSave(profileName, Host::scmDefaultFontFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        pHost->setDisplayFontSize(20);
        QCOMPARE(pHost->getDisplayFont().pointSize(), 20);

        auto install = [&](const QString& fileName, const QFont& font) {
            const QString path = mArchiveDir.filePath(fileName);
            QVERIFY2(writeHostPackageXml(path, font), "could not write the package XML");
            QVERIFY2(pHost->installPackage(path, enums::PackageModuleType::Package).first, "the package did not install");
            pHost->waitForProfileSave();
        };

        install(qsl("font-too-large-pt.xml"), QFont(mOtherBundledFamily, 999999));
        QCOMPARE(QFontInfo(pHost->getDisplayFont()).family(), mOtherBundledFamily);
        QCOMPARE(pHost->getDisplayFont().pointSize(), 20);

        QFont tooManyPixels(Host::scmDefaultFontFamily);
        tooManyPixels.setPixelSize(999999);
        install(qsl("font-too-large-px.xml"), tooManyPixels);
        QCOMPARE(QFontInfo(pHost->getDisplayFont()).family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFont().pointSize(), 20);

        // A font in use given in pixels has no point size to keep
        QFont inPixels(mOtherBundledFamily);
        inPixels.setPixelSize(20);
        install(qsl("font-in-pixels.xml"), inPixels);
        const int sizeInPixelsAsPoints = QFontInfo(pHost->getDisplayFont()).pointSize();
        QVERIFY(sizeInPixelsAsPoints > 0);
        install(qsl("font-too-large-after-px.xml"), QFont(Host::scmDefaultFontFamily, 999999));
        QCOMPARE(QFontInfo(pHost->getDisplayFont()).family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFont().pointSize(), sizeInPixelsAsPoints);
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
        if (FontManager::availableFonts().contains(mOtherBundledFamily, Qt::CaseInsensitive)) {
            QSKIP("the family the module supplies is already installed on this machine, so this cannot tell whether the module was waited for");
        }

        const QString profileName = qsl("MissingDisplayFont-Module-Test");
        QVERIFY2(writeProfileSave(profileName, mOtherBundledFamily, moduleName, modulePath), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");

        QVERIFY2(FontManager::availableFonts().contains(mOtherBundledFamily, Qt::CaseInsensitive), "the module's font was never registered, so the profile's font really was missing");
        QCOMPARE(pHost->getDisplayFont().family(), mOtherBundledFamily);
        const QString shown = consoleText(pHost);
        QVERIFY2(!shown.contains(qsl("is not installed on this computer")), qPrintable(qsl("a font the module supplies was reported missing; the console holds: %1").arg(shown)));
    }

    // The mirror of the module case: a package's fonts are registered before its
    // scripts run, so a package can ship a font and ask for it as it installs -
    // which is what MedUI in the package repository does. Uninstalling takes the
    // font back off the machine, so the display font has to be checked again or
    // the console silently drops to whatever family Qt picks next.
    void test_uninstallingThePackageAFontCameFromFallsBackToTheBundledDefault()
    {
        const QString packageName = qsl("font-uninstall");
        const QString packagePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(packageName));
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily);
        QVERIFY2(!fontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        const QList<std::pair<QString, QByteArray>> entries{{qsl("%1.ttf").arg(packageName), fontBytes}, {qsl("%1.xml").arg(packageName), minimalPackageXml(packageName)}};
        QVERIFY2(writeArchive(packagePath, entries), "could not write the test package archive");

        const QString profileName = qsl("MissingDisplayFont-Uninstall-Test");
        QVERIFY2(writeProfileSave(profileName, Host::scmDefaultFontFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        QVERIFY2(!FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive),
                 "the package's family is already installed, so this cannot tell whether uninstalling removed it");

        QVERIFY2(pHost->installPackage(packagePath, enums::PackageModuleType::Package).first, "the package carrying the font did not install");
        pHost->waitForProfileSave();
        QVERIFY2(FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's font was never registered");

        // What the package's own install-time script does with setFont()
        QVERIFY(pHost->setDisplayFont(QFont(mPackageSuppliedFamily, 12), Host::DisplayFontChange::UserChoice).first);
        QCOMPARE(pHost->getDisplayFont().family(), mPackageSuppliedFamily);

        QVERIFY2(pHost->uninstallPackage(packageName, enums::PackageModuleType::Package), "the package did not uninstall");
        pHost->waitForProfileSave();
        QVERIFY2(!FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's font is still registered, so nothing has gone missing to notice");

        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        const QString shown = consoleText(pHost);
        QVERIFY2(shown.contains(qsl("[ WARN ]")), qPrintable(qsl("no warning reached the main console; it holds: %1").arg(shown)));
        QVERIFY2(shown.contains(mPackageSuppliedFamily), qPrintable(qsl("the warning does not name the font that went missing; the console holds: %1").arg(shown)));
    }

    // The profile still asks for the family that went missing, so once a package
    // brings it again the console returns to it
    void test_reinstallingThePackageAFontCameFromBringsTheFontBack()
    {
        const QString packageName = qsl("font-reinstall");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");

        Host* pHost = openProfile(qsl("MissingDisplayFont-Reinstall-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pHost);
        QVERIFY2(!FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's family is already installed, so this cannot tell whether it went away");

        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        QFont boldFont(mPackageSuppliedFamily, 12);
        boldFont.setWeight(QFont::Bold);
        QVERIFY(pHost->setDisplayFont(boldFont, Host::DisplayFontChange::UserChoice).first);
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall");
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);

        // The fonts are registered for the whole application, so another profile wanting the family gets it back too
        Host* pOtherHost = openProfile(qsl("MissingDisplayFont-Reinstall-Other-Test"), mPackageSuppliedFamily);
        QVERIFY(pOtherHost);
        QCOMPARE(pOtherHost->getDisplayFont().family(), Host::scmDefaultFontFamily);

        // An open preferences dialog must not keep offering the stand-in, which its next size change would put back
        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        QCOMPARE(pPreferences->fontComboBox_displayFont->currentFont().family(), Host::scmDefaultFontFamily);

        // Opening the other profile ran the save the uninstall queued, and an install waits for a save
        pHost->waitForProfileSave();
        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install again");
        QVERIFY2(FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's font was not registered again");

        QCOMPARE(pHost->getDisplayFont().family(), mPackageSuppliedFamily);
        QCOMPARE(pHost->getDisplayFont().pointSize(), 12);
        QCOMPARE(pHost->getDisplayFont().weight(), QFont::Bold);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        const QString shown = consoleText(pHost);
        QVERIFY2(shown.contains(restoredMessage(mPackageSuppliedFamily)), qPrintable(qsl("nothing told the player the font is back; the console holds: %1").arg(shown)));
        QCOMPARE(pPreferences->fontComboBox_displayFont->currentFont().family(), mPackageSuppliedFamily);
        pPreferences.reset();
        QCOMPARE(pOtherHost->getDisplayFont().family(), mPackageSuppliedFamily);

        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall at the end");
    }

    // Putting the family back runs the profile's sysSettingChanged handlers, and one that
    // takes the package away again must leave the family missing, not forgotten
    void test_aHandlerTakingTheFontAwayAsItComesBackLeavesItMissing()
    {
        const QString packageName = qsl("font-handler");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Handler-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pHost);
        QVERIFY2(!FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's family is already installed, so this cannot tell whether it went away");

        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        QVERIFY(pHost->setDisplayFont(QFont(mPackageSuppliedFamily, 12), Host::DisplayFontChange::UserChoice).first);
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall");
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);

        QVERIFY(runLua(pHost,
                       qsl("fontHandlerTestEvents = {} "
                           "for _, event in ipairs({'sysInstall', 'sysInstallPackage', 'sysUninstall', 'sysUninstallPackage'}) do "
                           "  fontHandlerTestIds[#fontHandlerTestIds + 1] = registerAnonymousEventHandler(event, function(name, package) "
                           "    fontHandlerTestEvents[#fontHandlerTestEvents + 1] = name .. ':' .. package end) "
                           "end "
                           "fontHandlerTestIds[#fontHandlerTestIds + 1] = registerAnonymousEventHandler('sysSettingChanged', function(_, setting, family) "
                           "  if setting == 'main window font' and family == '%1' then "
                           "    fontHandlerTestEvents[#fontHandlerTestEvents + 1] = 'handler removes it' "
                           "    uninstallPackage('%2') "
                           "  end "
                           "end)")
                               .arg(mPackageSuppliedFamily, packageName)));
        const QString before = consoleText(pHost);
        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install again");
        QVERIFY2(!FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the handler did not take the package away again, so this proves nothing");

        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        // Install events first: a handler must not hear the package go before it heard it arrive
        const QString events = luaString(pHost, qsl("table.concat(fontHandlerTestEvents, ',')"));
        QVERIFY2(events.startsWith(qsl("sysInstall:%1,sysInstallPackage:%1,handler removes it").arg(packageName)), qPrintable(qsl("the handlers heard: %1").arg(events)));
        const QString added = consoleText(pHost).mid(before.size());
        QVERIFY2(!added.contains(restoredMessage(mPackageSuppliedFamily)), qPrintable(qsl("the player was told a font that went straight away again is in use; the console added: %1").arg(added)));
    }

    // Opening a profile registers its packages' fonts for the whole application too,
    // so one already open that stood in for the family gets it back
    void test_openingAProfileWhosePackageCarriesTheFontBringsItBackForTheOthers()
    {
        const QString packageName = qsl("font-on-open");
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily);
        QVERIFY2(!fontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        QVERIFY2(!FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive),
                 "the package's family is already installed, so this cannot tell whether opening the profile brought it");

        Host* pWaitingHost = openProfile(qsl("MissingDisplayFont-Open-Waiting-Test"), mPackageSuppliedFamily);
        QVERIFY(pWaitingHost);
        QCOMPARE(pWaitingHost->getDisplayFont().family(), Host::scmDefaultFontFamily);

        const QString carryingProfileName = qsl("MissingDisplayFont-Open-Carrying-Test");
        QVERIFY2(writeProfileSave(carryingProfileName, Host::scmDefaultFontFamily, QString(), QString(), packageName), "could not write the carrying test profile save");
        const QString packageFolder = MudletApp::getMudletPath(enums::profilePackagePath, carryingProfileName, packageName);
        QVERIFY(QDir().mkpath(packageFolder));
        QFile fontFile(qsl("%1/%2.ttf").arg(packageFolder, packageName));
        QVERIFY(fontFile.open(QIODevice::WriteOnly));
        QCOMPARE(fontFile.write(fontBytes), fontBytes.size());
        fontFile.close();

        mFontsToUnload.append({carryingProfileName, packageName});
        Host* pCarryingHost = openProfile(carryingProfileName, Host::scmDefaultFontFamily, false);
        QVERIFY(pCarryingHost);
        QVERIFY2(FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "opening the profile did not register its package's font");

        QCOMPARE(pWaitingHost->getDisplayFont().family(), mPackageSuppliedFamily);
        QCOMPARE(pWaitingHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        const QString shown = consoleText(pWaitingHost);
        QVERIFY2(shown.contains(restoredMessage(mPackageSuppliedFamily)), qPrintable(qsl("nothing told the player the font is back; the console holds: %1").arg(shown)));
    }

    // The way out has to reach every profile the way back does: one left naming a family
    // that has gone is drawn in whatever Qt picks, with nothing said and nothing remembered
    void test_uninstallingInOneProfileMovesEveryProfileOffTheFont()
    {
        const QString packageName = qsl("font-shared");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        QVERIFY2(!FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's family is already installed, so this cannot tell whether it went away");

        Host* pOwner = openProfile(qsl("MissingDisplayFont-Shared-Owner-Test"), mPackageSuppliedFamily);
        Host* pOther = openProfile(qsl("MissingDisplayFont-Shared-Other-Test"), mPackageSuppliedFamily);
        QVERIFY(pOwner && pOther);
        QVERIFY2(installFontPackage(pOwner, packagePath, packageName), "the package carrying the font did not install");
        QCOMPARE(pOther->getDisplayFont().family(), mPackageSuppliedFamily);

        const QString before = consoleText(pOther);
        QVERIFY2(uninstallFontPackage(pOwner, packageName), "the package did not uninstall");
        QCOMPARE(pOther->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pOther->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        const QString added = consoleText(pOther).mid(before.size());
        QVERIFY2(added.contains(qsl("[ WARN ]")) && added.contains(mPackageSuppliedFamily), qPrintable(qsl("the other profile was not told its font went; its console added: %1").arg(added)));

        // Still remembered, so the next install brings it back here as well
        QVERIFY2(installFontPackage(pOwner, packagePath, packageName), "the package carrying the font did not install again");
        QCOMPARE(pOther->getDisplayFont().family(), mPackageSuppliedFamily);
        QVERIFY2(uninstallFontPackage(pOwner, packageName), "the package did not uninstall at the end");
    }

    // A handler that answers the move to the stand-in by installing the font again: the
    // family has to be on record as missing by then, or the restore finds nothing to do
    void test_aHandlerBringingTheFontBackAsItLeavesRestoresIt()
    {
        const QString packageName = qsl("font-handler-back");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Handler-Back-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pHost);
        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        QVERIFY(pHost->setDisplayFont(QFont(mPackageSuppliedFamily, 12), Host::DisplayFontChange::UserChoice).first);

        QVERIFY(runLua(pHost,
                       qsl("fontHandlerTestIds[#fontHandlerTestIds + 1] = registerAnonymousEventHandler('sysSettingChanged', function(_, setting, family) "
                           "  if setting == 'main window font' and family == '%1' then installPackage([[%2]]) end "
                           "end)")
                               .arg(Host::scmDefaultFontFamily, packagePath)));
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall");
        killLuaHandlers(pHost);
        QVERIFY2(FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the handler did not install the package again, so this proves nothing");

        QCOMPARE(pHost->getDisplayFont().family(), mPackageSuppliedFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall at the end");
    }

    // A handler that picks yet another font as the family comes back: the player must not be
    // told the family they asked for is the one in use
    void test_aHandlerChoosingAnotherFontAsItComesBackIsNotReportedAsTheRestore()
    {
        const QString packageName = qsl("font-handler-other");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Handler-Other-Test"), mPackageSuppliedFamily);
        QVERIFY(pHost);
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);

        QVERIFY(runLua(pHost,
                       qsl("fontHandlerTestIds[#fontHandlerTestIds + 1] = registerAnonymousEventHandler('sysSettingChanged', function(_, setting, family) "
                           "  if setting == 'main window font' and family == '%1' then setFont('main', '%2') end "
                           "end)")
                               .arg(mPackageSuppliedFamily, mOtherBundledFamily)));
        const QString before = consoleText(pHost);
        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        killLuaHandlers(pHost);

        QCOMPARE(pHost->getDisplayFont().family(), mOtherBundledFamily);
        const QString added = consoleText(pHost).mid(before.size());
        QVERIFY2(!added.contains(mPackageSuppliedFamily), qPrintable(qsl("the player was told about a font that a handler replaced; the console added: %1").arg(added)));
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall at the end");
    }

    // An old-style "Family Style" name comes back as the base family with that weight
    void test_aMissingStyleSuffixedFamilyComesBackWithItsWeight()
    {
        const QString packageName = qsl("font-styled");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Styled-Test"), qsl("%1 Bold").arg(mPackageSuppliedFamily));
        QVERIFY(pHost);
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);

        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        QCOMPARE(pHost->getDisplayFont().family(), mPackageSuppliedFamily);
        QCOMPARE(pHost->getDisplayFont().weight(), QFont::Bold);
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall at the end");
    }

    // Choosing the default while the family was missing settles it: the family coming back later
    // is no reason to move the console off the player's choice
    void test_aChosenDefaultIsKeptWhenTheMissingFamilyComesBack()
    {
        const QString packageName = qsl("font-chosen-default");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Chosen-Default-Test"), mPackageSuppliedFamily);
        QVERIFY(pHost);
        QVERIFY(runLua(pHost, qsl("setFont('main', '%1')").arg(Host::scmDefaultFontFamily)));
        QCOMPARE(pHost->getDisplayFontForSaving().family(), Host::scmDefaultFontFamily);

        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), Host::scmDefaultFontFamily);
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall at the end");
    }

    // A family that comes back but cannot be drawn: the player, who was told to install it, is told
    // why it is still not in use - and only once, however many installs follow
    void test_aFontThatComesBackUnusableIsReportedOnce()
    {
        const QString packageName = qsl("font-unusable");
        const QByteArray fontBytes = zeroWidthFont(renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily));
        QVERIFY2(!fontBytes.isEmpty(), "could not make a font whose letters have no width");
        const QString packagePath = writeFontPackage(packageName, fontBytes);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Unusable-Test"), mPackageSuppliedFamily);
        QVERIFY(pHost);

        const QString before = consoleText(pHost);
        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        QVERIFY2(QFontMetrics(QFont(mPackageSuppliedFamily, 14)).averageCharWidth() == 0, "the font can be drawn after all, so this proves nothing");
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);

        const QString otherName = qsl("fontless-after-unusable");
        const QString otherPath = mArchiveDir.filePath(qsl("%1.mpackage").arg(otherName));
        const QList<std::pair<QString, QByteArray>> otherEntries{{qsl("%1.xml").arg(otherName), minimalPackageXml(otherName)}};
        QVERIFY2(writeArchive(otherPath, otherEntries), "could not write the fontless package archive");
        QVERIFY2(installFontPackage(pHost, otherPath, otherName), "the fontless package did not install");

        const QString added = consoleText(pHost).mid(before.size());
        QCOMPARE(added.count(qsl("is available now, but it cannot be used")), 1);
        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall at the end");
    }

    // A profile being closed has saved and goes on the next pass of the event loop: until then
    // nothing another profile does may change its font or run its handlers
    void test_aClosingProfileIsNotRestored()
    {
        const QString packageName = qsl("font-closing");
        const QString packagePath = writeFontPackage(packageName);
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily);
        QVERIFY2(!packagePath.isEmpty() && !fontBytes.isEmpty(), "could not write the test package archive");
        Host* pInstalling = openProfile(qsl("MissingDisplayFont-Closing-Installing-Test"), Host::scmDefaultFontFamily);
        Host* pClosing = openProfile(qsl("MissingDisplayFont-Closing-Test"), mPackageSuppliedFamily);
        QVERIFY(pInstalling && pClosing);
        QSignalSpy fontChanges(pClosing, &Host::signal_consoleFontChanged);
        QPointer<Host> closing(pClosing);

        mudlet::self()->slot_closeProfileByName(pClosing->getName());
        QVERIFY(pClosing->isClosingDown());

        // A profile opened in that pass whose package carries the font...
        const QString carryingProfileName = qsl("MissingDisplayFont-Closing-Carrying-Test");
        QVERIFY2(writeProfileSave(carryingProfileName, Host::scmDefaultFontFamily, QString(), QString(), packageName), "could not write the carrying test profile save");
        const QString packageFolder = MudletApp::getMudletPath(enums::profilePackagePath, carryingProfileName, packageName);
        QVERIFY(QDir().mkpath(packageFolder));
        QFile fontFile(qsl("%1/%2.ttf").arg(packageFolder, packageName));
        QVERIFY(fontFile.open(QIODevice::WriteOnly));
        QCOMPARE(fontFile.write(fontBytes), fontBytes.size());
        fontFile.close();
        mFontsToUnload.append({carryingProfileName, packageName});
        QVERIFY(mudlet::self()->loadProfile(carryingProfileName, false));
        QVERIFY2(FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "opening the profile did not register its package's font");
        QVERIFY2(closing, "the closing profile went before the other one opened, so this proves nothing");
        // ...or an install, as a script's timer firing in that pass would make
        mFontsToUnload.append({pInstalling->getName(), packageName});
        QVERIFY2(pInstalling->installPackage(packagePath, enums::PackageModuleType::Package).first, "the package carrying the font did not install");
        runQueuedEvents();

        QCOMPARE(fontChanges.count(), 0);
        mudlet::self()->slot_connectionDialogueFinished(carryingProfileName, false);
        QVERIFY2(uninstallFontPackage(pInstalling, packageName), "the package did not uninstall at the end");
    }

    // A package whose script installs another: the inner one finishing must not put the font back
    // before the outer one, which brought it, has been heard to arrive
    void test_aNestedInstallPutsTheFontBackOnlyAfterTheOuterInstallEvents()
    {
        const QString innerName = qsl("font-nested-inner");
        const QString innerPath = mArchiveDir.filePath(qsl("%1.mpackage").arg(innerName));
        const QList<std::pair<QString, QByteArray>> innerEntries{{qsl("%1.xml").arg(innerName), minimalPackageXml(innerName)}};
        QVERIFY2(writeArchive(innerPath, innerEntries), "could not write the inner package archive");
        const QString outerName = qsl("font-nested-outer");
        const QString outerPath = mArchiveDir.filePath(qsl("%1.mpackage").arg(outerName));
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily);
        QVERIFY2(!fontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        const QByteArray outerXml = qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                        "<!DOCTYPE MudletPackage>\n"
                                        "<MudletPackage version=\"1.001\">\n"
                                        "<ScriptPackage>\n"
                                        "<Script isActive=\"yes\" isFolder=\"no\">\n"
                                        "<name>%1 script</name><packageName></packageName>\n"
                                        "<script>installPackage([[%2]])</script>\n"
                                        "<eventHandlerList />\n"
                                        "</Script>\n"
                                        "</ScriptPackage>\n"
                                        "</MudletPackage>\n")
                                            .arg(outerName, innerPath)
                                            .toUtf8();
        const QList<std::pair<QString, QByteArray>> outerEntries{{qsl("%1.ttf").arg(outerName), fontBytes}, {qsl("%1.xml").arg(outerName), outerXml}};
        QVERIFY2(writeArchive(outerPath, outerEntries), "could not write the outer package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Nested-Test"), mPackageSuppliedFamily);
        QVERIFY(pHost);
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);

        QVERIFY(runLua(pHost,
                       qsl("fontHandlerTestEvents = {} "
                           "for _, event in ipairs({'sysInstall', 'sysSettingChanged'}) do "
                           "  fontHandlerTestIds[#fontHandlerTestIds + 1] = registerAnonymousEventHandler(event, function(name, argument) "
                           "    fontHandlerTestEvents[#fontHandlerTestEvents + 1] = name .. ':' .. argument end) "
                           "end")));
        QVERIFY2(installFontPackage(pHost, outerPath, outerName), "the outer package did not install");
        mFontsToUnload.append({pHost->getName(), innerName});

        QCOMPARE(pHost->getDisplayFont().family(), mPackageSuppliedFamily);
        const QString events = luaString(pHost, qsl("table.concat(fontHandlerTestEvents, ',')"));
        QCOMPARE(events, qsl("sysInstall:%1,sysInstall:%2,sysSettingChanged:main window font").arg(innerName, outerName));
    }

    // The restore is for every open profile, so the one whose install queued it closing first must not cancel it
    void test_aProfileClosingAsItInstallsStillRestoresTheOthers()
    {
        const QString packageName = qsl("font-closing-installer");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pWaiting = openProfile(qsl("MissingDisplayFont-ClosingInstaller-Waiting-Test"), mPackageSuppliedFamily);
        Host* pInstalling = openProfile(qsl("MissingDisplayFont-ClosingInstaller-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pWaiting && pInstalling);
        QCOMPARE(pWaiting->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QPointer<Host> installing(pInstalling);
        mFontsToUnload.append({pInstalling->getName(), packageName});

        // The close goes ahead on the next pass of the event loop, as a timer the profile set might install in this one
        mudlet::self()->slot_closeProfileByName(pInstalling->getName());
        QVERIFY(pInstalling->isClosingDown());
        QVERIFY2(pInstalling->installPackage(packagePath, enums::PackageModuleType::Package).first, "the package carrying the font did not install");
        runQueuedEvents();
        QTRY_VERIFY2(!installing, "the closing profile never went");
        runQueuedEvents();

        QVERIFY2(FontManager::availableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the font went with the closing profile, so this proves nothing");
        QCOMPARE(pWaiting->getDisplayFont().family(), mPackageSuppliedFamily);
    }

    // Choosing another font and losing the family again later is a fresh loss, so its unusable copy is news again
    void test_aFontLostAgainIsReportedUnusableAgain()
    {
        const QString unusableName = qsl("font-unusable-again");
        const QString unusablePath =
                writeFontPackage(unusableName, zeroWidthFont(renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily)));
        const QString usableName = qsl("font-usable-between");
        const QString usablePath = writeFontPackage(usableName);
        QVERIFY2(!unusablePath.isEmpty() && !usablePath.isEmpty(), "could not write the test package archives");
        Host* pHost = openProfile(qsl("MissingDisplayFont-UnusableAgain-Test"), mPackageSuppliedFamily);
        QVERIFY(pHost);
        const QString before = consoleText(pHost);

        QVERIFY2(installFontPackage(pHost, unusablePath, unusableName), "the unusable font package did not install");
        QVERIFY(runLua(pHost, qsl("setFont('main', '%1')").arg(mOtherBundledFamily)));
        QVERIFY2(uninstallFontPackage(pHost, unusableName), "the unusable font package did not uninstall");
        QVERIFY2(installFontPackage(pHost, usablePath, usableName), "the usable font package did not install");
        QVERIFY(runLua(pHost, qsl("setFont('main', '%1')").arg(mPackageSuppliedFamily)));
        QCOMPARE(pHost->getDisplayFont().family(), mPackageSuppliedFamily);
        QVERIFY2(uninstallFontPackage(pHost, usableName), "the usable font package did not uninstall");
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        QVERIFY2(installFontPackage(pHost, unusablePath, unusableName), "the unusable font package did not install again");

        QCOMPARE(consoleText(pHost).mid(before.size()).count(qsl("is available now, but it cannot be used")), 2);
        QVERIFY2(uninstallFontPackage(pHost, unusableName), "the package did not uninstall at the end");
    }

    // A profile still opening has not registered its modules' fonts, so another profile's uninstall
    // that runs mid-load (here from a handler its restore set off) must not judge its font missing
    void test_aProfileStillOpeningIsNotJudgedByAnotherProfilesUninstall()
    {
        // The same length as the family it is renamed from
        const QString moduleFamily = qsl("Zqxwvu Mono");
        const QByteArray moduleFontBytes = renamedFontBytes(qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf"), mOtherBundledFamily, moduleFamily);
        QVERIFY2(!moduleFontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        const QString moduleName = qsl("font-opening-module");
        const QString modulePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(moduleName));
        const QList<std::pair<QString, QByteArray>> moduleEntries{{qsl("%1.ttf").arg(moduleName), moduleFontBytes}, {qsl("%1.xml").arg(moduleName), minimalPackageXml(moduleName)}};
        QVERIFY2(writeArchive(modulePath, moduleEntries), "could not write the test module archive");
        QVERIFY2(!FontManager::availableFonts().contains(moduleFamily, Qt::CaseInsensitive), "the module's family is already installed, so this proves nothing");

        const QString fontlessName = qsl("fontless-in-waiting-profile");
        const QString fontlessPath = mArchiveDir.filePath(qsl("%1.mpackage").arg(fontlessName));
        const QList<std::pair<QString, QByteArray>> fontlessEntries{{qsl("%1.xml").arg(fontlessName), minimalPackageXml(fontlessName)}};
        QVERIFY2(writeArchive(fontlessPath, fontlessEntries), "could not write the fontless package archive");
        Host* pWaiting = openProfile(qsl("MissingDisplayFont-Opening-Waiting-Test"), mPackageSuppliedFamily);
        QVERIFY(pWaiting);
        QVERIFY2(installFontPackage(pWaiting, fontlessPath, fontlessName), "the fontless package did not install");
        pWaiting->waitForProfileSave();
        QVERIFY(runLua(pWaiting,
                       qsl("fontHandlerTestIds[#fontHandlerTestIds + 1] = registerAnonymousEventHandler('sysSettingChanged', function(_, setting, family) "
                           "  if setting == 'main window font' and family == '%1' then uninstallPackage('%2') end "
                           "end)")
                               .arg(mPackageSuppliedFamily, fontlessName)));

        // The opening profile's package brings the waiting one's family, and its module its own
        const QString openingName = qsl("MissingDisplayFont-Opening-Test");
        const QString packageName = qsl("font-opening-package");
        QVERIFY2(writeProfileSave(openingName, moduleFamily, moduleName, modulePath, packageName), "could not write the opening test profile save");
        const QString packageFolder = MudletApp::getMudletPath(enums::profilePackagePath, openingName, packageName);
        QVERIFY(QDir().mkpath(packageFolder));
        QFile fontFile(qsl("%1/%2.ttf").arg(packageFolder, packageName));
        QVERIFY(fontFile.open(QIODevice::WriteOnly));
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mPackageSuppliedFamily);
        QCOMPARE(fontFile.write(fontBytes), fontBytes.size());
        fontFile.close();
        mFontsToUnload.append({openingName, packageName});
        mFontsToUnload.append({openingName, moduleName});
        Host* pOpening = mudlet::self()->loadProfile(openingName, false);
        QVERIFY(pOpening);
        mudlet::self()->slot_connectionDialogueFinished(openingName, false);
        runQueuedEvents();

        QVERIFY2(!pWaiting->mInstalledPackages.contains(fontlessName), "the waiting profile's handler never ran, so this proves nothing");
        QCOMPARE(pOpening->getDisplayFont().family(), moduleFamily);
        const QString shown = consoleText(pOpening);
        QVERIFY2(!shown.contains(qsl("is not installed on this computer")), qPrintable(qsl("a font the profile's own module brings was reported missing; the console holds: %1").arg(shown)));
    }

    // A script changing the font behind an open preferences dialog
    void test_thePreferencesFollowAFontChangedByAScript()
    {
        Host* pHost = openProfile(qsl("MissingDisplayFont-Preferences-Script-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pHost);
        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        QCOMPARE(pPreferences->spinBox_displayFontSize->value(), 14);

        QVERIFY(runLua(pHost, qsl("setFontSize('main', 20)")));
        QCOMPARE(pPreferences->spinBox_displayFontSize->value(), 20);

        // The change was not an edit of the user's, which would keep the dialog from re-reading the rest
        pHost->mUSE_IRE_DRIVER_BUGFIX = !pHost->mUSE_IRE_DRIVER_BUGFIX;
        QEvent activate(QEvent::WindowActivate);
        QCoreApplication::sendEvent(pPreferences.get(), &activate);
        QCOMPARE(pPreferences->checkBox_USE_IRE_DRIVER_BUGFIX->isChecked(), pHost->mUSE_IRE_DRIVER_BUGFIX);

        // The dialog's next edit builds the whole font from its controls
        pPreferences->checkBox_antiAlias->click();
        QCOMPARE(pHost->getDisplayFont().pointSize(), 20);
    }

    // A script choosing another family behind an open preferences dialog
    void test_thePreferencesStillReReadTheRestAfterAScriptChangesTheFamily()
    {
        Host* pHost = openProfile(qsl("MissingDisplayFont-Preferences-Script-Family-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pHost);
        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        QVERIFY(runLua(pHost, qsl("setFont('main', '%1')").arg(mOtherBundledFamily)));
        QCOMPARE(pPreferences->fontComboBox_displayFont->currentFont().family(), mOtherBundledFamily);

        pHost->mUSE_IRE_DRIVER_BUGFIX = !pHost->mUSE_IRE_DRIVER_BUGFIX;
        QEvent activate(QEvent::WindowActivate);
        QCoreApplication::sendEvent(pPreferences.get(), &activate);
        QCOMPARE(pPreferences->checkBox_USE_IRE_DRIVER_BUGFIX->isChecked(), pHost->mUSE_IRE_DRIVER_BUGFIX);
    }

    // The font list moves off a family by itself as the family leaves the font database, and that
    // must not be taken for the user choosing what it moved to: that would forget the family
    // without a word
    void test_thePreferencesDoNotTakeAFontLeavingForAChoice()
    {
        const QString packageName = qsl("font-preferences-leaving");
        const QString packagePath = writeFontPackage(packageName);
        QVERIFY2(!packagePath.isEmpty(), "could not write the test package archive");
        Host* pHost = openProfile(qsl("MissingDisplayFont-Preferences-Leaving-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pHost);
        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        QVERIFY2(installFontPackage(pHost, packagePath, packageName), "the package carrying the font did not install");
        QVERIFY(runLua(pHost, qsl("setFont('main', '%1')").arg(mPackageSuppliedFamily)));
        QCOMPARE(pPreferences->fontComboBox_displayFont->currentFont().family(), mPackageSuppliedFamily);

        QVERIFY2(uninstallFontPackage(pHost, packageName), "the package did not uninstall");
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        QCOMPARE(pPreferences->fontComboBox_displayFont->currentFont().family(), Host::scmDefaultFontFamily);
    }

    // Once its profile has gone the dialog's controls are cleared and greyed out, and stay so
    void test_thePreferencesForAProfileThatHasGoneIgnoreItsFont()
    {
        Host* pHost = openProfile(qsl("MissingDisplayFont-Preferences-Gone-Test"), Host::scmDefaultFontFamily);
        QVERIFY(pHost);
        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        QVERIFY(QMetaObject::invokeMethod(pPreferences.get(), "slot_handleHostDeletion", Q_ARG(Host*, pHost)));
        QVERIFY(pPreferences->fontComboBox_displayFont->currentText().isEmpty());

        QVERIFY(pHost->setDisplayFont(QFont(mOtherBundledFamily, 13), Host::DisplayFontChange::UserChoice).first);
        QVERIFY2(pPreferences->fontComboBox_displayFont->currentText().isEmpty(),
                 qPrintable(qsl("the cleared font list was filled in again with \"%1\"").arg(pPreferences->fontComboBox_displayFont->currentText())));
    }

    // Re-picking the family on show is how a player settles for the stand-in. The
    // combo box only says its font changed when the picked row is another one.
    void test_pickingTheStandInFamilyInThePreferencesSettlesForIt()
    {
        const QString profileName = qsl("MissingDisplayFont-Preferences-Test");
        QVERIFY2(writeProfileSave(profileName, mMissingFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);

        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        pPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(pPreferences.get()));
        QFontComboBox* pFamilies = pPreferences->fontComboBox_displayFont;
        QCOMPARE(pFamilies->currentFont().family(), Host::scmDefaultFontFamily);

        clickPopupRow(pFamilies, pFamilies->currentIndex());

        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), Host::scmDefaultFontFamily);
    }

    void test_untickingAntiAliasInThePreferencesStaysUnticked()
    {
        const QString profileName = qsl("MissingDisplayFont-AntiAliasToggle-Test");
        QVERIFY2(writeProfileSave(profileName, Host::scmDefaultFontFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");

        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        pPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(pPreferences.get()));
        QCheckBox* pAntiAlias = pPreferences->checkBox_antiAlias;
        QVERIFY2(pAntiAlias->isChecked(), "the profile should start with anti-aliasing on");

        pAntiAlias->click();
        QVERIFY2(pHost->getDisplayFont().styleStrategy() & QFont::NoAntialias, "unticking did not reach the font on show");
        QVERIFY2(!pAntiAlias->isChecked(), "the checkbox went back to ticked while the text is drawn without anti-aliasing");

        pPreferences->spinBox_displayFontSize->setValue(pPreferences->spinBox_displayFontSize->value() + 1);
        QVERIFY2(pHost->getDisplayFont().styleStrategy() & QFont::NoAntialias, "a size change put anti-aliasing back on");
        QVERIFY(!pAntiAlias->isChecked());

        // The apply timer stores the toggle, and the dialog then re-reads the profile
        QTRY_VERIFY2(!pHost->fontsAntiAlias(), "the profile never stored the unticked box, so it is saved with anti-aliasing on");
        QEvent activate(QEvent::WindowActivate);
        QApplication::sendEvent(pPreferences.get(), &activate);
        QVERIFY2(!pAntiAlias->isChecked(), "re-reading the profile ticked the box again");
        pPreferences->spinBox_displayFontSize->setValue(pPreferences->spinBox_displayFontSize->value() + 1);
        QVERIFY2(pHost->getDisplayFont().styleStrategy() & QFont::NoAntialias, "a later size change put anti-aliasing back on");
    }

    // The size and antialiasing controls send the family on show back to the Host
    // too, and must not settle for the stand-in. Once they have, the font on show
    // is exactly what the dialog would send, so a re-pick changes nothing at all
    // and still has to settle.
    void test_rePickingTheStandInAfterAdjustingItInThePreferencesSettlesForIt()
    {
        const QString profileName = qsl("MissingDisplayFont-PreferencesAdjust-Test");
        QVERIFY2(writeProfileSave(profileName, mMissingFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);

        auto pPreferences = std::make_unique<dlgProfilePreferences>(mudlet::self(), pHost);
        pPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(pPreferences.get()));
        QFontComboBox* pFamilies = pPreferences->fontComboBox_displayFont;
        QSpinBox* pSize = pPreferences->spinBox_displayFontSize;
        const int size = pSize->value();

        pSize->setValue(size + 1);
        QCOMPARE(pHost->getDisplayFont().pointSize(), size + 1);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);
        pPreferences->checkBox_antiAlias->click();
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);
        pPreferences->checkBox_antiAlias->click();
        pSize->setValue(size);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);

        QFont wouldSend = pFamilies->currentFont();
        wouldSend.setPointSize(pSize->value());
        wouldSend.setStyleHint(QFont::AnyStyle,
                               pPreferences->checkBox_antiAlias->isChecked() ? static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferQuality)
                                                                             : static_cast<QFont::StyleStrategy>(QFont::NoAntialias | QFont::PreferQuality));
        QVERIFY2(TFontAttributes(pHost->getDisplayFont()) == TFontAttributes(wouldSend),
                 "the font on show differs from what the dialog sends, so this case cannot show a re-pick that changes nothing");

        clickPopupRow(pFamilies, pFamilies->currentIndex());
        QCOMPARE(pHost->getDisplayFontForSaving().family(), Host::scmDefaultFontFamily);

        auto [saved, xmlPath, saveError] = pHost->saveProfile(mSaveDir.path(), qsl("settled"));
        QVERIFY2(saved, qPrintable(saveError));
        pHost->waitForProfileSave();
        QCOMPARE(savedDisplayFontFamily(xmlPath), Host::scmDefaultFontFamily);
    }

    // A reload is an uninstall and an install both marked as a module sync. The
    // uninstall must leave the module's fonts registered, and the install has to
    // register any font the module's new version brings.
    void test_reloadingTheModuleAFontCameFromKeepsTheFont()
    {
        const QString moduleName = qsl("font-reload");
        const QString modulePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(moduleName));
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mModuleSuppliedFamily);
        QVERIFY2(!fontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        const QList<std::pair<QString, QByteArray>> entries{{qsl("%1.ttf").arg(moduleName), fontBytes}, {qsl("%1.xml").arg(moduleName), minimalPackageXml(moduleName)}};
        QVERIFY2(writeArchive(modulePath, entries), "could not write the test module archive");

        const QString profileName = qsl("MissingDisplayFont-Reload-Test");
        QVERIFY2(writeProfileSave(profileName, Host::scmDefaultFontFamily), "could not write the test profile save");

        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        QVERIFY2(!FontManager::availableFonts().contains(mModuleSuppliedFamily, Qt::CaseInsensitive), "the module's family is already installed, so this cannot tell whether it went away");
        QVERIFY2(!FontManager::availableFonts().contains(mUpdatedModuleFamily, Qt::CaseInsensitive),
                 "the updated module's family is already installed, so this cannot tell whether the reload registered it");

        QVERIFY2(pHost->installPackage(modulePath, enums::PackageModuleType::ModuleFromUI).first, "the module carrying the font did not install");
        pHost->waitForProfileSave();
        QVERIFY2(FontManager::availableFonts().contains(mModuleSuppliedFamily, Qt::CaseInsensitive), "the module's font was never registered");
        QVERIFY(pHost->setDisplayFont(QFont(mModuleSuppliedFamily, 12), Host::DisplayFontChange::UserChoice).first);

        // The new version adds a font and renames its alias, which is how the
        // case tells a reload that ran from one that never happened
        const QByteArray addedFontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mUpdatedModuleFamily);
        QVERIFY2(!addedFontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        const QString updatedAliasOwner = qsl("%1 updated").arg(moduleName);
        const QList<std::pair<QString, QByteArray>> updatedEntries{
                {qsl("%1.ttf").arg(moduleName), fontBytes}, {qsl("added.ttf"), addedFontBytes}, {qsl("%1.xml").arg(moduleName), minimalPackageXml(updatedAliasOwner)}};
        QVERIFY2(writeArchive(modulePath, updatedEntries), "could not write the updated test module archive");

        pHost->reloadModule(moduleName);
        pHost->waitForProfileSave();
        QVERIFY2(pHost->mInstalledModules.contains(moduleName), "the module did not come back from its reload");
        QVERIFY2(pHost->getAliasUnit()->findFirstAlias(qsl("%1 alias").arg(updatedAliasOwner)), "the module's new version was never read in, so the reload did not run");
        QVERIFY2(FontManager::availableFonts().contains(mModuleSuppliedFamily, Qt::CaseInsensitive), "reloading the module took its font off the machine");
        // QFont::family() hands back the family asked for whether or not it is drawn
        QCOMPARE(QFontInfo(pHost->getDisplayFont()).family(), mModuleSuppliedFamily);
        QVERIFY2(FontManager::availableFonts().contains(mUpdatedModuleFamily, Qt::CaseInsensitive), "the font the module's new version brings was not registered by the reload");

        QVERIFY2(pHost->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI), "the module did not uninstall at the end");
        pHost->waitForProfileSave();
        QVERIFY2(!FontManager::availableFonts().contains(mModuleSuppliedFamily, Qt::CaseInsensitive), "uninstalling the module left its font on the machine");
        QVERIFY2(!FontManager::availableFonts().contains(mUpdatedModuleFamily, Qt::CaseInsensitive), "uninstalling the module left the font its new version brought on the machine");
    }

    // The unzip rewrites a font file already registered by its path, so what the
    // file now holds is what the reload has to register.
    void test_reloadingAModuleWhoseFontFileNowHoldsAnotherFamilyRegistersIt()
    {
        const QString moduleName = qsl("font-rewrite");
        const QString modulePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(moduleName));
        const QString fontFileName = qsl("%1.ttf").arg(moduleName);
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mRewrittenModuleFamily);
        const QByteArray renamedBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mRenamedModuleFamily);
        QVERIFY2(!fontBytes.isEmpty() && !renamedBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        const QList<std::pair<QString, QByteArray>> entries{{fontFileName, fontBytes}, {qsl("%1.xml").arg(moduleName), minimalPackageXml(moduleName)}};
        QVERIFY2(writeArchive(modulePath, entries), "could not write the test module archive");

        const QString profileName = qsl("MissingDisplayFont-Rewrite-Test");
        QVERIFY2(writeProfileSave(profileName, Host::scmDefaultFontFamily), "could not write the test profile save");
        Host* pHost = mudlet::self()->loadProfile(profileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "the test profile save could not be loaded");
        mudlet::self()->slot_connectionDialogueFinished(profileName, false);
        QVERIFY2(pHost->mainConsoleView(), "the profile came up without a main console");
        QVERIFY2(!FontManager::availableFonts().contains(mRenamedModuleFamily, Qt::CaseInsensitive),
                 "the new version's family is already installed, so this cannot tell whether the reload registered it");

        QVERIFY2(pHost->installPackage(modulePath, enums::PackageModuleType::ModuleFromUI).first, "the module carrying the font did not install");
        pHost->waitForProfileSave();
        QVERIFY2(FontManager::availableFonts().contains(mRewrittenModuleFamily, Qt::CaseInsensitive), "the module's font was never registered");

        const QString updatedAliasOwner = qsl("%1 updated").arg(moduleName);
        const QList<std::pair<QString, QByteArray>> updatedEntries{{fontFileName, renamedBytes}, {qsl("%1.xml").arg(moduleName), minimalPackageXml(updatedAliasOwner)}};
        QVERIFY2(writeArchive(modulePath, updatedEntries), "could not write the updated test module archive");

        pHost->reloadModule(moduleName);
        pHost->waitForProfileSave();
        QVERIFY2(pHost->getAliasUnit()->findFirstAlias(qsl("%1 alias").arg(updatedAliasOwner)), "the module's new version was never read in, so the reload did not run");
        QVERIFY2(FontManager::availableFonts().contains(mRenamedModuleFamily, Qt::CaseInsensitive), "the font the rewritten file now holds was not registered by the reload");
        QVERIFY(pHost->setDisplayFont(QFont(mRenamedModuleFamily, 12), Host::DisplayFontChange::UserChoice).first);
        QCOMPARE(QFontInfo(pHost->getDisplayFont()).family(), mRenamedModuleFamily);
        QVERIFY2(!FontManager::availableFonts().contains(mRewrittenModuleFamily, Qt::CaseInsensitive), "the family the file no longer holds is still registered");

        QVERIFY2(pHost->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI), "the module did not uninstall at the end");
        pHost->waitForProfileSave();
        QVERIFY2(!FontManager::availableFonts().contains(mRenamedModuleFamily, Qt::CaseInsensitive), "uninstalling the module left its font on the machine");
    }

    // Saving a profile reloads its synced modules in every other profile from
    // the saving profile's XML, which brings no fonts - so the other profile's
    // own registration of them has to survive.
    void test_syncingAModuleFromAnotherProfileKeepsTheFont()
    {
        const QString moduleName = qsl("font-sync");
        const QString modulePath = mArchiveDir.filePath(qsl("%1.mpackage").arg(moduleName));
        const QByteArray fontBytes = renamedFontBytes(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), Host::scmDefaultFontFamily, mSyncedModuleFamily);
        QVERIFY2(!fontBytes.isEmpty(), "the bundled font could not be read out of the Qt resources and renamed");
        const QList<std::pair<QString, QByteArray>> entries{{qsl("%1.ttf").arg(moduleName), fontBytes}, {qsl("%1.xml").arg(moduleName), minimalPackageXml(moduleName)}};
        QVERIFY2(writeArchive(modulePath, entries), "could not write the test module archive");

        QList<Host*> hosts;
        for (const QString& profileName : {qsl("MissingDisplayFont-Sync-From-Test"), qsl("MissingDisplayFont-Sync-To-Test")}) {
            QVERIFY2(writeProfileSave(profileName, Host::scmDefaultFontFamily), "could not write a test profile save");
            Host* pHost = mudlet::self()->loadProfile(profileName, false);
            QVERIFY(pHost);
            QVERIFY2(pHost->mLoadedOk, "a test profile save could not be loaded");
            mudlet::self()->slot_connectionDialogueFinished(profileName, false);
            QVERIFY2(pHost->mainConsoleView(), "a profile came up without a main console");
            hosts.append(pHost);
        }
        Host* pFrom = hosts.at(0);
        Host* pTo = hosts.at(1);
        QVERIFY2(!FontManager::availableFonts().contains(mSyncedModuleFamily, Qt::CaseInsensitive), "the module's family is already installed, so this cannot tell whether it went away");

        for (Host* pHost : hosts) {
            QVERIFY2(pHost->installPackage(modulePath, enums::PackageModuleType::ModuleFromUI).first, "the module carrying the font did not install");
            pHost->waitForProfileSave();
            QVERIFY(pHost->changeModuleSync(moduleName, QLatin1String("1")).first);
            // Only a module with a priority syncs, and a fresh install has none until the module manager or setModulePriority() gives it one
            pHost->mModulePriorities[moduleName] = 0;
        }
        QVERIFY(pTo->setDisplayFont(QFont(mSyncedModuleFamily, 12), Host::DisplayFontChange::UserChoice).first);

        // A change in the saving profile, so its arrival shows the sync ran
        const QString syncedAliasName = qsl("%1 synced alias").arg(moduleName);
        TAlias* pAlias = pFrom->getAliasUnit()->findFirstAlias(qsl("%1 alias").arg(moduleName));
        QVERIFY(pAlias);
        pAlias->setName(syncedAliasName);
        QVERIFY(std::get<0>(pFrom->saveProfile(QString(), QString(), true)));
        pFrom->waitForProfileSave();
        pTo->waitForProfileSave();
        QVERIFY2(pTo->getAliasUnit()->findFirstAlias(syncedAliasName), "the module was not synced into the other profile");

        // The saving profile has the same family registered from its own copy
        QVERIFY2(pFrom->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI), "the module did not uninstall from the saving profile");
        pFrom->waitForProfileSave();
        QVERIFY2(pTo->mInstalledModules.contains(moduleName), "the module did not come back from its sync");
        QVERIFY2(FontManager::availableFonts().contains(mSyncedModuleFamily, Qt::CaseInsensitive), "syncing the module took its font off the machine");
        QCOMPARE(QFontInfo(pTo->getDisplayFont()).family(), mSyncedModuleFamily);

        QVERIFY2(pTo->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI), "the module did not uninstall at the end");
        pTo->waitForProfileSave();
        QVERIFY2(!FontManager::availableFonts().contains(mSyncedModuleFamily, Qt::CaseInsensitive), "uninstalling the module left its font on the machine");
    }
};

#include "MissingDisplayFontTest.moc"
MUDLET_GROUPED_TEST_MAIN(MissingDisplayFontTest)
