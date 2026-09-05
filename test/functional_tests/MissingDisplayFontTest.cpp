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

#include <QFont>
#include <QFontDatabase>
#include <QTemporaryDir>
#include <zip.h>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextBox.h"
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
    bool writeHostPackageXml(const QString& path, const QString& fontFamily)
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
                                    .arg(QFont(fontFamily, 12).toString());
        return file.write(xml.toUtf8()) != -1;
    }

    bool writeProfileSave(const QString& profileName, const QString& fontFamily, const QString& moduleName = QString(), const QString& modulePath = QString())
    {
        const QString folder = MudletPaths::getMudletPath(enums::profileXmlFilesPath, profileName);
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
        QVERIFY2(MudletPaths::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "test config dir redirection did not take effect");

        QVERIFY(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()));
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);

        QVERIFY2(!mudlet::self()->getAvailableFonts().contains(mMissingFamily, Qt::CaseInsensitive), "the stand-in for an uninstalled font turns out to be installed");
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
        QVERIFY2(pHost->mpConsole, "the profile came up without a main console");

        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createTextEdit('main', 'mdfTextEdit', 0, 0, 100, 50)")));
        auto* pTextEdit = pHost->mpConsole->textBoxWidget(qsl("mdfTextEdit"));
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
        QVERIFY2(pHost->mpConsole, "the profile came up without a main console");
        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);

        const QString absentFamily = qsl("Another Font That Is Not There");
        const QString absentPath = mArchiveDir.filePath(qsl("font-absent.xml"));
        QVERIFY2(writeHostPackageXml(absentPath, absentFamily), "could not write the package XML");
        QVERIFY2(pHost->installPackage(absentPath, enums::PackageModuleType::Package).first, "the package naming an uninstalled font did not install");
        // An install saves the profile, and installPackage() defers a second one
        // for as long as that save is in flight - so without this the next install
        // is only queued and the case would pass on a font that never arrived.
        pHost->waitForProfileSave();
        QCOMPARE(pHost->getDisplayFont().family(), absentFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mMissingFamily);

        const QString presentPath = mArchiveDir.filePath(qsl("font-present.xml"));
        QVERIFY2(writeHostPackageXml(presentPath, mOtherBundledFamily), "could not write the package XML");
        QVERIFY2(pHost->installPackage(presentPath, enums::PackageModuleType::Package).first, "the package naming an installed font did not install");
        pHost->waitForProfileSave();
        QCOMPARE(pHost->getDisplayFont().family(), mOtherBundledFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mOtherBundledFamily);
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
        QVERIFY2(pHost->mpConsole, "the profile came up without a main console");
        QVERIFY2(!mudlet::self()->getAvailableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive),
                 "the package's family is already installed, so this cannot tell whether uninstalling removed it");

        QVERIFY2(pHost->installPackage(packagePath, enums::PackageModuleType::Package).first, "the package carrying the font did not install");
        pHost->waitForProfileSave();
        QVERIFY2(mudlet::self()->getAvailableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's font was never registered");

        // What the package's own install-time script does with setFont()
        QVERIFY(pHost->setDisplayFont(QFont(mPackageSuppliedFamily, 12), Host::DisplayFontChange::UserChoice).first);
        QCOMPARE(pHost->getDisplayFont().family(), mPackageSuppliedFamily);

        QVERIFY2(pHost->uninstallPackage(packageName, enums::PackageModuleType::Package), "the package did not uninstall");
        pHost->waitForProfileSave();
        QVERIFY2(!mudlet::self()->getAvailableFonts().contains(mPackageSuppliedFamily, Qt::CaseInsensitive), "the package's font is still registered, so nothing has gone missing to notice");

        QCOMPARE(pHost->getDisplayFont().family(), Host::scmDefaultFontFamily);
        QCOMPARE(pHost->getDisplayFontForSaving().family(), mPackageSuppliedFamily);
        const QString shown = consoleText(pHost);
        QVERIFY2(shown.contains(qsl("[ WARN ]")), qPrintable(qsl("no warning reached the main console; it holds: %1").arg(shown)));
        QVERIFY2(shown.contains(mPackageSuppliedFamily), qPrintable(qsl("the warning does not name the font that went missing; the console holds: %1").arg(shown)));
    }
};

#include "MissingDisplayFontTest.moc"
MUDLET_GROUPED_TEST_MAIN(MissingDisplayFontTest)
