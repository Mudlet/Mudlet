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
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QProxyStyle>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStyleOptionSlider>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>
#include <cmath>

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

// Reports what the console's scroll bar looks like with and without the handle
// this platform's own style would have drawn, by writing the two renders out as
// images for a human to compare. #9341 is a Windows problem, but the fix paints
// the handle on every platform, so what Linux and macOS look like afterwards is
// a question only a picture answers.
//
// Writes to $MUDLET_SCREENSHOT_DIR (the working directory if unset) and asserts
// nothing about which of the two looks better - the measurements it prints and
// the images it leaves behind are the output.
class ScrollBarScreenshotTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "ScrollBarScreenshot-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";
    QString mOutputDir;

    // Kept in step with ConsoleScrollBarStyle in TConsole.cpp by hand: the style
    // lives in an anonymous namespace, so the name cannot be shared.
    static constexpr const char* csHandleColorProperty = "mudletScrollBarHandleColor";

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

    static QString platformName()
    {
#if defined(Q_OS_MACOS)
        return qsl("macOS");
#elif defined(Q_OS_LINUX)
        return qsl("Linux");
#else
        return qsl("Windows");
#endif
    }

    QScrollBar* scrollBar() const { return mpHost->mpConsole->mpScrollBar; }
    QWidget* display() const { return mpHost->mpConsole->mpMainDisplay; }

    // What the platform actually draws with. Mudlet's application style is a proxy
    // with no object name of its own, so the name has to come from the style it
    // wraps - "fusion", "macOS", "windows11".
    static QString baseStyleName()
    {
        const QStyle* pStyle = QApplication::style();
        const auto* pProxy = qobject_cast<const QProxyStyle*>(pStyle);
        const QString name = pStyle->objectName().isEmpty() && pProxy ? pProxy->baseStyle()->objectName() : pStyle->objectName();
        return name.isEmpty() ? QString::fromLatin1(pStyle->metaObject()->className()) : name;
    }

    // Clearing the handle colour is how the "before" render is taken: the style the
    // fix installs falls straight through to the platform's own drawing when the
    // property is not a valid colour, so the bar paints exactly as it did before
    // the fix. Taking the style off the widget instead would not survive an
    // application style sheet, which wraps a widget's own style in a QStyleSheetStyle
    // that setStyle(nullptr) then destroys.
    void useFix(const bool enabled, const QVariant& handleColor)
    {
        scrollBar()->setProperty(csHandleColorProperty, enabled ? handleColor : QVariant());
        scrollBar()->update();
        QTest::qWait(50ms);
    }

    void initOption(QStyleOptionSlider& option) const
    {
        QScrollBar* pScrollBar = scrollBar();
        option.initFrom(pScrollBar);
        option.orientation = pScrollBar->orientation();
        option.minimum = pScrollBar->minimum();
        option.maximum = pScrollBar->maximum();
        option.pageStep = pScrollBar->pageStep();
        option.singleStep = pScrollBar->singleStep();
        option.sliderPosition = pScrollBar->sliderPosition();
        option.sliderValue = pScrollBar->value();
    }

    QRect handleRect() const
    {
        QStyleOptionSlider option;
        initOption(option);
        return scrollBar()->style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, scrollBar());
    }

    // Magenta, so anything the widget tree leaves unpainted is obvious in the image
    // rather than passing for a colour a console could have.
    //
    // A widget render can only show the state the bar is really in, and no pointer
    // is going anywhere near it in a headless run - so the hovered render is drawn
    // through the style over the top of the bar instead. That matters because the
    // hover feedback belongs to the handle the fix masks out.
    QImage renderDisplay(const bool hovered) const
    {
        QImage shot(display()->size(), QImage::Format_RGB32);
        shot.fill(Qt::magenta);
        display()->render(&shot, QPoint(), QRegion(), QWidget::DrawChildren);
        if (!hovered) {
            return shot;
        }

        QPainter painter(&shot);
        painter.translate(scrollBar()->mapTo(display(), QPoint()));
        QStyleOptionSlider option;
        initOption(option);
        option.rect = QRect(QPoint(), scrollBar()->size());
        option.state |= QStyle::State_MouseOver;
        option.activeSubControls = QStyle::SC_ScrollBarSlider;
        option.subControls = QStyle::SC_All;
        scrollBar()->style()->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter, scrollBar());
        return shot;
    }

    struct Capture
    {
        QImage context; // the right-hand edge of the console, scroll bar included
        QImage zoom;    // the bar alone, enlarged
        QColor handle;
        QColor groove;
        qreal handleOnGroove = 1.0;
        qreal handleOnBackground = 1.0;
    };

    static constexpr int csContextWidth = 300;
    static constexpr int csContextHeight = 340;
    static constexpr int csZoomFactor = 5;
    // csContextHeight / csZoomFactor, so the enlargement ends up as tall as the crop
    // it sits beside
    static constexpr int csZoomHeight = csContextHeight / csZoomFactor;

    Capture capture(const QColor& background, const bool hovered = false)
    {
        const QImage shot = renderDisplay(hovered);
        const QRect bar(scrollBar()->mapTo(display(), QPoint()), scrollBar()->size());
        const QRect handle = handleRect().translated(bar.topLeft());

        // Centred on the handle so the crop shows it wherever the console put it.
        const int top = qBound(0, handle.center().y() - csContextHeight / 2, qMax(0, shot.height() - csContextHeight));
        const QRect context(qMax(0, bar.right() + 1 - csContextWidth), top, qMin(csContextWidth, bar.right() + 1), qMin(csContextHeight, shot.height() - top));
        // A window around the handle rather than the whole crop: the point of the
        // enlargement is the handle's shape and edges.
        const QRect strip(bar.left() - 1, qBound(0, handle.center().y() - csZoomHeight / 2, qMax(0, shot.height() - csZoomHeight)), bar.width() + 2, csZoomHeight);

        Capture result;
        result.context = shot.copy(context);
        result.zoom = shot.copy(strip).scaled(strip.width() * csZoomFactor, strip.height() * csZoomFactor, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        result.handle = shot.pixelColor(handle.center());
        // Half way between the top of the bar and the top of the handle is track on
        // every style here - past the arrow button of the ones that have one.
        result.groove = shot.pixelColor(bar.center().x(), (bar.top() + handle.top()) / 2);
        result.handleOnGroove = contrastRatio(result.handle, result.groove);
        result.handleOnBackground = contrastRatio(result.handle, background);
        return result;
    }

    static void drawColumn(QPainter& painter, const QPoint& origin, const QString& title, const Capture& capture)
    {
        painter.setPen(Qt::black);
        QFont heading = painter.font();
        heading.setBold(true);
        painter.setFont(heading);
        painter.drawText(QRect(origin.x(), origin.y(), csContextWidth, 20), Qt::AlignLeft | Qt::AlignVCenter, title);
        heading.setBold(false);
        painter.setFont(heading);

        const QPoint imageAt(origin.x(), origin.y() + 24);
        painter.drawImage(imageAt, capture.context);
        painter.drawImage(QPoint(imageAt.x() + capture.context.width() + 12, imageAt.y()), capture.zoom);
        painter.setPen(QColor(60, 60, 60));
        painter.drawRect(QRect(imageAt, capture.context.size()));
        painter.drawRect(QRect(QPoint(imageAt.x() + capture.context.width() + 12, imageAt.y()), capture.zoom.size()));

        painter.setPen(Qt::black);
        const int textTop = imageAt.y() + capture.context.height() + 8;
        painter.drawText(QRect(origin.x(), textTop, csContextWidth + capture.zoom.width() + 12, 18), Qt::AlignLeft, qsl("handle %1, groove %2").arg(capture.handle.name(), capture.groove.name()));
        painter.drawText(QRect(origin.x(), textTop + 18, csContextWidth + capture.zoom.width() + 12, 18),
                         Qt::AlignLeft,
                         qsl("contrast vs groove %1:1, vs console %2:1").arg(capture.handleOnGroove, 0, 'f', 2).arg(capture.handleOnBackground, 0, 'f', 2));
    }

    void writeComparison(const QString& tag, const QString& caption, const Capture& before, const Capture& after)
    {
        const int columnWidth = csContextWidth + 12 + before.zoom.width();
        const int width = 24 + columnWidth + 40 + columnWidth + 24;
        const int height = 30 + 24 + qMax(before.context.height(), after.context.height()) + 56;

        QImage comparison(width, height, QImage::Format_RGB32);
        comparison.fill(QColor(245, 245, 245));
        QPainter painter(&comparison);
        QFont title = painter.font();
        title.setBold(true);
        painter.setFont(title);
        painter.setPen(Qt::black);
        painter.drawText(QRect(24, 6, width - 48, 20), Qt::AlignLeft | Qt::AlignVCenter, caption);
        title.setBold(false);
        painter.setFont(title);

        drawColumn(painter, QPoint(24, 30), qsl("before - the platform's own handle"), before);
        drawColumn(painter, QPoint(24 + columnWidth + 40, 30), qsl("after - the handle this PR paints"), after);
        painter.end();

        const QString path = qsl("%1/%2-%3.png").arg(mOutputDir, platformName().toLower(), tag);
        QVERIFY2(comparison.save(path), qPrintable(qsl("could not write %1").arg(path)));
        QVERIFY2(before.context.save(qsl("%1/%2-%3-before.png").arg(mOutputDir, platformName().toLower(), tag)), "could not write the before crop");
        QVERIFY2(after.context.save(qsl("%1/%2-%3-after.png").arg(mOutputDir, platformName().toLower(), tag)), "could not write the after crop");
        qInfo().noquote() << qsl("%1 | %2 | before: handle %3 on groove %4 (%5:1), on console (%6:1) | after: handle %7 on groove %8 (%9:1), on console (%10:1)")
                                     .arg(platformName(), caption, before.handle.name(), before.groove.name())
                                     .arg(before.handleOnGroove, 0, 'f', 2)
                                     .arg(before.handleOnBackground, 0, 'f', 2)
                                     .arg(after.handle.name(), after.groove.name())
                                     .arg(after.handleOnGroove, 0, 'f', 2)
                                     .arg(after.handleOnBackground, 0, 'f', 2);
    }

    void captureScenario(const QString& tag, const QString& what, const QColor& background, const bool hovered = false)
    {
        const QVariant handleColor = scrollBar()->property(csHandleColorProperty);
        QVERIFY2(handleColor.value<QColor>().isValid(), "the console never gave its scroll bar a handle colour - is this build missing the fix?");

        useFix(false, handleColor);
        const Capture before = capture(background, hovered);
        useFix(true, handleColor);
        const Capture after = capture(background, hovered);
        writeComparison(tag, qsl("%1 - %2 style - %3").arg(platformName(), baseStyleName(), what), before, after);
    }

    void fillConsoleSoTheHandleHasSomewhereToSit()
    {
        // Long enough to reach the right-hand edge of the console, so the crop beside
        // the scroll bar shows text rather than empty background.
        runLua(qsl("for i = 1, 500 do echo(('the quick brown fox jumps over the lazy dog '):rep(3) .. 'line ' .. i .. '\\n') end"));
        QTRY_VERIFY(scrollBar()->maximum() > scrollBar()->minimum());
        // Half way up the scrollback, so the handle sits in the middle of the crop
        // rather than parked against an end stop.
        scrollBar()->setValue(scrollBar()->maximum() / 2);
        QTest::qWait(100ms);
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

        mOutputDir = qEnvironmentVariable("MUDLET_SCREENSHOT_DIR", QDir::currentPath());
        QVERIFY2(QDir().mkpath(mOutputDir), qPrintable(qsl("could not create %1").arg(mOutputDir)));

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
        QVERIFY(!scrollBar()->size().isEmpty());

        // Which is also the assertion that this build carries the fix at all - an
        // unpatched console leaves the scroll bar on the application's style.
        QVERIFY2(scrollBar()->testAttribute(Qt::WA_SetStyle), "the console's scroll bar has no style of its own - is this build missing the fix?");

        fillConsoleSoTheHandleHasSomewhereToSit();
        qInfo().noquote() << qsl("%1: application style %2, writing to %3").arg(platformName(), baseStyleName(), mOutputDir);
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

    // What all but a handful of profiles look like: Mudlet's own black console.
    void test_captureBlackConsole()
    {
        runLua(qsl("setBackgroundColor(0, 0, 0)"));
        QTest::qWait(50ms);
        captureScenario(qsl("black-console"), qsl("black console, light appearance"), QColor(0, 0, 0));
    }

    // The other end of the range a profile can set, where the fix picks a black
    // handle instead of a white one.
    void test_captureWhiteConsole()
    {
        runLua(qsl("setBackgroundColor(255, 255, 255)"));
        QTest::qWait(50ms);
        captureScenario(qsl("white-console"), qsl("white console, light appearance"), QColor(255, 255, 255));
    }

    // The platform draws its hover feedback on the handle, and the fix masks that
    // handle out - so this is where any feedback lost to the fix shows up.
    void test_captureHoveredHandle()
    {
        runLua(qsl("setBackgroundColor(0, 0, 0)"));
        QTest::qWait(50ms);
        captureScenario(qsl("black-console-hovered"), qsl("black console, pointer over the handle"), QColor(0, 0, 0), true);
    }

    // Dark appearance repaints the groove around the handle, so the pairing the
    // fix makes is a different one here.
    void test_captureBlackConsoleInDarkAppearance()
    {
        mudlet::self()->setAppearance(enums::Appearance::dark);
        runLua(qsl("setBackgroundColor(0, 0, 0)"));
        QTest::qWait(100ms);
        captureScenario(qsl("black-console-dark-appearance"), qsl("black console, dark appearance"), QColor(0, 0, 0));
        mudlet::self()->setAppearance(enums::Appearance::light);
        QTest::qWait(50ms);
    }
};

#include "ScrollBarScreenshotTest.moc"
MUDLET_GROUPED_TEST_MAIN(ScrollBarScreenshotTest)
