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

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "HostManager.h"
#include "PortableModeTestHelper.h"
#include "TConsoleModel.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TMapViewFrontend.h"
#include "TMapViewsFrontend.h"
#include "TMxpFrameManager.h"
#include "TNullAppFrontend.h"
#include "TNullConsoleFrontend.h"
#include "TPrintSink.h"
#include "TRoomDB.h"

#include "GroupedTest.h"

namespace {

QString call(const char* name, const QStringList& args = {})
{
    return qsl("%1(%2)").arg(QLatin1String(name), args.join(qsl(", ")));
}

QString flag(const bool value)
{
    return value ? qsl("true") : qsl("false");
}

QString number(const qreal value)
{
    return QString::number(value);
}

QString rect(const QRect& value)
{
    return qsl("%1 %2 %3x%4").arg(value.x()).arg(value.y()).arg(value.width()).arg(value.height());
}

QString size(const QSize& value)
{
    return qsl("%1x%2").arg(value.width()).arg(value.height());
}

// Writes down what reaches a frame's sink, then passes it on
class RecordingSink final : public TPrintSink
{
public:
    QStringList& mCalls;
    TPrintSink* mpSink = nullptr;

    explicit RecordingSink(QStringList& calls)
    : mCalls(calls)
    {
    }

    void printFormatted(const QString& text, const std::vector<TChar>& formatting, const TLinkStore& sourceLinkStore) override
    {
        mCalls << call("printFormatted", {text});
        mpSink->printFormatted(text, formatting, sourceLinkStore);
    }
    void discardAll() override
    {
        mCalls << call("discardAll");
        mpSink->discardAll();
    }
    void discardLastLine() override
    {
        mCalls << call("discardLastLine");
        mpSink->discardLastLine();
    }
};

// Writes down what TMxpFrameManager asks of the frames, then does what the null view does. It
// measures the main console as a view would, so frames are laid out against a real size.
class RecordingMxpFrames final : public TMxpFrameFrontend
{
public:
    static constexpr QSize scmMainWindowSize{800, 600};

    // Written by the const queries too
    mutable QStringList mCalls;

    RecordingMxpFrames(TMxpFrameFrontend& frames, Host* pHost)
    : mFrames(frames)
    , mpHost(pHost)
    {
    }

    void createInternalFrame(const QString& name, const QString& hostName, const QString& title, const QRect& geometry, bool showHeader, bool scrolling) override
    {
        mCalls << call("createInternalFrame", {name, hostName, title, rect(geometry), flag(showHeader), flag(scrolling)});
        mFrames.createInternalFrame(name, hostName, title, geometry, showHeader, scrolling);
    }
    std::optional<QSize> createExternalFrame(const QString& name, const QString& title, const QSize& frameSize, bool scrolling) override
    {
        mCalls << call("createExternalFrame", {name, title, size(frameSize), flag(scrolling)});
        return mFrames.createExternalFrame(name, title, frameSize, scrolling);
    }
    void createTabFrame(const QString& name, const QString& title, const QString& parentName, const QSize& frameSize, bool scrolling, bool select) override
    {
        mCalls << call("createTabFrame", {name, title, parentName, size(frameSize), flag(scrolling), flag(select)});
        mFrames.createTabFrame(name, title, parentName, frameSize, scrolling, select);
    }
    bool removeFromParentTabs(const QString& name, const QString& parentName) override
    {
        mCalls << call("removeFromParentTabs", {name, parentName});
        return mFrames.removeFromParentTabs(name, parentName);
    }
    void destroyFrame(const QString& name) override
    {
        mCalls << call("destroyFrame", {name});
        mFrames.destroyFrame(name);
    }
    void showFrame(const QString& name) override { mCalls << call("showFrame", {name}); }
    void focusFrame(const QString& name) override { mCalls << call("focusFrame", {name}); }
    void setGeometry(const QString& name, const QRect& geometry) override { mCalls << call("setGeometry", {name, rect(geometry)}); }
    void reportSize() override
    {
        mCalls << call("reportSize");
        mpHost->mMxpFrameManager.setMainConsoleSize(scmMainWindowSize, scmMainWindowSize);
    }
    TPrintSink* sink(const QString& name) const override
    {
        mCalls << call("sink", {name});
        mSink.mpSink = mFrames.sink(name);
        return mSink.mpSink ? &mSink : nullptr;
    }
    bool hasFrameWidget(const QString& name) const override
    {
        mCalls << call("hasFrameWidget", {name});
        return mFrames.hasFrameWidget(name);
    }

private:
    TMxpFrameFrontend& mFrames;
    Host* mpHost;
    mutable RecordingSink mSink{mCalls};
};

// Writes down what core code asks of the view, then does what the null view does, so the windows it
// makes still get a model and a place in the window registry for the calls that follow.
class RecordingConsoleFrontend final : public TNullConsoleFrontend
{
public:
    explicit RecordingConsoleFrontend(Host* pHost)
    : TNullConsoleFrontend(pHost)
    , mFrames(TNullConsoleFrontend::mxpFrames(), pHost)
    {
    }

