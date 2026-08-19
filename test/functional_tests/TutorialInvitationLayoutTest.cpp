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
 * The tutorial invitation is the whole of what a brand new user sees, so it has
 * to fit: the dialog shrinks around it, and a QTextBrowser's size hint says
 * nothing about the document it holds, which used to cut off the invitation's
 * last line and leave it scrolling.
 *
 * A fresh install is the only state that shows the invitation, so this runs in
 * a process of its own - mudlet::experiencedMudletPlayer() memoises for the
 * life of the process, and TutorialInvitationGateTest owns the opposite case.
 *
 * Run with: ctest -R TutorialInvitationLayoutTest -V
 */

#include <QtTest/QtTest>

#include <QPushButton>
#include <QScrollBar>

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class TutorialInvitationLayoutTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        // $XDG_CONFIG_HOME/mudlet/profiles is the opt-in marker, without which
        // setupConfig() would keep using a legacy ~/.config/mudlet
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(mudlet::getQSettings()->allKeys().isEmpty(), "a fresh config dir must start out with an empty Mudlet.ini - something wrote settings before init()");
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(!mudlet::self()->experiencedMudletPlayer(), "a config dir this test just created has to read as a brand new install");
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_invitationIsShownWholeOnAFreshInstall()
    {
        // filled out before it is shown, as mudlet::slot_showConnectionDialog()
        // does it: nothing is laid out yet at that point
        auto* dlg = new dlgConnectionProfiles();
        dlg->fillout_form();
        dlg->show();

        auto* skipButton = dlg->findChild<QPushButton*>(qsl("skipToGamesButton"));
        QVERIFY(skipButton);
        QVERIFY2(skipButton->isVisibleTo(dlg), "a fresh install has to be offered the tutorial instead of the games list");
        QVERIFY2(dlg->welcome_message->isVisibleTo(dlg), "the invitation itself has to be up");

        verifyInvitationFits(dlg);

        // filling the form again with the dialog already up takes the other
        // path, which measures inline instead of waiting for showEvent()
        dlg->fillout_form();
        verifyInvitationFits(dlg);

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

private:
    // Being unable to scroll it is the behavioural question; the floor under it
    // is what the offscreen platform can still answer, as it never shrinks the
    // dialog around the invitation the way a real window manager does
    void verifyInvitationFits(dlgConnectionProfiles* dlg) const
    {
        auto* pWelcome = dlg->welcome_message;
        pWelcome->document()->setTextWidth(pWelcome->viewport()->width());
        const int documentHeight = qCeil(pWelcome->document()->size().height());
        QVERIFY2(documentHeight > 0, "an invitation that measures as empty proves nothing - are the bundled fonts registered?");
        QVERIFY2(pWelcome->minimumHeight() >= documentHeight,
                 qPrintable(qsl("the invitation may be shrunk to %1 pixels although its text needs %2, which cuts off its last line").arg(pWelcome->minimumHeight()).arg(documentHeight)));
        QVERIFY2(pWelcome->verticalScrollBar()->maximum() == 0, qPrintable(qsl("the invitation does not fit: it can be scrolled by %1 pixels").arg(pWelcome->verticalScrollBar()->maximum())));
    }
};

#include "TutorialInvitationLayoutTest.moc"
MUDLET_GROUPED_TEST_MAIN(TutorialInvitationLayoutTest)
