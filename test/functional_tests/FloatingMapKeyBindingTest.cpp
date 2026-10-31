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
 * A floating map is a window of its own, so a key pressed after a click on it
 * goes to that window rather than to the command line that runs key bindings
 * (#6512). Lua cannot press a key, so this cannot be a busted spec.
 *
 * Run with: ctest -R FloatingMapKeyBindingTest -V
 */

#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TCommandLine.h"
#include "TMainConsole.h"
#include "TMap.h"
#include "TMapView.h"
#include "TMapViewManager.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class FloatingMapKeyBindingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("FloatingMapKeyBinding-Test");
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    QString runLua(const QString& code) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const QString error = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return error;
    }

    int luaGlobalNumber(const QString& name) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name.toUtf8().constData());
        const int value = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // Pressed as the window system would, on the window, so whichever widget has focus in it gets them first
    void keysAfterAClickReachTheCommandLine(T2DMap* p2dMap) const
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        pCommandLine->clear();
        const auto restore = qScopeGuard([pCommandLine]() {
            pCommandLine->clear();
        });
        QCOMPARE(runLua(qsl("keyPresses = 0")), QString());

        QWidget* pWindow = p2dMap->window();
        QVERIFY(activate(pWindow));
        QTest::mouseClick(p2dMap, Qt::LeftButton, Qt::NoModifier, p2dMap->rect().center());
        QVERIFY(QApplication::activeWindow() == pWindow);
        QTest::keyClick(pWindow->windowHandle(), Qt::Key_F1);
        QCOMPARE(luaGlobalNumber(qsl("keyPresses")), 1);

        QTest::keyClick(pWindow->windowHandle(), Qt::Key_S);
        QCOMPARE(pCommandLine->toPlainText(), qsl("s"));
    }

    static bool activate(QWidget* pWindow)
    {
        pWindow->activateWindow();
        return QTest::qWaitFor([pWindow]() {
            return QApplication::activeWindow() == pWindow;
        });
    }

    QDockWidget* floatingMapDock() const
    {
        mpHost->showHideOrCreateMapper(false);
        TMainConsole* pConsole = mpHost->mainConsoleView();
        QDockWidget* pDock = pConsole ? pConsole->mpDockableMapWidget.data() : nullptr;
        if (!pDock) {
            return nullptr;
        }
        pDock->setFloating(true);
        pDock->show();
        return QTest::qWaitForWindowExposed(pDock) ? pDock : nullptr;
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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3s), "could not connect to the telnet stub");
        QCOMPARE(runLua(qsl("keyPresses = 0; tempKey(%1, function() keyPresses = keyPresses + 1 end)").arg(static_cast<int>(Qt::Key_F1))), QString());
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aKeyBindingWorksAfterAClickOnAFloatingMap()
    {
        QDockWidget* pDock = floatingMapDock();
        QVERIFY(pDock);
        T2DMap* p2dMap = mpHost->mpMap->mapper()->mp2dMap;
        QVERIFY(p2dMap);
        keysAfterAClickReachTheCommandLine(p2dMap);
    }

    void test_aKeyBindingWorksAfterAClickOnAMapView()
    {
        TMapViewManager* pViewManager = mpHost->mpMap->getViewManager();
        QVERIFY(pViewManager);
        const auto [viewId, message] = pViewManager->createView(0);
        QVERIFY2(viewId, qPrintable(message));
        const auto close = qScopeGuard([pViewManager, viewId = viewId]() {
            pViewManager->closeView(viewId);
        });
        TMapView* pView = pViewManager->getView(viewId);
        QVERIFY(pView);
        QVERIFY(pView->window() != mudlet::self());
        QVERIFY(QTest::qWaitForWindowExposed(pView->window()));
        keysAfterAClickReachTheCommandLine(pView->get2DMap());
    }
};

#include "FloatingMapKeyBindingTest.moc"
MUDLET_GROUPED_TEST_MAIN(FloatingMapKeyBindingTest)