    QStringList mCalls;
    RecordingMxpFrames mFrames;

    TMxpFrameFrontend& mxpFrames() override { return mFrames; }
    const TMxpFrameFrontend& mxpFrames() const override { return mFrames; }

    void createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough) override
    {
        mCalls << call("createLabel", {windowname, name, QString::number(x), QString::number(y), QString::number(width), QString::number(height), flag(fillBackground), flag(clickThrough)});
        TNullConsoleFrontend::createLabel(windowname, name, x, y, width, height, fillBackground, clickThrough);
    }
    bool setLabelText(const QString& name, const QString& text) override
    {
        mCalls << call("setLabelText", {name, text});
        return TNullConsoleFrontend::setLabelText(name, text);
    }
    bool resizeLabel(const QString& name, int width, int height) override
    {
        mCalls << call("resizeLabel", {name, QString::number(width), QString::number(height)});
        return TNullConsoleFrontend::resizeLabel(name, width, height);
    }
    bool moveLabel(const QString& name, int x, int y) override
    {
        mCalls << call("moveLabel", {name, QString::number(x), QString::number(y)});
        return TNullConsoleFrontend::moveLabel(name, x, y);
    }
    void addMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height) override
    {
        mCalls << call("addMiniConsole", {windowname, name, QString::number(x), QString::number(y), QString::number(width), QString::number(height)});
        TNullConsoleFrontend::addMiniConsole(windowname, name, x, y, width, height);
    }
    bool resizeSubConsole(const QString& name, int width, int height) override
    {
        mCalls << call("resizeSubConsole", {name, QString::number(width), QString::number(height)});
        return TNullConsoleFrontend::resizeSubConsole(name, width, height);
    }
    bool moveSubConsole(const QString& name, int x, int y) override
    {
        mCalls << call("moveSubConsole", {name, QString::number(x), QString::number(y)});
        return TNullConsoleFrontend::moveSubConsole(name, x, y);
    }
    void applyBorders() override
    {
        mCalls << call("applyBorders");
        TNullConsoleFrontend::applyBorders();
    }
    void showNewLines() override
    {
        mCalls << call("showNewLines");
        TNullConsoleFrontend::showNewLines();
    }
    void showCommandEcho(const TConsoleModel::CommandEcho& echo) override
    {
        mCalls << call("showCommandEcho");
        TNullConsoleFrontend::showCommandEcho(echo);
    }
    void markSelectionDirty() override
    {
        mCalls << call("markSelectionDirty");
        TNullConsoleFrontend::markSelectionDirty();
    }
};

// Answers with values the null view never gives, so a script that sees them got them from here.
class RecordingAppFrontend final : public TNullAppFrontend
{
public:
    // Written by profileTabIndex(), which is const
    mutable QStringList mCalls;

    bool openWebPage(const QString& path) override
    {
        mCalls << call("openWebPage", {path});
        return true;
    }
    void showNotification(const QString& title, const QString& text, std::optional<int> expiryMs) override
    {
        mCalls << call("showNotification", {title, text, expiryMs ? QString::number(*expiryMs) : qsl("none")});
    }
    int profileTabIndex(const QString& name) const override
    {
        mCalls << call("profileTabIndex", {name});
        return 2;
    }
};

// In the mapper's place, which nothing fills on a profile with no main window. Its answers are
// ones no mapper gives on its own, so a script that sees them got them from here.
class RecordingMapView final : public QObject, public TMapViewFrontend
{
public:
    // Written by the const queries too
    mutable QStringList mCalls;
    QSet<int> mSelection;
    bool mSelecting = false;
    int mShownAreaId = 0;
    bool mShowing3D = false;

