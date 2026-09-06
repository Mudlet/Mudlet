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

    // The groove is deliberately left unpainted, so a screen grab would only prove
    // that whatever sits behind the console is visible.
    QImage renderScrollBarOn(const QColor& background)
    {
        QScrollBar* pScrollBar = mpHost->mpConsole->mpScrollBar;
        QImage shot(pScrollBar->size(), QImage::Format_RGB32);
        shot.fill(background);
        pScrollBar->render(&shot, QPoint(), QRegion(), QWidget::DrawChildren);
        return shot;
    }

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
        // if the fix is absent:
        qApp->setStyle(new ColourSchemeOnlyScrollBarStyle);
        QTest::qWait(50ms);
    }

    void cleanupTestCase()
    {
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
        QVERIFY2(contrast >= 3.0, qPrintable(qsl("handle contrast against a white console was only %1:1").arg(contrast, 0, 'f', 2)));
    }

    // The hardest background to sit on - neither a white nor a black handle has much
    // room, so too transparent a handle shows up here first.
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

        QImage shot(pDisplay->size(), QImage::Format_RGB32);
        shot.fill(Qt::magenta);
        pDisplay->render(&shot, QPoint(), QRegion(), QWidget::DrawChildren);

        const QRect handle = handleRect(pScrollBar).translated(pScrollBar->mapTo(pDisplay, QPoint()));
        const QColor image = shot.pixelColor(handle.center().x(), handle.top() - 6);
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
        QVERIFY2(image == QColor(Qt::white), qPrintable(qsl("the background image did not reach the scroll bar - it rendered %1").arg(image.name())));
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
