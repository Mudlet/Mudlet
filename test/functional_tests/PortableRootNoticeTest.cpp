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
 * The notice shown when portable.txt names a data directory Mudlet cannot use.
 *
 * It must never wait to be dismissed, since a headless `mudlet --profile Foo`
 * has nobody to dismiss it, and main() opens it rather than init(), which runs
 * before the window it sits on is up. main()'s call has no test here.
 * ConfigDirOverrideTest covers the fallback itself but never runs init().
 *
 * Run with: ctest -R PortableRootNoticeTest -V
 */

#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <QApplication>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

class PortableRootNoticeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mHome;
    QString mConfigDir;
    QString mMarker;
    QByteArray mSavedHome;
    QByteArray mSavedXdg;
    // Dismisses any modal that waits in a nested event loop, so one that does
    // fails the case instead of stalling it until ctest's timeout
    QTimer mProbe;
    QStringList mDismissed;

    // Every widget in the process, not the main window's child tree: a change that
    // stopped parenting the notice under the main window would make the cases that
    // assert nothing is shown pass by finding nothing to look at. The parenting is
    // pinned separately, by the case that asserts parentWidget().
    static QList<QMessageBox*> shownNotices()
    {
        QList<QMessageBox*> shown;
        for (auto* widget : QApplication::allWidgets()) {
            auto* box = qobject_cast<QMessageBox*>(widget);
            if (box && box->isVisible()) {
                shown.append(box);
            }
        }
        return shown;
    }

    bool writeRejectedMarker() const
    {
        QFile marker(mMarker);
        if (!marker.open(QIODevice::WriteOnly)) {
            return false;
        }
        const QByteArray target = qsl("%1/no-such-parent/portable").arg(mHome.path()).toUtf8();
        return marker.write(target) == target.size();
    }

