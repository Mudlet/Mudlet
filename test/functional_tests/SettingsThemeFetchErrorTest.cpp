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
 * When the editor themes cannot be fetched, the Editor settings page says why
 * under the theme preview, and keeps saying so while the dialog is open. The
 * reason is often a network error message, which is far longer than the width
 * of the page - and the page is capped to a reading width that it does not
 * scroll sideways, so a line too long for it is simply cut off and the part
 * that says what went wrong is never seen.
 *
 * The fetch itself runs here too, from a file or a local server rather than the
 * internet, so each way it can fail does so the same way every run.
 *
 * A spec cannot reach this: the settings dialog is C++ only, with no Lua way to
 * open it or to measure what it draws.
 *
 * Run with: ctest -R SettingsThemeFetchErrorTest -V
 */

#include <QDir>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTranslator>
#include <QtTest/QtTest>

#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <memory>
#include <zip.h>

class SettingsThemeFetchErrorTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mCacheDir;
    QByteArray mSavedXdg;
    QByteArray mSavedXdgCache;
    QByteArray mSavedNoThemeDownload;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsThemeFetchError-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    // A message shaped like a real theme-fetch failure, set on the label by hand
    // for the layout tests: what they test is that the label wraps it. Kept in
    // step with the wording built at dlgProfilePreferences::maybeDownloadEditorThemes().
    QByteArray mServedArchive;
    const QString mError = qsl("Could not update themes: Error transferring https://github.com/Colorsublime/Colorsublime-Themes/archive/master.zip - server replied: Not Found");

    void selectCategory(const QString& key)
    {
        auto* pList = TestSettings::sidebar(mpPreferences);
        QVERIFY2(pList, "the settings shell has no category sidebar");
        const int row = TestSettings::rowOf(mpPreferences, key);
        QVERIFY2(row >= 0, qPrintable(qsl("no sidebar item for category '%1'").arg(key)));
        pList->setCurrentRow(row);
        QCoreApplication::processEvents();
    }

    void openPreferences(const QSize& size)
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(size);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
    }

    // A label's height-for-width is recalculated on a layout request that is
    // posted, not delivered, by setText() - and answering it posts the next one
    // up the chain. Delivering them is deterministic where a sleep is not.
    void flushLayout()
    {
        for (int pass = 0; pass < 4; ++pass) {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QCoreApplication::processEvents();
        }
    }

    QLabel* showTheError()
    {
        selectCategory(qsl("editor"));
        QLabel* pLabel = mpPreferences->theme_download_label;
        if (!pLabel) {
            return nullptr;
        }

        pLabel->show();
        pLabel->setText(mError);
        flushLayout();
        return pLabel;
    }

    // The page does not scroll sideways, so every line of the message has to be
    // laid out inside the label's own width for any of it to be readable.
    void verifyTheWholeErrorIsOnShow()
    {
        QLabel* pLabel = showTheError();
        QVERIFY2(pLabel, "the Editor page has no label to report a failed theme update in");

        const QFontMetrics metrics(pLabel->font());
        const int available = pLabel->contentsRect().width();
        QVERIFY2(metrics.horizontalAdvance(mError) > available,
                 qPrintable(qsl("SETUP: the message needs %1px and has %2px, so it was never at risk of being cut off")
                                    .arg(QString::number(metrics.horizontalAdvance(mError)), QString::number(available))));

        const QRect wrapped = metrics.boundingRect(QRect(0, 0, available, 0), Qt::TextWordWrap, mError);
        QVERIFY2(pLabel->height() >= wrapped.height(),
                 qPrintable(qsl("the failed theme update is %1px tall wrapped into the %2px the page allows it, but the label is only %3px tall, so the rest of the message is cut off")
                                    .arg(QString::number(wrapped.height()), QString::number(available), QString::number(pLabel->height()))));
        // wrapping cannot break inside a word, so a long enough run of
        // unbreakable characters would still run off the right-hand edge
        QVERIFY2(wrapped.width() <= available,
                 qPrintable(qsl("the failed theme update needs %1px on its widest line but the page allows it %2px, so that line is cut off at the edge")
                                    .arg(QString::number(wrapped.width()), QString::number(available))));
    }

    // The fetch is let through for this one dialog, from the given URL in place
    // of the real one
    void fetchThemesFrom(const QUrl& url)
    {
        MudletApp::getQSettings()->setValue(qsl("colorSublimeThemesURL"), url.toString());
        qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        openPreferences(QSize(1060, 760));
        selectCategory(qsl("editor"));
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");
        MudletApp::getQSettings()->remove(qsl("colorSublimeThemesURL"));
    }

    // Laid out as the real Colorsublime archive is, with one theme in it
    bool writeThemesArchive(const QString& path)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        static const QByteArray themes = R"([{"Title": "Fetched Theme", "FileName": "Fetched.tmTheme"}])";
        static const QByteArray theme = "<plist/>";
        const std::pair<const char*, const QByteArray*> entries[] = {{"Colorsublime-Themes-master/themes.json", &themes}, {"Colorsublime-Themes-master/themes/Fetched.tmTheme", &theme}};
        for (const auto& [name, pContents] : entries) {
            zip_source* source = zip_source_buffer(archive, pContents->constData(), pContents->size(), 0);
            if (!source || zip_file_add(archive, name, source, ZIP_FL_ENC_UTF_8) < 0) {
                zip_source_free(source);
                zip_discard(archive);
                return false;
            }
        }
        return zip_close(archive) == 0;
    }

    // Answers every request with what mServedArchive holds at that moment, in a
    // response the client is allowed to cache, and counts the requests
    void serveArchives(QTcpServer& server, int& requests)
    {
        connect(&server, &QTcpServer::newConnection, &server, [this, &server, &requests] {
            while (QTcpSocket* pClient = server.nextPendingConnection()) {
                auto request = std::make_shared<QByteArray>();
                connect(pClient, &QTcpSocket::readyRead, pClient, [this, pClient, request, &requests] {
                    request->append(pClient->readAll());
                    if (!request->contains("\r\n\r\n")) {
                        return;
                    }
                    request->clear();
                    ++requests;
                    pClient->write(qsl("HTTP/1.1 200 OK\r\nContent-Type: application/zip\r\nContent-Length: %1\r\nCache-Control: max-age=3600\r\n\r\n").arg(mServedArchive.size()).toLatin1()
                                   + mServedArchive);
                });
            }
        });
    }

    QByteArray readFile(const QString& path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    // The log is the only record of a failure while the Editor page is not on show
    void expectTheFailureToBeLogged(const QString& reason)
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("^dlgProfilePreferences: could not update the editor themes: \".*%1").arg(QRegularExpression::escape(reason))));
    }

    void verifyTheFailureIsOnShow(const QString& situation, const QString& reason = QString())
    {
        QLabel* pLabel = mpPreferences->theme_download_label;
        QTRY_VERIFY2_WITH_TIMEOUT(pLabel->text().startsWith(qsl("Could not update themes: ")) && pLabel->text().contains(reason),
                                  qPrintable(qsl("%1, but the label says \"%2\" and is %3").arg(situation, pLabel->text(), pLabel->isVisible() ? qsl("on show") : qsl("hidden"))),
                                  10000);
        QVERIFY2(pLabel->isVisible(), qPrintable(qsl("%1, and the label saying so is hidden").arg(situation)));
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
        // a fetch keeps what it downloads in the cache directory
        QVERIFY(mCacheDir.isValid());
        mSavedXdgCache = qgetenv("XDG_CACHE_HOME");
        qputenv("XDG_CACHE_HOME", mCacheDir.path().toUtf8());
        // the Editor page fetches the editor themes as it is built, and a real
        // reply landing mid-test would overwrite the message under measurement
        mSavedNoThemeDownload = qgetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestSettings::deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        mSavedXdgCache.isNull() ? qunsetenv("XDG_CACHE_HOME") : qputenv("XDG_CACHE_HOME", mSavedXdgCache);
        mSavedNoThemeDownload.isNull() ? qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD") : qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", mSavedNoThemeDownload);
    }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        // Themes unpacked by one case would be young enough for the next to skip its fetch
        const QString themesFolder = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("edbee"));
        QFileInfo(themesFolder).isDir() ? QDir(themesFolder).removeRecursively() : QFile::remove(themesFolder);
    }

    void test_aFailedThemeUpdateIsReadableAtTheDefaultSize()
    {
        openPreferences(QSize(1060, 760));
        verifyTheWholeErrorIsOnShow();
    }

    void test_aThemeDownloadThatCannotBeUnpackedSaysSo()
    {
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        QFile notAnArchive(downloads.filePath(qsl("themes.zip")));
        QVERIFY(notAnArchive.open(QIODevice::WriteOnly));
        QVERIFY(notAnArchive.write("this is not a zip archive") > 0);
        notAnArchive.close();

        const QString reason = qsl("the downloaded themes could not be unpacked");
        expectTheFailureToBeLogged(reason);
        fetchThemesFrom(QUrl::fromLocalFile(notAnArchive.fileName()));
        if (QTest::currentTestFailed()) {
            return;
        }
        verifyTheFailureIsOnShow(reason, reason);
    }

    // The label is in the user's language, the log in the one a bug report is read in
    void test_anUnpackFailureIsLoggedUntranslated()
    {
        class Translator : public QTranslator
        {
        public:
            bool isEmpty() const override { return false; }
            QString translate(const char* context, const char* sourceText, const char*, int) const override
            {
                return qstrcmp(context, "dlgProfilePreferences") == 0 && qstrcmp(sourceText, "the downloaded themes could not be unpacked") == 0 ? qsl("TRANSLATED") : QString();
            }
        } translator;
        QCoreApplication::installTranslator(&translator);
        auto restore = qScopeGuard([&translator] {
            QCoreApplication::removeTranslator(&translator);
        });

        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        QFile notAnArchive(downloads.filePath(qsl("themes.zip")));
        QVERIFY(notAnArchive.open(QIODevice::WriteOnly));
        QVERIFY(notAnArchive.write("this is not a zip archive") > 0);
        notAnArchive.close();

        expectTheFailureToBeLogged(qsl("the downloaded themes could not be unpacked"));
        fetchThemesFrom(QUrl::fromLocalFile(notAnArchive.fileName()));
        if (QTest::currentTestFailed()) {
            return;
        }
        verifyTheFailureIsOnShow(qsl("the themes could not be unpacked"), qsl("TRANSLATED"));
    }

    // The themes on offer are still not the latest a few seconds later, so the
    // message must not go away - nor be swapped for one saying an update is under way
    void test_aFailedThemeDownloadStaysOnShow()
    {
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        const QString missing = qsl("there-is-no-such-file.zip");
        expectTheFailureToBeLogged(missing);
        fetchThemesFrom(QUrl::fromLocalFile(downloads.filePath(missing)));
        if (QTest::currentTestFailed()) {
            return;
        }
        verifyTheFailureIsOnShow(qsl("the themes could not be downloaded"), missing);
        if (QTest::currentTestFailed()) {
            return;
        }
        const QString failure = mpPreferences->theme_download_label->text();

        QTest::qWait(6000);
        QVERIFY2(mpPreferences->theme_download_label->isVisible(), "the failed theme update was hidden again after a few seconds");
        QCOMPARE(mpPreferences->theme_download_label->text(), failure);
    }

    // Coming back to the dialog re-reads every setting into it, the Editor page included
    void test_aFailedThemeUpdateIsStillOnShowAfterComingBackToTheDialog()
    {
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        const QString missing = qsl("there-is-no-such-file.zip");
        expectTheFailureToBeLogged(missing);
        fetchThemesFrom(QUrl::fromLocalFile(downloads.filePath(missing)));
        if (QTest::currentTestFailed()) {
            return;
        }
        verifyTheFailureIsOnShow(qsl("the themes could not be downloaded"), missing);
        if (QTest::currentTestFailed()) {
            return;
        }

        QEvent activation(QEvent::WindowActivate);
        QCoreApplication::sendEvent(mpPreferences, &activation);
        QCoreApplication::processEvents();
        QVERIFY2(mpPreferences->theme_download_label->isVisible(), "the failed theme update was hidden on coming back to the dialog");
        QVERIFY(mpPreferences->theme_download_label->text().contains(missing));
    }

    void test_aThemeArchiveThatCannotBeSavedSaysSo()
    {
#if defined(Q_OS_WIN)
        QSKIP("the temporary directory is steered by TMPDIR on Unix only");
#endif
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        const QString archive = downloads.filePath(qsl("themes.zip"));
        QVERIFY(writeThemesArchive(archive));
        // a temporary file cannot be made inside a file
        const QByteArray savedTmpDir = qgetenv("TMPDIR");
        qputenv("TMPDIR", archive.toUtf8());
        auto restore = qScopeGuard([&savedTmpDir] {
            savedTmpDir.isNull() ? qunsetenv("TMPDIR") : qputenv("TMPDIR", savedTmpDir);
        });

        expectTheFailureToBeLogged(QString());
        fetchThemesFrom(QUrl::fromLocalFile(archive));
        if (QTest::currentTestFailed()) {
            return;
        }
        verifyTheFailureIsOnShow(qsl("the downloaded themes could not be saved"));
        QVERIFY2(!mpPreferences->theme_download_label->text().contains(qsl("unpacked")), "the archive could not be saved, but the label blames unpacking it");
    }

    void test_aSuccessfulThemeUpdateListsTheNewThemesAndPutsTheLabelAway()
    {
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        const QString archive = downloads.filePath(qsl("themes.zip"));
        QVERIFY(writeThemesArchive(archive));

        MudletApp::getQSettings()->setValue(qsl("colorSublimeThemesURL"), QUrl::fromLocalFile(archive).toString());
        qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        openPreferences(QSize(1060, 760));
        QSignalSpy completed(mpPreferences, &dlgProfilePreferences::signal_themeUpdateCompleted);
        selectCategory(qsl("editor"));
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");
        MudletApp::getQSettings()->remove(qsl("colorSublimeThemesURL"));

        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 10000);
        QVERIFY2(!mpPreferences->theme_download_label->isVisible(), qPrintable(qsl("the themes were updated, but the label still says \"%1\"").arg(mpPreferences->theme_download_label->text())));
        QVERIFY(mpPreferences->code_editor_theme_selection_combobox->findText(qsl("Fetched Theme")) >= 0);
    }

    // The profile's half of the dialog is greyed out and the label put away by then
    void test_aThemeUpdateFailingAfterTheProfileClosedStaysOutOfSight()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        expectTheFailureToBeLogged(qsl("Not Found"));
        fetchThemesFrom(QUrl(qsl("http://127.0.0.1:%1/themes.zip").arg(server.serverPort())));
        if (QTest::currentTestFailed()) {
            return;
        }
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        QTcpSocket* pClient = server.nextPendingConnection();

        mpPreferences->slot_handleHostDeletion(mpHost);
        QVERIFY(!mpPreferences->theme_download_label->isVisible());
        pClient->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");

        QTRY_VERIFY_WITH_TIMEOUT(mpPreferences->theme_download_label->text().contains(qsl("Not Found")), 10000);
        QVERIFY2(!mpPreferences->theme_download_label->isVisible(), "the failed theme update was put on show under a profile that has gone");
    }

    // Mudlet cannot tell what is wrong with the folder from why unpacking stopped,
    // but the log has to name it for anyone to put it right
    void test_aThemeFolderThatCannotBeWrittenToIsNamedInTheLog()
    {
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        const QString archive = downloads.filePath(qsl("themes.zip"));
        QVERIFY(writeThemesArchive(archive));
        QFile inTheWay(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("edbee")));
        QVERIFY(inTheWay.open(QIODevice::WriteOnly));
        inTheWay.close();

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("^utils::unzip\\(.*could not create the folder \".*edbee/Colorsublime-Themes-master")));
        expectTheFailureToBeLogged(qsl("the downloaded themes could not be unpacked"));
        fetchThemesFrom(QUrl::fromLocalFile(archive));
        if (QTest::currentTestFailed()) {
            return;
        }
        verifyTheFailureIsOnShow(qsl("the themes folder is a file"), qsl("unpacked"));
    }

    // A captive portal answers with a page of its own: kept in the cache, that
    // page would be what every later dialog was given in place of the themes
    void test_anArchiveThatCannotBeUnpackedIsNotFetchedFromTheCacheAgain()
    {
        QTemporaryDir downloads;
        QVERIFY(downloads.isValid());
        const QString archive = downloads.filePath(qsl("themes.zip"));
        QVERIFY(writeThemesArchive(archive));

        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        int requests = 0;
        serveArchives(server, requests);
        const QUrl url(qsl("http://127.0.0.1:%1/themes.zip").arg(server.serverPort()));

        mServedArchive = "<html>Sign in to the network</html>";
        expectTheFailureToBeLogged(qsl("the downloaded themes could not be unpacked"));
        fetchThemesFrom(url);
        if (QTest::currentTestFailed()) {
            return;
        }
        verifyTheFailureIsOnShow(qsl("the portal's page could not be unpacked"), qsl("unpacked"));
        delete mpPreferences;
        mpPreferences = nullptr;

        mServedArchive = readFile(archive);
        QVERIFY(!mServedArchive.isEmpty());
        fetchThemesFrom(url);
        if (QTest::currentTestFailed()) {
            return;
        }
        QTRY_VERIFY2_WITH_TIMEOUT(
                !mpPreferences->theme_download_label->isVisible(),
                qPrintable(qsl("the network now has the themes on offer, but after %1 request(s) the label says \"%2\"").arg(QString::number(requests), mpPreferences->theme_download_label->text())),
                10000);
        QCOMPARE(requests, 2);
    }

    // A line the reading column cannot fit does not make the page scroll to it:
    // the column is capped and the page has no horizontal scrollbar, so an
    // unwrapped message is simply clipped. This is the invariant
    // SettingsShellNavigationTest holds every page to, checked with the error
    // showing, at the narrowest the dialog will go.
    void test_aFailedThemeUpdateDoesNotMakeTheEditorPageClipOrSideScroll()
    {
        openPreferences(QSize(780, 560));
        QVERIFY(showTheError());

        QScrollArea* pPage = TestSettings::pageOf(mpPreferences, qsl("editor"));
        QVERIFY2(pPage, "the Editor category has no page");
        QWidget* pColumn = pPage->widget();
        QCOMPARE(pPage->horizontalScrollBar()->maximum(), 0);
        QVERIFY2(pColumn->width() >= pColumn->minimumSizeHint().width(),
                 qPrintable(qsl("the Editor column is %1px wide but its cards need %2px with the failed theme update showing, so it is clipping them")
                                    .arg(QString::number(pColumn->width()), QString::number(pColumn->minimumSizeHint().width()))));
    }
};

#include "SettingsThemeFetchErrorTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsThemeFetchErrorTest)
