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
 * The colour-trigger picker writes its result straight into the TTrigger it was
 * handed and is only ever reached from a button inside the trigger editor's
 * pattern row, so there is no Lua entry point a spec could use: the scripting
 * API can set a colour trigger's ANSI numbers but never goes through the
 * dialog's slider arithmetic or its "ignore"/"default" sentinels.
 */

#include <QDialogButtonBox>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgColorTrigger.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ColorTriggerDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    TTrigger* mpTrigger = nullptr;
    const QString mProfileName = qsl("ColorTrigger-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void resetTrigger()
    {
        mpTrigger->mColorTrigger = false;
        mpTrigger->mColorTriggerFgAnsi = TTrigger::scmIgnored;
        mpTrigger->mColorTriggerBgAnsi = TTrigger::scmIgnored;
        mpTrigger->mColorTriggerFgColor = QColor();
        mpTrigger->mColorTriggerBgColor = QColor();
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(1000), "Could not connect with the host.");

        mpTrigger = new TTrigger(nullptr, mpHost);
        mpTrigger->setRegexCodeList({qsl("^colour$")}, {REGEX_PERL});
        mpTrigger->registerTrigger();
        mpTrigger->setName(qsl("qaColourTrigger"));
    }

    void cleanupTestCase()
    {
        mpTrigger = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_basicColorButtonRecordsItsAnsiNumber_data()
    {
        QTest::addColumn<bool>("isBackground");
        QTest::addColumn<QString>("buttonName");
        QTest::addColumn<int>("ansiNumber");

        QTest::newRow("foreground red") << false << "pushButton_red" << 1;
        QTest::newRow("foreground white") << false << "pushButton_white" << 7;
        QTest::newRow("foreground light magenta") << false << "pushButton_Lmagenta" << 13;
        QTest::newRow("background blue") << true << "pushButton_blue" << 4;
        QTest::newRow("background light black") << true << "pushButton_Lblack" << 8;
    }

    void test_basicColorButtonRecordsItsAnsiNumber()
    {
        QFETCH(bool, isBackground);
        QFETCH(QString, buttonName);
        QFETCH(int, ansiNumber);

        resetTrigger();
        dlgColorTrigger dialog(nullptr, mpTrigger, isBackground);
        auto* button = dialog.findChild<QPushButton*>(buttonName);
        QVERIFY2(button, qPrintable(qsl("the dialog has no %1").arg(buttonName)));
        QVERIFY2(!mpTrigger->mColorTrigger, "the trigger was already a colour trigger before the button was clicked");

        button->click();

        QVERIFY2(mpTrigger->mColorTrigger, "clicking a colour did not turn the trigger into a colour trigger");
        if (isBackground) {
            QCOMPARE(mpTrigger->mColorTriggerBgAnsi, ansiNumber);
            QCOMPARE(mpTrigger->mColorTriggerBgColor, mpHost->getAnsiColor(ansiNumber, true));
            QCOMPARE(mpTrigger->mColorTriggerFgAnsi, TTrigger::scmIgnored);
        } else {
            QCOMPARE(mpTrigger->mColorTriggerFgAnsi, ansiNumber);
            QCOMPARE(mpTrigger->mColorTriggerFgColor, mpHost->getAnsiColor(ansiNumber, false));
            QCOMPARE(mpTrigger->mColorTriggerBgAnsi, TTrigger::scmIgnored);
        }
    }

    // The 6x6x6 cube: ANSI 16 + 36r + 6g + b, painted on a 51-per-step ramp
    void test_rgbSlidersPickFromThe216ColorCube()
    {
        resetTrigger();
        dlgColorTrigger dialog(nullptr, mpTrigger, false);

        dialog.horizontalSlider_red->setValue(3);
        dialog.horizontalSlider_green->setValue(2);
        dialog.horizontalSlider_blue->setValue(1);
        QCOMPARE(dialog.label_rgbValue->text(), qsl("[137]"));

        dialog.pushButton_setUsingRgbValue->click();

        QCOMPARE(mpTrigger->mColorTriggerFgAnsi, 137);
        QCOMPARE(mpTrigger->mColorTriggerFgColor, QColor(153, 102, 51));
        QVERIFY(mpTrigger->mColorTrigger);
    }

    // The 24 greys: ANSI 232 + n, each grey being 10n + 8 on all three channels
    void test_graySliderPicksFromThe24GrayRamp()
    {
        resetTrigger();
        dlgColorTrigger dialog(nullptr, mpTrigger, true);

        dialog.horizontalSlider_gray->setValue(7);
        QCOMPARE(dialog.label_grayValue->text(), qsl("[239]"));

        dialog.pushButton_setUsingGrayValue->click();

        QCOMPARE(mpTrigger->mColorTriggerBgAnsi, 239);
        QCOMPARE(mpTrigger->mColorTriggerBgColor, QColor(78, 78, 78));
        QVERIFY(mpTrigger->mColorTrigger);
    }

    // "Ignore" clears one side only, and the trigger stops being a colour
    // trigger just when neither side is left set
    void test_ignoreClearsOnlyItsOwnSide()
    {
        resetTrigger();
        mpTrigger->mColorTrigger = true;
        mpTrigger->mColorTriggerFgAnsi = 2;
        mpTrigger->mColorTriggerFgColor = mpHost->getAnsiColor(2, false);
        mpTrigger->mColorTriggerBgAnsi = 4;
        mpTrigger->mColorTriggerBgColor = mpHost->getAnsiColor(4, true);

        {
            dlgColorTrigger dialog(nullptr, mpTrigger, true);
            dialog.buttonBox->button(QDialogButtonBox::Ignore)->click();
        }
        QCOMPARE(mpTrigger->mColorTriggerBgAnsi, TTrigger::scmIgnored);
        QVERIFY2(!mpTrigger->mColorTriggerBgColor.isValid(), "ignoring the background left a colour behind");
        QCOMPARE(mpTrigger->mColorTriggerFgAnsi, 2);
        QVERIFY2(mpTrigger->mColorTrigger, "ignoring one side stopped it being a colour trigger while the other side was still set");

        {
            dlgColorTrigger dialog(nullptr, mpTrigger, false);
            dialog.buttonBox->button(QDialogButtonBox::Ignore)->click();
        }
        QCOMPARE(mpTrigger->mColorTriggerFgAnsi, TTrigger::scmIgnored);
        QVERIFY2(!mpTrigger->mColorTrigger, "ignoring both sides left it as a colour trigger");
    }

    void test_defaultButtonRecordsTheDefaultSentinel()
    {
        resetTrigger();
        // start from a concrete colour on both sides so that neither the
        // sentinel nor the cleared colour can be what was there beforehand
        mpTrigger->mColorTrigger = true;
        mpTrigger->mColorTriggerFgAnsi = 2;
        mpTrigger->mColorTriggerFgColor = mpHost->getAnsiColor(2, false);
        mpTrigger->mColorTriggerBgAnsi = 4;
        mpTrigger->mColorTriggerBgColor = mpHost->getAnsiColor(4, true);
        QVERIFY2(mpTrigger->mColorTriggerFgColor.isValid(), "the foreground did not start out carrying a concrete colour");
        QVERIFY2(mpTrigger->mColorTriggerBgColor.isValid(), "the background did not start out carrying a concrete colour");

        {
            dlgColorTrigger dialog(nullptr, mpTrigger, false);
            dialog.buttonBox->button(QDialogButtonBox::Reset)->click();
        }
        QCOMPARE(mpTrigger->mColorTriggerFgAnsi, TTrigger::scmDefault);
        QVERIFY2(!mpTrigger->mColorTriggerFgColor.isValid(), "the default foreground should not carry a concrete colour");

        {
            dlgColorTrigger dialog(nullptr, mpTrigger, true);
            dialog.buttonBox->button(QDialogButtonBox::Reset)->click();
        }
        QCOMPARE(mpTrigger->mColorTriggerBgAnsi, TTrigger::scmDefault);
        QVERIFY2(!mpTrigger->mColorTriggerBgColor.isValid(), "the default background should not carry a concrete colour");
    }

    // Reopening on a trigger that already uses an extended colour has to unfold
    // the extra controls and put the sliders back where that colour came from
    void test_reopeningOnAnExtendedColorRestoresItsControls()
    {
        resetTrigger();
        mpTrigger->mColorTrigger = true;
        mpTrigger->mColorTriggerFgAnsi = 137;

        {
            dlgColorTrigger dialog(nullptr, mpTrigger, false);
            auto* moreColors = dialog.buttonBox->button(QDialogButtonBox::Apply);
            QVERIFY2(moreColors->isChecked() && !moreColors->isEnabled(), "an RGB-cube colour did not unfold the extra controls");
            QCOMPARE(dialog.horizontalSlider_red->value(), 3);
            QCOMPARE(dialog.horizontalSlider_green->value(), 2);
            QCOMPARE(dialog.horizontalSlider_blue->value(), 1);
            QCOMPARE(dialog.label_rgbValue->text(), qsl("[137]"));
        }

        mpTrigger->mColorTriggerBgAnsi = 239;
        {
            dlgColorTrigger dialog(nullptr, mpTrigger, true);
            auto* moreColors = dialog.buttonBox->button(QDialogButtonBox::Apply);
            QVERIFY2(moreColors->isChecked() && !moreColors->isEnabled(), "a grey-ramp colour did not unfold the extra controls");
            QCOMPARE(dialog.horizontalSlider_gray->value(), 7);
            QCOMPARE(dialog.label_grayValue->text(), qsl("[239]"));
        }
    }

    // On a basic colour the extra controls stay folded away until asked for,
    // and "More colors" is deliberately one-shot
    void test_moreColorsUnfoldsOnceForABasicColor()
    {
        resetTrigger();
        mpTrigger->mColorTriggerFgAnsi = 3;

        dlgColorTrigger dialog(nullptr, mpTrigger, false);
        QVERIFY2(dialog.groupBox_rgbScale->isHidden(), "the RGB controls were showing for a basic colour");
        QVERIFY2(dialog.groupBox_grayScale->isHidden(), "the grey controls were showing for a basic colour");

        auto* moreColors = dialog.buttonBox->button(QDialogButtonBox::Apply);
        QVERIFY(moreColors->isEnabled());
        moreColors->click();

        QVERIFY2(!dialog.groupBox_rgbScale->isHidden(), "More colors did not unfold the RGB controls");
        QVERIFY2(!dialog.groupBox_grayScale->isHidden(), "More colors did not unfold the grey controls");
        QVERIFY2(!moreColors->isEnabled(), "More colors is still offered after it has been used");
    }

    // The colour buttons carry their colour only in their stylesheet, so the
    // editor reads it back out with this pair
    void test_buttonStyleSheetColorsRoundTrip()
    {
        const QColor background(0x12, 0x34, 0x56);
        const QString styleSheet = dlgTriggerEditor::generateButtonStyleSheet(background);

        QCOMPARE(dlgTriggerEditor::parseButtonStyleSheetColors(styleSheet, false), background);
        const QColor foreground = dlgTriggerEditor::parseButtonStyleSheetColors(styleSheet, true);
        QVERIFY2(foreground.isValid(), "no readable foreground colour was generated for the button");
        QVERIFY2(foreground != background, "the generated foreground is the same as the background, so the label would be unreadable");

        // A disabled swatch keeps its lightness but is desaturated, so the
        // number it stands for must still be readable back off it
        const QString disabledStyleSheet = dlgTriggerEditor::generateButtonStyleSheet(background, false);
        QVERIFY2(disabledStyleSheet != styleSheet, "disabling the swatch made no difference to it");
        QVERIFY(dlgTriggerEditor::parseButtonStyleSheetColors(disabledStyleSheet, false).isValid());
        // the stylesheet names the colour, so it resolves through the CSS list rather than Qt::darkGray
        QCOMPARE(dlgTriggerEditor::parseButtonStyleSheetColors(disabledStyleSheet, true), QColor(qsl("darkGray")));

        QVERIFY2(dlgTriggerEditor::generateButtonStyleSheet(QColor()).isEmpty(), "an invalid colour still produced a stylesheet");
        QVERIFY2(!dlgTriggerEditor::parseButtonStyleSheetColors(QString(), false).isValid(), "an empty stylesheet produced a colour");
        QVERIFY2(!dlgTriggerEditor::parseButtonStyleSheetColors(qsl("QPushButton {border: 1px solid #8f8f91;}"), false).isValid(), "a stylesheet with no colour produced one");
    }
};

#include "ColorTriggerDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(ColorTriggerDialogTest)
