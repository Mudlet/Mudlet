/***************************************************************************
 *   Copyright (C) 2025 by Nicolas Keita - nicolaskeita2@@gmail.com        *
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

#include <QtTest/QtTest>

#include <cstdlib>

#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

class TelnetTextDisplayedTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-Telnet";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_TelnetTextDisplayed()
    {
        QString messageFromTheMud("\x1B[1z<B>Greetings < hunters & sorcerers</B>\x1B[7z");
        QString messageToExpect("Greetings < hunters & sorcerers");

        mpServer->setWelcomeMessage(messageFromTheMud);
        startProfile(mpHostname, mpLocalhost, mpPort);

        QVERIFY2(waitForTextInBuffer(messageToExpect), qPrintable(qsl("Expected text '%1' not found in console buffer").arg(messageToExpect)));
    }

    // An unescaped '&' directly followed by (or running into) a non-ASCII character
    // is not a valid entity; the original raw bytes must be passed through unchanged
    // so the charset decoder can reassemble the multi-byte characters (follow-up to #9439)
    void test_MalformedEntityKeepsNonAsciiBytes()
    {
        QString messageFromTheMud("\x1B[1zKäse&Brötchen and &Ф too");
        QString messageToExpect("Käse&Brötchen and &Ф too");

        mpServer->setWelcomeMessage(messageFromTheMud);
        startProfile(mpHostname, mpLocalhost, mpPort);

        QVERIFY2(waitForTextInBuffer(messageToExpect), qPrintable(qsl("Expected text '%1' not found in console buffer, which contains:\n%2").arg(messageToExpect, bufferContents())));
    }

    // A custom <!ENTITY> with a non-Latin1 value must resolve to that value intact
    // in a UTF-8 session (the case #9439 fixed)
    void test_CustomEntityKeepsNonAsciiValue()
    {
        QString messageFromTheMud("\x1B[1z<!ENTITY storm \"Гроза\">The &storm; rages");
        QString messageToExpect("The Гроза rages");

        mpServer->setWelcomeMessage(messageFromTheMud);
        startProfile(mpHostname, mpLocalhost, mpPort);

        QVERIFY2(waitForTextInBuffer(messageToExpect), qPrintable(qsl("Expected text '%1' not found in console buffer, which contains:\n%2").arg(messageToExpect, bufferContents())));
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

    // Utility function to manually start a profile like a user would do via the
    // GUI
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Polls the console buffer until the expected text appears on any line, with
    // a timeout
    bool waitForTextInBuffer(const QString& text, int timeoutMs = 5000)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        return QTest::qWaitFor(
                [&]() {
                    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
                        if (console->buffer.line(i) == text) {
                            return true;
                        }
                    }
                    return false;
                },
                timeoutMs);
    }

    // All buffer lines joined together, for failure diagnostics
    QString bufferContents()
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QStringList lines;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            lines << console->buffer.line(i);
        }
        return lines.join(QChar::LineFeed);
    }

    // Utility function
    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            qInfo() << "Profile directory does not exist:" << path;
            return;
        }
        dir.removeRecursively();
    }
};

void initializeQRCResources()
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

#include "TelnetTextDisplayedTest.moc"
QTEST_MAIN(TelnetTextDisplayedTest)
