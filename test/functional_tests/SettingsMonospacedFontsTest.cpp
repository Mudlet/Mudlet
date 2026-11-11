/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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
 * "Show only monospaced fonts" under the display font list (#6123) leaves the
 * proportional families out of it. Ticking it never changes the profile's font:
 * while that font is a proportional one, every font stays listed. Keys, the
 * wheel and typing a name only reach listed fonts. The choice is remembered.
 *
 * Run with: ctest -R SettingsMonospacedFontsTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QListView>
#include <QProxyStyle>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QWheelEvent>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class NoWheelStyle : public QProxyStyle
{
public:
    int styleHint(StyleHint hint, const QStyleOption* pOption, const QWidget* pWidget, QStyleHintReturn* pReturn) const override
    {
        return hint == QStyle::SH_ComboBox_AllowWheelScrolling ? 0 : QProxyStyle::styleHint(hint, pOption, pWidget, pReturn);
    }
};

class SettingsMonospacedFontsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    QList<int> mApplicationFontIds;
    const QString mProfileName = qsl("SettingsMonospacedFonts-Test");
    const QString mLocalhost = qsl("localhost");
    const QString mMonospacedFamily = qsl("Bitstream Vera Sans Mono");
    const QString mProportionalFamily = qsl("Bitstream Vera Sans");
    const QString mSettingsKey = qsl("showOnlyMonospacedFonts");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
    }

    void useDisplayFont(const QString& family)
    {
        QFont font(family, 12);
        QVERIFY2(mpHost->setDisplayFont(font).first, qPrintable(qsl("the profile refused %1").arg(family)));
        QCOMPARE(mpHost->getDisplayFont().family(), family);
    }

    int rowOf(const QString& family) const
    {
        const QAbstractItemModel* pModel = mpPreferences->fontComboBox_displayFont->model();
        for (int row = 0, total = pModel->rowCount(); row < total; ++row) {
            if (pModel->index(row, 0).data().toString() == family) {
                return row;
            }
        }
        return -1;
    }

    bool listed(const QString& family) const
    {
        const int row = rowOf(family);
        auto* pView = qobject_cast<QListView*>(mpPreferences->fontComboBox_displayFont->view());
        return row >= 0 && pView && !pView->isRowHidden(row);
    }

    bool anyProportionalListed() const
    {
        const QAbstractItemModel* pModel = mpPreferences->fontComboBox_displayFont->model();
        auto* pView = qobject_cast<QListView*>(mpPreferences->fontComboBox_displayFont->view());
        const int current = mpPreferences->fontComboBox_displayFont->currentIndex();
        for (int row = 0, total = pModel->rowCount(); row < total; ++row) {
            if (row != current && !pView->isRowHidden(row) && !QFontDatabase::isFixedPitch(pModel->index(row, 0).data().toString())) {
                return true;
            }
        }
        return false;
    }

private slots:
    void initTestCase()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so the bundled Bitstream Vera fonts this tells apart are not in the resources");
