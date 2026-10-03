/***************************************************************************
 *   Copyright (C) 2026 by Andrew Johnson - andrew@johnson5.net            *
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
 * A lone Alt tap moves focus to the menu bar when MudletProxyStyle answers
 * QStyle::SH_MenuBar_AltKeyNavigation with 1. The player can choose always or
 * never; by default it follows the operating system's screen reader flag.
 *
 * On Linux and the BSDs that flag is org.a11y.Status.ScreenReaderEnabled on the
 * D-Bus session bus, so those cases run against a private dbus-daemon hosting a
 * stand-in for org.a11y.Bus, and never depend on the desktop the test runs on.
 * Qt can open the session bus while the QApplication starts, so the daemon is
 * started in initMain(), before that.
 *
 * Run with: ctest -R MudletProxyStyleTest -V
 */

#include <QtTest/QtTest>

#include "MudletProxyStyle.h"
#include "utils.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QProcess>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#endif

#include "GroupedTest.h"

namespace {

int altKeyNavigation(const MudletProxyStyle& style)
{
    return style.styleHint(QStyle::SH_MenuBar_AltKeyNavigation, nullptr, nullptr, nullptr);
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
const QString csmService = qsl("org.a11y.Bus");
const QString csmPath = qsl("/org/a11y/bus");
const QString csmStatusInterface = qsl("org.a11y.Status");
const QString csmPropertiesInterface = qsl("org.freedesktop.DBus.Properties");
const QString csmStandInConnection = qsl("MudletProxyStyleTest stand-in");

// dbus-daemon's own session.conf less its service directories, so asking for a
// missing org.a11y.Bus cannot start the real one installed on the machine
const QByteArray csmDaemonConfig = QByteArrayLiteral("<busconfig>"
                                                     "<type>session</type>"
                                                     "<listen>unix:tmpdir=/tmp</listen>"
                                                     "<auth>EXTERNAL</auth>"
                                                     "<policy context=\"default\">"
                                                     "<allow send_destination=\"*\" eavesdrop=\"true\"/>"
                                                     "<allow eavesdrop=\"true\"/>"
                                                     "<allow own=\"*\"/>"
                                                     "</policy>"
                                                     "</busconfig>");

// Set by initMain(), which runs before the test object exists
QProcess* spDaemon = nullptr;
QString sDaemonAddress;
QString sDaemonProblem;

QString busId(const QDBusConnection& connection)
{
    const QDBusReply<QString> reply = connection.call(QDBusMessage::createMethodCall(qsl("org.freedesktop.DBus"), qsl("/org/freedesktop/DBus"), qsl("org.freedesktop.DBus"), qsl("GetId")));
    return reply.isValid() ? reply.value() : QString();
}
#endif

} // namespace

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
// Qt answers org.freedesktop.DBus.Properties.Get from an adaptor's properties
class StandInA11yStatus : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.a11y.Status")
    Q_PROPERTY(bool ScreenReaderEnabled READ screenReaderEnabled)
    Q_PROPERTY(bool IsEnabled READ enabled)

public:
    explicit StandInA11yStatus(QObject* pParent)
    : QDBusAbstractAdaptor(pParent)
    {
    }
    bool screenReaderEnabled() const { return mScreenReaderEnabled; }
    bool enabled() const { return mEnabled; }

    bool mScreenReaderEnabled = false;
    bool mEnabled = false;
};
#endif

class MudletProxyStyleTest : public QObject
{
    Q_OBJECT

public:
    static void initMain();

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void alwaysMovesFocusToTheMenuBar()
    {
        MudletProxyStyle style;
        MudletProxyStyle::setAltKeyNavigation(enums::MenuBarAltKeyNavigation::Always);
        QCOMPARE(altKeyNavigation(style), 1);
    }

    void neverMovesFocusToTheMenuBar()
    {
        MudletProxyStyle style;
        MudletProxyStyle::setAltKeyNavigation(enums::MenuBarAltKeyNavigation::Never);
        QCOMPARE(altKeyNavigation(style), 0);
    }

    void detectsAScreenReaderAlreadyRunning();
    void followsAScreenReaderStartingAndStopping();
    void ignoresAnAccessibilityBusWithoutAScreenReader();
    void neverOverridesADetectedScreenReader();
    void detectsNothingWithoutAnAccessibilityBus();

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
private:
    bool announce(const QVariantMap& changedProperties);
    bool roundTrip();

    QObject mStandInObject;
    StandInA11yStatus* mpStandIn = nullptr;
#endif
};

