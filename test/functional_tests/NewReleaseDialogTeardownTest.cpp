/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@mudlet.org         *
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

#include "updater.h"
#include "updater/UpdateDialog.h"
#include "utils.h"

#include <QtTest/QtTest>

#include <QApplication>
#include <QDateTime>
#include <QPointer>
#include <QScopeGuard>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>

#include <memory>

/*
 * What the Updater has to clean up after itself: the dialog it owns, and the
 * files it leaves in the temp directory.
 *
 * Regression test for https://github.com/Mudlet/Mudlet/issues/9122:
 * STATUS_HEAP_CORRUPTION at application close on Windows.
 *
 * The Updater is parented to the application object (#9388) and used to delete
 * its unparented top-level UpdateDialog in ~Updater. That destructor only runs
 * inside the application's own destructor - after ~QApplication has torn down
 * all widget infrastructure - and deleting a QWidget that late corrupts the
 * heap. The dialog must instead be destroyed on aboutToQuit, while the
 * application is still fully alive.
 *
 * The timing is pinned from three sides: the dialog must still be alive when
 * the last window closes (its purpose is to offer an update at exactly that
 * point, #9388), must survive the quit it triggers itself, and must be gone
 * once the event loop has exited.
 *
 * The middle one is https://github.com/Mudlet/Mudlet/issues/9967: quit() emits
 * aboutToQuit synchronously and the dialog quits the application when it is
 * dismissed, so an aboutToQuit handler that deletes the dialog outright frees
 * it underneath its own dismissal - and underneath the caller that dismissed
 * it. Applying an update does exactly that: it calls close() and then done()
 * on the dialog, which crashed on Windows.
 *
 * QTEST_APPLESS_MAIN is used because the test itself must own the
 * QApplication lifetime to walk it through quit and destruction.
 */
class NewReleaseDialogTeardownTest : public QObject
{
    Q_OBJECT

private slots:
    void updateDialogDestroyedBeforeApplicationTeardown();
    void staleUpdateFilesAreSweptAtStartup();
};

/*
 * Regression test for https://github.com/Mudlet/Mudlet/issues/9985: nothing ever
 * deleted the installer copied for the batch file to run, nor a download Mudlet
 * went away without recording, so each update left ~135MB in the temp directory.
 *
 * What must survive matters as much as what goes: UpdateDialog records a
 * download and reuses it on the next launch, and an installer waiting for the
 * batch file is seconds old - sweeping either turns a disk fix into a 135MB
 * re-download or a lost update.
 */
void NewReleaseDialogTeardownTest::staleUpdateFilesAreSweptAtStartup()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QStringList leftovers{qsl("mudlet-update-Mudlet-5.0.0-linux-x64.AppImage-abc123.tar"), qsl("mudlet-update-81fcfb86-2e9a-40f5-8f40-42fd5f689e5c.SDYFjK"), qsl("mudlet-setup-1786938971.exe")};
    const QStringList bystanders{qsl("mudlet-update.bat"), qsl("mudlet_updated_at"), qsl("setup.exe"), qsl("Mudlet-5.0.0-windows-64-installer.exe")};
    const QString pendingDownload = qsl("mudlet-update-still-wanted.exe");
    const QString justCopiedInstaller = qsl("mudlet-setup-1786999999.exe");

    const QDateTime longEnoughAgo = QDateTime::currentDateTime().addSecs(-7200);
    for (const QString& name : leftovers + bystanders + QStringList{pendingDownload}) {
        QFile file(tempDir.filePath(name));
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(name));
        QVERIFY(file.setFileTime(longEnoughAgo, QFileDevice::FileModificationTime));
        file.close();
    }
    QFile freshFile(tempDir.filePath(justCopiedInstaller));
    QVERIFY(freshFile.open(QIODevice::WriteOnly));
    freshFile.close();

    Updater::cleanupStaleUpdateFiles(tempDir.path(), tempDir.filePath(pendingDownload));

    for (const QString& name : leftovers) {
        QVERIFY2(!QFile::exists(tempDir.filePath(name)), qPrintable(qsl("%1 should have been swept - see #9985").arg(name)));
    }
    for (const QString& name : bystanders) {
        QVERIFY2(QFile::exists(tempDir.filePath(name)), qPrintable(qsl("%1 is not ours to delete").arg(name)));
    }
    QVERIFY2(QFile::exists(tempDir.filePath(pendingDownload)), "the download UpdateDialog means to reuse must survive, or every launch re-downloads it");
    QVERIFY2(QFile::exists(tempDir.filePath(justCopiedInstaller)), "an installer this recent may still be waiting for the batch file that runs it");
}

