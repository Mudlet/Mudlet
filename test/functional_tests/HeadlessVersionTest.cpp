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
 * --help and --version only print text, so they have to work where there is no
 * display: a packaging script, a container, a machine logged into over ssh.
 * They are answered inside main(), which a test that links mudlet_core cannot
 * reach, so the shipped binary is run as a child here - as
 * AppStartupTeardownTest does for the way out of main().
 *
 * main() decides from the raw command line, before any application object
 * exists, whether a run only prints text. Getting that decision wrong in either
 * direction is what the rows below pin: a run that is given a QCoreApplication
 * and then carries on into window code aborts on "Must construct a
 * QGuiApplication", which every row checks for whatever else it asserts.
 *
 * Two ways of denying the child a display are used. Taking DISPLAY,
 * WAYLAND_DISPLAY and QT_QPA_PLATFORM out of its environment is the real-world
 * shape, but it only bites on Linux - on macOS and Windows a QApplication
 * constructs without any of them. Naming a platform plugin that does not exist
 * denies one everywhere, and it is also proof of which application object was
 * built: QT_QPA_PLATFORM is read by QGuiApplication and ignored entirely by
 * QCoreApplication, so a run that prints and exits 0 under a bogus plugin name
 * never constructed the GUI one.
 *
 * Run with: ctest -R HeadlessVersionTest -V
 */

#include <QtTest/QtTest>

#include "GroupedTest.h"

#ifndef MUDLET_APP_BINARY
// Set for the whole teardown group by test/functional_tests/CMakeLists.txt. Say
// plainly what is missing if this file is ever built outside that group, rather
// than failing inside appBinary() with an undeclared identifier.
#error "HeadlessVersionTest needs MUDLET_APP_BINARY - see test/functional_tests/CMakeLists.txt"
#endif

class HeadlessVersionTest : public QObject
{
    Q_OBJECT

private:
    static constexpr int scmStartTimeoutMs = 10000;
    static constexpr int scmFinishTimeoutMs = 20000;

    static QString appBinary() { return QString::fromUtf8(MUDLET_APP_BINARY); }

    // Names a plugin no Qt build ships, so a QGuiApplication cannot start
    // whatever the runner's display situation is
    static QString bogusPlatformPlugin() { return qsl("no-such-platform-xyz"); }

    struct RunOutcome
    {
        int exitCode = -1;
        QProcess::ExitStatus exitStatus = QProcess::CrashExit;
        QString standardOutput;
        QString standardError;
        // Empty when the child started and finished on its own
        QString processFailure;
    };

    // A portable.txt beside the shipped binary, or in $HOME/.config/mudlet,
    // outranks XDG_CONFIG_HOME (MudletPaths::resolveConfigRoot()), so the child
    // would read the real install's Mudlet.ini for its interface language
    // instead of the sandbox's and answer in a language the rows below do not
    // expect. Note the directory checked is the application's, not this test
    // binary's. AppStartupTeardownTest and DialogTeardownTest skip for the same
    // reason.
    static bool portableMarkerWouldWin()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QFileInfo(appBinary()).absolutePath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    static QProcessEnvironment childEnvironment(const QTemporaryDir& sandbox, const QString& platformPlugin)
    {
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        // ctest sets QT_QPA_PLATFORM=offscreen for every functional test and the
        // child would inherit it - that is the one setting that hid this bug.
        environment.remove(qsl("QT_QPA_PLATFORM"));
        environment.remove(qsl("DISPLAY"));
        environment.remove(qsl("WAYLAND_DISPLAY"));
        if (!platformPlugin.isEmpty()) {
            environment.insert(qsl("QT_QPA_PLATFORM"), platformPlugin);
        }
        // Both texts are translated, so the markers the rows look for only hold
        // in English. The C locale also makes loadTranslationsForCommandLine()
        // return early, rather than the rows depending on which catalogues this
        // build ships.
        environment.insert(qsl("LC_ALL"), qsl("C"));
        environment.insert(qsl("LANG"), qsl("C"));
        // A root of its own per run, so the child reads none of the developer's
        // profiles and leaves nothing behind.
        environment.insert(qsl("XDG_CONFIG_HOME"), qsl("%1/config").arg(sandbox.path()));
        environment.insert(qsl("XDG_DATA_HOME"), qsl("%1/data").arg(sandbox.path()));
        // Keeps a WITH_SENTRY build's crashpad database out of the real cache
        environment.insert(qsl("XDG_CACHE_HOME"), qsl("%1/cache").arg(sandbox.path()));
        environment.insert(qsl("MUDLET_TEST_MODE"), qsl("1"));
        // The TLS warm-up main() starts leaves Qt's CA store loaded for the
        // process lifetime, so a leak check here would only ever report that.
        // Appended rather than replacing what ctest set, since the runtime takes
        // the last setting of a flag and the earlier ones stay.
        const QString inheritedSanitizerOptions = environment.value(qsl("ASAN_OPTIONS"));
        environment.insert(qsl("ASAN_OPTIONS"), inheritedSanitizerOptions.isEmpty() ? qsl("detect_leaks=0") : qsl("%1:detect_leaks=0").arg(inheritedSanitizerOptions));
        return environment;
    }