    bool onScreen() const override
    {
        mCalls << call("onScreen");
        return true;
    }
    void showMapProgress(const QString& label, bool cancelable) override { mCalls << call("showMapProgress", {label, flag(cancelable)}); }
    void setMapProgressLabel(const QString& text) override { mCalls << call("setMapProgressLabel", {text}); }
    void setMapProgressRange(int minimum, int maximum) override { mCalls << call("setMapProgressRange", {QString::number(minimum), QString::number(maximum)}); }
    void setMapProgressValue(int value) override { mCalls << call("setMapProgressValue", {QString::number(value)}); }
    int mapProgressMaximum() const override
    {
        mCalls << call("mapProgressMaximum");
        return 0;
    }
    void setMapProgressCancelable(bool cancelable) override { mCalls << call("setMapProgressCancelable", {flag(cancelable)}); }
    void hideMapProgress() override { mCalls << call("hideMapProgress"); }
    bool isMapProgressVisible() const override
    {
        mCalls << call("isMapProgressVisible");
        return false;
    }

    bool selectingRooms() const override
    {
        mCalls << call("selectingRooms");
        return mSelecting;
    }
    QSet<int> selectedRooms() const override
    {
        mCalls << call("selectedRooms");
        return mSelection;
    }
    int centerSelectedRoom() const override
    {
        mCalls << call("centerSelectedRoom");
        return 7;
    }
    void clearRoomSelection() override
    {
        mCalls << call("clearRoomSelection");
        mSelection.clear();
    }

    int shownAreaId() const override
    {
        mCalls << call("shownAreaId");
        return mShownAreaId;
    }
    std::pair<bool, QString> setMapZoom(qreal zoom, int areaId) override
    {
        mCalls << call("setMapZoom", {number(zoom), QString::number(areaId)});
        return zoom < 1.0 ? std::pair{false, qsl("routed zoom refusal")} : std::pair{true, QString()};
    }
    std::pair<bool, QString> exportAreaToImage(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels) override
    {
        mCalls << call("exportAreaToImage", {QString::number(areaId), filePath, zLevel ? QString::number(*zLevel) : qsl("none"), number(zoom), flag(exportAllZLevels)});
        return exportAllZLevels ? std::pair{true, QString()} : std::pair{false, qsl("routed export refusal")};
    }

    void show3DView(bool shown) override
    {
        mCalls << call("show3DView", {flag(shown)});
        mShowing3D = shown;
    }
    bool showing3DView() const override
    {
        mCalls << call("showing3DView");
        return mShowing3D;
    }
    void recreate3DView() override { mCalls << call("recreate3DView"); }
    void shift3DViewCamera(float verticalAngle, float horizontalAngle, float rotationAngle) override
    {
        mCalls << call("shift3DViewCamera", {number(verticalAngle), number(horizontalAngle), number(rotationAngle)});
    }
    void set3DViewCameraPosition(float r, float theta, float phi) override { mCalls << call("set3DViewCameraPosition", {number(r), number(theta), number(phi)}); }
};

// One secondary map view, writing into its manager's list so the order of the two shows
class RecordingSecondaryMapView final : public TSecondaryMapViewFrontend
{
public:
    explicit RecordingSecondaryMapView(QStringList& calls)
    : mCalls(calls)
    {
    }

    QStringList& mCalls;
    int mAcceptedRoomId = 0;

    std::pair<bool, QString> centerOnRoom(int roomId) override
    {
        mCalls << call("centerOnRoom", {QString::number(roomId)});
        return roomId == mAcceptedRoomId ? std::pair{true, QString()} : std::pair{false, qsl("routed centring refusal")};
    }
    std::pair<bool, QString> setZoom(qreal zoom) override
    {
        mCalls << call("setZoom", {number(zoom)});
        return zoom < 1.0 ? std::pair{false, qsl("routed view zoom refusal")} : std::pair{true, QString()};
    }
    int getCurrentAreaId() const override
    {
        mCalls << call("getCurrentAreaId");
        return 21;
    }
    int getCenteredRoomId() const override
    {
        mCalls << call("getCenteredRoomId");
        return 31;
    }
    qreal getZoom() const override
    {
        mCalls << call("getZoom");
        return 2.75;
    }
    int getZLevel() const override
    {
        mCalls << call("getZLevel");
        return -2;
    }
};

// The secondary map views' manager. Of the views it lists, it hands out only 7.
class RecordingMapViews final : public TMapViewsFrontend
{
public:
    // Written by getViewIds(), which is const
    mutable QStringList mCalls;
    RecordingSecondaryMapView mView{mCalls};
    int mAcceptedAreaId = 0;