void NewReleaseDialogTeardownTest::updateDialogDestroyedBeforeApplicationTeardown()
{
    // Keeps checkUpdatesOnStart() away from real user data - on Windows it
    // deletes stale installer files from the genuine GenericDataLocation
    QStandardPaths::setTestModeEnabled(true);

    QTemporaryDir settingsDir;
    QVERIFY(settingsDir.isValid());

    // Test mode does not redirect TempLocation, and checkUpdatesOnStart() sweeps
    // leftover update files out of it - point it somewhere disposable so a real
    // pending update on this machine is not collected by the test run (#9985)
    QTemporaryDir sweepableTempDir;
    QVERIFY(sweepableTempDir.isValid());
    // TMPDIR is what Qt reads on Unix, TMP and TEMP on Windows
    const QList<QByteArray> tempVariables{"TMPDIR", "TMP", "TEMP"};
    QList<QByteArray> realTempDirs;
    for (const QByteArray& variable : tempVariables) {
        realTempDirs.append(qgetenv(variable.constData()));
        qputenv(variable.constData(), sweepableTempDir.path().toLocal8Bit());
    }
    const auto restoreTempDirs = qScopeGuard([&tempVariables, &realTempDirs]() {
        for (int i = 0; i < tempVariables.size(); ++i) {
            qputenv(tempVariables.at(i).constData(), realTempDirs.at(i));
        }
    });
    QSettings settings(settingsDir.filePath(qsl("updater-test.ini")), QSettings::IniFormat);

    int argc = 1;
    char appName[] = "NewReleaseDialogTeardownTest";
    char* argv[] = {appName, nullptr};
    const auto app = std::make_unique<QApplication>(argc, argv);

    auto* updater = new Updater(app.get(), &settings);

    // Connected before checkUpdatesOnStart() creates the dialog, so with
    // direct connections firing in connection order this probe runs before
    // the dialog's own last-window-closed handler, which quits the
    // application when there is no update to offer
    QPointer<dblsqd::UpdateDialog> dialog;
    bool dialogAliveAtLastWindowClosed = false;
    connect(app.get(), &QGuiApplication::lastWindowClosed, this, [&dialog, &dialogAliveAtLastWindowClosed]() {
        dialogAliveAtLastWindowClosed = !dialog.isNull();
    });

    // Also fires the feed's update check; the request is torn down with the
    // application before any response arrives and nothing below depends on it
    updater->checkUpdatesOnStart();

    const auto topLevels = QApplication::topLevelWidgets();
    for (auto* widget : topLevels) {
        if ((dialog = qobject_cast<dblsqd::UpdateDialog*>(widget))) {
            break;
        }
    }
    QVERIFY2(dialog, "expected the Updater to have created its UpdateDialog");

    // The Updater connected its own aboutToQuit handler in its constructor, so
    // this direct connection runs straight after it and sees what it did
    bool aboutToQuitFired = false;
    bool dialogAliveInAboutToQuit = false;
    connect(app.get(), &QCoreApplication::aboutToQuit, this, [&dialog, &aboutToQuitFired, &dialogAliveInAboutToQuit]() {
        aboutToQuitFired = true;
        dialogAliveInAboutToQuit = !dialog.isNull();
    });

    auto* window = new QWidget;
    window->show();
    bool dialogAliveAfterQuit = false;
    QTimer::singleShot(0, app.get(), [&window, &dialog, &dialogAliveAfterQuit]() {
        window->close();
        delete window;
        // The dialog's own last-window-closed handler quits when no update is
        // available; quit explicitly so the test cannot hang if an update is
        // available (the dialog then shows itself and waits for the user)
        QCoreApplication::quit();
        // Read before anything can return to the event loop: this is the
        // window in which applying an update goes on using the dialog
        dialogAliveAfterQuit = !dialog.isNull();
        if (!dialog.isNull()) {
            // the pair that crashed on Windows, in the order the updater runs it
            dialog->close();
            dialog->done(0);
        }
    });
    app->exec();

    QVERIFY2(dialogAliveAtLastWindowClosed, "the UpdateDialog must still be alive when the last window closes so it can offer an update at that point - see #9388");
    QVERIFY2(aboutToQuitFired, "aboutToQuit never fired, so the checks either side of it prove nothing");
    QVERIFY2(dialogAliveInAboutToQuit, "the Updater's aboutToQuit handler must not destroy the UpdateDialog outright - see #9967");
    QVERIFY2(dialogAliveAfterQuit, "the UpdateDialog must outlive the quit it triggers: applying an update keeps using it afterwards - see #9967");
    QVERIFY2(dialog.isNull(), "UpdateDialog must be destroyed when the application quits: deleting it any later (from ~Updater, inside the application's destructor) corrupts the heap - see #9122");
}

QTEST_APPLESS_MAIN(NewReleaseDialogTeardownTest)
#include "NewReleaseDialogTeardownTest.moc"