    // Reports rather than asserts: a QVERIFY2 in a helper returns from the
    // helper only, which would let a failing row carry on into its later checks
    static RunOutcome runChild(const QStringList& arguments, const QString& platformPlugin, const QString& settingsLanguage)
    {
        RunOutcome outcome;

        QTemporaryDir sandbox;
        if (!sandbox.isValid()) {
            outcome.processFailure = sandbox.errorString();
            return outcome;
        }
        // Creating mudlet/profiles is what makes XDG_CONFIG_HOME outrank the
        // legacy ~/.config/mudlet, see MudletPaths::xdgConfigDir()
        if (!QDir().mkpath(qsl("%1/config/mudlet/profiles").arg(sandbox.path()))) {
            outcome.processFailure = qsl("could not make a config root under %1").arg(sandbox.path());
            return outcome;
        }
        if (!settingsLanguage.isEmpty()) {
            QSettings settings(qsl("%1/config/mudlet/Mudlet.ini").arg(sandbox.path()), QSettings::IniFormat);
            settings.setValue(qsl("interfaceLanguage"), settingsLanguage);
            settings.sync();
            if (settings.status() != QSettings::NoError) {
                outcome.processFailure = qsl("could not write a settings file under %1").arg(sandbox.path());
                return outcome;
            }
        }

        QProcess mudlet;
        mudlet.setProcessEnvironment(childEnvironment(sandbox, platformPlugin));
        mudlet.setProgram(appBinary());
        mudlet.setArguments(arguments);
        mudlet.start();

        if (!mudlet.waitForStarted(scmStartTimeoutMs)) {
            outcome.processFailure = mudlet.errorString();
            return outcome;
        }
        if (!mudlet.waitForFinished(scmFinishTimeoutMs)) {
            mudlet.kill();
            mudlet.waitForFinished(scmFinishTimeoutMs);
            outcome.processFailure = qsl("never finished");
            return outcome;
        }

        outcome.exitCode = mudlet.exitCode();
        outcome.exitStatus = mudlet.exitStatus();
        outcome.standardOutput = QString::fromUtf8(mudlet.readAllStandardOutput());
        outcome.standardError = QString::fromUtf8(mudlet.readAllStandardError());
        return outcome;
    }

private slots:
    void printsAndExits_data()
    {
        QTest::addColumn<QStringList>("arguments");
        // Empty asks for the display variables to be taken away instead
        QTest::addColumn<QString>("platformPlugin");
        // Empty leaves the sandbox without a settings file at all
        QTest::addColumn<QString>("settingsLanguage");
        QTest::addColumn<QStringList>("mustContain");
        QTest::addColumn<QStringList>("mustNotContain");

        // The licence line appears only in the version text and the bug address
        // only in the help text, so a row that answers the other option fails
        const QStringList versionMarker{qsl("GPLv3")};
        const QStringList helpMarker{qsl("Report bugs to")};

        QTest::newRow("--version, no display") << QStringList{qsl("--version")} << QString() << QString() << versionMarker << helpMarker;
        QTest::newRow("-v, no display") << QStringList{qsl("-v")} << QString() << QString() << versionMarker << helpMarker;
        QTest::newRow("--help, no display") << QStringList{qsl("--help")} << QString() << QString() << helpMarker << versionMarker;
        QTest::newRow("-h, no display") << QStringList{qsl("-h")} << QString() << QString() << helpMarker << versionMarker;
        // Help wins when both are given, as the order of the two branches in
        // main() has always made it
        QTest::newRow("--help --version, no display") << QStringList{qsl("--help"), qsl("--version")} << QString() << QString() << helpMarker << versionMarker;

        // Answering under a platform plugin that cannot load is what proves the
        // run was given a QCoreApplication rather than a QApplication
        QTest::newRow("--version, no platform plugin") << QStringList{qsl("--version")} << bogusPlatformPlugin() << QString() << versionMarker << helpMarker;
        QTest::newRow("--help, no platform plugin") << QStringList{qsl("--help")} << bogusPlatformPlugin() << QString() << helpMarker << versionMarker;

        // A single-dash group main() does not know must not swallow the option
        // that follows it: -p and -o take a value, so reading such a group as
        // compacted short options makes --version the name of a profile and
        // leaves the run with no application object of either kind
        QTest::newRow("-sp --version") << QStringList{qsl("-sp"), qsl("--version")} << bogusPlatformPlugin() << QString() << versionMarker << helpMarker;
        QTest::newRow("-so --version") << QStringList{qsl("-so"), qsl("--version")} << bogusPlatformPlugin() << QString() << versionMarker << helpMarker;
        QTest::newRow("-mp -h") << QStringList{qsl("-mp"), qsl("-h")} << bogusPlatformPlugin() << QString() << helpMarker << versionMarker;

        // Qt's own options are still in the list such a run is read from, so
        // they must neither be compacted into a letter that means something to
        // Mudlet - the 'h' of -stylesheet answering with the help page - nor be
        // reported as unknown
        QTest::newRow("-stylesheet, then --version") << QStringList{qsl("-stylesheet"), qsl("headless-version-test.qss"), qsl("--version")} << bogusPlatformPlugin() << QString() << versionMarker
                                                     << QStringList{qsl("Report bugs to"), qsl("Warning:")};
        QTest::newRow("--reverse --version") << QStringList{qsl("--reverse"), qsl("--version")} << bogusPlatformPlugin() << QString() << versionMarker
                                             << QStringList{qsl("Report bugs to"), qsl("Warning:")};

        // An option that really is unknown is still tolerated, and still said so
        QTest::newRow("--bogus --version") << QStringList{qsl("--bogus"), qsl("--version")} << bogusPlatformPlugin() << QString() << QStringList{qsl("GPLv3"), qsl("Warning:")} << helpMarker;

        // These texts are translated before they are printed, and the interface
        // language is read from the settings file without setupConfig() having
        // resolved the config root first. A root read from the wrong place, or
        // not read at all, would simply answer in English - so the row asks for
        // a language and checks the English wording is gone. The bug address is
        // a URL, so it survives translation.
        QTest::newRow("--help, settings ask for German") << QStringList{qsl("--help")} << bogusPlatformPlugin() << qsl("de_DE") << QStringList{qsl("github.com/Mudlet/Mudlet/issues")}
                                                         << QStringList{qsl("Usage:")};
    }

