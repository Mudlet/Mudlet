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
 * The mapper's own controls in the settings: the colour buttons that stand for
 * the map's colours, and the room size that the 2D map is drawn at. Both write
 * somewhere other than the Host alone - a button without alpha carries its
 * colour as a stylesheet, and the room size has to reach the widget as well as
 * the profile.
 *
 * Run with: ctest -R SettingsMapControlsTest -V
 */

#include <algorithm>

#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QApplication>
#include <QImage>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QSpinBox>
#include <QTimer>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "dlgMapper.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class SettingsMapControlsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    QPointer<QTimer> mpColorDialogTimer;
    const QString mProfileName = qsl("SettingsMapControls-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // Reports rather than asserting: a QVERIFY here would only return from this
    // helper, leaving the caller reading widgets off a dialog that never opened.
    bool openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        return QTest::qWaitForWindowExposed(mpPreferences);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
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
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        delete mpColorDialogTimer;
        delete mpPreferences;
        mpPreferences = nullptr;
        if (mpHost) {
            mpHost->waitForProfileSave();
        }
    }

    // Resetting the map colours has to reach the buttons that stand for them,
    // not only the profile: the buttons are the only thing in the dialog that
    // says which colour the map will be drawn in (#1049).
    void test_resettingTheMapColoursShowsThemOnTheButtons()
    {
        const QColor priorBlack = mpHost->mBlack_2;
        const QColor priorWhite = mpHost->mLightWhite_2;
        const auto restoreColours = qScopeGuard([this, priorBlack, priorWhite]() {
            mpHost->mBlack_2 = priorBlack;
            mpHost->mLightWhite_2 = priorWhite;
        });

        // The dialog draws the buttons from the profile as it opens, so colours
        // picked before it opens are the ones they come up showing. Two
        // different ones, so a button drawn from the wrong field still shows.
        const QColor pickedBlack(17, 34, 51);
        const QColor pickedWhite(204, 187, 170);
        mpHost->mBlack_2 = pickedBlack;
        mpHost->mLightWhite_2 = pickedWhite;
        QVERIFY2(openPreferences(), "the settings dialog never appeared");

        QPushButton* pBlack = mpPreferences->pushButton_black_2;
        QPushButton* pWhite = mpPreferences->pushButton_Lwhite_2;
        QVERIFY2(pBlack->styleSheet().contains(pickedBlack.name()), qPrintable(qsl("the map colour button does not show the colour the profile holds: '%1'").arg(pBlack->styleSheet())));
        QVERIFY2(pWhite->styleSheet().contains(pickedWhite.name()), qPrintable(qsl("the map colour button does not show the colour the profile holds: '%1'").arg(pWhite->styleSheet())));

        mpPreferences->reset_colors_button_2->click();

        QCOMPARE(mpHost->mBlack_2, QColor(QColorConstants::Black));
        QCOMPARE(mpHost->mLightWhite_2, QColor(QColorConstants::White));
        QVERIFY2(pBlack->styleSheet().contains(QColor(QColorConstants::Black).name()), qPrintable(qsl("the reset left the map colour button showing the old colour: '%1'").arg(pBlack->styleSheet())));
        QVERIFY2(pWhite->styleSheet().contains(QColor(QColorConstants::White).name()), qPrintable(qsl("the reset left the map colour button showing the old colour: '%1'").arg(pWhite->styleSheet())));
    }

    // The map draws environments 257 to 272 from its own copy of the sixteen
    // ANSI map colours, so a reset that skips that copy leaves the rooms drawn
    // in the old colours while the buttons show the defaults (#10679).
    void test_resettingTheMapColoursReachesTheMapEnvironments()
    {
        const QColor priorRed = mpHost->mRed_2;
        const auto restoreColours = qScopeGuard([this, priorRed]() {
            mpHost->mRed_2 = priorRed;
            mpHost->mpMap->restore16ColorSet();
        });

        const QColor pickedRed(10, 20, 30);
        mpHost->mRed_2 = pickedRed;
        mpHost->mpMap->restore16ColorSet();
        QCOMPARE(mpHost->mpMap->mCustomEnvColors.value(257), pickedRed);
        QVERIFY2(openPreferences(), "the settings dialog never appeared");

        mpPreferences->reset_colors_button_2->click();

        QCOMPARE(mpHost->mRed_2, QColor(QColorConstants::DarkRed));
        QCOMPARE(mpHost->mpMap->mCustomEnvColors.value(257), QColor(QColorConstants::DarkRed));
    }

    // Must run before any case that opens the mapper: it needs a profile whose mapper was never created
    void test_theMapBackgroundCanBePickedBeforeTheMapIsOpened()
    {
        TMap* pMap = mpHost->mpMap.data();
        QVERIFY2(!pMap->mapper(), "the profile already has a mapper, so this cannot pick the colour without one");

        const QColor priorBackground = mpHost->mBgColor_2;
        const int areaId = pMap->mpRoomDB->addArea(qsl("Map background test"));
        QVERIFY(areaId > 0);
        const auto restore = qScopeGuard([this, pMap, areaId, priorBackground]() {
            mpHost->mBgColor_2 = priorBackground;
            pMap->mpRoomDB->removeArea(areaId);
        });

        const QColor picked(12, 34, 56);
        QVERIFY(mpHost->mBgColor_2 != picked);
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        mpColorDialogTimer = TestSettings::pickInTheNextColorDialog(this, picked);
        mpPreferences->pushButton_background_color_2->click();
        QCOMPARE(mpHost->mBgColor_2, picked);

        // With no room to centre on, the 2D map draws a message rather than its background
        const int roomId = 1;
        QVERIFY(pMap->addRoom(roomId) && pMap->setRoomArea(roomId, areaId) && pMap->setRoomCoordinates(roomId, 0, 0, 0));
        mpHost->showHideOrCreateMapper(true);
        QVERIFY2(pMap->mapper() && pMap->mapper()->mp2dMap, "opening the map left no 2D map to draw the background");
        T2DMap* p2dMap = pMap->mapper()->mp2dMap;
        p2dMap->resize(200, 200);
        QImage frame(p2dMap->size(), QImage::Format_ARGB32);
        frame.fill(Qt::magenta);
        p2dMap->render(&frame);
        QCOMPARE(frame.pixelColor(frame.width() - 5, frame.height() - 5), picked);
    }

    // macOS's native colour panel hands the activation to the main window as
    // it closes, cancelled or not, so after every colour dialog the settings
    // take it back and the well that was clicked takes the keyboard. The
    // dialog itself is modal and native, so it is the step after it that is
    // tested here, with the main window holding the activation as the panel
    // leaves it.
    void test_theSettingsTakeTheActivationBackAfterAColourDialog()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        mpPreferences->setTab(qsl("mapper"));
        QPushButton* pWell = mpPreferences->pushButton_mapInfoBg;
        QVERIFY2(pWell->isVisible(), "the map info background well is not on show");

        mudlet::self()->show();
        mudlet::self()->activateWindow();
        QTRY_COMPARE(QApplication::activeWindow(), mudlet::self());
        QVERIFY(QApplication::focusWidget() != pWell);

        mpPreferences->returnFromColourDialog(pWell);
        QTRY_COMPARE(QApplication::activeWindow(), mpPreferences);
        QCOMPARE(QApplication::focusWidget(), pWell);
    }

    // The spin box counts in tenths of a room, and the profile and the 2D map
    // have to end up holding the same size - the widget was once handed the
    // spin box's own number, so the map drew rooms ten times too big (#8513).
    void test_theRoomSizeControlGivesTheMapTheSameSizeAsTheProfile()
    {
        // Only when there is none: on an existing mapper this toggles it hidden
        if (!mpHost->mpMap->mapper()) {
            mpHost->showHideOrCreateMapper(true);
        }
        QVERIFY2(mpHost->mpMap && mpHost->mpMap->mapper() && mpHost->mpMap->mapper()->mp2dMap, "the mapper left no 2D map for the room size to reach");
        T2DMap* p2dMap = mpHost->mpMap->mapper()->mp2dMap;

        const double priorSize = p2dMap->rSize;
        // setRoomSize() writes the profile as well, so this puts both back
        const auto restoreSize = qScopeGuard([p2dMap, priorSize]() {
            p2dMap->setRoomSize(priorSize);
        });

        const int tenths = 7;
        const double wanted = tenths / 10.0;
        QVERIFY(!qFuzzyCompare(priorSize, wanted));

        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        mpPreferences->spinBox_roomSize->setValue(tenths);

        // The number reaches both through a float, so a tenth of it survives
        // only to float precision. setRoomSize() writes the profile itself, so
        // each has to be pinned to the size asked for rather than to the other.
        QVERIFY2(qAbs(mpHost->mRoomSize - wanted) < 1.0e-6, qPrintable(qsl("the profile was given a room size of %1 rather than %2").arg(mpHost->mRoomSize).arg(wanted)));
        QVERIFY2(qAbs(p2dMap->rSize - wanted) < 1.0e-6, qPrintable(qsl("the map was given a room size of %1 rather than %2").arg(p2dMap->rSize).arg(wanted)));
    }

    // saveJsonMap() hands a script the reason, so the dialog's own export should not keep it from the player
    void test_aJsonExportThatFailsSaysWhy()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString name = qsl("missing&folder/exported.json");
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto& console = mpHost->mainConsoleView()->buffer;
        const int mark = std::max(0, console.getLastLineNumber());

        // A native dialog has no file name field to type into
        const bool priorNative = QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);
        const auto restoreNative = qScopeGuard([priorNative]() {
            QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, priorNative);
        });
        mpPreferences->pushButton_saveMap->click();
        QFileDialog* pDialog = nullptr;
        QVERIFY(QTest::qWaitFor(
                [&]() {
                    pDialog = mpPreferences->findChild<QFileDialog*>();
                    return pDialog && pDialog->isVisible();
                },
                5s));
        // Typed, as selectFile() given a path into a missing folder keeps only the name
        auto* pName = pDialog->findChild<QLineEdit*>(qsl("fileNameEdit"));
        QVERIFY(pName);
        pDialog->setDirectory(dir.path());
        pName->setText(name);
        QVERIFY2(pDialog->selectedFiles().value(0).endsWith(name), qPrintable(pDialog->selectedFiles().value(0)));
        static_cast<QDialog*>(pDialog)->accept();

        // As one line, since the console wraps long messages
        const auto shown = [&]() {
            QStringList lines;
            for (int line = mark; line <= console.getLastLineNumber(); ++line) {
                lines << console.line(line);
            }
            return lines.join(QChar::Space).simplified();
        };
        const auto saysWhy = [&]() {
            return shown().contains(qsl("could not open save file"));
        };
        QVERIFY2(QTest::qWaitFor(saysWhy, 5s), qPrintable(qsl("the console does not say why: '%1'").arg(shown())));
        // Spaces dropped, since a wrap can fall inside the path
        QVERIFY2(shown().remove(QChar::Space).contains(qsl("/%1\"").arg(name)), qPrintable(qsl("the console does not show the path as typed: '%1'").arg(shown())));
    }
};

#include "SettingsMapControlsTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsMapControlsTest)
