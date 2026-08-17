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
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>

#include <memory>

/*
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
};

void NewReleaseDialogTeardownTest::updateDialogDestroyedBeforeApplicationTeardown()
{
    // Keeps checkUpdatesOnStart() away from real user data - on Windows it
    // deletes stale installer files from the genuine GenericDataLocation
    QStandardPaths::setTestModeEnabled(true);

    QTemporaryDir settingsDir;
    QVERIFY(settingsDir.isValid());
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
