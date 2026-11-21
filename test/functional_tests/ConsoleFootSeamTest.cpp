/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet development team                     *
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
 * A console's foot is parted from the text above by one 1px seam mixed from the profile's
 * command-line colours, and the input has no frame of its own, so a border a profile gives it
 * through setCmdLineStyleSheet() is the only one drawn there.
 *
 * Run with: ctest -R ConsoleFootSeamTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TCommandLine.h"
#include "TMainConsole.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

class ConsoleFootSeamTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("ConsoleFootSeam-Test-Host");
    const QString mMiniConsoleName = qsl("footSeamMiniConsole");
    const QString mColourMiniConsoleName = qsl("footSeamColourMiniConsole");
    QSize mWindowSize;

    static QWidget* seamOf(TConsole* pConsole) { return pConsole->findChild<QWidget*>(qsl("consoleFootSeam")); }

    // 0.20 written out rather than read from the constant, so the test says what was asked for
    QColor expectedSeamColour() const { return uiDesign::blend(mpHost->mCommandLineBgColor, mpHost->mCommandLineFgColor, 0.20); }

    // As a QRgb: a grab holds eight bits a channel, where blend() mixes in floating point
    static QRgb pixelAt(QWidget* pGrabbed, QWidget* pWidget, const QPoint& local)
    {
        const QImage shot = pGrabbed->grab().toImage();
        const QPoint at = pWidget->mapTo(pGrabbed, local) * shot.devicePixelRatio();
        return shot.pixel(at);
    }

    static QString describe(const QRgb colour) { return QColor::fromRgba(colour).name(QColor::HexArgb); }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mHostname), qPrintable(mSession.why()));
        mpHost = mSession.host();

        // Nothing is laid out, let alone painted, until the window is up
        mWindowSize = mudlet::self()->size();
        mudlet::self()->resize(1200, 800);
        mudlet::self()->show();
        QVERIFY2(QTest::qWaitForWindowExposed(mudlet::self()), "the main window never came up");
        TestPixels::settle();
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            mudlet::self()->hide();
            mudlet::self()->resize(mWindowSize);
        }
        mSession.stop();
    }

    void test_theMainCommandLineHasNoFrame()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        QVERIFY(pCommandLine);

        QCOMPARE(pCommandLine->frameShape(), QFrame::NoFrame);
        QCOMPARE(pCommandLine->frameWidth(), 0);
        const QRgb edge = pixelAt(mpHost->mainConsoleView(), pCommandLine, QPoint(0, pCommandLine->height() / 2));
        QVERIFY2(
                edge == mpHost->mCommandLineBgColor.rgb(),
                qPrintable(qsl("the command line's leading edge is %1, not its background %2, so something is still drawn round it").arg(describe(edge), describe(mpHost->mCommandLineBgColor.rgb()))));
    }

    // The search box, buttons and latency readout share a splitter with the command line, so a
    // seam on the command line alone would miss them
    void test_theSeamSpansTheWholeFootOfTheMainWindow()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        QWidget* pSeam = seamOf(pConsole);
        QVERIFY(pSeam);
        QWidget* pFoot = pConsole->findChild<QWidget*>(qsl("consoleFoot"));
        QVERIFY(pFoot);
        QVERIFY2(pConsole->mpButtonMainLayer->isVisible(), "the search box and buttons are not on show, so the seam's reach past the command line is not tested");

        QVERIFY(pSeam->isVisible());
        QCOMPARE(pSeam->height(), 1);
        const QPoint seamInConsole = pSeam->mapTo(pConsole, QPoint());
        const QPoint footInConsole = pFoot->mapTo(pConsole, QPoint());
        QCOMPARE(seamInConsole.x(), 0);
        QCOMPARE(seamInConsole.x(), footInConsole.x());
        QCOMPARE(seamInConsole.y(), footInConsole.y());
        QCOMPARE(pSeam->width(), pFoot->width());
        QCOMPARE(pSeam->width(), pConsole->width());
        QVERIFY2(pSeam->width() > pConsole->layerCommandLine->width(),
                 qPrintable(qsl("the seam is %1 wide, no wider than the command line's %2").arg(pSeam->width()).arg(pConsole->layerCommandLine->width())));

        QCOMPARE(pConsole->mpCommandLine->mapTo(pConsole, QPoint()).y(), seamInConsole.y() + 1);
        QCOMPARE(pConsole->mpButtonMainLayer->mapTo(pConsole, QPoint()).y(), seamInConsole.y() + 1);

        const int textBottom = pConsole->mpMainDisplay->mapTo(pConsole, QPoint(0, pConsole->mpMainDisplay->height())).y();
        QVERIFY2(textBottom <= seamInConsole.y(), qPrintable(qsl("the text area ends at %1, under the seam at %2").arg(textBottom).arg(seamInConsole.y())));
    }

    void test_theSeamIsMixedFromTheProfilesCommandLineColours()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        QWidget* pSeam = seamOf(pConsole);
        QVERIFY(pSeam);
        const QColor expected = expectedSeamColour();
        QVERIFY2(expected != mpHost->mCommandLineBgColor, "the profile's two command-line colours are the same, so the mix proves nothing");

        QCOMPARE(pSeam->palette().color(QPalette::Window), expected);
        for (const int x : {0, pSeam->width() / 2, pSeam->width() - 1}) {
            const QRgb painted = pixelAt(pConsole, pSeam, QPoint(x, 0));
            QVERIFY2(painted == expected.rgb(), qPrintable(qsl("the seam is painted %1 at x=%2, not %3").arg(describe(painted)).arg(x).arg(describe(expected.rgb()))));
        }
    }

    // No script can set these colours; import and the settings dialog write the two members and
    // restyle the main console alone, which has to carry the change to every other console's foot
    void test_theSeamFollowsTheProfilesColoursWhenTheyChange()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        QWidget* pSeam = seamOf(pConsole);
        QVERIFY(pSeam);
        TConsole* pMiniConsole = pConsole->createMiniConsole(QString(), mColourMiniConsoleName, 0, 0, 400, 150);
        QVERIFY2(pMiniConsole, "could not create the miniconsole");
        const auto removeIt = qScopeGuard([this]() {
            mpHost->deleteMiniConsole(mColourMiniConsoleName);
        });
        QVERIFY(runLua(qsl("enableCommandLine('%1')").arg(mColourMiniConsoleName)));
        TestPixels::settle();
        QWidget* pMiniSeam = seamOf(pMiniConsole);
        QVERIFY(pMiniSeam);
        QVERIFY(pMiniSeam->isVisible());
        const QColor priorBackground = mpHost->mCommandLineBgColor;
        const QColor priorText = mpHost->mCommandLineFgColor;
        const auto restore = qScopeGuard([this, priorBackground, priorText]() {
            mpHost->mCommandLineBgColor = priorBackground;
            mpHost->mCommandLineFgColor = priorText;
            mpHost->applyMainConsoleColors();
        });
        const QColor before = pSeam->palette().color(QPalette::Window);

        mpHost->mCommandLineBgColor = QColor(250, 240, 220);
        mpHost->mCommandLineFgColor = QColor(20, 40, 120);
        mpHost->applyMainConsoleColors();
        TestPixels::settle();

        const QColor expected = expectedSeamColour();
        QVERIFY2(expected != before, "the new colours mix to the old seam colour, so nothing is tested");
        QCOMPARE(pSeam->palette().color(QPalette::Window), expected);
        const QRgb painted = pixelAt(pConsole, pSeam, QPoint(pSeam->width() / 2, 0));
        QVERIFY2(painted == expected.rgb(), qPrintable(qsl("after the change the seam is painted %1, not %2").arg(describe(painted), describe(expected.rgb()))));

        QCOMPARE(pMiniSeam->palette().color(QPalette::Window), expected);
        const QRgb miniPainted = pixelAt(pMiniConsole, pMiniSeam, QPoint(pMiniSeam->width() / 2, 0));
        QVERIFY2(miniPainted == expected.rgb(), qPrintable(qsl("after the change the miniconsole's seam is painted %1, not %2").arg(describe(miniPainted), describe(expected.rgb()))));
    }

    // Hiding is tested here because Lua refuses to hide the main command line
    void test_aMiniConsoleCommandLineCarriesTheSeamToo()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        TConsole* pMiniConsole = pConsole->createMiniConsole(QString(), mMiniConsoleName, 0, 0, 400, 150);
        QVERIFY2(pMiniConsole, "could not create the miniconsole");
        const auto removeIt = qScopeGuard([this]() {
            mpHost->deleteMiniConsole(mMiniConsoleName);
        });
        QVERIFY(runLua(qsl("enableCommandLine('%1')").arg(mMiniConsoleName)));
        TestPixels::settle();
        QVERIFY(pMiniConsole->mpCommandLine);
        QCOMPARE(pMiniConsole->mpCommandLine->frameShape(), QFrame::NoFrame);

        QWidget* pSeam = seamOf(pMiniConsole);
        QVERIFY(pSeam);
        QVERIFY(pSeam->isVisible());
        QCOMPARE(pSeam->height(), 1);
        QCOMPARE(pSeam->mapTo(pMiniConsole, QPoint()).x(), 0);
        QCOMPARE(pSeam->width(), pMiniConsole->width());
        QCOMPARE(pSeam->mapTo(pMiniConsole, QPoint()).y() + 1, pMiniConsole->mpCommandLine->mapTo(pMiniConsole, QPoint()).y());
        // The seam adds a pixel to the foot rather than taking one off the input
        QCOMPARE(pMiniConsole->layerCommandLine->height(), pMiniConsole->mpCommandLine->height() + 1);
        const int priorMinimumHeight = mpHost->commandLineMinimumHeight;
        const auto restoreMinimumHeight = qScopeGuard([this, pMiniConsole, priorMinimumHeight]() {
            mpHost->commandLineMinimumHeight = priorMinimumHeight;
            pMiniConsole->mpCommandLine->adjustHeight();
        });
        mpHost->commandLineMinimumHeight = 60;
        pMiniConsole->mpCommandLine->adjustHeight();
        TestPixels::settle();
        QCOMPARE(pMiniConsole->mpCommandLine->height(), 60);
        QCOMPARE(pMiniConsole->layerCommandLine->height(), 61);
        QCOMPARE(pSeam->palette().color(QPalette::Window), expectedSeamColour());

        QVERIFY(runLua(qsl("disableCommandLine('%1')").arg(mMiniConsoleName)));
        TestPixels::settle();
        QVERIFY2(!pSeam->isVisible(), "the miniconsole's seam stayed on show with its command line disabled");
    }

    void test_aProfileStyleSheetBorderIsTheOnlyFrameOnTheCommandLine()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        TCommandLine* pCommandLine = pConsole->mpCommandLine;
        QWidget* pSeam = seamOf(pConsole);
        QVERIFY(pSeam);
        const auto restore = qScopeGuard([this]() {
            runLua(qsl("setCmdLineStyleSheet('main', '')"));
        });

        QVERIFY(runLua(qsl("setCmdLineStyleSheet('main', [[QPlainTextEdit { border: 2px solid #ff0000; }]])")));
        TestPixels::settle();

        const QRgb red = qRgb(255, 0, 0);
        const QRgb edge = pixelAt(pConsole, pCommandLine, QPoint(0, pCommandLine->height() / 2));
        QVERIFY2(edge == red, qPrintable(qsl("the command line's leading edge is %1 under a sheet giving it a red border").arg(describe(edge))));
        QVERIFY2(pSeam->isVisible(), "the profile's sheet took the seam away");
        const QRgb seam = pixelAt(pConsole, pSeam, QPoint(pSeam->width() - 1, 0));
        QVERIFY2(seam == expectedSeamColour().rgb(), qPrintable(qsl("with the profile's sheet on the seam is painted %1, not %2").arg(describe(seam), describe(expectedSeamColour().rgb()))));
    }
};

#include "ConsoleFootSeamTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConsoleFootSeamTest)
