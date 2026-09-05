/***************************************************************************
 *   Copyright (C) 2026 by Morquin - morquin@morquin.dk                    *
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
 * An SVG background is a layer of its own, drawn over the label's background
 * colour and under whatever the label holds as content. What a label ends up
 * showing is not something Lua can read back, so the pixels are the test.
 *
 * Run with: ctest -R LabelSvgBackgroundTest -V
 */

#include <QMovie>
#include <QSignalSpy>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class LabelSvgBackgroundTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mFixtureDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("LabelSvgBackground-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    QString mSvgPath;
    QString mBrokenSvgPath;
    QString mPngPath;
    QString mGifPath;

    // the backdrop sits under every target label, in a colour nothing else on
    // screen uses, so that "the label painted nothing here" is unmistakable
    static constexpr QRect backdropArea{0, 0, 200, 200};
    static QColor backdropColour() { return QColor(255, 0, 255); }
    // the fixture SVG is opaque, so any interior pixel reads exactly this
    static QColor svgColour() { return QColor(255, 0, 0); }

    // written out with escaped quotes rather than as a raw string because moc
    // reads the "//" of the namespace URL inside one as the start of a comment,
    // swallows the rest of the line and then finds no Q_OBJECT in the file
    static QByteArray squareSvg()
    {
        return QByteArray("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 10 10\" width=\"10\" height=\"10\"><rect width=\"10\" height=\"10\" fill=\"#ff0000\"/></svg>");
    }

    // three frames and a 60 second frame delay, so the animation never advances
    // between two reads
    static QByteArray threeFrameGif()
    {
        QByteArray gif("GIF89a");
        gif.append(QByteArray::fromHex("01000100910000"));
        gif.append(QByteArray::fromHex("ff000000ff000000ff000000"));
        const QByteArray frame = QByteArray::fromHex("21f90400701700002c00000000010001000002024c0100");
        gif.append(frame).append(frame).append(frame);
        gif.append(QByteArray::fromHex("3b"));
        return gif;
    }

    static bool writeFixture(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    // the grab comes back in device pixels on a HiDPI screen, so a logical point
    // has to be scaled before it names the same place on the label
    QColor paintedColour(const QPoint& logical)
    {
        QTest::qWait(50ms);
        const QImage shot = mpHost->mpConsole->mpMainFrame->grab(backdropArea).toImage();
        return shot.pixelColor((QPointF(logical) * shot.devicePixelRatio()).toPoint());
    }

    TLabel* target() const { return mpHost->mpConsole->mLabelMap.value(qsl("svgTarget")); }

    void createTarget(int width, int height)
    {
        runLua(qsl("createLabel('svgTarget', 0, 0, %1, %2, 1)").arg(width).arg(height));
        QVERIFY(target());
    }

    void setLabelImage(const QString& path) { runLua(qsl("setBackgroundImage('svgTarget', [[%1]])").arg(path)); }

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

        QVERIFY(mFixtureDir.isValid());
        mSvgPath = qsl("%1/square.svg").arg(mFixtureDir.path());
        mBrokenSvgPath = qsl("%1/broken.svg").arg(mFixtureDir.path());
        mPngPath = qsl("%1/square.png").arg(mFixtureDir.path());
        mGifPath = qsl("%1/movie.gif").arg(mFixtureDir.path());
        QVERIFY(writeFixture(mSvgPath, squareSvg()));
        QVERIFY(writeFixture(mBrokenSvgPath, QByteArray("this is not svg")));
        QVERIFY(writeFixture(mGifPath, threeFrameGif()));
        QImage raster(10, 10, QImage::Format_ARGB32);
        raster.fill(QColor(0, 255, 0));
        QVERIFY(raster.save(mPngPath, "PNG"));
        QVERIFY2(QSvgRenderer(mSvgPath).isValid(), "the generated fixture is not an SVG Qt can read");
        QVERIFY2(!QSvgRenderer(mBrokenSvgPath).isValid(), "the fixture meant to be unreadable parses as an SVG");
        QVERIFY2(QMovie(mGifPath).isValid(), "the generated fixture is not a movie Qt can read");

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

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        runLua(qsl("createLabel('svgBackdrop', 0, 0, %1, %2, 1)").arg(backdropArea.width()).arg(backdropArea.height()));
        runLua(qsl("setBackgroundColor('svgBackdrop', %1, %2, %3, 255)").arg(backdropColour().red()).arg(backdropColour().green()).arg(backdropColour().blue()));
        QCOMPARE(paintedColour(QPoint(10, 10)), backdropColour());
    }

    void cleanup()
    {
        for (const auto& name : {qsl("svgTarget"), qsl("svgBackdrop")}) {
            mpHost->mpConsole->deleteLabel(name);
        }
    }

    void test_textStaysWhenTheLabelIsResized()
    {
        createTarget(100, 100);
        setLabelImage(mSvgPath);
        runLua(qsl("echo('svgTarget', 'Hello')"));

        TLabel* pLabel = target();
        QVERIFY(pLabel);
        QVERIFY2(pLabel->text().contains(qsl("Hello")), qPrintable(pLabel->text()));
        QCOMPARE(paintedColour(QPoint(10, 10)), svgColour());

        runLua(qsl("resizeWindow('svgTarget', 120, 120)"));

        QVERIFY2(pLabel->text().contains(qsl("Hello")), qPrintable(pLabel->text()));
        QVERIFY2(pLabel->pixmap().isNull(), "the SVG took the label's content slot, which is where the text lives");
        QCOMPARE(paintedColour(QPoint(10, 10)), svgColour());
    }

    void test_movieStaysWhenTheLabelIsResized()
    {
        createTarget(100, 100);
        setLabelImage(mSvgPath);

        auto [loaded, message] = mpHost->setMovie(qsl("svgTarget"), mGifPath);
        QVERIFY2(loaded, qPrintable(message));
        TLabel* pLabel = target();
        QVERIFY(pLabel);
        QVERIFY(pLabel->movie());

        runLua(qsl("resizeWindow('svgTarget', 120, 120)"));

        QVERIFY2(pLabel->movie(), "the resize replaced the movie with the SVG");
        QCOMPARE(paintedColour(QPoint(10, 10)), svgColour());
    }

    // the label's own background is painted before paintEvent() runs, so an SVG
    // drawn there has to end up on top of it - and a square document in a label
    // twice as wide has to leave that background showing down either side
    void test_svgFitsInsideTheLabelOverItsOwnBackground()
    {
        createTarget(200, 100);
        runLua(qsl("setBackgroundColor('svgTarget', 0, 0, 255, 255)"));
        QCOMPARE(paintedColour(QPoint(10, 50)), QColor(0, 0, 255));

        setLabelImage(mSvgPath);

        QCOMPARE(paintedColour(QPoint(100, 50)), svgColour());
        QCOMPARE(paintedColour(QPoint(10, 50)), QColor(0, 0, 255));
    }

    void test_anUnreadableSvgLeavesTheCurrentOneInPlace()
    {
        createTarget(100, 100);
        setLabelImage(mSvgPath);
        runLua(qsl("setSvgTint('svgTarget', 0, 0, 255)"));
        QCOMPARE(paintedColour(QPoint(50, 50)), QColor(0, 0, 255));

        runLua(qsl("local ok, err = setBackgroundImage('svgTarget', [[%1]]); assert(ok == nil and type(err) == 'string', tostring(err))").arg(mBrokenSvgPath));

        QCOMPARE(paintedColour(QPoint(50, 50)), QColor(0, 0, 255));
    }

    void test_resetBackgroundImageOnALabelWithNoImageKeepsTheText()
    {
        createTarget(100, 100);
        runLua(qsl("echo('svgTarget', 'Hello')"));

        runLua(qsl("resetBackgroundImage('svgTarget')"));

        TLabel* pLabel = target();
        QVERIFY(pLabel);
        QVERIFY2(pLabel->text().contains(qsl("Hello")), qPrintable(pLabel->text()));
    }

    // QLabel puts its content inside contentsRect(), so the SVG layer has to go
    // there too rather than over the border a stylesheet draws
    void test_svgStaysInsideAStyleSheetBorder()
    {
        createTarget(100, 100);
        runLua(qsl("setLabelStyleSheet('svgTarget', [[border: 10px solid rgb(0, 0, 255);]])"));

        setLabelImage(mSvgPath);

        TLabel* pLabel = target();
        QVERIFY(pLabel);
        QVERIFY2(pLabel->contentsRect() != pLabel->rect(),
                 qPrintable(qsl("a stylesheet border does not shrink contentsRect(): rect %1x%2, contents %3x%4 at %5,%6")
                                    .arg(pLabel->rect().width())
                                    .arg(pLabel->rect().height())
                                    .arg(pLabel->contentsRect().width())
                                    .arg(pLabel->contentsRect().height())
                                    .arg(pLabel->contentsRect().left())
                                    .arg(pLabel->contentsRect().top())));
        QCOMPARE(paintedColour(QPoint(50, 50)), svgColour());
        QCOMPARE(paintedColour(QPoint(3, 50)), QColor(0, 0, 255));
    }

    void test_resetBackgroundImageKeepsTheText()
    {
        createTarget(100, 100);
        setLabelImage(mSvgPath);
        runLua(qsl("echo('svgTarget', 'Hello')"));
        QCOMPARE(paintedColour(QPoint(10, 10)), svgColour());

        runLua(qsl("resetBackgroundImage('svgTarget')"));

        TLabel* pLabel = target();
        QVERIFY(pLabel);
        QVERIFY2(pLabel->text().contains(qsl("Hello")), qPrintable(pLabel->text()));
        QVERIFY2(paintedColour(QPoint(10, 10)) != svgColour(), "the SVG is still on screen after being reset");
    }

    void test_tintRecoloursTheSvg()
    {
        createTarget(100, 100);
        setLabelImage(mSvgPath);
        QCOMPARE(paintedColour(QPoint(50, 50)), svgColour());

        runLua(qsl("setSvgTint('svgTarget', 0, 0, 255)"));
        QCOMPARE(paintedColour(QPoint(50, 50)), QColor(0, 0, 255));

        runLua(qsl("resetSvgTint('svgTarget')"));
        QCOMPARE(paintedColour(QPoint(50, 50)), svgColour());
    }

    void test_switchingFromARasterToAnSvgDropsTheRaster()
    {
        createTarget(100, 100);
        setLabelImage(mPngPath);
        TLabel* pLabel = target();
        QVERIFY(pLabel);
        QVERIFY2(!pLabel->pixmap().isNull(), "the raster image never reached the label");

        setLabelImage(mSvgPath);

        QVERIFY2(pLabel->pixmap().isNull(), "the raster image is still the label's content, sitting on top of the SVG layer");
        QCOMPARE(paintedColour(QPoint(50, 50)), svgColour());
    }
};

#include "LabelSvgBackgroundTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelSvgBackgroundTest)