private slots:
    void initTestCase()
    {
        mSavedHome = qgetenv("HOME");
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
#ifdef Q_OS_WIN
        QSKIP("QDir::homePath() does not follow HOME on Windows");
#endif
        QVERIFY(mHome.isValid());
        qputenv("HOME", mHome.path().toUtf8());
        qunsetenv("XDG_CONFIG_HOME");
        if (portableMarkerPresent()) {
            QSKIP("portable.txt beside the executable outranks the one this test writes");
        }
        mConfigDir = qsl("%1/.config/mudlet").arg(mHome.path());
        QVERIFY(QDir().mkpath(qsl("%1/profiles").arg(mConfigDir)));
        mMarker = qsl("%1/portable.txt").arg(mConfigDir);
        QVERIFY(writeRejectedMarker());

        mudlet::start();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("which Mudlet cannot use")));
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), mConfigDir);
        MudletApp::getQSettings()->setValue(qsl("uiTourShown"), true);
        MudletApp::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));

        mProbe.setInterval(20);
        connect(&mProbe, &QTimer::timeout, this, [this]() {
            QWidget* modal = QApplication::activeModalWidget();
            if (!modal) {
                return;
            }
            mDismissed.append(QString::fromLatin1(modal->metaObject()->className()));
            if (auto* dialog = qobject_cast<QDialog*>(modal)) {
                dialog->reject();
            } else {
                modal->close();
            }
        });
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedHome.isNull() ? qunsetenv("HOME") : qputenv("HOME", mSavedHome);
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // setupConfig() runs more than once in a process, and the notice may only
    // name the marker that lost the resolution main() is about to show
    void test_aResolutionThatGoesThroughForgetsAnEarlierRejectedMarker()
    {
        QVERIFY(QFile::remove(mMarker));
        mudlet::self()->setupConfig();
        mudlet::self()->warnAboutRejectedPortableRoot();
        QVERIFY2(shownNotices().isEmpty(), "the notice named a portable.txt that no longer governs anything");

        // and put the rejected marker back, which is what the cases below need
        QVERIFY(writeRejectedMarker());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("which Mudlet cannot use")));
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), mConfigDir);
    }

    // mudlet --profile Foo opens no connection dialog, so the notice falls back to
    // the main window. That is the arm the headless case actually takes, and the
    // dialog case below cannot reach it.
    void test_theNoticeFallsBackToTheMainWindowWithNoConnectionDialog()
    {
        QVERIFY2(!mudlet::self()->mpConnectionDialog, "this case has to run before the connection dialog is up");

        mDismissed.clear();
        mProbe.start();
        mudlet::self()->warnAboutRejectedPortableRoot();
        mProbe.stop();

        QVERIFY2(mDismissed.isEmpty(), "the notice waited to be dismissed, and a headless run has nobody to dismiss it");
        const QList<QMessageBox*> shown = shownNotices();
        QCOMPARE(shown.size(), 1);
        QMessageBox* notice = shown.first();
        QVERIFY2(notice->parentWidget() == mudlet::self(), "with no connection dialog the notice has to sit on the main window");
        QVERIFY2(notice->text().contains(mMarker), qPrintable(qsl("the notice does not name %1: \"%2\"").arg(mMarker, notice->text())));
        notice->close();

        // and leave a rejected marker for the cases below, which the call above consumed
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("which Mudlet cannot use")));
        mudlet::self()->setupConfig();
    }

    void test_initLeavesTheNoticeToMain()
    {
        mDismissed.clear();
        mProbe.start();
        mudlet::self()->init();
        mProbe.stop();

        QVERIFY2(mDismissed.isEmpty(), qPrintable(qsl("init() waited on a %1").arg(mDismissed.value(0))));
        QVERIFY2(shownNotices().isEmpty(), "init() opened the notice, before the window it sits on is up");
    }

    void test_noticeOpensOnTheConnectionDialogWithoutWaiting()
    {
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5000));

        mDismissed.clear();
        mProbe.start();
        mudlet::self()->warnAboutRejectedPortableRoot();
        mProbe.stop();

        QVERIFY2(mDismissed.isEmpty(), "the notice waited to be dismissed, and a headless run has nobody to dismiss it");
        const QList<QMessageBox*> shown = shownNotices();
        QCOMPARE(shown.size(), 1);
        QMessageBox* notice = shown.first();
        QVERIFY2(notice->text().contains(mMarker), qPrintable(qsl("the notice does not name %1: \"%2\"").arg(mMarker, notice->text())));
        QVERIFY2(notice->informativeText().contains(mConfigDir), qPrintable(qsl("the notice does not name %1: \"%2\"").arg(mConfigDir, notice->informativeText())));
        QVERIFY2(notice->parentWidget() == mudlet::self()->mpConnectionDialog, "the notice must sit on the connection dialog, which covers the main window");
        notice->close();
    }

    // Deliberately runs on the state the case above leaves behind: it showed the
    // notice, which consumed the rejected marker. main() calls this on every
    // startup, so a second call with nothing left to report must open nothing.
    void test_noticeStaysShutWithoutARejectedMarker()
    {
        QVERIFY(shownNotices().isEmpty());
        mudlet::self()->warnAboutRejectedPortableRoot();

        QVERIFY2(shownNotices().isEmpty(), "a second call, with the marker already reported, must open nothing");
    }

    // The store is parented to the application rather than the main window
    // precisely so the Updater can keep using it after the window deletes itself
    // on close. Declared last because QtTest runs slots in declaration order and
    // this one destroys the singleton every case above needs - cleanupTestCase()
    // is declared up with initTestCase(), so it is no guide to what runs when.
    void test_theSettingsStoreOutlivesTheMainWindow()
    {
        auto* settings = MudletApp::getQSettings();
        QVERIFY(settings);
        settings->setValue(qsl("portableRootNoticeProbe"), 42);
        settings->sync();

        delete mudlet::self();

        QVERIFY2(MudletApp::getQSettings(), "the settings store went away with the main window");
        QCOMPARE(MudletApp::getQSettings()->value(qsl("portableRootNoticeProbe")).toInt(), 42);
    }
};

#include "PortableRootNoticeTest.moc"
MUDLET_GROUPED_TEST_MAIN(PortableRootNoticeTest)
