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
    // outranks XDG_CONFIG_HOME (MudletApp::resolveConfigRoot()), so the child
    // would read the real install's Mudlet.ini for its interface language
    // instead of the sandbox's and answer in a language the rows below do not
    // expect. Note the directory checked is the application's, not this test
    // binary's. AppStartupTeardownTest and DialogTeardownTest skip for the same
    // reason.
    static bool portableMarkerWouldWin()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QFileInfo(appBinary()).absolutePath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    static QProcessEnvironment childEnvironment(const QTemporaryDir& sandbox, const QString& platformPlugin, const QString& systemLocale)
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
        environment.insert(qsl("LC_ALL"), systemLocale);
        environment.insert(qsl("LANG"), systemLocale);
        // Set, it stops applyHighDpiRoundingPolicyFromConfig() reading Mudlet.ini,
        // and so changes which reader is first to parse it and hear of its errors
        environment.remove(qsl("QT_SCALE_FACTOR_ROUNDING_POLICY"));
        // A root of its own per run, so the child reads none of the developer's
        // profiles and leaves nothing behind.
        environment.insert(qsl("XDG_CONFIG_HOME"), qsl("%1/config").arg(sandbox.path()));
        environment.insert(qsl("XDG_DATA_HOME"), qsl("%1/data").arg(sandbox.path()));
        // Keeps a WITH_SENTRY build's crashpad database out of the real cache
        environment.insert(qsl("XDG_CACHE_HOME"), qsl("%1/cache").arg(sandbox.path()));
        environment.insert(qsl("MUDLET_TEST_MODE"), qsl("1"));
        return environment;
    }

    // Reports rather than asserts: a QVERIFY2 in a helper returns from the
    // helper only, which would let a failing row carry on into its later checks.
    // An unreadable settings file is a directory in its place, as a test run as
    // root reads a file whatever its permissions say. Raw settings contents are
    // written byte for byte, for a file QSettings itself would never write.
    static RunOutcome runChild(const QStringList& arguments,
                               const QString& platformPlugin,
                               const QString& settingsLanguage,
                               const bool settingsUnreadable = false,
                               const QByteArray& settingsContents = QByteArray(),
                               const QString& systemLocale = qsl("C"))
    {
        RunOutcome outcome;

        QTemporaryDir sandbox;
        if (!sandbox.isValid()) {
            outcome.processFailure = sandbox.errorString();
            return outcome;
        }
        // Creating mudlet/profiles is what makes XDG_CONFIG_HOME outrank the
        // legacy ~/.config/mudlet, see MudletApp::xdgConfigDir()
        if (!QDir().mkpath(qsl("%1/config/mudlet/profiles").arg(sandbox.path()))) {
            outcome.processFailure = qsl("could not make a config root under %1").arg(sandbox.path());
            return outcome;
        }
        if (settingsUnreadable && !QDir().mkpath(qsl("%1/config/mudlet/Mudlet.ini").arg(sandbox.path()))) {
            outcome.processFailure = qsl("could not put a directory where the settings file goes under %1").arg(sandbox.path());
            return outcome;
        }
        if (!settingsContents.isEmpty()) {
            QFile settingsFile(qsl("%1/config/mudlet/Mudlet.ini").arg(sandbox.path()));
            if (!settingsFile.open(QIODevice::WriteOnly) || settingsFile.write(settingsContents) != settingsContents.size()) {
                outcome.processFailure = qsl("could not write the settings file under %1").arg(sandbox.path());
                return outcome;
            }
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
        mudlet.setProcessEnvironment(childEnvironment(sandbox, platformPlugin, systemLocale));
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
        // they must not be compacted into a letter that means something to
        // Mudlet - the 'h' of -stylesheet answering with the help page. That
        // they are not called unknown either is checked on standard error, by
        // diagnosticsStayOffStandardOutput()
        QTest::newRow("-stylesheet, then --version") << QStringList{qsl("-stylesheet"), qsl("headless-version-test.qss"), qsl("--version")} << bogusPlatformPlugin() << QString() << versionMarker
                                                     << QStringList{qsl("Report bugs to"), qsl("Warning:")};
        QTest::newRow("--reverse --version") << QStringList{qsl("--reverse"), qsl("--version")} << bogusPlatformPlugin() << QString() << versionMarker
                                             << QStringList{qsl("Report bugs to"), qsl("Warning:")};

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

    // Standard output is what a script reading --version parses, so anything
    // said about the command line goes to standard error instead
    void diagnosticsStayOffStandardOutput_data()
    {
        QTest::addColumn<QStringList>("arguments");
        // Text only the page asked for prints, and text only the other page
        // prints, so a run that printed nothing, the other page or both fails
        QTest::addColumn<QString>("outputMarker");
        QTest::addColumn<QString>("otherPageMarker");
        QTest::addColumn<QStringList>("errorMustContain");
        QTest::addColumn<QStringList>("errorMustNotContain");

        const QString versionMarker{qsl("GPLv3")};
        const QString helpMarker{qsl("Report bugs to")};
        QTest::newRow("--bogus --version") << QStringList{qsl("--bogus"), qsl("--version")} << versionMarker << helpMarker << QStringList{qsl("Unknown option 'bogus'")} << QStringList();
        QTest::newRow("--bogus --help") << QStringList{qsl("--bogus"), qsl("--help")} << helpMarker << versionMarker << QStringList{qsl("Unknown option 'bogus'")} << QStringList();

        // Qt's own options that the help text does not document are still
        // Qt's, and not unknown
        const QStringList notUnknown{qsl("Unknown option")};
        // A real plugin here would outrank the bogus one in the environment, so
        // a run that built a QApplication would start and pass
        QTest::newRow("-platform <bogus> --version") << QStringList{qsl("-platform"), bogusPlatformPlugin(), qsl("--version")} << versionMarker << helpMarker << QStringList() << notUnknown;
        QTest::newRow("-session x --version") << QStringList{qsl("-session"), qsl("x"), qsl("--version")} << versionMarker << helpMarker << QStringList() << notUnknown;
        QTest::newRow("-qwindowgeometry 100x100 --help") << QStringList{qsl("-qwindowgeometry"), qsl("100x100"), qsl("--help")} << helpMarker << versionMarker << QStringList() << notUnknown;
        QTest::newRow("-qwindowtitle t -display :0 --version") << QStringList{qsl("-qwindowtitle"), qsl("t"), qsl("-display"), qsl(":0"), qsl("--version")} << versionMarker << helpMarker
                                                               << QStringList() << notUnknown;

        // Every option Qt 6 strips from a GUI run's list, written out here
        // rather than read from main(), with the xcb plugin's as well. A flag
        // declared as taking a value would take --version as it, and leave the
        // run with a QApplication that cannot start.
        for (const QString& flag : {qsl("dograb"), qsl("nograb"), qsl("qdebug"), qsl("qdevel"), qsl("reverse"), qsl("sync"), qsl("testability"), qsl("widgetcount")}) {
            for (const QString& spelling : {qsl("-%1").arg(flag), qsl("--%1").arg(flag)}) {
                QTest::newRow(qPrintable(qsl("%1 --version").arg(spelling))) << QStringList{spelling, qsl("--version")} << versionMarker << helpMarker << QStringList() << notUnknown;
            }
        }
        // An option declared as a flag would leave the -h after it to ask for
        // the help page, so only one that takes it as its value prints the
        // version alone
        for (const QString& valued : {qsl("style"),
                                      qsl("stylesheet"),
                                      qsl("qmljsdebugger"),
                                      qsl("platform"),
                                      qsl("platformpluginpath"),
                                      qsl("platformtheme"),
                                      qsl("plugin"),
                                      qsl("qwindowgeometry"),
                                      qsl("qwindowicon"),
                                      qsl("qwindowtitle"),
                                      qsl("session"),
                                      qsl("display"),
                                      qsl("geometry"),
                                      qsl("icon"),
                                      qsl("name"),
                                      qsl("title"),
                                      qsl("visual")}) {
            QTest::newRow(qPrintable(qsl("-%1 -h --version").arg(valued)))
                    << QStringList{qsl("-%1").arg(valued), qsl("-h"), qsl("--version")} << versionMarker << helpMarker << QStringList() << notUnknown;
            QTest::newRow(qPrintable(qsl("--%1=v --version").arg(valued))) << QStringList{qsl("--%1=v").arg(valued), qsl("--version")} << versionMarker << helpMarker << QStringList() << notUnknown;
        }
    }

    void diagnosticsStayOffStandardOutput()
    {
        QFETCH(QStringList, arguments);
        QFETCH(QString, outputMarker);
        QFETCH(QString, otherPageMarker);
        QFETCH(QStringList, errorMustContain);
        QFETCH(QStringList, errorMustNotContain);

        QVERIFY2(QFileInfo::exists(appBinary()), qPrintable(qsl("no application binary at %1").arg(appBinary())));
        if (portableMarkerWouldWin()) {
            QSKIP("a portable.txt would send the child at the real config instead of the sandbox");
        }

        const RunOutcome outcome = runChild(arguments, bogusPlatformPlugin(), QString());
        const QString where = arguments.join(QChar::Space);
        QVERIFY2(outcome.processFailure.isEmpty(), qPrintable(qsl("mudlet %1: %2").arg(where, outcome.processFailure)));

        const QString diagnostics = qsl("%1, stdout:\n%2\nstderr:\n%3").arg(where, outcome.standardOutput, outcome.standardError);
        QVERIFY2(!outcome.standardError.contains(qsl("QGuiApplication")), qPrintable(qsl("main() reached GUI code with no QApplication; %1").arg(diagnostics)));
        QVERIFY2(outcome.exitStatus == QProcess::NormalExit && outcome.exitCode == 0, qPrintable(qsl("mudlet %1 did not exit cleanly; %2").arg(where, diagnostics)));
        QVERIFY2(!outcome.standardOutput.contains(qsl("Warning:")), qPrintable(qsl("mudlet %1 warned on standard output; %2").arg(where, diagnostics)));
        QVERIFY2(outcome.standardOutput.contains(outputMarker), qPrintable(qsl("mudlet %1 printed nothing that says \"%2\"; %3").arg(where, outputMarker, diagnostics)));
        QVERIFY2(!outcome.standardOutput.contains(otherPageMarker), qPrintable(qsl("mudlet %1 printed the other page too, with \"%2\"; %3").arg(where, otherPageMarker, diagnostics)));

        for (const QString& expected : errorMustContain) {
            QVERIFY2(outcome.standardError.contains(expected), qPrintable(qsl("mudlet %1 said nothing on standard error that says \"%2\"; %3").arg(where, expected, diagnostics)));
        }
        for (const QString& unwanted : errorMustNotContain) {
            QVERIFY2(!outcome.standardError.contains(unwanted), qPrintable(qsl("mudlet %1 said \"%2\" on standard error; %3").arg(where, unwanted, diagnostics)));
        }
    }

    // A GUI run reparses the list its QApplication has tidied, so what it calls
    // unknown is whatever Qt left there: on any platform but xcb, the xcb
    // plugin's options. The child is stopped once its main window is being built.
    void guiRunWarnsOnStandardError_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::addColumn<QStringList>("errorMustContain");
        QTest::addColumn<QStringList>("errorMustNotContain");

        // Written with '=' so that no value is left over as a package to open,
        // which a second Mudlet would hand to one already running
        QTest::newRow("--bogus") << QStringList{qsl("--bogus")} << QStringList{qsl("Unknown option 'bogus'")} << QStringList();
        QTest::newRow("--title=t, not on xcb") << QStringList{qsl("--title=t")} << QStringList{qsl("Unknown option 'title'")} << QStringList();
        QTest::newRow("--qwindowtitle=t -qdevel") << QStringList{qsl("--qwindowtitle=t"), qsl("-qdevel")} << QStringList() << QStringList{qsl("Unknown option")};
    }

    void guiRunWarnsOnStandardError()
    {
        QFETCH(QStringList, arguments);
        QFETCH(QStringList, errorMustContain);
        QFETCH(QStringList, errorMustNotContain);

#if !defined(Q_OS_LINUX)
        // On Windows main() reattaches a GUI run's streams to the parent's
        // console, and Qt logs a GUI application to the debugger there
        QSKIP("a GUI run's standard error only reaches this test on Linux");
#endif
        QVERIFY2(QFileInfo::exists(appBinary()), qPrintable(qsl("no application binary at %1").arg(appBinary())));
        if (portableMarkerWouldWin()) {
            QSKIP("a portable.txt would send the child at the real config instead of the sandbox");
        }

        QTemporaryDir sandbox;
        QVERIFY2(sandbox.isValid(), qPrintable(sandbox.errorString()));
        QVERIFY(QDir().mkpath(qsl("%1/config/mudlet/profiles").arg(sandbox.path())));

        QProcess mudlet;
        mudlet.setProcessEnvironment(childEnvironment(sandbox, qsl("offscreen"), qsl("C")));
        mudlet.setProgram(appBinary());
        mudlet.setArguments(arguments);
        mudlet.start();
        const QString where = arguments.join(QChar::Space);
        QVERIFY2(mudlet.waitForStarted(scmStartTimeoutMs), qPrintable(qsl("mudlet %1: %2").arg(where, mudlet.errorString())));

        // Logged by mudlet's constructor, which runs after the command line has been read
        const QString startedMarker{qsl("has been detected as the style factory in use")};
        QString standardError;
        QElapsedTimer waited;
        waited.start();
        while (!standardError.contains(startedMarker) && mudlet.state() == QProcess::Running && waited.elapsed() < 2 * scmFinishTimeoutMs) {
            mudlet.waitForReadyRead(1000);
            standardError += QString::fromUtf8(mudlet.readAllStandardError());
        }
        mudlet.kill();
        mudlet.waitForFinished(scmFinishTimeoutMs);
        standardError += QString::fromUtf8(mudlet.readAllStandardError());
        const QString standardOutput = QString::fromUtf8(mudlet.readAllStandardOutput());

        const QString diagnostics = qsl("%1, stdout:\n%2\nstderr:\n%3").arg(where, standardOutput, standardError);
        QVERIFY2(standardError.contains(startedMarker), qPrintable(qsl("mudlet %1 never got as far as its main window; %2").arg(where, diagnostics)));
        QVERIFY2(!standardOutput.contains(qsl("Warning:")), qPrintable(qsl("mudlet %1 warned on standard output; %2").arg(where, diagnostics)));
        for (const QString& expected : errorMustContain) {
            QVERIFY2(standardError.contains(expected), qPrintable(qsl("mudlet %1 said nothing on standard error that says \"%2\"; %3").arg(where, expected, diagnostics)));
        }
        for (const QString& unwanted : errorMustNotContain) {
            QVERIFY2(!standardError.contains(unwanted), qPrintable(qsl("mudlet %1 said \"%2\" on standard error; %3").arg(where, unwanted, diagnostics)));
        }
    }

    // A language asked for and not given has to say so, on standard error, or
    // English help reads as though none had been set
    void languageProblemsAreReported_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::addColumn<QString>("settingsLanguage");
        QTest::addColumn<bool>("settingsUnreadable");
        QTest::addColumn<QByteArray>("settingsContents");
        QTest::addColumn<QString>("systemLocale");
        QTest::addColumn<QStringList>("errorMustContain");
        QTest::addColumn<QStringList>("errorMustNotContain");

        const QStringList help{qsl("--help")};
        const QByteArray notIni("[General]\ninterfaceLanguage=de_DE\n[Broken\n");

        QTest::newRow("--help, settings ask for a language with no translation") << help << qsl("sw_KE") << false << QByteArray() << qsl("C") << QStringList{qsl("\"sw_KE\"")} << QStringList();
        QTest::newRow("--help, settings ask for no locale at all") << help << qsl("xx-not-a-locale") << false << QByteArray() << qsl("C") << QStringList{qsl("\"xx-not-a-locale\"")} << QStringList();
        QTest::newRow("--help, settings file unreadable") << help << QString() << true << QByteArray() << qsl("C") << QStringList{qsl("Mudlet.ini\" is not readable")} << QStringList();
        // Qt keeps the keys it could parse, so the German asked for here may
        // still be applied, and the warning must not say otherwise
        QTest::newRow("--help, settings file not valid INI") << help << QString() << false << notIni << qsl("C") << QStringList{qsl("Mudlet.ini\" is not valid INI")}
                                                             << QStringList{qsl("is not used")};
        QTest::newRow("--version, settings file not valid INI") << QStringList{qsl("--version")} << QString() << false << notIni << qsl("C") << QStringList{qsl("Mudlet.ini\" is not valid INI")}
                                                                << QStringList();
        // A language nobody set is not worth a warning, even when the system's
        // has no translation
        QTest::newRow("--help, system locale with no translation") << help << QString() << false << QByteArray() << qsl("sw_KE.UTF-8") << QStringList() << QStringList{qsl("WARNING")};
        QTest::newRow("--help, settings ask for German") << help << qsl("de_DE") << false << QByteArray() << qsl("C") << QStringList() << QStringList{qsl("WARNING")};
    }

    void languageProblemsAreReported()
    {
        QFETCH(QStringList, arguments);
        QFETCH(QString, settingsLanguage);
        QFETCH(bool, settingsUnreadable);
        QFETCH(QByteArray, settingsContents);
        QFETCH(QString, systemLocale);
        QFETCH(QStringList, errorMustContain);
        QFETCH(QStringList, errorMustNotContain);

        QVERIFY2(QFileInfo::exists(appBinary()), qPrintable(qsl("no application binary at %1").arg(appBinary())));
        if (portableMarkerWouldWin()) {
            QSKIP("a portable.txt would send the child at the real config instead of the sandbox");
        }

        const RunOutcome outcome = runChild(arguments, bogusPlatformPlugin(), settingsLanguage, settingsUnreadable, settingsContents, systemLocale);
        const QString where = arguments.join(QChar::Space);
        QVERIFY2(outcome.processFailure.isEmpty(), qPrintable(qsl("mudlet %1: %2").arg(where, outcome.processFailure)));

        const QString diagnostics = qsl("%1, stdout:\n%2\nstderr:\n%3").arg(where, outcome.standardOutput, outcome.standardError);
        QVERIFY2(!outcome.standardError.contains(qsl("QGuiApplication")), qPrintable(qsl("main() reached GUI code with no QApplication; %1").arg(diagnostics)));
        QVERIFY2(outcome.exitStatus == QProcess::NormalExit && outcome.exitCode == 0, qPrintable(qsl("mudlet %1 did not exit cleanly; %2").arg(where, diagnostics)));

        for (const QString& expected : errorMustContain) {
            QVERIFY2(outcome.standardError.contains(expected), qPrintable(qsl("mudlet %1 said nothing on standard error that says \"%2\"; %3").arg(where, expected, diagnostics)));
        }
        for (const QString& unwanted : errorMustNotContain) {
            QVERIFY2(!outcome.standardError.contains(unwanted), qPrintable(qsl("mudlet %1 said \"%2\" on standard error; %3").arg(where, unwanted, diagnostics)));
        }
    }
};

#include "HeadlessVersionTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessVersionTest)
