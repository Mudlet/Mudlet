
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

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "mudlet.h"

#include "GroupedTest.h"

// The main window's file pickers and clipboard as an embedder's code reaches them without waiting for the player
class AppFrontendAsyncTest : public QObject
{
    Q_OBJECT

private:
    // Stands in for none in what the callbacks are told
    inline static const QString scmNoAnswer = qsl("<no answer>");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static QFileDialog* shownFileDialog()
    {
        for (auto* pWidget : QApplication::topLevelWidgets()) {
            if (auto* pDialog = qobject_cast<QFileDialog*>(pWidget); pDialog && pDialog->isVisible()) {
                return pDialog;
            }
        }
        return nullptr;
    }

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
        // A native dialog would run its own loop, which no timer here could answer
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        QVERIFY(TAppFrontend::hasView());
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aFileChosenIsToldOnceTheDialogAnswers()
    {
        const QString chosen = mConfigDir.filePath(qsl("chosen.txt"));
        QFile file(chosen);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        std::optional<QString> answer;
        TAppFrontend::instance()->getOpenFileNameAsync(qsl("Pick a file"), mConfigDir.path(), this, [&answer](const std::optional<QString>& path) {
            answer = path.value_or(scmNoAnswer);
        });
        QVERIFY2(!answer.has_value(), "The picker answered before the player could choose.");
        QFileDialog* pDialog = shownFileDialog();
        QVERIFY2(pDialog, "No file dialog was shown.");
        QVERIFY2(pDialog->parentWidget() == mudlet::self(), "The picker would outlive the main window.");
        auto* pFileName = pDialog->findChild<QLineEdit*>(qsl("fileNameEdit"));
        QVERIFY(pFileName);
        pFileName->setText(chosen);
        QMetaObject::invokeMethod(pDialog, "accept");
        QTRY_VERIFY(answer.has_value());
        QCOMPARE(QFileInfo(*answer).canonicalFilePath(), QFileInfo(chosen).canonicalFilePath());
        QTRY_VERIFY(!shownFileDialog());
    }

    void test_aCancelledFolderPickerIsToldEmpty()
    {
        std::optional<QString> answer;
        TAppFrontend::instance()->getExistingDirectoryAsync(qsl("Pick a folder"), mConfigDir.path(), this, [&answer](const std::optional<QString>& path) {
            answer = path.value_or(scmNoAnswer);
        });
        QVERIFY(!answer.has_value());
        QFileDialog* pDialog = shownFileDialog();
        QVERIFY2(pDialog, "No folder dialog was shown.");
        QCOMPARE(pDialog->fileMode(), QFileDialog::Directory);
        pDialog->reject();
        QTRY_VERIFY(answer.has_value());
        QCOMPARE(*answer, QString());
    }

    void test_aPickerWhoseAskerHasGoneClosesAndTellsNothing()
    {
        bool told = false;
        {
            QObject asker;
            TAppFrontend::instance()->getOpenFileNameAsync(qsl("Pick a file"), mConfigDir.path(), &asker, [&told](const std::optional<QString>&) {
                told = true;
            });
        }
        // Nobody is left to tell what is picked, so it is not left up for the player
        QTRY_VERIFY(!shownFileDialog());
        QTest::qWait(50);
        QVERIFY(!told);
    }

    void test_theClipboardIsReadFromTheEventLoop()
    {
        TAppFrontend::instance()->setClipboardText(qsl("set without waiting"));
        QCOMPARE(QGuiApplication::clipboard()->text(), qsl("set without waiting"));
        std::optional<QString> answer;
        TAppFrontend::instance()->clipboardTextAsync(this, [&answer](const std::optional<QString>& text) {
            answer = text.value_or(scmNoAnswer);
        });
        QVERIFY(!answer.has_value());
        QTRY_VERIFY(answer.has_value());
        QCOMPARE(*answer, qsl("set without waiting"));
    }
};

#include "AppFrontendAsyncTest.moc"
MUDLET_GROUPED_TEST_MAIN(AppFrontendAsyncTest)
