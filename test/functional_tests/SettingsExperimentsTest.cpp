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
 * The Experiments card on the Advanced settings page: one checkbox for every
 * experiment the Host accepts, kept in step with the Host whichever way an
 * experiment is switched, never showing two members of one group on at once,
 * and saved with the profile like the settings around it.
 *
 * The dialog has no Lua entry point, so this cannot be a spec.
 *
 * Run with: ctest -R SettingsExperimentsTest -V
 */

#include <memory>

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QSignalSpy>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "XMLimport.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

#if defined(INCLUDE_3DMAPPER)
#include "TMap.h"

#include <QOpenGLWidget>
#endif

class SettingsExperimentsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QByteArray mSavedNoThemeDownload;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsExperiments-Test");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    const QString mModernMapper = qsl("experiment.3dmap.modernmapper");
    const QString mPlayerIcon = qsl("experiment.3d-player-icon");
    const QString mOriginalish = qsl("experiment.rendering.originalish");
    const QString mMoreTransparent = qsl("experiment.rendering.more-transparent");

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
    }

    QCheckBox* experimentBox(const QString& key) const { return mpPreferences->findChild<QCheckBox*>(qsl("checkBox_%1").arg(key)); }

    // Sent rather than driven through the window manager - see SettingsLiveSyncTest
    void returnToTheDialog()
    {
        QEvent activation(QEvent::WindowActivate);
        QCoreApplication::sendEvent(mpPreferences, &activation);
        QCoreApplication::processEvents();
    }

    void runLua(const QString& code) { mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    void switchAllExperimentsOff()
    {
        for (const auto& key : mpHost->getValidExperiments()) {
            mpHost->setExperimentEnabled(key, false);
        }
    }

    void requireExperiments(const QStringList& keys)
    {
        const QStringList known = mpHost->getValidExperiments();
        for (const auto& key : keys) {
            if (!known.contains(key)) {
                QSKIP(qPrintable(qsl("%1 is no longer an experiment, so this case has nothing to drive").arg(key)));
            }
        }
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        mSavedNoThemeDownload = qgetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestSettings::deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        mSavedNoThemeDownload.isNull() ? qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD") : qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", mSavedNoThemeDownload);
    }

    void init() { switchAllExperimentsOff(); }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        if (mpHost) {
#if defined(INCLUDE_3DMAPPER)
            if (mpHost->mpMap->mpM) {
                runLua(qsl("setConfig('show3dMapView', false) closeMapWidget()"));
            }
#endif
            switchAllExperimentsOff();
        }
    }

    // Read off the Host's own list, so an experiment added to it needs no change here either
    void test_everyExperimentHasALabelledCheckboxOnTheAdvancedPage()
    {
        openPreferences();
        auto* pCard = mpPreferences->findChild<QGroupBox*>(qsl("card_experiments"));
        QVERIFY2(pCard, "there is no Experiments card");
        QScrollArea* pAdvanced = TestSettings::pageOf(mpPreferences, qsl("advanced"));
        QVERIFY2(pAdvanced && pAdvanced->isAncestorOf(pCard), "the Experiments card is not on the Advanced page");
#if defined(INCLUDE_3DMAPPER)
        QVERIFY2(!pCard->isHidden(), "the Experiments card is hidden");
#else
        QVERIFY2(pCard->isHidden(), "the Experiments card is offered in a build where none of them does anything");
#endif

        const QStringList keys = mpHost->getValidExperiments();
        QVERIFY(!keys.isEmpty());
        for (const auto& key : keys) {
            QCheckBox* pBox = experimentBox(key);
            QVERIFY2(pBox, qPrintable(qsl("%1 has no checkbox").arg(key)));
            QVERIFY2(pCard->isAncestorOf(pBox), qPrintable(qsl("%1's checkbox is not on the Experiments card").arg(key)));
            QVERIFY2(!pBox->text().isEmpty(), qPrintable(qsl("%1's checkbox has no label").arg(key)));
            QVERIFY2(pBox->isEnabled(), qPrintable(qsl("%1's checkbox is greyed out with a profile loaded").arg(key)));
        }
        QCOMPARE(pCard->findChildren<QCheckBox*>().size(), keys.size());
    }

    void test_anExperimentSwitchedByAScriptShowsInTheDialog()
    {
        requireExperiments({mModernMapper});
        runLua(qsl("setConfig('%1', true)").arg(mModernMapper));
        QVERIFY2(mpHost->experimentEnabled(mModernMapper), "SETUP: the script did not switch the experiment on");

        openPreferences();
        QCheckBox* pBox = experimentBox(mModernMapper);
        QVERIFY(pBox);
        QVERIFY2(pBox->isChecked(), "the dialog opened with a script-enabled experiment unticked");

        runLua(qsl("setConfig('%1', false)").arg(mModernMapper));
        returnToTheDialog();
        QVERIFY2(!pBox->isChecked(), "coming back to the dialog did not pick up the script switching the experiment off");
    }

    void test_tickingAnExperimentSwitchesItForTheProfile()
    {
        requireExperiments({mModernMapper});
        openPreferences();
        QCheckBox* pBox = experimentBox(mModernMapper);
        QVERIFY(pBox);
        QVERIFY(!pBox->isChecked());

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pBox->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QVERIFY2(mpHost->experimentEnabled(mModernMapper), "ticking the box did not switch the experiment on");

        applySpy.clear();
        pBox->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QVERIFY2(!mpHost->experimentEnabled(mModernMapper), "unticking the box did not switch the experiment off");
    }

    // The Host lets only one experiment of a group run, so the dialog must not show two ticked
    void test_tickingOneMemberOfAGroupUnticksTheOther()
    {
        requireExperiments({mOriginalish, mMoreTransparent});
        QCOMPARE(Host::experimentGroup(mOriginalish), Host::experimentGroup(mMoreTransparent));
        openPreferences();
        QCheckBox* pFirst = experimentBox(mOriginalish);
        QCheckBox* pSecond = experimentBox(mMoreTransparent);
        QVERIFY(pFirst && pSecond);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pFirst->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QVERIFY(mpHost->experimentEnabled(mOriginalish));

        applySpy.clear();
        pSecond->click();
        QVERIFY2(!pFirst->isChecked(), "both members of one group show ticked");
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QVERIFY(mpHost->experimentEnabled(mMoreTransparent));
        QVERIFY(!mpHost->experimentEnabled(mOriginalish));
        QVERIFY(pSecond->isChecked());
        QVERIFY(!pFirst->isChecked());

        // Switched by a script while the dialog is open, the group still shows one at a time
        runLua(qsl("setConfig('%1', true)").arg(mOriginalish));
        returnToTheDialog();
        QVERIFY(pFirst->isChecked());
        QVERIFY(!pSecond->isChecked());
    }

    // "experiment.3dmap" is a group of one, and "experiment.3d-player-icon" is in
    // no group: the Host lets both run, so the dialog must not untick either
    void test_experimentsInDifferentGroupsCanBothBeOn()
    {
        requireExperiments({mModernMapper, mPlayerIcon});
        QVERIFY(Host::experimentGroup(mPlayerIcon).isEmpty());
        QVERIFY(Host::experimentGroup(mModernMapper) != Host::experimentGroup(mPlayerIcon));
        openPreferences();
        QCheckBox* pMapper = experimentBox(mModernMapper);
        QCheckBox* pIcon = experimentBox(mPlayerIcon);
        QVERIFY(pMapper && pIcon);

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pMapper->click();
        pIcon->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");

        QVERIFY(pMapper->isChecked());
        QVERIFY(pIcon->isChecked());
        QVERIFY(mpHost->experimentEnabled(mModernMapper));
        QVERIFY(mpHost->experimentEnabled(mPlayerIcon));
    }