void MudletProxyStyleTest::cleanup()
{
    MudletProxyStyle::setAltKeyNavigation(enums::MenuBarAltKeyNavigation::WhenScreenReaderRunning);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    // detectsNothingWithoutAnAccessibilityBus() gives the name up
    if (mpStandIn && !QDBusConnection(csmStandInConnection).interface()->isServiceRegistered(csmService)) {
        QVERIFY(QDBusConnection(csmStandInConnection).registerService(csmService));
    }
#endif
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
void MudletProxyStyleTest::initMain()
{
    const QString daemon = QStandardPaths::findExecutable(qsl("dbus-daemon"));
    if (daemon.isEmpty()) {
        return;
    }

    QTemporaryFile config;
    if (!config.open() || config.write(csmDaemonConfig) != csmDaemonConfig.size() || !config.flush()) {
        sDaemonProblem = qsl("could not write the dbus-daemon configuration: %1").arg(config.errorString());
        return;
    }

    // No QCoreApplication yet, so no event loop: QProcess is only waited on
    spDaemon = new QProcess();
    spDaemon->start(daemon, {qsl("--config-file=%1").arg(config.fileName()), qsl("--nofork"), qsl("--print-address=1")});
    if (!spDaemon->waitForStarted()) {
        sDaemonProblem = qsl("dbus-daemon did not start: %1").arg(spDaemon->errorString());
        return;
    }
    while (!spDaemon->canReadLine() && spDaemon->waitForReadyRead()) {
    }
    sDaemonAddress = QString::fromUtf8(spDaemon->readLine()).trimmed();
    if (sDaemonAddress.isEmpty()) {
        sDaemonProblem = qsl("dbus-daemon printed no address: %1").arg(QString::fromLocal8Bit(spDaemon->readAllStandardError()));
        return;
    }
    qputenv("DBUS_SESSION_BUS_ADDRESS", sDaemonAddress.toUtf8());
}

void MudletProxyStyleTest::initTestCase()
{
    QVERIFY2(sDaemonProblem.isEmpty(), qPrintable(sDaemonProblem));
    if (!spDaemon) {
        return;
    }

    QDBusConnection standIn = QDBusConnection::connectToBus(sDaemonAddress, csmStandInConnection);
    QVERIFY2(standIn.isConnected(), qPrintable(standIn.lastError().message()));
    mpStandIn = new StandInA11yStatus(&mStandInObject);
    QVERIFY(standIn.registerObject(csmPath, &mStandInObject));
    QVERIFY(standIn.registerService(csmService));

    // Every case below would pass against a desktop with no screen reader, so
    // prove the style's connection is on the private bus before trusting them
    const QString privateBus = busId(standIn);
    QVERIFY(!privateBus.isEmpty());
    QCOMPARE(busId(QDBusConnection::sessionBus()), privateBus);
}

void MudletProxyStyleTest::init()
{
    if (mpStandIn) {
        mpStandIn->mScreenReaderEnabled = false;
        mpStandIn->mEnabled = false;
    }
}

void MudletProxyStyleTest::cleanupTestCase()
{
    if (mpStandIn) {
        QDBusConnection standIn(csmStandInConnection);
        standIn.unregisterService(csmService);
        standIn.unregisterObject(csmPath);
    }
    QDBusConnection::disconnectFromBus(csmStandInConnection);
    if (spDaemon) {
        spDaemon->terminate();
        spDaemon->waitForFinished();
        delete spDaemon;
        spDaemon = nullptr;
    }
}

bool MudletProxyStyleTest::announce(const QVariantMap& changedProperties)
{
    QDBusMessage signal = QDBusMessage::createSignal(csmPath, csmPropertiesInterface, qsl("PropertiesChanged"));
    signal << csmStatusInterface << changedProperties << QStringList();
    return QDBusConnection(csmStandInConnection).send(signal);
}

// A bus keeps the order of the messages between two connections, and Qt hands
// them on in that order. So once the stand-in has answered this, whatever the
// style asked of the bus before it has been handled, and every signal the
// stand-in sent before answering has reached the style.
bool MudletProxyStyleTest::roundTrip()
{
    QDBusMessage request = QDBusMessage::createMethodCall(csmService, csmPath, csmPropertiesInterface, qsl("Get"));
    request << csmStatusInterface << qsl("IsEnabled");
    QDBusPendingCallWatcher watcher(QDBusConnection::sessionBus().asyncCall(request));
    QSignalSpy finished(&watcher, &QDBusPendingCallWatcher::finished);
    return finished.wait();
}

// QSKIP returns from the function it is written in, so this cannot be a helper.
// CI sets MUDLET_TEST_REQUIRE_DBUS where dbus-daemon is installed, so a skip
// there would be hiding these cases rather than reporting an environment.
#define SKIP_WITHOUT_PRIVATE_BUS()                                                                                                                                                                     \
    if (!mpStandIn) {                                                                                                                                                                                  \
        if (qEnvironmentVariableIsSet("MUDLET_TEST_REQUIRE_DBUS")) {                                                                                                                                   \
            QFAIL("MUDLET_TEST_REQUIRE_DBUS is set, but dbus-daemon is not installed");                                                                                                                \
        }                                                                                                                                                                                              \
        QSKIP("dbus-daemon is not installed, so there is no private session bus to detect a screen reader on");                                                                                        \
    }

void MudletProxyStyleTest::detectsAScreenReaderAlreadyRunning()
{
    SKIP_WITHOUT_PRIVATE_BUS();
    mpStandIn->mScreenReaderEnabled = true;
    MudletProxyStyle style;
    QTRY_COMPARE(altKeyNavigation(style), 1);
}

void MudletProxyStyleTest::followsAScreenReaderStartingAndStopping()
{
    SKIP_WITHOUT_PRIVATE_BUS();
    MudletProxyStyle style;
    QVERIFY(roundTrip());
    QCOMPARE(altKeyNavigation(style), 0);

    mpStandIn->mScreenReaderEnabled = true;
    QVERIFY(announce({{qsl("ScreenReaderEnabled"), true}}));
    QTRY_COMPARE(altKeyNavigation(style), 1);

    mpStandIn->mScreenReaderEnabled = false;
    QVERIFY(announce({{qsl("ScreenReaderEnabled"), false}}));
    QTRY_COMPARE(altKeyNavigation(style), 0);
}

// Most X11 sessions start the accessibility bus with no screen reader running,
// and moving focus for them is what made sighted players lose the command line
// to a mistimed Alt keybinding (Mudlet/Mudlet#4280)
void MudletProxyStyleTest::ignoresAnAccessibilityBusWithoutAScreenReader()
{
    SKIP_WITHOUT_PRIVATE_BUS();
    MudletProxyStyle style;
    QVERIFY(roundTrip());

    mpStandIn->mEnabled = true;
    QVERIFY(announce({{qsl("IsEnabled"), true}}));
    QVERIFY(roundTrip());
    QCOMPARE(altKeyNavigation(style), 0);
}

void MudletProxyStyleTest::neverOverridesADetectedScreenReader()
{
    SKIP_WITHOUT_PRIVATE_BUS();
    mpStandIn->mScreenReaderEnabled = true;
    MudletProxyStyle style;
    QTRY_COMPARE(altKeyNavigation(style), 1);

    MudletProxyStyle::setAltKeyNavigation(enums::MenuBarAltKeyNavigation::Never);
    QCOMPARE(altKeyNavigation(style), 0);
}

void MudletProxyStyleTest::detectsNothingWithoutAnAccessibilityBus()
{
    SKIP_WITHOUT_PRIVATE_BUS();
    // Were the style to reach the stand-in after all, it would read true
    mpStandIn->mScreenReaderEnabled = true;
    QVERIFY(QDBusConnection(csmStandInConnection).unregisterService(csmService));

    QTest::ignoreMessage(QtDebugMsg, QRegularExpression(qsl("org\\.freedesktop\\.DBus\\.Error\\.ServiceUnknown")));
    MudletProxyStyle style;
    QVERIFY(roundTrip());
    QCOMPARE(altKeyNavigation(style), 0);
}
#else
void MudletProxyStyleTest::initMain() {}
void MudletProxyStyleTest::initTestCase() {}
void MudletProxyStyleTest::init() {}
void MudletProxyStyleTest::cleanupTestCase() {}

const char* const csmOffLinux = "screen readers are only detected over D-Bus on Linux and the BSDs";

void MudletProxyStyleTest::detectsAScreenReaderAlreadyRunning()
{
    QSKIP(csmOffLinux);
}
void MudletProxyStyleTest::followsAScreenReaderStartingAndStopping()
{
    QSKIP(csmOffLinux);
}
void MudletProxyStyleTest::ignoresAnAccessibilityBusWithoutAScreenReader()
{
    QSKIP(csmOffLinux);
}
void MudletProxyStyleTest::neverOverridesADetectedScreenReader()
{
    QSKIP(csmOffLinux);
}
void MudletProxyStyleTest::detectsNothingWithoutAnAccessibilityBus()
{
    QSKIP(csmOffLinux);
}
#endif

#include "MudletProxyStyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(MudletProxyStyleTest)
