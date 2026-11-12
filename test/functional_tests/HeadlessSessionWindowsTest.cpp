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
 * The windows a profile's scripts make, as an app that embeds Mudlet reads them
 * from THeadlessSession to draw them. Each test makes a session of its own, and
 * with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionWindowsTest -V
 */

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

using Kind = THeadlessSession::ScriptWindow::Kind;

class HeadlessSessionWindowsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(QByteArrayLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)"));
        return true;
    }

    static QString luaString(THeadlessSession& session, const QString& expression)
    {
        if (!session.runLua(qsl("headlessWindowsAnswer = tostring(%1)").arg(expression))) {
            return {};
        }
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessWindowsAnswer");
        const QString answer = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return answer;
    }

    static qsizetype indexOf(const QList<THeadlessSession::ScriptWindow>& windows, const QString& name, const Kind kind)
    {
        for (qsizetype i = 0; i < windows.size(); ++i) {
            if (windows.at(i).name == name && windows.at(i).kind == kind) {
                return i;
            }
        }
        return -1;
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void test_eachWindowReadsBackWithItsKindParentAndPlace()
    {
        const QString name = qsl("Headless Windows List");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(session.scriptWindows().isEmpty());
        QVERIFY(!session.scriptWindow(qsl("mc")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("mc", 10, 20, 300, 200)
enableCommandLine("mc")
setBackgroundColor("mc", 10, 20, 30, 255)
openUserWindow("uw")
createLabel("uw", "lbl", 5, 6, 70, 80, 1)
createScrollBox("main", "sb", 400, 10, 200, 100)
createTextEdit("sb", "tb", 1, 2, 50, 40)
hpGauge = Geyser.Gauge:new({name = "hp", x = 0, y = 300, width = 200, height = 20})
openMapWidget()
)lua")));

        const QList<THeadlessSession::ScriptWindow> windows = session.scriptWindows();
        const std::optional<THeadlessSession::ScriptWindow> mc = session.scriptWindow(qsl("mc"));
        QVERIFY(mc);
        QCOMPARE(mc->kind, Kind::miniConsole);
        QCOMPARE(mc->parent, QString());
        QCOMPARE(mc->geometry, QRect(10, 20, 300, 200));
        QVERIFY(mc->visible && mc->shown);
        QVERIFY(!mc->inDock);
        QCOMPARE(mc->background, QColor(10, 20, 30));
        QCOMPARE(mc->font.pointSize(), 12);

        // Its own command line shares its name
        const std::optional<THeadlessSession::ScriptWindow> line = session.scriptWindow(qsl("mc"), Kind::commandLine);
        QVERIFY(line);
        QCOMPARE(line->parent, qsl("mc"));
        QVERIFY(line->visible && line->shown);

        const std::optional<THeadlessSession::ScriptWindow> uw = session.scriptWindow(qsl("uw"));
        QVERIFY(uw);
        QCOMPARE(uw->kind, Kind::userWindow);
        QCOMPARE(uw->parent, QString());
        QVERIFY(uw->inDock);
        const std::optional<THeadlessSession::ScriptWindow> label = session.scriptWindow(qsl("lbl"));
        QVERIFY(label);
        QCOMPARE(label->kind, Kind::label);
        QCOMPARE(label->parent, qsl("uw"));
        QCOMPARE(label->geometry, QRect(5, 6, 70, 80));
        QCOMPARE(label->background, QColor(32, 32, 32));

        const std::optional<THeadlessSession::ScriptWindow> box = session.scriptWindow(qsl("sb"));
        QVERIFY(box);
        QCOMPARE(box->kind, Kind::scrollBox);
        QCOMPARE(box->geometry, QRect(400, 10, 200, 100));
        const std::optional<THeadlessSession::ScriptWindow> textBox = session.scriptWindow(qsl("tb"));
        QVERIFY(textBox);
        QCOMPARE(textBox->kind, Kind::textBox);
        QCOMPARE(textBox->parent, qsl("sb"));
        QCOMPARE(textBox->geometry, QRect(1, 2, 50, 40));

        // A Geyser gauge is the labels it is made of
        for (const QString& part : {qsl("hp_back"), qsl("hp_front"), qsl("hp_text")}) {
            const std::optional<THeadlessSession::ScriptWindow> gaugeLabel = session.scriptWindow(part);
            QVERIFY2(gaugeLabel, qPrintable(part));
            QCOMPARE(gaugeLabel->kind, Kind::label);
            QCOMPARE(gaugeLabel->parent, QString());
        }
        const std::optional<THeadlessSession::ScriptWindow> back = session.scriptWindow(qsl("hp_back"));
        QVERIFY(back);
        QCOMPARE(back->geometry, QRect(0, 300, 200, 20));

        // The dock openMapWidget() opened, beside the main window's area as a user window is
        const std::optional<THeadlessSession::ScriptWindow> map = session.scriptWindow(qsl("mapper"));
        QVERIFY(map);
        QCOMPARE(map->kind, Kind::mapper);
        QVERIFY(map->inDock);

        // Parents before their children, and the main window's area before the docks
        QVERIFY(indexOf(windows, qsl("uw"), Kind::userWindow) < indexOf(windows, qsl("lbl"), Kind::label));
        QVERIFY(indexOf(windows, qsl("mc"), Kind::miniConsole) < indexOf(windows, qsl("mc"), Kind::commandLine));
        QVERIFY(indexOf(windows, qsl("sb"), Kind::scrollBox) < indexOf(windows, qsl("tb"), Kind::textBox));
        QVERIFY(indexOf(windows, qsl("hp_text"), Kind::label) < indexOf(windows, qsl("uw"), Kind::userWindow));
        QCOMPARE(windows.size(), 10);

        // Hidden, the console's own flag is off, and what is in it is not shown while keeping its own
        QVERIFY(session.runLua(qsl(R"(hideWindow("mc") hideWindow("uw"))")));
        const std::optional<THeadlessSession::ScriptWindow> hiddenMc = session.scriptWindow(qsl("mc"));
        const std::optional<THeadlessSession::ScriptWindow> hiddenLine = session.scriptWindow(qsl("mc"), Kind::commandLine);
        const std::optional<THeadlessSession::ScriptWindow> hiddenLabel = session.scriptWindow(qsl("lbl"));
        QVERIFY(hiddenMc && hiddenLine && hiddenLabel);
        QVERIFY(!hiddenMc->visible);
        QVERIFY(!hiddenMc->shown);
        QVERIFY(hiddenLine->visible);
        QVERIFY(!hiddenLine->shown);
        QVERIFY(hiddenLabel->visible);
        QVERIFY(!hiddenLabel->shown);

        // Made later is drawn over, until a raise or a lower changes it; -1 for one that is not there
        const auto order = [&session](const QString& window) {
            const std::optional<THeadlessSession::ScriptWindow> found = session.scriptWindow(window);
            return found ? found->stackOrder : -1;
        };
        // The docks are stacked apart from the main window's area, which the 5 windows in it count up in
        QCOMPARE(order(qsl("uw")), 0);
        QCOMPARE(order(qsl("mapper")), 1);
        QCOMPARE(order(qsl("hp_text")), 4);
        QVERIFY(order(qsl("hp_back")) >= 0);
        QVERIFY(order(qsl("hp_back")) < order(qsl("hp_front")));
        QVERIFY(session.runLua(qsl(R"(raiseWindow("hp_back"))")));
        QVERIFY(order(qsl("hp_back")) > order(qsl("hp_front")));
        QVERIFY(order(qsl("hp_back")) > order(qsl("mc")));
        QCOMPARE(order(qsl("hp_back")), 4);
        QCOMPARE(order(qsl("mapper")), 1);
        QVERIFY(session.runLua(qsl(R"(lowerWindow("hp_back"))")));
        QCOMPARE(order(qsl("hp_back")), 0);
        QCOMPARE(luaString(session, qsl("raiseWindow('nonesuch')")), qsl("false"));

        QVERIFY(session.shutdown(false));
        QVERIFY(session.scriptWindows().isEmpty());
    }

    void test_eachWindowIsListedInItsOwnParent()
    {
        const QString name = qsl("Headless Windows Parents");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        // A scroll box may share its name with a mini console, made here first so that it is listed first
        QVERIFY(session.runLua(qsl(R"lua(
createMiniConsole("dup", 0, 0, 100, 100)
createScrollBox("main", "dup", 200, 0, 100, 100)
createLabel("dup", "inBox", 1, 2, 10, 10, 1)
openUserWindow("uw")
createMapper(0, 0, 100, 100)
setWindow("uw", "mapper", 40, 50, true)
)lua")));
        const QList<THeadlessSession::ScriptWindow> windows = session.scriptWindows();
        const std::optional<THeadlessSession::ScriptWindow> inBox = session.scriptWindow(qsl("inBox"));
        QVERIFY(inBox);
        QCOMPARE(inBox->parent, qsl("dup"));
        QCOMPARE(inBox->parentKind, Kind::scrollBox);
        QVERIFY(indexOf(windows, qsl("dup"), Kind::miniConsole) < indexOf(windows, qsl("dup"), Kind::scrollBox));
        QVERIFY(indexOf(windows, qsl("dup"), Kind::scrollBox) < indexOf(windows, qsl("inBox"), Kind::label));

        // setWindow() moves the mapper to its point in the window it is put in, as QWidget::move() does
        std::optional<THeadlessSession::ScriptWindow> mapper = session.scriptWindow(qsl("mapper"));
        QVERIFY(mapper);
        QCOMPARE(mapper->parent, qsl("uw"));
        QCOMPARE(mapper->parentKind, Kind::userWindow);
        QCOMPARE(mapper->geometry, QRect(40, 50, 100, 100));
        QVERIFY(session.runLua(qsl(R"(setWindow("main", "mapper", 7, 8, true))")));
        mapper = session.scriptWindow(qsl("mapper"));
        QVERIFY(mapper);
        QCOMPARE(mapper->parent, QString());
        QCOMPARE(mapper->geometry, QRect(7, 8, 100, 100));

        // What a deleted scroll box held stays, in its user window, until deferred deletes run
        QVERIFY(session.runLua(qsl(R"(createScrollBox("uw", "gone", 0, 0, 50, 50) createLabel("gone", "orphan", 1, 1, 5, 5, 1) deleteScrollBox("gone"))")));
        const QList<THeadlessSession::ScriptWindow> afterDelete = session.scriptWindows();
        const std::optional<THeadlessSession::ScriptWindow> orphan = session.scriptWindow(qsl("orphan"));
        QVERIFY(orphan);
        QCOMPARE(orphan->parent, qsl("uw"));
        QCOMPARE(orphan->parentKind, Kind::userWindow);
        QVERIFY(indexOf(afterDelete, qsl("uw"), Kind::userWindow) < indexOf(afterDelete, qsl("orphan"), Kind::label));
        QVERIFY(session.shutdown(false));
    }

    void test_aTurnsChangesAreToldOnce()
    {
        const QString name = qsl("Headless Windows Changes");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QSignalSpy changes(&session, &THeadlessSession::scriptWindowsChanged);
        const auto toldOnce = [&changes](const QStringList& names) {
            if (!changes.wait(1000)) {
                return false;
            }
            // Anything more this turn would be queued by now
            QCoreApplication::processEvents();
            QCoreApplication::processEvents();
            const bool once = changes.count() == 1 && changes.at(0).at(0).toString() == qsl("Headless Windows Changes") && changes.at(0).at(1).toStringList() == names;
            changes.clear();
            return once;
        };

        QVERIFY(session.runLua(qsl(R"(createLabel("old", 0, 0, 10, 10, 1))")));
        QVERIFY(toldOnce({qsl("old")}));

        QVERIFY(session.runLua(qsl(R"lua(
createLabel("fresh", 0, 0, 10, 10, 1)
moveWindow("fresh", 50, 60)
resizeWindow("fresh", 30, 40)
hideWindow("fresh")
deleteLabel("old")
)lua")));
        QVERIFY(toldOnce({qsl("fresh"), qsl("old")}));
        QVERIFY(!session.scriptWindow(qsl("old")));
        const std::optional<THeadlessSession::ScriptWindow> fresh = session.scriptWindow(qsl("fresh"));
        QVERIFY(fresh);
        QCOMPARE(fresh->geometry, QRect(50, 60, 30, 40));

        QVERIFY(session.runLua(qsl(R"(moveWindow("fresh", 1, 2))")));
        QVERIFY(toldOnce({qsl("fresh")}));
        QVERIFY(session.runLua(qsl(R"(resizeWindow("fresh", 5, 6))")));
        QVERIFY(toldOnce({qsl("fresh")}));
        QVERIFY(session.runLua(qsl(R"(showWindow("fresh"))")));
        QVERIFY(toldOnce({qsl("fresh")}));
        QVERIFY(session.runLua(qsl(R"(createMiniConsole("mc", 0, 0, 100, 100))")));
        QVERIFY(toldOnce({qsl("mc")}));
        QVERIFY(session.runLua(qsl(R"(setBackgroundColor("mc", 1, 2, 3, 255))")));
        QVERIFY(toldOnce({qsl("mc")}));
        // A dock is not stacked with the main window's area, so raising a window there does not move it
        QVERIFY(session.runLua(qsl(R"(openUserWindow("uw"))")));
        QVERIFY(toldOnce({qsl("uw")}));
        QVERIFY(session.runLua(qsl(R"(raiseWindow("fresh"))")));
        QVERIFY(toldOnce({qsl("fresh"), qsl("mc")}));

        QVERIFY(session.runLua(qsl(R"(moveWindow("fresh", 1, 2) moveWindow("mc", 0, 0))")));
        QVERIFY(!changes.wait(200));

        // The mapper goes with the user window it is in only as deferred deletes run, after the window's own change
        // has been told
        QVERIFY(session.runLua(qsl(R"(openUserWindow("mapWindow") createMapper("mapWindow", 0, 0, 50, 50))")));
        QVERIFY(changes.wait(1000));
        QVERIFY(session.scriptWindow(qsl("mapper")));
        changes.clear();
        QVERIFY(session.runLua(qsl(R"(deleteMiniConsole("mapWindow"))")));
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QVERIFY(changes.count() >= 1);
        QVERIFY(session.scriptWindow(qsl("mapper")));
        changes.clear();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!session.scriptWindow(qsl("mapper")));
        QVERIFY(toldOnce({qsl("mapper")}));

        QVERIFY(session.shutdown(false));
    }

    void test_eachLabelReadsBackWhatItShows()
    {
        const QString name = qsl("Headless Label Content");
        QVERIFY(writeProfile(name));
        QTemporaryDir imageDir;
        const QString image = imageDir.filePath(qsl("red.png"));
        QImage red(4, 4, QImage::Format_ARGB32);
        red.fill(Qt::red);
        QVERIFY(red.save(image));
        THeadlessSession session;
        QVERIFY(!session.labelContent(qsl("lbl")));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QSignalSpy changes(&session, &THeadlessSession::scriptWindowsChanged);
        QVERIFY(session.runLua(qsl(R"lua(
createLabel("lbl", 0, 0, 100, 20, 1)
createMiniConsole("mc", 0, 100, 100, 100)
hpGauge = Geyser.Gauge:new({name = "hp", x = 0, y = 300, width = 200, height = 20})
hpGauge:setValue(25, 100)
)lua")));
        QVERIFY(changes.wait(1000));
        changes.clear();
        QVERIFY(!session.labelContent(qsl("mc")));
        QVERIFY(!session.labelContent(qsl("nonesuch")));
        std::optional<THeadlessSession::LabelContent> content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->text, QString());

        QVERIFY(session.runLua(qsl(R"(echo("lbl", "<b>hp</b>"))")));
        QVERIFY(changes.wait(1000));
        QCOMPARE(changes.takeFirst().at(1).toStringList(), QStringList{qsl("lbl")});
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QVERIFY(content->text.contains(qsl("<b>hp</b>")));

        QVERIFY(session.runLua(qsl(R"(setBackgroundImage("lbl", [[%1]]))").arg(image)));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->image, image);
        QVERIFY(!content->svg);
        // A raster image takes the text down, as QLabel::setPixmap() does
        QCOMPARE(content->text, QString());

        QVERIFY(session.runLua(qsl(R"lua(
setLabelToolTip("lbl", "your health", 3)
setLabelCursor("lbl", 2)
setLabelStyleSheet("lbl", "color: red;")
)lua")));
        QVERIFY(changes.wait(1000));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->toolTip, qsl("your health"));
        QCOMPARE(content->toolTipDuration, 3.0);
        QCOMPARE(content->cursor, std::optional(Qt::CrossCursor));
        QCOMPARE(content->styleSheet, qsl("color: red;"));
        QCOMPARE(content->accessibleName, qsl("your health"));
        QVERIFY(!content->accessibleButton);
        QVERIFY(!content->clickThrough);
        changes.clear();
        QVERIFY(session.runLua(qsl(R"(setLabelAccessibleName("lbl", "health"))")));
        QVERIFY(changes.wait(1000));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->accessibleName, qsl("health"));

        // A movie's playback is told as scripts change it
        const QString movie = imageDir.filePath(qsl("two.gif"));
        QFile gif(movie);
        QVERIFY(gif.open(QIODevice::WriteOnly));
        // Two frames of one pixel, as GeyserLabel_spec.lua writes them
        gif.write(QByteArray::fromHex("47494638396101000100800000000000ffffff21ff0b4e45545343415045322e30030100000021f9"
                                      "04000a0000002c000000000100010000020244010021f904000a0000002c00000000010001000002024c01003b"));
        gif.close();
        QVERIFY(session.runLua(qsl(R"(setMovie("lbl", [[%1]]))").arg(movie)));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QCOMPARE(content->movie, movie);
        QVERIFY(!content->moviePaused);
        QCOMPARE(content->movieFrame, -1);
        QCOMPARE(content->movieSpeed, 100);
        QVERIFY(!content->movieSize.isValid());
        QVERIFY(changes.wait(1000));
        changes.clear();
        // Each call is told on its own; the frame to jump to is the one the pause did not stop on
        const auto told = [&](const QString& call) {
            if (!session.runLua(call) || !changes.wait(1000)) {
                return false;
            }
            const bool named = changes.takeFirst().at(1).toStringList() == QStringList{qsl("lbl")};
            changes.clear();
            return named;
        };
        QVERIFY(told(qsl("pauseMovie('lbl')")));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QVERIFY(content->moviePaused);
        const int other = 1 - content->movieFrame;
        QVERIFY(told(qsl("setMovieFrame('lbl', %1)").arg(other)));
        QVERIFY(told(qsl("setMovieSpeed('lbl', 50)")));
        QVERIFY(told(qsl("scaleMovie('lbl')")));
        content = session.labelContent(qsl("lbl"));
        QVERIFY(content);
        QVERIFY(content->moviePaused);
        QCOMPARE(content->movieFrame, other);
        QCOMPARE(content->movieSpeed, 50);
        QCOMPARE(content->movieSize, QSize(100, 20));
        QVERIFY(session.runLua(qsl("startMovie('lbl')")));
        QVERIFY(changes.wait(1000));
        changes.clear();
        QVERIFY(!session.labelContent(qsl("lbl"))->moviePaused);
        QCOMPARE(session.labelContent(qsl("lbl"))->movieFrame, -1);

        // One that does not loop stops by itself on its last frame, and that is told too
        const QString once = imageDir.filePath(qsl("once.gif"));
        QFile onceGif(once);
        QVERIFY(onceGif.open(QIODevice::WriteOnly));
        onceGif.write(QByteArray::fromHex("47494638396101000100800000000000ffffff21f904000a0000002c000000000100010000020244010021f904000a0000002c"
                                          "00000000010001000002024c01003b"));
        onceGif.close();
        QVERIFY(session.runLua(qsl(R"(createLabel("onceLabel", 0, 400, 10, 10, 1) setMovie("onceLabel", [[%1]]))").arg(once)));
        QVERIFY(changes.wait(1000));
        changes.clear();
        QTRY_VERIFY_WITH_TIMEOUT(changes.count() > 0, 3000);
        QCOMPARE(changes.takeFirst().at(1).toStringList(), QStringList{qsl("onceLabel")});
        content = session.labelContent(qsl("onceLabel"));
        QVERIFY(content);
        QVERIFY(content->moviePaused);
        QCOMPARE(content->movieFrame, 1);

        // A gauge's value is its front label's width
        const std::optional<THeadlessSession::ScriptWindow> back = session.scriptWindow(qsl("hp_back"));
        const std::optional<THeadlessSession::ScriptWindow> front = session.scriptWindow(qsl("hp_front"));
        QVERIFY(back && front);
        QCOMPARE(back->geometry.width(), 200);
        QCOMPARE(front->geometry.width(), 50);
        QVERIFY(session.labelContent(qsl("hp_front")));

        QVERIFY(session.shutdown(false));
        QVERIFY(!session.labelContent(qsl("lbl")));
    }

    void test_userWindowsArePlacedAsTheGuisDocksAre()
    {
        const QString name = qsl("Headless User Window Place");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(!session.setUserWindowGeometry(qsl("uw"), QRect(0, 0, 250, 400), false));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setMainWindowSize(QSize(800, 600)));
        QVERIFY(session.runLua(qsl(R"lua(
openUserWindow("uw", false, false, "left")
openUserWindow("floater", false, true, "f")
setUserWindowTitle("floater", "Chat")
createLabel("lbl", 0, 0, 10, 10, 1)
resizes = {}
registerAnonymousEventHandler("sysUserWindowResizeEvent", function(_, width, height, window)
  resizes[#resizes + 1] = string.format("%s %d %d", window, width, height)
end)
)lua")));
        const std::optional<THeadlessSession::ScriptWindow> uw = session.scriptWindow(qsl("uw"));
        QVERIFY(uw && uw->dock);
        QVERIFY(!uw->dock->autoDock);
        QCOMPARE(uw->dock->area, Qt::LeftDockWidgetArea);
        QVERIFY(!uw->dock->floating);
        QCOMPARE(uw->dock->title, qsl("uw"));
        const std::optional<THeadlessSession::ScriptWindow> floater = session.scriptWindow(qsl("floater"));
        QVERIFY(floater && floater->dock);
        QVERIFY(floater->dock->autoDock && floater->dock->floating);
        QCOMPARE(floater->dock->title, qsl("Chat"));
        const std::optional<THeadlessSession::ScriptWindow> label = session.scriptWindow(qsl("lbl"));
        QVERIFY(label);
        QVERIFY(!label->dock);

        QVERIFY(session.setUserWindowGeometry(qsl("uw"), QRect(10, 20, 250, 400), false));
        QCOMPARE(luaString(session, qsl("table.concat(resizes, ';')")), qsl("uw 250 400"));
        QCOMPARE(luaString(session, qsl("table.concat({getUserWindowSize('uw')}, ',')")), qsl("250,400"));
        QCOMPARE(luaString(session, qsl("table.concat({getWindowGeometry('uw')}, ',')")), qsl("10,20,250,400"));
        std::optional<THeadlessSession::ScriptWindow> placed = session.scriptWindow(qsl("uw"));
        QVERIFY(placed && placed->dock);
        QCOMPARE(placed->geometry, QRect(10, 20, 250, 400));
        // Moved, not resized, as a dock's moveEvent() raises nothing
        QVERIFY(session.setUserWindowGeometry(qsl("uw"), QRect(30, 40, 250, 400), true));
        QCOMPARE(luaString(session, qsl("#resizes")), qsl("1"));
        placed = session.scriptWindow(qsl("uw"));
        QVERIFY(placed && placed->dock);
        QVERIFY(placed->dock->floating);
        // The area is what the script asked for, wherever the app docks it
        QCOMPARE(placed->dock->area, Qt::LeftDockWidgetArea);

        QVERIFY(session.closeUserWindow(qsl("uw")));
        QCOMPARE(luaString(session, qsl("windowVisible('uw')")), qsl("false"));
        // A size given while it is closed reaches it as it next shows
        QVERIFY(session.setUserWindowGeometry(qsl("uw"), QRect(0, 0, 300, 300), false));
        QCOMPARE(luaString(session, qsl("#resizes")), qsl("1"));
        QVERIFY(session.runLua(qsl(R"(showWindow("uw"))")));
        QCOMPARE(luaString(session, qsl("resizes[2]")), qsl("uw 300 300"));

        // A handler of the resize that takes the window away, or opens another of its name in its place, leaves it unplaced
        QVERIFY(session.runLua(qsl(R"lua(
openUserWindow("gone")
openUserWindow("again")
registerAnonymousEventHandler("sysUserWindowResizeEvent", function(_, width, height, window)
  if window == "gone" or (window == "again" and width == 222) then
    deleteMiniConsole(window)
  end
  if window == "again" and width == 222 then
    openUserWindow("again")
  end
end)
)lua")));
        QVERIFY(!session.setUserWindowGeometry(qsl("gone"), QRect(0, 0, 111, 111), false));
        QVERIFY(session.errorString().contains(qsl("taken away")));
        QVERIFY(!session.scriptWindow(qsl("gone")));
        QVERIFY(!session.setUserWindowGeometry(qsl("again"), QRect(0, 0, 222, 222), false));
        QVERIFY(session.errorString().contains(qsl("made again")));
        const std::optional<THeadlessSession::ScriptWindow> again = session.scriptWindow(qsl("again"));
        QVERIFY(again);
        QVERIFY(again->geometry.size() != QSize(222, 222));

        QVERIFY(!session.setUserWindowGeometry(qsl("lbl"), QRect(0, 0, 5, 5), false));
        QVERIFY(session.errorString().contains(qsl("lbl")));
        QVERIFY(!session.closeUserWindow(qsl("nonesuch")));
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.closeUserWindow(qsl("uw")));
    }
};

#include "HeadlessSessionWindowsTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionWindowsTest)
