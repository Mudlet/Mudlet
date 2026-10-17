/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

// isUnsaved(), signal_mapLabelsChanged and the debug console's LuaWarning lines
// cannot be seen from Lua, so these checks are a functional test, not a spec

#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TDebug.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
class RecordingDebugSink : public TDebug::Sink
{
public:
    QStringList lines;

    ~RecordingDebugSink()
    {
        if (TDebug::sink() == this) {
            TDebug::setSink(nullptr);
        }
    }

    void printDebugLine(const QString& text, const QColor&, const QColor&, const QString&) override { lines.append(text); }
};
} // namespace

class MapLabelDeleteTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mImageDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Map-Label-Delete-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(mImageDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mHostname);

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
        QVERIFY(mpHost->mpMap);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        delete mudlet::self();
        TestProfile::removeProfileDirectory(mHostname);
    }

    void test_deletingAnImageLabelAtTheOriginMarksTheMapUnsaved()
    {
        const QString imagePath = qsl("%1/label.png").arg(mImageDir.path());
        QImage image(8, 8, QImage::Format_ARGB32);
        image.fill(Qt::red);
        QVERIFY(image.save(imagePath));

        QVERIFY(runLua(qsl("mapLabelDeleteTestArea = assert(addAreaName(\"Map-Label-Delete-Area\"))\n"
                           "mapLabelDeleteTestLabel = createMapImageLabel(mapLabelDeleteTestArea, [[%1]], 0, 0, 0, 2, 2, 10, false)\n"
                           "assert(mapLabelDeleteTestLabel >= 0)")
                               .arg(imagePath)));

        QSignalSpy labelsChangedSpy(mpHost->mpMap.data(), &TMap::signal_mapLabelsChanged);
        mpHost->mpMap->resetUnsaved();
        QVERIFY(runLua(qsl("assert(deleteMapLabel(mapLabelDeleteTestArea, mapLabelDeleteTestLabel) == true)")));
        QVERIFY(runLua(qsl("assert(next(getMapLabel(mapLabelDeleteTestArea, mapLabelDeleteTestLabel) or {}) == nil)")));
        QVERIFY2(mpHost->mpMap->isUnsaved(), "deleting a real image label at 0,0,0 left the map marked as saved");
        QCOMPARE(labelsChangedSpy.count(), 1);
    }

    void test_menuEventRefusalsWarnInTheDebugConsole()
    {
        RecordingDebugSink sink;
        auto* previousSink = TDebug::sink();
        const bool previousDebugMode = TDebug::smDebugMode;
        const bool previousCategory = TDebug::categoryEnabled(TDebug::Category::LuaWarning);
        TDebug::setSink(&sink);
        TDebug::smDebugMode = true;
        TDebug::setCategoryEnabled(TDebug::Category::LuaWarning, true);

        QVERIFY(runLua(qsl("assert(removeMapEvent(\"MapLabelDeleteTestNoSuchEvent\") == nil)")));
        QVERIFY(runLua(qsl("assert(removeCommandLineMenuEvent(\"MapLabelDeleteTestNoSuchItem\") == nil)")));

        TDebug::setCategoryEnabled(TDebug::Category::LuaWarning, previousCategory);
        TDebug::smDebugMode = previousDebugMode;
        TDebug::setSink(previousSink);

        const QString all = sink.lines.join(QChar::LineFeed);
        QVERIFY2(all.contains(qsl("removeMapEvent")), "removeMapEvent's refusal wrote no LuaWarning line");
        QVERIFY2(all.contains(qsl("removeCommandLineMenuEvent")), "removeCommandLineMenuEvent's refusal wrote no LuaWarning line");
    }

private:
    bool runLua(const QString& script) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(script); }
};

#include "MapLabelDeleteTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapLabelDeleteTest)
