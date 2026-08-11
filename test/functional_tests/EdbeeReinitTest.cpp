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
 * mudlet::initEdbee() only runs once per process, so destroying a mudlet
 * instance and constructing another - what every functional test with a
 * per-method init() does - must leave the edbee singleton fully usable: Lua
 * grammar, Mudlet theme, and a script editor that opens against them.
 *
 * Run with: ctest -R EdbeeReinitTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "edbee/edbee.h"
#include "edbee/models/textgrammar.h"
#include "edbee/views/texttheme.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

static void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

class EdbeeReinitTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EdbeeReinit-Test-Profile");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists() && !dir.removeRecursively()) {
            qWarning() << "deleteProfileDirectory: could not remove" << path << "- later failures may stem from this stale state";
        }
    }

    static void bootMudlet()
    {
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void startProfile(const QString& profileName, const QString& address, const QString& port)
    {
        QTimer::singleShot(0ms, qApp, [profileName, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);

            dlgConnectionProfiles* connectionDialog = mudlet::self()->mpConnectionDialog;
            if (!connectionDialog || !connectionDialog->new_profile_button) {
                qWarning() << "startProfile: connection dialog did not appear";
                return;
            }
            QTest::mouseClick(connectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);

            const auto focusedWidget = [](const char* step) -> QWidget* {
                QWidget* widget = QApplication::focusWidget();
                if (!widget) {
                    qWarning() << "startProfile: no focused widget at step" << step;
                }
                return widget;
            };

            QWidget* nameField = focusedWidget("profile name");
            if (!nameField) {
                return;
            }
            QTest::keyClicks(nameField, profileName);
            QTest::qWait(100ms);
            QTest::keyClick(nameField, Qt::Key_Tab);
            QTest::qWait(100ms);

            QWidget* addressField = focusedWidget("address");
            if (!addressField) {
                return;
            }
            QTest::keyClicks(addressField, address);
            QTest::qWait(100ms);
            QTest::keyClick(addressField, Qt::Key_Tab);
            QTest::qWait(100ms);

            QWidget* portField = focusedWidget("port");
            if (!portField) {
                return;
            }
            QTest::keyClicks(portField, port);
            QTest::qWait(100ms);
            QTest::keyClick(portField, Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(2000)) {
            QFAIL("Profile took too long to load.");
        }

        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

private slots:
    void initTestCase()
    {
        initializeQRCResources();
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mProfileName);
        delete mudlet::self();
    }

    void test_editorAliveAfterMudletReconstruction()
    {
        bootMudlet();
        delete mudlet::self();

        bootMudlet();
        deleteProfileDirectory(mProfileName);

        auto* edbee = edbee::Edbee::instance();
        auto* luaGrammar = edbee->grammarManager()->get(qsl("source.lua"));
        QVERIFY2(luaGrammar, "Lua grammar gone after mudlet reconstruction - initEdbee()'s once-guard left edbee unprimed");
        // the editor picks its grammar by filename, so that path must agree
        QCOMPARE(edbee->grammarManager()->detectGrammarWithFilename(qsl("Buck.lua")), luaGrammar);
        QVERIFY2(edbee->themeManager()->theme(qsl("Mudlet")), "Mudlet editor theme gone after mudlet reconstruction");

        startProfile(mProfileName, mLocalhost, mPort);
        if (QTest::currentTestFailed()) {
            return;
        }

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        QVERIFY2(mpHost->mpEditorDialog, "Script editor did not open on the reconstructed mudlet instance");
    }
};

#include "EdbeeReinitTest.moc"
QTEST_MAIN(EdbeeReinitTest)