    void printsAndExits()
    {
        QFETCH(QStringList, arguments);
        QFETCH(QString, platformPlugin);
        QFETCH(QString, settingsLanguage);
        QFETCH(QStringList, mustContain);
        QFETCH(QStringList, mustNotContain);

        QVERIFY2(QFileInfo::exists(appBinary()), qPrintable(qsl("no application binary at %1").arg(appBinary())));
        if (portableMarkerWouldWin()) {
            QSKIP("a portable.txt would send the child at the real config instead of the sandbox");
        }

        const RunOutcome outcome = runChild(arguments, platformPlugin, settingsLanguage);
        const QString where = arguments.join(QChar::Space);
        QVERIFY2(outcome.processFailure.isEmpty(), qPrintable(qsl("mudlet %1: %2").arg(where, outcome.processFailure)));

        const QString diagnostics = qsl("%1, stderr:\n%2").arg(where, outcome.standardError);
        // Whichever application object a run is given, it must never reach
        // window code without a QApplication - Qt names the one it wanted when
        // it does, whatever the argv shape that got it there
        QVERIFY2(!outcome.standardError.contains(qsl("QGuiApplication")), qPrintable(qsl("main() reached GUI code with no QApplication; %1").arg(diagnostics)));
        QVERIFY2(outcome.exitStatus == QProcess::NormalExit, qPrintable(qsl("mudlet %1 was killed; %2").arg(where, diagnostics)));
        QVERIFY2(outcome.exitCode == 0, qPrintable(qsl("mudlet %1 exited %2; %3").arg(where, QString::number(outcome.exitCode), diagnostics)));

        for (const QString& expected : mustContain) {
            QVERIFY2(outcome.standardOutput.contains(expected), qPrintable(qsl("mudlet %1 printed nothing that says \"%2\"; %3").arg(where, expected, diagnostics)));
        }
        for (const QString& unwanted : mustNotContain) {
            QVERIFY2(!outcome.standardOutput.contains(unwanted), qPrintable(qsl("mudlet %1 answered with \"%2\" in it; %3").arg(where, unwanted, diagnostics)));
        }
    }
};

#include "HeadlessVersionTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessVersionTest)
