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

// The Mudlet.ini values the main window reads through HostManager::readSettings() as it starts - the
// pool's mutes and two process-wide statics - and writes back as it closes. HeadlessHostSmokeTest covers
// the same values for a pool with no main window. Also whether the toolbar's icons are grey, which the
// main window reads itself.

#include <QtTest/QtTest>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QScopeGuard>
#include <QSettings>
#include <QTemporaryDir>

#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "MudletMedia.h"
#include "PortableModeTestHelper.h"
#include "TMap.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MudletIniLateSettingsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    QString iniPath() const { return qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()); }

    // The most saturated solid pixel of Map's glyph on the main toolbar, or -1 with no glyph to read
    static qreal mapSaturation()
    {
        const QAction* pMap = mudlet::self()->findChild<QAction*>(qsl("map_action"));
        const QImage image = pMap ? pMap->icon().pixmap(QSize(64, 64)).toImage().convertToFormat(QImage::Format_ARGB32) : QImage();
        if (image.isNull()) {
            return -1.0;
        }
        qreal most = 0.0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                most = pixel.alphaF() > 0.95 ? std::max(most, static_cast<qreal>(pixel.hslSaturationF())) : most;
            }
        }
        return most;
    }

private slots:
    void initTestCase()
    {
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        // Each unlike its default, and in place before the main window reads anything. The last is what every
        // build with the colored icons experiment wrote for a player who never ticked it.
        QFile ini(iniPath());
        QVERIFY(ini.open(QIODevice::WriteOnly | QIODevice::Text));
        ini.write("[General]\n"
                  "enableMuteAPI=true\n"
                  "enableMuteGame=true\n"
                  "reportMapIssuesToConsole=true\n"
                  "storePasswordsSecurely=false\n"
                  "colouredIcons=false\n");
        ini.close();
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getQSettings()->fileName(), iniPath());
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_theMainWindowReadsMudletIniThroughItsPool()
    {
        QCOMPARE(MudletApp::storingPasswordsSecurely(), false);
        QCOMPARE(HostManager::self()->media().apiMuted(), true);
        QCOMPARE(HostManager::self()->media().gameMuted(), true);
        QCOMPARE(TMap::smShowMapAuditErrors, true);
    }

    // Grey only for a key of its own, as the experiment's old "colouredIcons=false" above was nobody's choice
    void test_theIconsAreInColourUnlessMudletIniAsksForGrey()
    {
        mudlet* pMudlet = mudlet::self();
        QSettings* settings = MudletApp::getQSettings();
        QVERIFY(settings);
        const auto restore = qScopeGuard([pMudlet, settings]() {
            pMudlet->setColouredIcons(true);
            settings->remove(qsl("greyscaleIcons"));
        });
        QVERIFY2(pMudlet->colouredIcons(), "a Mudlet.ini with colouredIcons=false and no greyscaleIcons started the icons grey");
        QVERIFY2(mapSaturation() > 0.3, qPrintable(qsl("Map on the toolbar started grey (saturation %1)").arg(QString::number(mapSaturation()))));

        // The toolbar is inked before Mudlet.ini is read, so reading grey has to ink it again
        settings->setValue(qsl("greyscaleIcons"), true);
        pMudlet->readLateSettings(*settings);
        QVERIFY2(!pMudlet->colouredIcons(), "greyscaleIcons=true in Mudlet.ini left the icons in colour");
        QVERIFY2(mapSaturation() >= 0.0 && mapSaturation() < 0.1, qPrintable(qsl("greyscaleIcons=true left Map on the toolbar in colour (saturation %1)").arg(QString::number(mapSaturation()))));

        settings->setValue(qsl("greyscaleIcons"), false);
        pMudlet->readLateSettings(*settings);
        QVERIFY2(pMudlet->colouredIcons(), "greyscaleIcons=false in Mudlet.ini left the icons grey");
        QVERIFY2(mapSaturation() > 0.3, qPrintable(qsl("greyscaleIcons=false left Map on the toolbar grey (saturation %1)").arg(QString::number(mapSaturation()))));

        // Gone first, so what is read back below can only be what writeSettings() put there
        settings->remove(qsl("greyscaleIcons"));
        pMudlet->setColouredIcons(false);
        pMudlet->writeSettings();
        QCOMPARE(settings->status(), QSettings::NoError);
        const QSettings written(iniPath(), QSettings::IniFormat);
        QVERIFY2(written.value(qsl("greyscaleIcons"), false).toBool(), "grey icons were not written to Mudlet.ini as greyscaleIcons=true");
    }

    // What writeSettings() saves as Mudlet closes is what was read, so a value the main window failed
    // to read would overwrite the player's choice with the default
    void test_writingSettingsKeepsThePlayersChoices()
    {
        const QList<QPair<QString, bool>> expected{{qsl("enableMuteAPI"), true}, {qsl("enableMuteGame"), true}, {qsl("reportMapIssuesToConsole"), true}, {qsl("storePasswordsSecurely"), false}};
        // Gone first, so what is read back below can only be what writeSettings() put there
        QSettings* settings = MudletApp::getQSettings();
        QVERIFY(settings);
        QCOMPARE(settings->group(), QString());
        for (const auto& [key, value] : expected) {
            settings->remove(key);
        }
        mudlet::self()->writeSettings();
        QCOMPARE(settings->status(), QSettings::NoError);
        const QSettings written(iniPath(), QSettings::IniFormat);
        for (const auto& [key, value] : expected) {
            QVERIFY2(written.contains(key), qPrintable(key));
            QCOMPARE(written.value(key).toBool(), value);
        }
    }

    // Closed while maximized, Mudlet has to come back un-maximizable to the size it had before
    void test_aMaximizedWindowKeepsItsNormalSizeAcrossARestart()
    {
        mudlet* pMudlet = mudlet::self();
        pMudlet->show();
        pMudlet->setWindowState(Qt::WindowNoState);
        const QSize normalSize(640, 480);
        pMudlet->resize(normalSize);
        QTRY_COMPARE(pMudlet->size(), normalSize);
        pMudlet->setWindowState(Qt::WindowMaximized);
        QTRY_VERIFY(pMudlet->size() != normalSize);

        pMudlet->writeSettings();
        QSettings* settings = MudletApp::getQSettings();
        QVERIFY(settings->value(qsl("maximized")).toBool());

        pMudlet->setWindowState(Qt::WindowNoState);
        pMudlet->resize(normalSize / 2);
        QTRY_COMPARE(pMudlet->size(), normalSize / 2);
        pMudlet->readLateSettings(*settings);
        QTRY_VERIFY(pMudlet->windowState() & Qt::WindowMaximized);

        pMudlet->setWindowState(Qt::WindowNoState);
        QTRY_COMPARE(pMudlet->size(), normalSize);
    }

    // An older Mudlet sharing this Mudlet.ini writes pos and size but leaves the newer geometry behind
    void test_aWindowLastPlacedByAnOlderMudletComesBackWhereThatPutIt()
    {
        mudlet* pMudlet = mudlet::self();
        pMudlet->setWindowState(Qt::WindowNoState);
        pMudlet->resize(640, 480);
        QTRY_COMPARE(pMudlet->size(), QSize(640, 480));
        pMudlet->writeSettings();

        QSettings* settings = MudletApp::getQSettings();
        QVERIFY(settings->contains(qsl("mainWindowGeometry")));
        const QSize olderSize(500, 400);
        settings->setValue(qsl("size"), olderSize);
        settings->setValue(qsl("maximized"), false);
        pMudlet->readLateSettings(*settings);
        QTRY_COMPARE(pMudlet->size(), olderSize);
    }
};

#include "MudletIniLateSettingsTest.moc"
MUDLET_GROUPED_TEST_MAIN(MudletIniLateSettingsTest)
