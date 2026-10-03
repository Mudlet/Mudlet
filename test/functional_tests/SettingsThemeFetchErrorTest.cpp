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
 * under the theme preview. The reason is a network error message, which is far
 * longer than the width of the page - and the page is capped to a reading width
 * that it does not scroll sideways, so a line too long for it is simply cut off
 * and the part that says what went wrong is never seen.
 *
 * A spec cannot reach this: the settings dialog is C++ only, with no Lua way to
 * open it or to measure what it draws.
 *
 * Run with: ctest -R SettingsThemeFetchErrorTest -V
 */

#include <QDir>
#include <QTemporaryDir>
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

class SettingsThemeFetchErrorTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QByteArray mSavedNoThemeDownload;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsThemeFetchError-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    // A message shaped like a real theme-fetch failure. The production path
    // cannot run here (the fetch is suppressed, see initTestCase), so this is
    // set on the label by hand: what is under test is that the label wraps it,
    // not that the error reaches it. Kept in step with the wording built at
    // dlgProfilePreferences::maybeDownloadEditorThemes().
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
        mSavedNoThemeDownload.isNull() ? qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD") : qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", mSavedNoThemeDownload);
    }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    void test_aFailedThemeUpdateIsReadableAtTheDefaultSize()
    {
        openPreferences(QSize(1060, 760));
        verifyTheWholeErrorIsOnShow();
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
