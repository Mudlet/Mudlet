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
 * What THeadlessSession tells an app that draws the main console as the GUI
 * draws it. Each test makes a session of its own, and with it the process's one
 * profile pool.
 *
 * Run with: ctest -R HeadlessSessionOutputTest -V
 */

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

class HeadlessSessionOutputTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name, const QString& script, const QString& hostXml = QString())
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host>%2</Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSessionOutput</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped(), hostXml)
                               .toUtf8());
        return true;
    }

    static QList<QVariantList> wrapsSent(const QSignalSpy& spy)
    {
        QList<QVariantList> wraps;
        for (const auto& arguments : spy) {
            wraps << arguments.mid(1);
        }
        return wraps;
    }

    static QVariantList wrap(const int wrapAt, const int indent, const int hangingIndent) { return {wrapAt, indent, hangingIndent}; }

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

    void test_theWrapAProfileLoadsWithIsKnownAtProfileLoaded()
    {
        const QString name = qsl("Headless Saved Wrap");
        QVERIFY(writeProfile(name, QString(), qsl("<wrapAt>80</wrapAt><wrapIndentCount>3</wrapIndentCount><wrapHangingIndentCount>5</wrapHangingIndentCount>")));
        THeadlessSession session;
        QCOMPARE(session.consoleWrap(), THeadlessSession::ConsoleWrap{});
        std::optional<THeadlessSession::ConsoleWrap> loadedWith;
        connect(&session, &THeadlessSession::profileLoaded, this, [&]() {
            loadedWith = session.consoleWrap();
        });
        QSignalSpy wraps(&session, &THeadlessSession::consoleWrapChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(loadedWith);
        QCOMPARE(loadedWith->wrapAt, 80);
        QCOMPARE(loadedWith->indent, 3);
        QCOMPARE(loadedWith->hangingIndent, 5);
        QTest::qWait(100);
        QCOMPARE(wraps.count(), 0);
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.consoleWrap(), THeadlessSession::ConsoleWrap{});
    }

    void test_eachWrapSetterReachesTheSession()
    {
        const QString name = qsl("Headless Wrap");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy wraps(&session, &THeadlessSession::consoleWrapChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QVERIFY(session.runLua(qsl(R"lua(setWindowWrap("main", 60))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(wraps.count(), 1, 10000);
        QCOMPARE(wraps.constFirst().at(0).toString(), name);
        QVERIFY(session.runLua(qsl(R"lua(setWindowWrapIndent("main", 2))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(wraps.count(), 2, 10000);
        QVERIFY(session.runLua(qsl(R"lua(setWindowWrapHangingIndent("main", 4))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(wraps.count(), 3, 10000);
        QCOMPARE(wrapsSent(wraps), (QList<QVariantList>{wrap(60, 0, 0), wrap(60, 2, 0), wrap(60, 2, 4)}));
        QCOMPARE(session.consoleWrap(), (THeadlessSession::ConsoleWrap{60, 2, 4}));

        // The same again, a mini console's, and a change undone before the event loop runs
        QVERIFY(session.runLua(qsl(R"lua(
setWindowWrap("main", 60) setWindowWrapIndent("main", 2) setWindowWrapHangingIndent("main", 4)
createMiniConsole("headlessWrapMini", 0, 0, 200, 100)
setWindowWrap("headlessWrapMini", 30) setWindowWrapIndent("headlessWrapMini", 1) setWindowWrapHangingIndent("headlessWrapMini", 1)
setWindowWrap("main", 61) setWindowWrap("main", 60)
)lua")));
        QTest::qWait(100);
        QCOMPARE(wraps.count(), 3);
        QVERIFY(session.shutdown(false));
    }

    void test_aWrapChangeOwedToAProfileLetGoIsNotSent()
    {
        const QString name = qsl("Headless Wrap Let Go");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy wraps(&session, &THeadlessSession::consoleWrapChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(setWindowWrap("main", 70))lua")));
        QVERIFY(session.shutdown(false));
        QTest::qWait(100);
        QCOMPARE(wraps.count(), 0);
    }
};

#include "HeadlessSessionOutputTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionOutputTest)