#if defined(INCLUDE_3DMAPPER)
    // The renderer is picked when the 3D view is created, so an open one has to be rebuilt
    void test_tickingTheModernMapperSwapsAnOpen3DView()
    {
        requireExperiments({mModernMapper});
        runLua(qsl("openMapWidget() setConfig('show3dMapView', true)"));
        QVERIFY2(mpHost->mpMap->mpM, "SETUP: no 3D view opened");
        QVERIFY2(!mpHost->mpMap->mpM->inherits("ModernGLWidget"), "SETUP: the 3D view opened on the modern renderer");

        openPreferences();
        QCheckBox* pBox = experimentBox(mModernMapper);
        QVERIFY(pBox);
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pBox->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QVERIFY2(mpHost->mpMap->mpM && mpHost->mpMap->mpM->inherits("ModernGLWidget"), "the open 3D view kept the original renderer");

        applySpy.clear();
        pBox->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QVERIFY2(mpHost->mpMap->mpM && !mpHost->mpMap->mpM->inherits("ModernGLWidget"), "the open 3D view kept the modern renderer");
    }
#endif

    // Experiments live in the profile's save, so one ticked here outlasts the session
    void test_aTickedExperimentIsSavedWithTheProfileAndReadBack()
    {
        requireExperiments({mPlayerIcon, mModernMapper});
        openPreferences();
        QCheckBox* pIcon = experimentBox(mPlayerIcon);
        QCheckBox* pMapper = experimentBox(mModernMapper);
        QVERIFY(pIcon && pMapper);
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        pIcon->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the debounce never wrote the settings back");
        QVERIFY(mpHost->experimentEnabled(mPlayerIcon));

        QTemporaryDir saveDir;
        QVERIFY(saveDir.isValid());
        mpHost->waitForProfileSave();
        const auto [saved, xmlPath, saveError] = mpHost->saveProfile(saveDir.path(), qsl("experiments"));
        QVERIFY2(saved, qPrintable(saveError));
        mpHost->waitForProfileSave();

        QFile file(xmlPath);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        const QString xml = QString::fromUtf8(file.readAll());
        QVERIFY2(xml.contains(qsl("<experiment key=\"%1\" enabled=\"yes\"").arg(mPlayerIcon)), "the ticked experiment is not in the profile's save");
        QVERIFY2(!xml.contains(qsl("<experiment key=\"%1\"").arg(mModernMapper)), "an unticked experiment was saved as on");

        mpHost->setExperimentEnabled(mPlayerIcon, false);
        file.seek(0);
        XMLimport importer(mpHost);
        const auto [imported, importMessage] = importer.importPackage(&file);
        QVERIFY2(imported, qPrintable(importMessage));
        QVERIFY2(mpHost->experimentEnabled(mPlayerIcon), "reading the save back did not switch the experiment on");
        QVERIFY2(!mpHost->experimentEnabled(mModernMapper), "reading the save back switched on an experiment that was off");

        returnToTheDialog();
        QVERIFY(pIcon->isChecked());
        QVERIFY(!pMapper->isChecked());
    }
};

#include "SettingsExperimentsTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsExperimentsTest)
