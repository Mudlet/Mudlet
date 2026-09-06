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

#include <QPainter>
#include <QProxyStyle>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStyleOptionSlider>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Stands in for the Windows 11 style of #9341, which colours the whole scroll bar
// from the application's colour scheme - black at 45% alpha - whatever it sits on.
// That style does not exist on Linux, so it is reproduced here.
class ColourSchemeOnlyScrollBarStyle : public QProxyStyle
{
public:
    void drawComplexControl(const ComplexControl control, const QStyleOptionComplex* pOption, QPainter* pPainter, const QWidget* pWidget) const override
    {
        if (control == CC_ScrollBar) {
            pPainter->fillRect(pOption->rect, QColor(0, 0, 0, 0x72));
            return;
        }
        QProxyStyle::drawComplexControl(control, pOption, pPainter, pWidget);
    }
};

// Lua cannot see a widget's pixels, so the contrast is checked here rather than
// in a spec.
class ScrollBarContrastTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "ScrollBarContrast-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    static qreal relativeLuminance(const QColor& colour)
    {
        auto channel = [](qreal value) {
            return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * channel(colour.redF()) + 0.7152 * channel(colour.greenF()) + 0.0722 * channel(colour.blueF());
    }

    static qreal contrastRatio(const QColor& first, const QColor& second)
    {
        const qreal a = relativeLuminance(first);
        const qreal b = relativeLuminance(second);
        return (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
    }

    // Pre-filled because the console background is painted by an ancestor: rendering
    // the bar on its own would otherwise leave whatever it does not cover undefined.
    static QImage renderWidget(QWidget* pWidget, const QColor& background)
    {
        QImage shot(pWidget->size(), QImage::Format_RGB32);
        shot.fill(background);
        pWidget->render(&shot, QPoint(), QRegion(), QWidget::DrawChildren);
        return shot;
    }

    QImage renderScrollBarOn(const QColor& background) { return renderWidget(mpHost->mpConsole->mpScrollBar, background); }

    // Sampled at the middle of the handle so a stray antialiased edge cannot stand
    // in for it.
    qreal handleContrastOn(const QColor& background)
    {
        runLua(qsl("setBackgroundColor(%1, %2, %3)").arg(background.red()).arg(background.green()).arg(background.blue()));
        QScrollBar* pScrollBar = mpHost->mpConsole->mpScrollBar;
        const QRect handle = handleRect(pScrollBar);
        if (handle.isEmpty()) {
            return 1.0;
        }

        return contrastRatio(renderScrollBarOn(background).pixelColor(handle.center()), background);
    }

    // Kept in step with ConsoleScrollBarStyle in TConsole.cpp by hand: the style lives
    // in an anonymous namespace, so the name cannot be shared.
    static constexpr const char* csHandleColorProperty = "mudletScrollBarHandleColor";

    static QRect handleRect(QScrollBar* pScrollBar)
    {
        QStyleOptionSlider option;
        option.initFrom(pScrollBar);
        option.orientation = pScrollBar->orientation();
        option.minimum = pScrollBar->minimum();
        option.maximum = pScrollBar->maximum();
        option.pageStep = pScrollBar->pageStep();
        option.singleStep = pScrollBar->singleStep();
        option.sliderPosition = pScrollBar->sliderPosition();
        option.sliderValue = pScrollBar->value();
        return pScrollBar->style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, pScrollBar);
    }

    static int countPixels(const QImage& shot, const QColor& wanted)
    {
        int painted = 0;
        for (int y = 0; y < shot.height(); ++y) {
            for (int x = 0; x < shot.width(); ++x) {
                if (shot.pixelColor(x, y) == wanted) {
                    ++painted;
                }
            }
        }
        return painted;
    }

    void fillConsoleSoTheHandleHasSomewhereToSit()
    {
        runLua(qsl("for i = 1, 500 do echo('scroll bar contrast line ' .. i .. '\\n') end"));
        QTRY_VERIFY(mpHost->mpConsole->mpScrollBar->maximum() > mpHost->mpConsole->mpScrollBar->minimum());
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
        QVERIFY(mpHost->mpConsole);
        QVERIFY(!mpHost->mpConsole->mpScrollBar->size().isEmpty());
        fillConsoleSoTheHandleHasSomewhereToSit();

        // The console keeps its own style, so this only reaches the scroll bar
        // if the fix is absent. Never put back, which is safe only because every
        // case in a grouped test binary runs in its own process.
        qApp->setStyle(new ColourSchemeOnlyScrollBarStyle);
        QTest::qWait(50ms);
    }

    void cleanupTestCase()
    {
        delete mudlet::smpDebugArea;
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_handleStandsOutOnTheDefaultBlackConsole()
    {
        const QColor background(0, 0, 0);
        const qreal contrast = handleContrastOn(background);
        QVERIFY2(contrast >= 3.0, qPrintable(qsl("handle contrast against a black console was only %1:1").arg(contrast, 0, 'f', 2)));
    }

    void test_handleStandsOutOnAWhiteConsole()
    {
        const QColor background(255, 255, 255);
        const qreal contrast = handleContrastOn(background);
        // A higher bar than the other backgrounds: black at 45% alpha over white is
        // #8d8d8d, which already clears 3:1, so only a stricter bound can fail here.
        QVERIFY2(contrast >= 4.5, qPrintable(qsl("handle contrast against a white console was only %1:1").arg(contrast, 0, 'f', 2)));
    }

    // The hardest background to sit on - neither a white nor a black handle has much
    // room, so an over-transparent handle, or one blended over the base style's own
    // handle instead of over the console, shows up here first.
    void test_handleStandsOutOnAMidGreyConsole()
    {
        const QColor background(127, 127, 127);
        const qreal contrast = handleContrastOn(background);
        QVERIFY2(contrast >= 3.0, qPrintable(qsl("handle contrast against a mid grey console was only %1:1").arg(contrast, 0, 'f', 2)));
    }

    // Blue is dark to the eye but middling by HSL lightness, so picking the
    // handle by lightness rather than by contrast gets this one wrong.
    void test_handleStandsOutOnABlueConsole()
    {
        const QColor background(0, 0, 255);
        const qreal contrast = handleContrastOn(background);
        QVERIFY2(contrast >= 3.0, qPrintable(qsl("handle contrast against a blue console was only %1:1").arg(contrast, 0, 'f', 2)));
    }

    // The debug console paints its own black background rather than the desktop
    // theme's, so it has the same problem and takes the same handle.
    void test_handleStandsOutOnTheDebugConsole()
    {
        mudlet::self()->attachDebugArea(mHostname);
        QVERIFY(!mudlet::smpDebugConsole.isNull());
        mudlet::smpDebugArea->show();
        QTest::qWait(100ms);

        QScrollBar* pScrollBar = mudlet::smpDebugConsole->mpScrollBar;
        pScrollBar->setRange(0, 100);
        pScrollBar->setPageStep(10);
        pScrollBar->setValue(50);

        const QColor background = mudlet::smpDebugConsole->getConsoleBgColor();
        const QRect handle = handleRect(pScrollBar);
        QVERIFY(!handle.isEmpty());

        const qreal contrast = contrastRatio(renderWidget(pScrollBar, background).pixelColor(handle.center()), background);
        // Nothing else destroys it: the debug area is parentless and only a profile
        // closing takes it down, so it would outlive the Host its console points at.
        delete mudlet::smpDebugArea;
        QVERIFY2(contrast >= 3.0, qPrintable(qsl("handle contrast against the debug console's %1 background was only %2:1").arg(background.name(), QString::number(contrast, 'f', 2))));
    }

    // The handle is inset on the axis it is thin on, so the two orientations take
    // opposite arms - swapping them leaves a handle that fills the bar's width.
    void test_theHorizontalHandleIsInsetFromTheLongEdges()
    {
        QScrollBar* pScrollBar = mpHost->mpConsole->mpHScrollBar;
        pScrollBar->show();
        QTest::qWait(50ms);
        pScrollBar->setRange(0, 100);
        pScrollBar->setPageStep(10);
        pScrollBar->setValue(50);

        // Only the handle changes between two renders that differ solely in the handle
        // colour, so the pixels that differ are exactly the ones Mudlet drew.
        const QVariant handleColor = pScrollBar->property(csHandleColorProperty);
        const QImage painted = renderWidget(pScrollBar, Qt::black);
        pScrollBar->setProperty(csHandleColorProperty, QColor(255, 0, 0, 200));
        const QImage recoloured = renderWidget(pScrollBar, Qt::black);
        pScrollBar->setProperty(csHandleColorProperty, handleColor);

        int drawnOnTheTopEdge = 0;
        int drawnInTheMiddle = 0;
        for (int x = 0; x < painted.width(); ++x) {
            drawnOnTheTopEdge += (painted.pixelColor(x, 0) != recoloured.pixelColor(x, 0)) ? 1 : 0;
            drawnInTheMiddle += (painted.pixelColor(x, painted.height() / 2) != recoloured.pixelColor(x, painted.height() / 2)) ? 1 : 0;
        }
        QVERIFY2(drawnInTheMiddle > 0, "nothing was drawn along the middle of the horizontal scroll bar");
        QVERIFY2(drawnOnTheTopEdge == 0, qPrintable(qsl("%1 pixels were drawn on the horizontal scroll bar's top edge, which should be groove").arg(drawnOnTheTopEdge)));
    }

    // A background image is painted on MainDisplay, an ancestor of the scroll bar,
    // so it shows through the groove: the handle has to stand out against the image
    // and not merely against the colour stored underneath it.
    void test_handleStandsOutOnALightBackgroundImage()
    {
        const QString imagePath = qsl("%1/white.png").arg(mConfigDir.path());
        QImage white(8, 8, QImage::Format_RGB32);
        white.fill(Qt::white);
        QVERIFY(white.save(imagePath));

        runLua(qsl("setBackgroundColor(0, 0, 0)"));
        runLua(qsl("setBackgroundImage([[%1]], 1)").arg(imagePath));
        QTest::qWait(100ms);

        QScrollBar* pScrollBar = mpHost->mpConsole->mpScrollBar;
        QWidget* pDisplay = pScrollBar;
        for (QWidget* pW = pScrollBar; pW; pW = pW->parentWidget()) {
            if (pW->objectName() == qsl("MainDisplay")) {
                pDisplay = pW;
            }
        }
        QVERIFY(pDisplay != pScrollBar);

        const QImage shot = renderWidget(pDisplay, Qt::magenta);

        const QRect handle = handleRect(pScrollBar).translated(pScrollBar->mapTo(pDisplay, QPoint()));
        // Measured against the image and not against the groove: a base style that
        // paints an opaque groove hides the image on that platform, but one that
        // leaves it clear - as Windows 11 does - puts the image right behind the handle.
        // Taken from what was written rather than sampled, because every pixel of the
        // display could be a glyph of the text the handle was given to scroll.
        const QColor image(Qt::white);
        int imagePixels = 0;
        for (int y = 0; y < shot.height(); y += 4) {
            for (int x = 0; x < shot.width(); x += 4) {
                imagePixels += (shot.pixelColor(x, y) == image) ? 1 : 0;
            }
        }

        int standingOut = 0;
        for (int y = handle.top(); y <= handle.bottom(); ++y) {
            for (int x = handle.left(); x <= handle.right(); ++x) {
                if (contrastRatio(shot.pixelColor(x, y), image) >= 3.0) {
                    ++standingOut;
                }
            }
        }

        runLua(qsl("resetBackgroundImage()"));
        QTest::qWait(50ms);
        const int sampled = ((shot.width() + 3) / 4) * ((shot.height() + 3) / 4);
        QVERIFY2(imagePixels > sampled / 2, qPrintable(qsl("the background image did not reach the console - only %1 of %2 sampled pixels were its white").arg(imagePixels).arg(sampled)));
        QVERIFY2(standingOut >= handle.height(), qPrintable(qsl("only %1 of the handle's %2 pixels stood out against the background image").arg(standingOut).arg(handle.width() * handle.height())));
    }

    // setAppStyleSheet() has to keep winning too - QApplication::setStyle() skips
    // widgets that carry their own style, so this path is easy to cut off.
    void test_anAppStyleSheetStillColoursTheScrollBar()
    {
        const QColor background(0, 0, 0);
        const QColor wanted(0, 255, 0);
        runLua(qsl("setBackgroundColor(0, 0, 0)"));
        runLua(qsl("setAppStyleSheet[[QScrollBar{background-color: rgb(0, 255, 0);}]]"));
        QTest::qWait(50ms);

        const int painted = countPixels(renderScrollBarOn(background), wanted);
        runLua(qsl("setAppStyleSheet[[]]"));
        QTest::qWait(50ms);
        QVERIFY2(painted > 0, "setAppStyleSheet() no longer reaches the console's scroll bar");
    }

    // A profile that styles its own scroll bars has to keep winning.
    void test_aProfileStyleSheetStillColoursTheScrollBar()
    {
        const QColor background(0, 0, 0);
        const QColor wanted(255, 0, 0);
        runLua(qsl("setBackgroundColor(0, 0, 0)"));
        runLua(qsl("setProfileStyleSheet[[QScrollBar{background-color: rgb(255, 0, 0);}]]"));
        QTest::qWait(50ms);

        const int painted = countPixels(renderScrollBarOn(background), wanted);
        runLua(qsl("setProfileStyleSheet[[]]"));
        QVERIFY2(painted > 0, "a profile style sheet no longer reaches the console's scroll bar");
    }
};

#include "ScrollBarContrastTest.moc"
MUDLET_GROUPED_TEST_MAIN(ScrollBarContrastTest)