#else
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        // A test binary never copies the bundled fonts out of the resources as main.cpp does
        for (const QString& resourcePath : {qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), qsl(":/fonts/ttf-bitstream-vera-1.10/Vera.ttf")}) {
            const int id = QFontDatabase::addApplicationFont(resourcePath);
            QVERIFY2(id != -1, qPrintable(qsl("could not register %1").arg(resourcePath)));
            mApplicationFontIds.append(id);
        }
        QVERIFY(QFontDatabase::isFixedPitch(mMonospacedFamily));
        QVERIFY(!QFontDatabase::isFixedPitch(mProportionalFamily));

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host after profile creation");
#endif
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        for (const int id : std::as_const(mApplicationFontIds)) {
            QFontDatabase::removeApplicationFont(id);
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init() { MudletApp::getQSettings()->remove(mSettingsKey); }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    void test_everyFontIsListedByDefault()
    {
        useDisplayFont(mMonospacedFamily);
        openPreferences();
        QVERIFY(!mpPreferences->checkBox_monospacedFontsOnly->isChecked());
        QVERIFY(listed(mProportionalFamily));
        QVERIFY(listed(mMonospacedFamily));
    }

    void test_tickingHidesProportionalFonts()
    {
        useDisplayFont(mMonospacedFamily);
        openPreferences();
        mpPreferences->checkBox_monospacedFontsOnly->setChecked(true);
        QVERIFY2(!listed(mProportionalFamily), "a proportional font is still in the list");
        QVERIFY2(!anyProportionalListed(), "some proportional font is still in the list");
        QVERIFY(listed(mMonospacedFamily));

        mpPreferences->checkBox_monospacedFontsOnly->setChecked(false);
        QVERIFY2(listed(mProportionalFamily), "unticking did not bring the proportional fonts back");
    }

    // The profile's own font is a proportional one: it stays in use, and so every font stays listed
    void test_theFontInUseStaysListedAndUnchanged()
    {
        useDisplayFont(mProportionalFamily);
        openPreferences();
        QCOMPARE(mpPreferences->fontComboBox_displayFont->currentFont().family(), mProportionalFamily);

        mpPreferences->checkBox_monospacedFontsOnly->setChecked(true);
        QCoreApplication::processEvents();
        QCOMPARE(mpPreferences->fontComboBox_displayFont->currentFont().family(), mProportionalFamily);
        QCOMPARE(mpHost->getDisplayFont().family(), mProportionalFamily);
        QVERIFY2(listed(mProportionalFamily), "the font in use was hidden from the list");
        QVERIFY2(anyProportionalListed(), "proportional fonts other than the one in use were left out while it is in use");

        // Once another font is picked, the proportional one goes the way of the others
        mpPreferences->fontComboBox_displayFont->setCurrentIndex(rowOf(mMonospacedFamily));
        QTRY_COMPARE(mpHost->getDisplayFont().family(), mMonospacedFamily);
        QVERIFY2(!listed(mProportionalFamily), "the font that was in use stayed listed after another was picked");
    }

    // A closed list steps through its model, not its view, from the keyboard and the wheel
    void test_stepsWithTheListClosedSkipHiddenFonts()
    {
        useDisplayFont(mMonospacedFamily);
        openPreferences();
        auto* pCombo = mpPreferences->fontComboBox_displayFont;
        mpPreferences->checkBox_monospacedFontsOnly->setChecked(true);
        pCombo->setFocus();
        QTRY_VERIFY(pCombo->hasFocus());

        const auto checkCurrentIsMonospaced = [this, pCombo](const char* how) {
            const QString family = pCombo->currentText();
            QVERIFY2(QFontDatabase::isFixedPitch(family), qPrintable(qsl("%1 went to the proportional font %2").arg(QLatin1String(how), family)));
            QCOMPARE(mpHost->getDisplayFont().family(), pCombo->currentFont().family());
            QVERIFY2(QFontDatabase::isFixedPitch(mpHost->getDisplayFont().family()), qPrintable(qsl("%1 gave the profile a proportional font").arg(QLatin1String(how))));
        };

        QTest::keyClick(pCombo, Qt::Key_Home);
        checkCurrentIsMonospaced("Home");
        for (int step = 0; step < 5; ++step) {
            QTest::keyClick(pCombo, Qt::Key_Down);
            checkCurrentIsMonospaced("Down");
        }
        QTest::keyClick(pCombo, Qt::Key_End);
        checkCurrentIsMonospaced("End");
        QTest::keyClick(pCombo, Qt::Key_Up);
        checkCurrentIsMonospaced("Up");
        QTest::keyClick(pCombo, Qt::Key_PageUp);
        checkCurrentIsMonospaced("Page Up");

        QWheelEvent wheel(QPointF(5, 5), pCombo->mapToGlobal(QPointF(5, 5)), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(pCombo, &wheel);
        checkCurrentIsMonospaced("The wheel");

        // Typing the start of a family's name, closed and then open: the hidden "Bitstream Vera Sans"
        // comes before the listed "Bitstream Vera Sans Mono"
        const QString typed = qsl("bitstream vera s");
        QTest::keyClick(pCombo, Qt::Key_Home);
        QTest::keyClicks(pCombo, typed);
        QCOMPARE(pCombo->currentText(), mMonospacedFamily);
        checkCurrentIsMonospaced("Typing a name");

        QTest::keyClick(pCombo, Qt::Key_Home);
        // Past the interval that joins keys typed in succession into one name
        QTest::qWait(QApplication::keyboardInputInterval() + 100);
        pCombo->showPopup();
        QTRY_VERIFY(pCombo->view()->isVisible());
        QTest::keyClicks(pCombo->view(), typed);
        QTest::keyClick(pCombo->view(), Qt::Key_Return);
        QTRY_VERIFY(!pCombo->view()->isVisible());
        QCOMPARE(pCombo->currentText(), mMonospacedFamily);
        checkCurrentIsMonospaced("Typing a name into the open list");
    }

    // Space opens a closed list, as it does with every font listed
    void test_spaceStillOpensTheList()
    {
        useDisplayFont(mMonospacedFamily);
        openPreferences();
        auto* pCombo = mpPreferences->fontComboBox_displayFont;
        mpPreferences->checkBox_monospacedFontsOnly->setChecked(true);
        pCombo->setFocus();
        QTRY_VERIFY(pCombo->hasFocus());
        QTest::keyClick(pCombo, Qt::Key_Space);
        QTRY_VERIFY2(pCombo->view()->isVisible(), "Space did not open the list");
        pCombo->hidePopup();
    }

    // A style that keeps the wheel off combo boxes, as macOS's does, keeps it off this one too
    void test_theWheelFollowsTheStyle()
    {
        useDisplayFont(mMonospacedFamily);
        openPreferences();
        auto* pCombo = mpPreferences->fontComboBox_displayFont;
        NoWheelStyle style;
        pCombo->setStyle(&style);
        const auto restoreStyle = qScopeGuard([pCombo] {
            pCombo->setStyle(nullptr);
        });
        mpPreferences->checkBox_monospacedFontsOnly->setChecked(true);
        pCombo->setFocus();
        QTRY_VERIFY(pCombo->hasFocus());
        const QString before = pCombo->currentText();
        QWheelEvent wheel(QPointF(5, 5), pCombo->mapToGlobal(QPointF(5, 5)), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(pCombo, &wheel);
        QCOMPARE(pCombo->currentText(), before);
        QCOMPARE(mpHost->getDisplayFont().family(), mMonospacedFamily);
    }

    // The open list scrolls to the font in use, also after the profile's font changed under it
    void test_theOpenListShowsTheFontInUse()
    {
        useDisplayFont(mMonospacedFamily);
        openPreferences();
        auto* pCombo = mpPreferences->fontComboBox_displayFont;
        mpPreferences->checkBox_monospacedFontsOnly->setChecked(true);
        QFont font(mMonospacedFamily, 15);
        QVERIFY(mpHost->setDisplayFont(font).first);
        QTRY_COMPARE(pCombo->currentText(), mMonospacedFamily);
        pCombo->showPopup();
        QTRY_VERIFY(pCombo->view()->isVisible());
        const QRect shown = pCombo->view()->visualRect(pCombo->view()->currentIndex());
        QVERIFY2(pCombo->view()->viewport()->rect().contains(shown), "the font in use is out of sight in the open list");
        QCOMPARE(pCombo->view()->currentIndex().data().toString(), mMonospacedFamily);
        pCombo->hidePopup();
    }

    void test_theChoiceIsRemembered()
    {
        useDisplayFont(mMonospacedFamily);
        openPreferences();
        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_monospacedFontsOnly->setChecked(true);
        QVERIFY2(TestSettings::waitForApply(applySpy), "ticking the box never wrote the settings back");
        QVERIFY(MudletApp::getQSettings()->value(mSettingsKey).toBool());

        delete mpPreferences;
        mpPreferences = nullptr;
        openPreferences();
        QVERIFY2(mpPreferences->checkBox_monospacedFontsOnly->isChecked(), "a new dialog forgot the choice");
        QVERIFY2(!listed(mProportionalFamily), "a new dialog lists proportional fonts though the box is ticked");
    }
};

#include "SettingsMonospacedFontsTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsMonospacedFontsTest)
