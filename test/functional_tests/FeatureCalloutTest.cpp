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

#include <QCheckBox>
#include <QPushButton>
#include <QtTest/QtTest>

#include "TFeatureCallout.h"
#include "enums.h"
#include "mudlet.h"

#include "GroupedTest.h"

/*
 * A feature callout is a Qt::ToolTip window, which the platforms Mudlet ships
 * on keep above ordinary windows - including the windows of whatever
 * application the player switches to. It therefore has to step aside while
 * Mudlet is not the active application, without that counting as the player
 * having dealt with it.
 *
 * Run with: ctest -R FeatureCalloutTest -V
 */
class FeatureCalloutTest : public QObject
{
    Q_OBJECT

private:
    QByteArray mSavedXdg;
    QTemporaryDir mConfig;
    const QString mFeatureId = qsl("test-callout");
    QWidget* mpWindow = nullptr;
    QCheckBox* mpAnchor = nullptr;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    QString dismissedKey() const { return qsl("whatsNew/%1/dismissed").arg(mFeatureId); }

    QString shownCountKey() const { return qsl("whatsNew/%1/shownCount").arg(mFeatureId); }

    bool dismissed() const { return mudlet::getQSettings()->value(dismissedKey(), false).toBool(); }

    int shownCount() const { return mudlet::getQSettings()->value(shownCountKey(), 0).toInt(); }

    int anchorCentre() const { return mpAnchor->mapToGlobal(QPoint(mpAnchor->width() / 2, 0)).x(); }

    // Whichever application state this platform happens to report is not the
    // one these cases are about, so pin it rather than wait on a real one
    TFeatureCallout* shownCallout()
    {
        auto* callout = new TFeatureCallout(mFeatureId, mpAnchor, qsl("Title"), qsl("Body"));
        callout->showAnchored();
        callout->slot_applicationStateChanged(Qt::ApplicationActive);
        return callout;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfig.isValid());
        // $XDG_CONFIG_HOME/mudlet/profiles is the opt-in that makes setupConfig()
        // adopt it, so this test never writes to the real settings
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfig.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfig.path().toUtf8());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfig.path()));
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        mudlet::getQSettings()->remove(dismissedKey());
        mudlet::getQSettings()->remove(shownCountKey());
        mpWindow = new QWidget;
        mpWindow->resize(400, 300);
        mpWindow->move(80, 80);
        mpAnchor = new QCheckBox(qsl("Undo the game's own wrapping"), mpWindow);
        mpAnchor->move(40, 120);
        mpWindow->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpWindow));
    }

    // Deletes any callout still open along with the window it is parented to
    void cleanup()
    {
        delete mpWindow;
        mpWindow = nullptr;
        mpAnchor = nullptr;
    }

    void test_stepsAsideWhileAnotherApplicationIsActive()
    {
        auto* callout = shownCallout();
        QVERIFY2(callout->isVisible(), "the callout did not appear while Mudlet was the active application");

        callout->slot_applicationStateChanged(Qt::ApplicationInactive);
        QVERIFY2(!callout->isVisible(), "the callout stayed on screen once Mudlet was no longer the active application");
        QVERIFY2(!callout->testAttribute(Qt::WA_WState_Created), "the callout kept its native window, which on X11 stays on screen however hidden the widget is");
        QVERIFY2(!dismissed(), "switching away from Mudlet was recorded as the player having dealt with the callout");

        callout->slot_applicationStateChanged(Qt::ApplicationActive);
        QVERIFY2(callout->isVisible(), "the callout did not come back when Mudlet became the active application again");
    }

    // The balloon has to appear without anything driving the application state,
    // which is the one thing every other case here has to fake
    void test_appearsWithNobodyDrivingTheApplicationState()
    {
        auto* callout = new TFeatureCallout(mFeatureId, mpAnchor, qsl("Title"), qsl("Body"));
        callout->showAnchored();

        QVERIFY2(QTest::qWaitFor(
                         [callout]() {
                             return callout->isVisible();
                         },
                         3000),
                 "the callout never appeared, so it is waiting for an activation that this platform does not report");
    }

    void test_waitsForActivationWhenRaisedWhileInTheBackground()
    {
        auto* callout = new TFeatureCallout(mFeatureId, mpAnchor, qsl("Title"), qsl("Body"));
        callout->slot_applicationStateChanged(Qt::ApplicationInactive);
        callout->showAnchored();
        QVERIFY2(!callout->isVisible(), "a callout raised while Mudlet was in the background put itself on top of the foreground application");

        callout->slot_applicationStateChanged(Qt::ApplicationActive);
        QVERIFY2(callout->isVisible(), "a callout held back while Mudlet was in the background never appeared");
    }

    void test_reanchorsToAWindowThatMovedWhileHidden()
    {
        auto* callout = shownCallout();
        const int centreBefore = anchorCentre();
        QCOMPARE(callout->x() + callout->width() / 2, centreBefore);

        callout->slot_applicationStateChanged(Qt::ApplicationInactive);
        mpWindow->move(mpWindow->x() + 120, mpWindow->y() + 60);
        QTest::qWait(50);
        callout->slot_applicationStateChanged(Qt::ApplicationActive);

        QVERIFY2(anchorCentre() != centreBefore, "the window did not actually move, so nothing was re-anchored");
        QCOMPARE(callout->x() + callout->width() / 2, anchorCentre());
    }

    // A balloon the player keeps ignoring gives up after a set number of
    // appearances, so coming back to Mudlet must not count as one of them
    void test_comingBackDoesNotSpendAnotherAppearance()
    {
        auto* callout = shownCallout();
        QCOMPARE(shownCount(), 1);

        for (int i = 0; i < 3; ++i) {
            callout->slot_applicationStateChanged(Qt::ApplicationInactive);
            callout->slot_applicationStateChanged(Qt::ApplicationActive);
        }

        QVERIFY2(callout->isVisible(), "the callout stopped coming back part way through");
        QCOMPARE(shownCount(), 1);
    }

    void test_anAnchorThatIsNotOnScreenSpendsNothing()
    {
        mpAnchor->hide();

        auto* callout = new TFeatureCallout(mFeatureId, mpAnchor, qsl("Title"), qsl("Body"));
        callout->showAnchored();

        QVERIFY2(!callout->isVisible(), "the callout appeared pointing at an anchor that is not on screen");
        QCOMPARE(shownCount(), 0);
    }

    void test_doesNotComeBackToAnAnchorThatWentAway()
    {
        QPointer<TFeatureCallout> callout = shownCallout();

        callout->slot_applicationStateChanged(Qt::ApplicationInactive);
        mpAnchor->hide();
        if (callout) {
            callout->slot_applicationStateChanged(Qt::ApplicationActive);
        }
        // the balloon closes itself, which is a deferred delete
        QTest::qWait(50);

        QVERIFY2(!callout || !callout->isVisible(), "the callout came back pointing at an anchor that is no longer on screen");
        QVERIFY2(!dismissed(), "the anchor going away was recorded as the player having dealt with the callout");
    }

    void test_gotItStillDismissesForGood()
    {
        auto* gotItButton = shownCallout()->findChild<QPushButton*>();
        QVERIFY(gotItButton);
        QTest::mouseClick(gotItButton, Qt::LeftButton);

        QVERIFY2(dismissed(), "clicking \"Got it\" no longer retires the callout");
    }
};

#include "FeatureCalloutTest.moc"
MUDLET_GROUPED_TEST_MAIN(FeatureCalloutTest)