    std::pair<int, QString> createView(int initialAreaId) override
    {
        mCalls << call("createView", {QString::number(initialAreaId)});
        return initialAreaId == mAcceptedAreaId ? std::pair{7, QString()} : std::pair{0, qsl("routed view refusal")};
    }
    std::pair<bool, QString> closeView(int viewId) override
    {
        mCalls << call("closeView", {QString::number(viewId)});
        return viewId == 7 ? std::pair{true, QString()} : std::pair{false, qsl("routed close refusal")};
    }
    int closeAllViews() override
    {
        mCalls << call("closeAllViews");
        return 2;
    }
    TSecondaryMapViewFrontend* view(int viewId) override
    {
        mCalls << call("view", {QString::number(viewId)});
        return viewId == 7 ? &mView : nullptr;
    }
    QList<int> getViewIds() const override
    {
        mCalls << call("getViewIds");
        return {7, 9};
    }
    void updateAllViews() override { mCalls << call("updateAllViews"); }
    void switchViewsShowingArea(int areaId) override { mCalls << call("switchViewsShowingArea", {QString::number(areaId)}); }
};

} // namespace

// What a Lua UI function asks of the frontend, read from a recording view on a profile with no main
// window: which method it reaches and with which arguments, so a view that is not TConsole and
// mudlet - a remote or web one - can rely on getting the same calls.
class FrontendRoutingContractTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    std::unique_ptr<HostManager> mpHostManager;
    const QString mHostname = qsl("Test-Frontend-Routing");
    Host* mpHost = nullptr;
    RecordingConsoleFrontend* mpRecorder = nullptr;
    RecordingMapView mMapView;
    RecordingMapViews mMapViews;
    int mAreaId = 0;
    int mShownAreaId = 0;

    QString runLua(const QString& chunk)
    {
        const QString wrapped = qsl("routingResult = 'not run'\nlocal ok, err = pcall(function()\n%1\nend)\nroutingResult = ok and 'ok' or tostring(err)\n").arg(chunk);
        if (!mpHost->getLuaInterpreter()->compileAndExecuteScript(wrapped)) {
            return qsl("the chunk did not compile");
        }
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "routingResult");
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    int luaGlobalInt(const char* name)
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const int value = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : -1;
        lua_pop(L, 1);
        return value;
    }

    // AREA in the chunk stands for the id of the area holding the map's rooms
    QString runMapLua(QString chunk) { return runLua(chunk.replace(qsl("AREA"), QString::number(mAreaId))); }

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

        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
        QVERIFY2(HostManager::self()->addHost(mHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        mpHost = HostManager::self()->getHost(mHostname);
        QVERIFY2(mpHost, "The profile is not in the pool.");
        QVERIFY2(!mpHost->hasConsoleView(), "The profile has a console view, so the recording view would never be asked.");

        auto recorder = std::make_unique<RecordingConsoleFrontend>(mpHost);
        mpRecorder = recorder.get();
        mpHost->mpNullConsoleFrontend = std::move(recorder);
        QVERIFY(mpHost->consoleFrontend() == mpRecorder);

        TMap* map = mpHost->mpMap.data();
        QVERIFY2(!map->mapViewFrontend() && !map->mapViewsFrontend(), "The profile has map views, so the recording ones would never be asked.");
        mAreaId = map->mpRoomDB->addArea(qsl("Routed area"));
        mShownAreaId = map->mpRoomDB->addArea(qsl("Routed shown area"));
        QVERIFY(mAreaId > 0 && mShownAreaId > 0);
        QVERIFY(map->addRoom(1) && map->setRoomArea(1, mAreaId) && map->setRoomCoordinates(1, 0, 0, 0));
        QVERIFY(map->addRoom(2) && map->setRoomArea(2, mAreaId) && map->setRoomCoordinates(2, 1, 0, 0));
        QVERIFY(map->mpRoomDB->set2DMapZoom(mShownAreaId, 4.25));
        mMapView.mShownAreaId = mShownAreaId;
        mMapViews.mAcceptedAreaId = mAreaId;
        mMapViews.mView.mAcceptedRoomId = 1;
        map->mpMapViewFrontend = &mMapView;
        map->mpMapViewObject = &mMapView;
        map->mpViewsFrontend = &mMapViews;
        QVERIFY(map->mapViewFrontend() == &mMapView && map->mapViewsFrontend() == &mMapViews);
    }

    void init()
    {
        if (mpRecorder) {
            mpRecorder->mCalls.clear();
            mpRecorder->mFrames.mCalls.clear();
        }
        mMapView.mCalls.clear();
        mMapView.mSelection.clear();
        mMapView.mSelecting = false;
        mMapViews.mCalls.clear();
    }

    void cleanupTestCase()
    {
        mpRecorder = nullptr;
        if (mpHost && mpHost->mpMap) {
            mpHost->mpMap->mpMapViewFrontend = nullptr;
            mpHost->mpMap->mpMapViewObject = nullptr;
            mpHost->mpMap->mpViewsFrontend = nullptr;
        }
        // The profile tears the recording view down as it would the null one
        mpHostManager.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_labelCallsReachTheView()
    {
        // No apostrophes in these chunks: moc misreads a raw string holding one and stops at the next macro
        const QString result = runLua(qsl(R"lua(
createLabel("routedLabel", 10, 20, 300, 40, 1, true)
echo("routedLabel", "hello label")
resizeWindow("routedLabel", 111, 222)
moveWindow("routedLabel", 7, 8)
)lua"));
        QCOMPARE(result, qsl("ok"));

        const QStringList expected{
                qsl("createLabel(main, routedLabel, 10, 20, 300, 40, true, true)"),
                qsl("setLabelText(routedLabel, hello label)"),
                qsl("resizeLabel(routedLabel, 111, 222)"),
                qsl("moveLabel(routedLabel, 7, 8)"),
        };
        QCOMPARE(mpRecorder->mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_miniConsoleGeometryReachesTheView()
    {
        const QString result = runLua(qsl(R"lua(
createMiniConsole("routedConsole", 1, 2, 3, 4)
resizeWindow("routedConsole", 50, 60)
moveWindow("routedConsole", 5, 6)
createMiniConsole("routedConsole", 9, 10, 11, 12)
)lua"));
        QCOMPARE(result, qsl("ok"));

        // Making one that exists moves and resizes it instead
        const QStringList expected{
                qsl("addMiniConsole(, routedConsole, 1, 2, 3, 4)"),
                qsl("resizeSubConsole(routedConsole, 50, 60)"),
                qsl("moveSubConsole(routedConsole, 5, 6)"),
                qsl("resizeSubConsole(routedConsole, 11, 12)"),
                qsl("moveSubConsole(routedConsole, 9, 10)"),
        };
        QCOMPARE(mpRecorder->mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_bordersReachTheViewOnlyWhenTheyChange()
    {
        const QString result = runLua(qsl(R"lua(
setBorderTop(33)
setBorderTop(33)
setBorderBottom(5)
)lua"));
        QCOMPARE(result, qsl("ok"));

        const QStringList expected{call("applyBorders"), call("applyBorders")};
        QCOMPARE(mpRecorder->mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_mainConsoleTextCuesTheViewThroughItsModel()
    {
        TConsoleModelNotifier& notifier = mpHost->mainConsoleModel().mNotifier;
        QSignalSpy newLines(&notifier, &TConsoleModelNotifier::newLinesWritten);
        QSignalSpy changedLines(&notifier, &TConsoleModelNotifier::linesChanged);
        // One action at a time, so a signal from one cannot hide another that sent none
        const auto runAlone = [&](const QString& chunk) {
            newLines.clear();
            changedLines.clear();
            return runLua(chunk);
        };
        const auto cues = [&] {
            return qsl("new lines %1, changed lines %2").arg(newLines.size()).arg(changedLines.size());
        };

        QCOMPARE(runAlone(qsl(R"lua(echo("routed main text\n"))lua")), qsl("ok"));
        QCOMPARE(cues(), qsl("new lines 1, changed lines 0"));
        QCOMPARE(runAlone(qsl(R"lua(insertText("inserted "))lua")), qsl("ok"));
        QCOMPARE(cues(), qsl("new lines 0, changed lines 1"));
        QCOMPARE(runAlone(qsl(R"lua(
assert(selectString("routed main", 1) > -1, "the echoed text is not in the main console")
setFgColor(255, 0, 0)
)lua")),
                 qsl("ok"));
        QCOMPARE(cues(), qsl("new lines 0, changed lines 1"));

        // A view repaints from the model's signals; a direct cue as well would draw the same text twice
        QCOMPARE(mpRecorder->mCalls.join(qsl("; ")), QString());
    }

    void test_mxpFrameTagsReachTheFrames()
    {
        const QString result = runLua(qsl(R"lua(
setBorderTop(0)
setBorderBottom(0)
setConfig("specialForceMXPProcessorOn", true)
feedTriggers([[<FRAME Name="routedFrame" Title="Routed" Align="right" Width="20%" Height="30%">]] .. "\n")
feedTriggers([[<FRAME Name="routedFrame" Title="Routed" Align="right" Width="20%" Height="30%">]] .. "\n")
feedTriggers([[<FRAME routedFrame ACTION="focus">]] .. "\n")
feedTriggers([[<FRAME Name="routedWindow" EXTERNAL Title="Away" Width="300" Height="200" SCROLLING="NO">]] .. "\n")
assert(windowType("routedWindow") == "miniconsole", "the external frame has no console")
feedTriggers([[<FRAME routedFrame ACTION="close">]] .. "\n")
feedTriggers([[<FRAME routedWindow ACTION="close">]] .. "\n")
setConfig("specialForceMXPProcessorOn", false)
)lua"));
        QCOMPARE(result, qsl("ok"));

        const QStringList expected{
                call("reportSize"),
                qsl("createInternalFrame(routedFrame, , Routed, 640 0 160x600, true, true)"),
                qsl("showFrame(routedFrame)"),
                qsl("focusFrame(routedFrame)"),
                call("reportSize"),
                qsl("createExternalFrame(routedWindow, Away, 300x200, false)"),
                qsl("destroyFrame(routedFrame)"),
                call("reportSize"),
                qsl("destroyFrame(routedWindow)"),
                call("reportSize"),
        };
        QCOMPARE(mpRecorder->mFrames.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_mxpDestTextReachesTheFrameSink()
    {
        const QString result = runLua(qsl(R"lua(
setConfig("specialForceMXPProcessorOn", true)
feedTriggers([[<FRAME Name="routedWindow" EXTERNAL Width="300" Height="200">]] .. "\n")
feedTriggers([[<DEST routedWindow>first routed</DEST>]] .. "\n")
feedTriggers([[<DEST routedWindow EOL>second routed</DEST>]] .. "\n")
feedTriggers([[<DEST routedWindow EOF>third routed</DEST>]] .. "\n")
feedTriggers([[<DEST routedWindow><FRAME Name="routedInside" Align="top" Height="50"></DEST>]] .. "\n")
local routedLines = table.concat(getLines("routedWindow", 0, getLineCount("routedWindow") + 1), "|")
assert(routedLines == "third routed|", "the frame holds " .. routedLines)
feedTriggers([[<FRAME routedWindow ACTION="close">]] .. "\n")
setConfig("specialForceMXPProcessorOn", false)
)lua"));
        QCOMPARE(result, qsl("ok"));

        const QStringList expected{
                call("reportSize"),
                qsl("createExternalFrame(routedWindow, routedWindow, 300x200, true)"),
                qsl("sink(routedWindow)"),
                qsl("sink(routedWindow)"),
                qsl("printFormatted(first routed)"),
                qsl("sink(routedWindow)"),
                qsl("discardLastLine()"),
                qsl("sink(routedWindow)"),
                qsl("printFormatted(second routed)"),
                qsl("sink(routedWindow)"),
                qsl("discardAll()"),
                qsl("sink(routedWindow)"),
                qsl("printFormatted(third routed)"),
                qsl("sink(routedWindow)"),
                call("reportSize"),
                qsl("hasFrameWidget(routedWindow)"),
                qsl("hasFrameWidget(routedWindow)"),
                qsl("createInternalFrame(routedInside, routedWindow, routedInside, 0 0 300x50, false, true)"),
                qsl("removeFromParentTabs(routedInside, routedWindow)"),
                qsl("destroyFrame(routedInside)"),
                call("reportSize"),
                qsl("destroyFrame(routedWindow)"),
                call("reportSize"),
        };
        QCOMPARE(mpRecorder->mFrames.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_appCallsReachTheAppView()
    {
        RecordingAppFrontend app;
        TAppFrontend::setInstance(&app);
        const QString result = runLua(qsl(R"lua(
assert(openWebPage("about:blank") == true, "openWebPage did not pass on what the view answered")
assert(showNotification("routed title", "routed text", 5) == true, "showNotification did not answer true")
showNotification("only a title")
assert(getProfileTabNumber() == 3, "getProfileTabNumber did not count from one")
)lua"));
        TAppFrontend::setInstance(nullptr);

        QCOMPARE(result, qsl("ok"));
        const QStringList expected{
                qsl("openWebPage(about:blank)"),
                qsl("showNotification(routed title, routed text, 5000)"),
                qsl("showNotification(only a title, only a title, none)"),
                qsl("profileTabIndex(%1)").arg(mHostname),
        };
        QCOMPARE(app.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_mapZoomReachesTheMapView()
    {
        const QString result = runMapLua(qsl(R"lua(
assert(setMapZoom(3.5, AREA) == true, "setMapZoom did not pass on the view accepting it")
local refused, why = setMapZoom(0.5, AREA)
assert(refused == nil and why == "routed zoom refusal", "setMapZoom did not pass on the view refusing it")
assert(getMapZoom() == 4.25, "getMapZoom did not read the zoom of the area the view shows")
)lua"));
        QCOMPARE(result, qsl("ok"));

        const QStringList expected{
                qsl("setMapZoom(3.5, %1)").arg(mAreaId),
                qsl("setMapZoom(0.5, %1)").arg(mAreaId),
                call("shownAreaId"),
        };
        QCOMPARE(mMapView.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_mapSelectionReachesTheMapView()
    {
        mMapView.mSelection = {12, 1, 7};
        QCOMPARE(runMapLua(qsl(R"lua(
local selection = getMapSelection()
assert(selection.center == 7, "getMapSelection did not pass on the centre room the view chose")
assert(table.concat(selection.rooms, ",") == "1,7,12", "getMapSelection did not list the rooms the view has selected")
assert(clearMapSelection() == true, "clearMapSelection did not report a selection to clear")
assert(clearMapSelection() == false, "clearMapSelection did not report that nothing was left selected")
)lua")),
                 qsl("ok"));
        mMapView.mSelecting = true;
        mMapView.mSelection = {1};
        QCOMPARE(runMapLua(qsl(R"lua(
local cleared, why = clearMapSelection()
assert(cleared == nil and why ~= nil, "clearMapSelection cleared a selection still being made")
)lua")),
                 qsl("ok"));

        const QStringList expected{
                call("selectedRooms"),
                call("centerSelectedRoom"),
                call("selectingRooms"),
                call("selectedRooms"),
                call("clearRoomSelection"),
                call("shownAreaId"),
                call("selectingRooms"),
                call("selectedRooms"),
                call("shownAreaId"),
                call("selectingRooms"),
        };
        QCOMPARE(mMapView.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_3DViewCallsReachTheMapView()
    {
#if defined(INCLUDE_3DMAPPER)
        const QString result = runMapLua(qsl(R"lua(
shiftMapPerspective(10, 20, 30)
setMapPerspective(400, 50, 60)
assert(getConfig("show3dMapView") == false, "getConfig did not pass on the view having no 3D view shown")
assert(setConfig("show3dMapView", true) == true, "setConfig refused to show the 3D view")
assert(getConfig("show3dMapView") == true, "getConfig did not pass on the view showing its 3D view")
setConfig("experiment.3dmap.modernmapper", true)
setConfig("experiment.3dmap.modernmapper", false)
)lua"));
        QCOMPARE(result, qsl("ok"));

        const QStringList expected{
                qsl("shift3DViewCamera(10, 20, 30)"),
                qsl("set3DViewCameraPosition(400, 50, 60)"),
                call("showing3DView"),
                qsl("show3DView(true)"),
                call("showing3DView"),
                call("recreate3DView"),
                call("recreate3DView"),
        };
        QCOMPARE(mMapView.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
#else
        QSKIP("Built without the 3D mapper, so there is no 3D view to drive.");
#endif
    }

    void test_areaImageExportReachesTheMapView()
    {
        const QString result = runMapLua(qsl(R"lua(
local exported, why = exportAreaImage(AREA, "/routed/level.png", 3)
assert(exported == false and why == "routed export refusal", "exportAreaImage did not pass on the view refusing it")
assert(exportAreaImage(AREA, "/routed/all.png", true) == true, "exportAreaImage did not pass on the view exporting")
)lua"));
        QCOMPARE(result, qsl("ok"));

        // exportAreaImage always exports at zoom 2; a script cannot choose it
        const QStringList expected{
                qsl("exportAreaToImage(%1, /routed/level.png, 3, 2, false)").arg(mAreaId),
                qsl("exportAreaToImage(%1, /routed/all.png, none, 2, true)").arg(mAreaId),
        };
        QCOMPARE(mMapView.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }

    void test_mapViewLifecycleReachesTheViews()
    {
        const QString result = runMapLua(qsl(R"lua(
assert(createMapView(AREA) == 7, "createMapView did not pass on the id of the view made")
local none, why = createMapView()
assert(none == nil and why == "routed view refusal", "createMapView did not pass on the views refusing it")
assert(table.concat(getMapViewIds(), ",") == "7,9", "getMapViewIds did not list the open views")
assert(closeMapView(7) == true, "closeMapView did not pass on the view closing")
local open, whyOpen = closeMapView(9)
assert(open == nil and whyOpen == "routed close refusal", "closeMapView did not pass on the views refusing it")
assert(closeAllMapViews() == 2, "closeAllMapViews did not pass on how many views closed")
)lua"));
        QCOMPARE(result, qsl("ok"));

        const QStringList expected{
                qsl("createView(%1)").arg(mAreaId),
                qsl("createView(0)"),
                call("getViewIds"),
                qsl("closeView(7)"),
                qsl("closeView(9)"),
                call("closeAllViews"),
        };
        QCOMPARE(mMapViews.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
        QVERIFY2(mMapView.mCalls.isEmpty(), qPrintable(mMapView.mCalls.join(qsl("; "))));
    }

    void test_mapViewNavigationReachesTheView()
    {
        const QString result = runMapLua(qsl(R"lua(
assert(centerview(1, 7) == true, "centerview did not pass on the view centring")
local refused, whyRefused = centerview(2, 7)
assert(refused == nil and whyRefused == "routed centring refusal", "centerview did not pass on the view refusing it")
local missing, whyMissing = centerview(1, 8)
assert(missing == nil and whyMissing == "view 8 not found", "centerview did not report a view that is not open")
assert(setMapZoom(1.5, AREA, 7) == true, "setMapZoom did not pass on the view zooming")
local unzoomed, whyUnzoomed = setMapZoom(0.5, AREA, 7)
assert(unzoomed == nil and whyUnzoomed == "routed view zoom refusal", "setMapZoom did not pass on the view refusing it")
assert(getMapZoom(AREA, 7) == 2.75, "getMapZoom did not pass on the zoom the view has")
local info = getMapViewInfo(7)
assert(info.areaId == 21 and info.centeredRoomId == 31 and info.zoom == 2.75 and info.zLevel == -2, "getMapViewInfo did not pass on what the view answered")
)lua"));
        QCOMPARE(result, qsl("ok"));

        // A secondary view is driven alone: the mapper is not asked
        const QStringList expected{
                qsl("view(7)"),
                qsl("centerOnRoom(1)"),
                qsl("view(7)"),
                qsl("centerOnRoom(2)"),
                qsl("view(8)"),
                qsl("view(7)"),
                qsl("setZoom(1.5)"),
                qsl("view(7)"),
                qsl("setZoom(0.5)"),
                qsl("view(7)"),
                call("getZoom"),
                qsl("view(7)"),
                call("getCurrentAreaId"),
                call("getCenteredRoomId"),
                call("getZoom"),
                call("getZLevel"),
        };
        QCOMPARE(mMapViews.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
        QVERIFY2(mMapView.mCalls.isEmpty(), qPrintable(mMapView.mCalls.join(qsl("; "))));
    }

    void test_areaChangesUpdateTheViews()
    {
        const QString result = runMapLua(qsl(R"lua(
routedAreaId = addAreaName("Routed new area")
assert(setAreaName(routedAreaId, "Routed renamed area") == true, "setAreaName did not rename the area")
assert(deleteArea(routedAreaId) == true, "deleteArea did not delete the area")
)lua"));
        QCOMPARE(result, qsl("ok"));
        const int areaId = luaGlobalInt("routedAreaId");
        QVERIFY(areaId > 0);

        // A view still showing a deleted area has to be moved off it
        const QStringList expected{
                call("updateAllViews"),
                call("updateAllViews"),
                call("updateAllViews"),
                qsl("switchViewsShowingArea(%1)").arg(areaId),
        };
        QCOMPARE(mMapViews.mCalls.join(qsl("; ")), expected.join(qsl("; ")));
    }
};

#include "FrontendRoutingContractTest.moc"
MUDLET_GROUPED_TEST_MAIN(FrontendRoutingContractTest)
