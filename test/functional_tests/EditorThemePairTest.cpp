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

/*
 * The two bundled editor themes pair only by the words "Light" and "Dark" in
 * their names, and a profile saved as "Mudlet" / "Mudlet.tmTheme" has to keep
 * resolving, or the preferences show no theme at all.
 *
 * Run with: ctest -R EditorThemePairTest -V
 */

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "HostDialogs.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "edbee/edbee.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/texttheme.h"

#include "GroupedTest.h"

class EditorThemePairTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("EditorThemePair-Test");

    // No "light" or "dark" in its name, so the counterpart rule finds it no pair
    const QString mLoneThemeName = qsl("PhaseDProbe");
    const QString mLoneThemeFile = qsl("PhaseDProbe.tmTheme");

    void writeEditorThemesFile()
    {
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QByteArray contents = qsl(R"([{"Title": "%1", "FileName": "%2"}])").arg(mLoneThemeName, mLoneThemeFile).toUtf8();
        QVERIFY(themes.write(contents) == contents.size());
    }

    // Written, not copied: a copy keeps the resource's read-only flag, Windows will not remove a
    // read-only file, and the next case's copy then fails on the file left behind
    bool putLoneThemeInCache(const QString& file) const
    {
        QFile source(qsl(":/edbee_defaults/Mudlet Light.tmTheme"));
        QFile target(file);
        return QDir().mkpath(QFileInfo(file).absolutePath()) && source.open(QIODevice::ReadOnly) && target.open(QIODevice::WriteOnly | QIODevice::Truncate) && target.write(source.readAll()) > 0;
    }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->setStyleSheet(mpHost->mProfileStyleSheet);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
    }

    QComboBox* themeBox() const { return mpPreferences->code_editor_theme_selection_combobox; }

    // A name of its own per case: edbee keeps every theme it loads for the life of the process, and a theme already
    // loaded hides the order it is loaded in
    static bool loaded(const QString& themeName) { return edbee::Edbee::instance()->themeManager()->theme(themeName) != nullptr; }

    // Through the control: the appearance slot it is wired to is private, and
    // this is the path a user takes
    void setAppearance(const enums::Appearance state)
    {
        mpPreferences->comboBox_appearance->setCurrentIndex(state);
        QCoreApplication::processEvents();
    }

    QStringList themesOffered() const
    {
        QStringList names;
        for (int index = 0; index < themeBox()->count(); ++index) {
            names << themeBox()->itemText(index);
        }
        return names;
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        writeEditorThemesFile();
        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        mSession.stop();
    }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        mudlet::self()->setAppearance(enums::Appearance::light);
        QCoreApplication::processEvents();
    }

    // The legacy file name has to resolve too, or a profile whose stored name was
    // not migrated gets a path into a cache directory that has no such file
    void test_bothBundledThemesResolveToTheResourceFile()
    {
        QCOMPARE(MudletApp::getMudletPath(enums::editorWidgetThemePathFile, MudletApp::scmEditorThemeFileLight), qsl(":/edbee_defaults/Mudlet Light.tmTheme"));
        QCOMPARE(MudletApp::getMudletPath(enums::editorWidgetThemePathFile, MudletApp::scmEditorThemeFileDark), qsl(":/edbee_defaults/Mudlet Dark.tmTheme"));
        QCOMPARE(MudletApp::getMudletPath(enums::editorWidgetThemePathFile, MudletApp::scmEditorThemeFileLegacy), qsl(":/edbee_defaults/Mudlet Light.tmTheme"));

        QVERIFY2(QFile::exists(qsl(":/edbee_defaults/Mudlet Light.tmTheme")), "the light theme is not in the resource file");
        QVERIFY2(QFile::exists(qsl(":/edbee_defaults/Mudlet Dark.tmTheme")), "the dark theme is not in the resource file");

        QVERIFY2(!MudletApp::getMudletPath(enums::editorWidgetThemePathFile, mLoneThemeFile).startsWith(QLatin1Char(':')), "a downloaded theme is being looked for in the resource file");

        QVERIFY2(mudlet::loadEdbeeTheme(MudletApp::scmEditorThemeNameDark, MudletApp::scmEditorThemeFileDark), "the dark theme did not load - it is not a theme file edbee can read");
    }

    // The theme list does not offer "Mudlet", so an unmigrated profile would open
    // the preferences on no theme, and saving that would wipe its choice
    void test_aProfileSavedBeforeTheRenameOpensOnTheLightTheme()
    {
        QString name = qsl("Mudlet");
        QString file = qsl("Mudlet.tmTheme");
        MudletApp::migrateBundledEditorTheme(name, file);
        QCOMPARE(name, QString(MudletApp::scmEditorThemeNameLight));
        QCOMPARE(file, QString(MudletApp::scmEditorThemeFileLight));

        mpHost->mEditorTheme = qsl("Mudlet");
        mpHost->mEditorThemeFile = qsl("Mudlet.tmTheme");
        mpHost->mEditorThemeDark.clear();
        mpHost->mEditorThemeFileDark.clear();
        MudletApp::migrateBundledEditorTheme(mpHost->mEditorTheme, mpHost->mEditorThemeFile);

        openPreferences();
        QVERIFY2(themesOffered().contains(MudletApp::scmEditorThemeNameLight),
                 qPrintable(qsl("the theme list does not offer the light theme at all, it offers: %1").arg(themesOffered().join(qsl(", ")))));
        QCOMPARE(themeBox()->currentText(), QString(MudletApp::scmEditorThemeNameLight));
        QCOMPARE(themeBox()->currentData().toString(), QString(MudletApp::scmEditorThemeFileLight));
        QCOMPARE(mpHost->getEditorTheme(), QString(MudletApp::scmEditorThemeNameLight));
        QCOMPARE(mpHost->getEditorThemeFile(), QString(MudletApp::scmEditorThemeFileLight));
    }

    void test_theTwoBundledThemesPairOnTheirNames()
    {
        mpHost->mEditorTheme = MudletApp::scmEditorThemeNameLight;
        mpHost->mEditorThemeFile = MudletApp::scmEditorThemeFileLight;
        mpHost->mEditorThemeDark.clear();
        mpHost->mEditorThemeFileDark.clear();

        openPreferences();
        QCOMPARE(themeBox()->currentText(), QString(MudletApp::scmEditorThemeNameLight));

        setAppearance(enums::Appearance::dark);
        QCOMPARE(themeBox()->currentText(), QString(MudletApp::scmEditorThemeNameDark));
        QCOMPARE(themeBox()->currentData().toString(), QString(MudletApp::scmEditorThemeFileDark));
        // Kept as the light choice, so going back is a return rather than a guess
        QCOMPARE(mpHost->mEditorTheme, QString(MudletApp::scmEditorThemeNameLight));
        QCOMPARE(mpHost->mEditorThemeFile, QString(MudletApp::scmEditorThemeFileLight));

        setAppearance(enums::Appearance::light);
        QCOMPARE(themeBox()->currentText(), QString(MudletApp::scmEditorThemeNameLight));
        QCOMPARE(mpHost->mEditorThemeDark, QString(MudletApp::scmEditorThemeNameDark));
        QCOMPARE(mpHost->mEditorThemeFileDark, QString(MudletApp::scmEditorThemeFileDark));
    }

    void test_aThemeWithNoCounterpartIsLeftAlone()
    {
        mpHost->mEditorTheme = mLoneThemeName;
        mpHost->mEditorThemeFile = mLoneThemeFile;
        mpHost->mEditorThemeDark.clear();
        mpHost->mEditorThemeFileDark.clear();

        openPreferences();
        QCOMPARE(themeBox()->currentText(), mLoneThemeName);

        setAppearance(enums::Appearance::dark);
        QVERIFY2(themeBox()->currentText() == mLoneThemeName, qPrintable(qsl("going dark moved a theme with no counterpart from %1 to %2").arg(mLoneThemeName, themeBox()->currentText())));
        QCOMPARE(mpHost->mEditorTheme, mLoneThemeName);

        setAppearance(enums::Appearance::light);
        QCOMPARE(themeBox()->currentText(), mLoneThemeName);
    }

    // edbee would answer a missing theme file with a fallback theme of its own
    void test_aThemeThisMachineDoesNotHaveFallsBackToTheBundledOne()
    {
        QVERIFY2(!MudletApp::editorThemeResolves(qsl("Monokai.tmTheme")), "the missing theme this case needs is somehow on this machine");

        mpHost->mEditorTheme = qsl("Monokai");
        mpHost->mEditorThemeFile = qsl("Monokai.tmTheme");
        mpHost->mEditorThemeDark.clear();
        mpHost->mEditorThemeFileDark.clear();

        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        QCOMPARE(mpHost->getEditorTheme(), QString(MudletApp::scmEditorThemeNameLight));
        QCOMPARE(mpHost->getEditorThemeFile(), QString(MudletApp::scmEditorThemeFileLight));

        TestPixels::setAppearanceAndWait(enums::Appearance::dark);
        QCOMPARE(mpHost->getEditorTheme(), QString(MudletApp::scmEditorThemeNameDark));
        QCOMPARE(mpHost->getEditorThemeFile(), QString(MudletApp::scmEditorThemeFileDark));

        TestPixels::setAppearanceAndWait(enums::Appearance::light);
    }

    // Resolved at load rather than migrated, so the theme comes back if it is
    // installed again and an older Mudlet reads what it always read
    void test_theFallbackDoesNotRewriteWhatTheProfileRecorded()
    {
        mpHost->mEditorTheme = qsl("Monokai");
        mpHost->mEditorThemeFile = qsl("Monokai.tmTheme");
        mpHost->mEditorThemeDark.clear();
        mpHost->mEditorThemeFileDark.clear();

        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        QCOMPARE(mpHost->getEditorTheme(), QString(MudletApp::scmEditorThemeNameLight));
        TestPixels::setAppearanceAndWait(enums::Appearance::dark);
        QCOMPARE(mpHost->getEditorTheme(), QString(MudletApp::scmEditorThemeNameDark));
        TestPixels::setAppearanceAndWait(enums::Appearance::light);

        QCOMPARE(mpHost->mEditorTheme, qsl("Monokai"));
        QCOMPARE(mpHost->mEditorThemeFile, qsl("Monokai.tmTheme"));
        QVERIFY2(mpHost->mEditorThemeDark.isEmpty(), "the fallback wrote a dark theme into the profile");
        QVERIFY2(mpHost->mEditorThemeFileDark.isEmpty(), "the fallback wrote a dark theme file into the profile");
    }

    void test_aThemeThisMachineDoesHaveIsUsedAsItStands()
    {
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemePathFile, mLoneThemeFile);
        QVERIFY2(putLoneThemeInCache(file), "could not put a readable theme file in the cache for this case");
        QVERIFY2(MudletApp::editorThemeResolves(mLoneThemeFile), "the theme file was written but does not resolve");

        mpHost->mEditorTheme = mLoneThemeName;
        mpHost->mEditorThemeFile = mLoneThemeFile;
        mpHost->mEditorThemeDark.clear();
        mpHost->mEditorThemeFileDark.clear();

        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        QCOMPARE(mpHost->getEditorTheme(), mLoneThemeName);
        QCOMPARE(mpHost->getEditorThemeFile(), mLoneThemeFile);

        TestPixels::setAppearanceAndWait(enums::Appearance::dark);
        QCOMPARE(mpHost->getEditorTheme(), mLoneThemeName);

        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        QFile::remove(file);
    }

    void test_aDarkHalfThatCannotBeFoundDoesNotCostTheLightOne()
    {
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemePathFile, mLoneThemeFile);
        QVERIFY2(putLoneThemeInCache(file), "could not put a readable theme file in the cache for this case");

        mpHost->mEditorTheme = mLoneThemeName;
        mpHost->mEditorThemeFile = mLoneThemeFile;
        mpHost->mEditorThemeDark = qsl("Monokai");
        mpHost->mEditorThemeFileDark = qsl("Monokai.tmTheme");

        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        QCOMPARE(mpHost->getEditorTheme(), mLoneThemeName);

        TestPixels::setAppearanceAndWait(enums::Appearance::dark);
        QCOMPARE(mpHost->getEditorTheme(), QString(MudletApp::scmEditorThemeNameDark));
        QCOMPARE(mpHost->getEditorThemeFile(), QString(MudletApp::scmEditorThemeFileDark));

        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        QCOMPARE(mpHost->getEditorTheme(), mLoneThemeName);
        QFile::remove(file);
    }

    // The settings page named the preview's theme before loading it, so the preview fell back to edbee's plain theme,
    // and naming the same theme again once it was loaded did not re-apply it
    void test_thePreviewWearsAThemeNothingHadLoadedYet()
    {
        const QString name = qsl("PreviewProbe");
        const QString fileName = qsl("PreviewProbe.tmTheme");
        QVERIFY2(!loaded(name), "SETUP: the theme is loaded already, so the order it is loaded in cannot show");
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemePathFile, fileName);
        QVERIFY2(putLoneThemeInCache(file), "could not put a readable theme file in the cache for this case");
        auto restore = qScopeGuard([this, file]() {
            QFile::remove(file);
            writeEditorThemesFile();
        });
        QFile themes(MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile));
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write(qsl(R"([{"Title": "%1", "FileName": "%2"}])").arg(name, fileName).toUtf8()) > 0);
        themes.close();

        mpHost->mEditorTheme = name;
        mpHost->mEditorThemeFile = fileName;
        mpHost->mEditorThemeDark.clear();
        mpHost->mEditorThemeFileDark.clear();
        openPreferences();

        QCOMPARE(themeBox()->currentText(), name);
        QCOMPARE(mpPreferences->edbeePreviewWidget->textRenderer()->themeName(), name);
    }

    // An appearance change can hand the profile a theme no editor has loaded, and the script editor names it on the
    // next item it shows
    void test_theScriptEditorWearsTheDarkHalfAfterTheAppearanceTurns()
    {
        const QString name = qsl("EditorDarkProbe");
        const QString fileName = qsl("EditorDarkProbe.tmTheme");
        QVERIFY2(!loaded(name), "SETUP: the theme is loaded already, so the order it is loaded in cannot show");
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemePathFile, fileName);
        QVERIFY2(putLoneThemeInCache(file), "could not put a readable theme file in the cache for this case");
        auto restore = qScopeGuard([file]() {
            QFile::remove(file);
        });

        mpHost->mEditorTheme = QString(MudletApp::scmEditorThemeNameLight);
        mpHost->mEditorThemeFile = QString(MudletApp::scmEditorThemeFileLight);
        mpHost->mEditorThemeDark = name;
        mpHost->mEditorThemeFileDark = fileName;
        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        mudlet::self()->slot_showTriggerDialog();
        dlgTriggerEditor* pEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(pEditor, "the script editor did not open");
        auto* pCode = pEditor->findChild<edbee::TextEditorWidget*>(qsl("edbeeEditorWidget"));
        QVERIFY2(pCode, "the script editor has no code pane");
        QVERIFY2(!loaded(name), "SETUP: opening the editor on the light appearance loaded the dark half already");

        TestPixels::setAppearanceAndWait(enums::Appearance::dark);
        QCOMPARE(mpHost->getEditorTheme(), name);
        mudlet::self()->slot_showAliasDialog();
        QCoreApplication::processEvents();
        QCOMPARE(pCode->textRenderer()->themeName(), name);
    }
};

#include "EditorThemePairTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorThemePairTest)
