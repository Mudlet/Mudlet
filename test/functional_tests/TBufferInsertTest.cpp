#include <QtTest/QtTest>
#include <cstdlib>

#include "TelnetServerStub.h"
#include "mudlet.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void        initializeQRCResources();

// https://github.com/Mudlet/Mudlet/issues/8945
class TBufferInsertTest : public QObject {
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-InsertText";
    const QString mpPort = "4001";
    const QString mpLocalhost = "localhost";

    Host* host() { return mudlet::self()->getActiveHost(); }

private slots:
    void initTestCase()
    {
        initializeQRCResources();
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, mpPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
        startProfile(mpHostname, mpLocalhost, mpPort);
    }

    void test_insertTextWithNewlineCreatesNewLine()
    {
        auto* mini = host()->mpConsole->createMiniConsole(qsl("main"), qsl("test_insert"), 0, 0, 800, 200);
        QVERIFY(mini);
        mini->setWrapAt(60);

        mini->print(qsl("line1\n"));
        mini->print(qsl("line2\n"));

        const int lineCountBefore = mini->getLineCount();

        mini->moveCursor(0, 0);
        mini->insertText(qsl("inserted line\n"));

        const int lineCountAfter = mini->getLineCount();

        QVERIFY2(lineCountAfter > lineCountBefore,
            qPrintable(QStringLiteral("insertText with \\n should create a new line, but line count went from %1 to %2")
                .arg(lineCountBefore).arg(lineCountAfter)));
    }

    void test_insertTextNewlineSplitsLine()
    {
        auto* mini = host()->mpConsole->createMiniConsole(qsl("main"), qsl("test_split"), 0, 0, 800, 200);
        QVERIFY(mini);
        mini->setWrapAt(60);

        mini->print(qsl("HelloWorld\n"));

        mini->moveCursor(5, 0);
        mini->insertText(qsl("\n"));

        const QString firstLine = mini->buffer.line(0);
        const QString secondLine = mini->buffer.line(1);

        QCOMPARE(firstLine, qsl("Hello"));
        QCOMPARE(secondLine, qsl("World"));
    }

    void test_insertTextIssue8945Scenario()
    {
        auto* mini = host()->mpConsole->createMiniConsole(qsl("main"), qsl("test_8945"), 0, 0, 800, 200);
        QVERIFY(mini);
        mini->setWrapAt(60);

        mini->print(qsl("test1---line1\n"));
        mini->print(qsl("test1---line2\n"));
        mini->print(qsl("test1---line3\n"));

        const int lineCountBefore = mini->getLineCount();

        mini->moveCursor(0, 0);
        mini->insertText(qsl("------- line inserted at: 0/0 -----\n"));

        const int lineCountAfter = mini->getLineCount();

        QVERIFY2(lineCountAfter > lineCountBefore,
            qPrintable(QStringLiteral("insertText with \\n should create a new line, but line count went from %1 to %2")
                .arg(lineCountBefore).arg(lineCountAfter)));

        const QString firstLine = mini->buffer.line(0);
        QCOMPARE(firstLine, qsl("------- line inserted at: 0/0 -----"));
    }

    void test_echoWithNewlineInTriggerMode()
    {
        TConsole* console = host()->mpConsole;
        QVERIFY(console);

        // Simulate trigger engine mode
        console->mTriggerEngineMode = true;

        // Add some initial content
        console->echo(qsl("before"));

        const int lineCountBefore = console->getLineCount();

        console->echo(qsl("\n after newline"));

        const int lineCountAfter = console->getLineCount();

        console->mTriggerEngineMode = false;

        QVERIFY2(lineCountAfter > lineCountBefore,
            qPrintable(QStringLiteral("echo with \\n in trigger mode should create a new line, but line count went from %1 to %2")
                .arg(lineCountBefore).arg(lineCountAfter)));
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

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
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        if (!mudlet::self()->getActiveHost()) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mudlet::self()->getActiveHost()->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

void initializeQRCResources() {
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

#include "TBufferInsertTest.moc"
QTEST_MAIN(TBufferInsertTest)
