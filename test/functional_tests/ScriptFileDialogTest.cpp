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
 * invokeFileDialog() puts up a modal file dialog and hands the script what the
 * player picked. A spec cannot answer a modal dialog, so this does it from a
 * timer that runs inside the dialog's own event loop.
 */

#include <QFileDialog>
#include <QLineEdit>
#include <QPointer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ScriptFileDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mPickDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Script-File-Dialog-Test-Host");
    const QString mLocalhost = qsl("localhost");
    QString mDialogTitle;
    QString mDialogStartDir;
    QFileDialog::FileMode mDialogFileMode = QFileDialog::AnyFile;
    QPointer<QTimer> mpPoll;

    // Picks path in the next file dialog to open, noting how it was set up. It
    // keeps trying while the dialog stays up, and cancels it after about ten
    // seconds so a dialog that never takes the answer fails the test, not hangs it.
    void answerNextFileDialog(const QString& path)
    {
        mDialogTitle.clear();
        mDialogStartDir.clear();
        auto* poll = new QTimer(this);
        mpPoll = poll;
        poll->setInterval(20ms);
        connect(poll, &QTimer::timeout, this, [this, path, attempts = 0]() mutable {
            QFileDialog* dialog = nullptr;
            const QWidgetList widgets = QApplication::topLevelWidgets();
            for (QWidget* widget : widgets) {
                if (auto* candidate = qobject_cast<QFileDialog*>(widget); candidate && candidate->isVisible()) {
                    dialog = candidate;
                    break;
                }
            }
            if (!dialog) {
                return;
            }
            // QFileDialog redeclares accept() as protected; QDialog's public slots reach the same overrides
            auto* asDialog = static_cast<QDialog*>(dialog);
            if (++attempts > 500) {
                asDialog->reject();
                return;
            }
            // A refused answer is reported in a message box over the dialog
            if (auto* box = qobject_cast<QDialog*>(QApplication::activeModalWidget()); box && box != dialog) {
                box->reject();
                return;
            }
            if (mDialogStartDir.isEmpty()) {
                mDialogStartDir = dialog->directory().absolutePath();
            }
            mDialogTitle = dialog->windowTitle();
            mDialogFileMode = dialog->fileMode();
            // A folder dialog answers with the folder it is showing, a file dialog
            // with the name typed into it. The name is typed rather than passed to
            // selectFile(), which leaves the field alone while it has the focus.
            if (dialog->fileMode() == QFileDialog::Directory) {
                dialog->setDirectory(path);
            } else {
                auto* nameField = dialog->findChild<QLineEdit*>(qsl("fileNameEdit"));
                if (!nameField) {
                    return;
                }
                dialog->setDirectory(QFileInfo(path).absolutePath());
                nameField->setText(QFileInfo(path).fileName());
            }
            // Queued, as Qt holds back a timer while its own slot runs, so this poll
            // could not dismiss a message box that accept() raised from inside it
            QMetaObject::invokeMethod(asDialog, &QDialog::accept, Qt::QueuedConnection);
        });
        poll->start();
    }

    QString luaString(const QString& name) const
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name.toUtf8().constData());
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

private slots:
    void initTestCase()
    {
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mPickDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mHostname);

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            TestProfile::removeProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Polling stops here rather than once the dialog closes, so a refused accept is retried
    void cleanup() { delete mpPoll; }

    void test_aFileIsPickedWhenTheFirstArgumentIsTrue()
    {
        const QString file = qsl("%1/picked.txt").arg(mPickDir.path());
        QFile touch(file);
        QVERIFY(touch.open(QIODevice::WriteOnly));
        touch.close();

        answerNextFileDialog(file);
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("_picked = invokeFileDialog(true, 'Pick a file', [[%1]])").arg(mPickDir.path())));

        QCOMPARE(mDialogTitle, qsl("Pick a file"));
        QCOMPARE(mDialogFileMode, QFileDialog::ExistingFile);
        QCOMPARE(mDialogStartDir, QDir(mPickDir.path()).absolutePath());
        QCOMPARE(luaString(qsl("_picked")), file);
    }

    void test_aFolderIsPickedWhenTheFirstArgumentIsFalse()
    {
        const QString folder = qsl("%1/chosen").arg(mPickDir.path());
        QVERIFY(QDir().mkpath(folder));
        // Not mPickDir, which the dialog may remember from the file test and open in anyway
        const QString start = qsl("%1/start").arg(mPickDir.path());
        QVERIFY(QDir().mkpath(start));

        answerNextFileDialog(folder);
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("_picked = invokeFileDialog(false, 'Pick a folder', [[%1]])").arg(start)));

        QCOMPARE(mDialogTitle, qsl("Pick a folder"));
        QCOMPARE(mDialogFileMode, QFileDialog::Directory);
        QCOMPARE(mDialogStartDir, QDir(start).absolutePath());
        QCOMPARE(luaString(qsl("_picked")), folder);
    }
};

#include "ScriptFileDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(ScriptFileDialogTest)
