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

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TUiTour.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "glwidget.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class LegacyMapPickTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("LegacyMapPick-Test");
    const QString mLocalhost = qsl("localhost");
    static constexpr int kPlayerRoomId = 1;

    TMap* map() const { return mpHost->mpMap.data(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY(mpServer->serverPort() != 0);

        mudlet::start();
        mudlet::self()->setupConfig();
        TUiTour::rememberShown();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY(connected.wait(3s));

        const int areaId = map()->mpRoomDB->addArea(qsl("Player Area"));
        QVERIFY(areaId > 0);
        for (int id = 1; id <= 9; ++id) {
            QVERIFY(map()->addRoom(id) && map()->setRoomArea(id, areaId) && map()->setRoomCoordinates(id, (id - 1) % 3 * 2, (id - 1) / 3 * 2, 0));
        }
        map()->mRoomIdHash[map()->mProfileName] = kPlayerRoomId;
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

    // Switching an open 3D view from the modern renderer to the classic one leaves the classic view without a context
    // of its own, as it cannot share with the window's core profile one, and a press on it then picked a room in
    // whichever context was current, which crashed in the driver
    void test_aPressBeforeTheMapHasAContextLeavesOtherContextsAlone()
    {
        QOffscreenSurface surface;
        surface.create();
        QOpenGLContext other;
        if (!other.create() || !other.makeCurrent(&surface)) {
            QSKIP("no OpenGL context can be made here");
        }
        auto* gl = other.functions();
        gl->glViewport(1, 2, 3, 4);

        GLWidget view(map(), mpHost);
        view.resize(200, 200);
        QVERIFY(!view.isValid());
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&view, &press);

        QCOMPARE(QOpenGLContext::currentContext(), &other);
        GLint viewport[4] = {};
        gl->glGetIntegerv(GL_VIEWPORT, viewport);
        other.doneCurrent();
        QVERIFY2(viewport[0] == 1 && viewport[1] == 2 && viewport[2] == 3 && viewport[3] == 4,
                 qPrintable(qsl("another context's viewport became %1,%2,%3,%4").arg(viewport[0]).arg(viewport[1]).arg(viewport[2]).arg(viewport[3])));
    }
};

#include "LegacyMapPickTest.moc"
MUDLET_GROUPED_TEST_MAIN(LegacyMapPickTest)
