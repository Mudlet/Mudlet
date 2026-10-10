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
#include "TNullAppFrontend.h"
#include "TNullConsoleFrontend.h"

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

// Writes down what core code asks of the view, then does what the null view does, so the windows it
// makes still get a model and a place in the window registry for the calls that follow.
class RecordingConsoleFrontend final : public TNullConsoleFrontend
{
public:
    using TNullConsoleFrontend::TNullConsoleFrontend;

    QStringList mCalls;

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
    }

    void init()
    {
        if (mpRecorder) {
            mpRecorder->mCalls.clear();
        }
    }

    void cleanupTestCase()
    {
        mpRecorder = nullptr;
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

        const QString result = runLua(qsl(R"lua(
echo("routed main text\n")
insertText("inserted ")
assert(selectString("routed main", 1) > -1, "the echoed text is not in the main console")
setFgColor(255, 0, 0)
)lua"));
        QCOMPARE(result, qsl("ok"));

        QVERIFY2(!newLines.isEmpty(), "echo() did not tell the main console's model it has new lines.");
        QVERIFY2(!changedLines.isEmpty(), "Recolouring a selection did not tell the main console's model which lines changed.");
        // A view repaints from the model's signals; a direct cue as well would draw the same text twice
        QCOMPARE(mpRecorder->mCalls.join(qsl("; ")), QString());
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
};

#include "FrontendRoutingContractTest.moc"
MUDLET_GROUPED_TEST_MAIN(FrontendRoutingContractTest)
