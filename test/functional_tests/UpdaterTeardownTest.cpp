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
 * The timing is pinned from both sides: the dialog must still be alive when
 * the last window closes (its purpose is to offer an update at exactly that
 * point, #9388), and must be gone once the event loop has exited. Note that
 * with no update to offer the dialog's own last-window-closed handler calls
 * quit(), which in Qt 6 emits aboutToQuit synchronously - so the dialog is
 * destroyed inside that cascade, before close() even returns.
 *
 * QTEST_APPLESS_MAIN is used because the test itself must own the
 * QApplication lifetime to walk it through quit and destruction.
 */
class UpdaterTeardownTest : public QObject
{
    Q_OBJECT

private slots:
    void updateDialogDestroyedBeforeApplicationTeardown();
};

void UpdaterTeardownTest::updateDialogDestroyedBeforeApplicationTeardown()
{
    // Keeps checkUpdatesOnStart() away from real user data - on Windows it
    // deletes stale installer files from the genuine GenericDataLocation
    QStandardPaths::setTestModeEnabled(true);

    QTemporaryDir settingsDir;
    QVERIFY(settingsDir.isValid());
    QSettings settings(settingsDir.filePath(qsl("updater-test.ini")), QSettings::IniFormat);

    int argc = 1;
    char appName[] = "UpdaterTeardownTest";
    char* argv[] = {appName, nullptr};
    const auto app = std::make_unique<QApplication>(argc, argv);

    auto* updater = new Updater(app.get(), &settings);

    // Connected before checkUpdatesOnStart() creates the dialog, so with
    // direct connections firing in connection order this probe runs before
    // the dialog's own last-window-closed handler - the last moment the
    // dialog is guaranteed to exist, as that handler quits when there is no
    // update to offer and the quit destroys the dialog
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

    auto* window = new QWidget;
    window->show();
    QTimer::singleShot(0, app.get(), [&window]() {
        window->close();
        delete window;
        // The dialog's own last-window-closed handler quits when no update is
        // available; quit explicitly so the test cannot hang if an update is
        // available (the dialog then shows itself and waits for the user)
        QCoreApplication::quit();
    });
    app->exec();

    QVERIFY2(dialogAliveAtLastWindowClosed, "the UpdateDialog must still be alive when the last window closes so it can offer an update at that point - see #9388");
    QVERIFY2(dialog.isNull(), "UpdateDialog must be destroyed when the application quits: deleting it any later (from ~Updater, inside the application's destructor) corrupts the heap - see #9122");
}

QTEST_APPLESS_MAIN(UpdaterTeardownTest)
#include "UpdaterTeardownTest.moc"
