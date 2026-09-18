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

    static QList<QMessageBox*> shownNotices()
    {
        QList<QMessageBox*> shown;
        for (auto* box : mudlet::self()->findChildren<QMessageBox*>()) {
            if (box->isVisible()) {
                shown.append(box);
            }
        }
        return shown;
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
        QFile marker(mMarker);
        QVERIFY(marker.open(QIODevice::WriteOnly));
        marker.write(qsl("%1/no-such-parent/portable").arg(mHome.path()).toUtf8());
        marker.close();

        mudlet::start();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("names a data directory Mudlet cannot use")));
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

    void test_noticeStaysShutWithoutARejectedMarker()
    {
        QVERIFY(shownNotices().isEmpty());
        mudlet::self()->warnAboutRejectedPortableRoot();

        QVERIFY2(shownNotices().isEmpty(), "main() calls this on every startup; with no rejected marker it must open nothing");
    }
};

#include "PortableRootNoticeTest.moc"
MUDLET_GROUPED_TEST_MAIN(PortableRootNoticeTest)
