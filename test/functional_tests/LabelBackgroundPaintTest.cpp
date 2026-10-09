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

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletApp.h"
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

class StyleChangeCounter : public QObject
{
public:
    int mCount = 0;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::StyleChange) {
            ++mCount;
        }
        return QObject::eventFilter(watched, event);
    }
};

// A label's background colour has to survive the script giving the label a
// stylesheet afterwards (#10019). What getBackgroundColor() reports is covered by
// UI_spec; this is what actually reaches the screen, which Lua cannot see.
class LabelBackgroundPaintTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "LabelBackgroundPaint-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";

    // the backdrop sits under every target label, in a colour nothing else on screen
    // uses, so that "the label painted nothing" is unmistakable
    static constexpr QRect backdropArea{0, 0, 200, 100};
    static QColor backdropColour() { return QColor(255, 0, 255); }

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    QColor paintedColour(const QPoint& at = QPoint(50, 50))
    {
        QTest::qWait(50ms);
        const QImage shot = mpHost->mainConsoleView()->mpMainFrame->grab(backdropArea).toImage();
        return shot.pixelColor(at);
    }

    // a pixel matching the backdrop is equally satisfied by a label that was never
    // created, so the tests expecting transparency prove the label is there first
    void createTargetCovering(int fillBackground)
    {
        runLua(qsl("createLabel('lbpTarget', 0, 0, %1, %2, %3)").arg(backdropArea.width()).arg(backdropArea.height()).arg(fillBackground));
        QVERIFY(mpHost->mainConsoleView()->labelWidget(qsl("lbpTarget")));
        QCOMPARE(mpHost->mainConsoleView()->labelWidget(qsl("lbpTarget"))->geometry(), backdropArea);
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

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2s)) {
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
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mainConsoleView());
        runLua(qsl("createLabel('lbpBackdrop', 0, 0, %1, %2, 1)").arg(backdropArea.width()).arg(backdropArea.height()));
        runLua(qsl("setBackgroundColor('lbpBackdrop', %1, %2, %3, 255)").arg(backdropColour().red()).arg(backdropColour().green()).arg(backdropColour().blue()));
        QCOMPARE(paintedColour(), backdropColour());
    }

    // the profile and application stylesheets outlive the test that set one, so they
    // are cleared here rather than at the end of a body a QVERIFY may have aborted
    void cleanup()
    {
        runLua(qsl("setProfileStyleSheet('')"));
        runLua(qsl("setAppStyleSheet('')"));
        for (const auto& name : {qsl("lbpTarget"), qsl("lbpGauge_back"), qsl("lbpGauge_front"), qsl("lbpGauge_text"), qsl("lbpBackdrop")}) {
            mpHost->mainConsoleView()->deleteLabel(name);
        }
    }

    void test_transparentLabelStaysTransparentUnderAStyleSheet()
    {
        createTargetCovering(1);
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        QCOMPARE(paintedColour(), QColor(255, 255, 0));

        runLua(qsl("setBackgroundColor('lbpTarget', 0, 0, 0, 0)"));
        runLua(qsl("setLabelStyleSheet('lbpTarget', [[qproperty-alignment: 'AlignHCenter';]])"));

        QCOMPARE(paintedColour(), backdropColour());
    }

    void test_opaqueLabelKeepsItsColourUnderAStyleSheet()
    {
        createTargetCovering(1);
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        runLua(qsl("setLabelStyleSheet('lbpTarget', [[qproperty-alignment: 'AlignHCenter';]])"));

        QCOMPARE(paintedColour(), QColor(255, 255, 0));
    }

    // the palette and the stylesheet must not both blend, which no opaque colour can show
    void test_semiTransparentLabelBlendsTheSameUnderAStyleSheet()
    {
        createTargetCovering(1);
        runLua(qsl("setBackgroundColor('lbpTarget', 0, 255, 0, 128)"));
        const QColor blended = paintedColour();
        QVERIFY2(blended != backdropColour(), "the half-transparent colour never reached the screen");

        runLua(qsl("setLabelStyleSheet('lbpTarget', [[qproperty-alignment: 'AlignHCenter';]])"));

        QCOMPARE(paintedColour(), blended);
    }

    // a background Qt honours only in a pseudo-state is not the label's own
    void test_hoverOnlyBackgroundIsNotTheLabelsBackground()
    {
        createTargetCovering(1);
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        runLua(qsl("setLabelStyleSheet('lbpTarget', [[QLabel:hover { background-color: rgb(0, 0, 255); }]])"));

        QCOMPARE(paintedColour(), QColor(255, 255, 0));
    }

    void test_colourSurvivesAProfileStyleSheet()
    {
        createTargetCovering(1);
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        runLua(qsl("setLabelStyleSheet('lbpTarget', [[qproperty-alignment: 'AlignHCenter';]])"));

        runLua(qsl("setProfileStyleSheet([[QScrollBar { background: #0e0a08; }]])"));

        QCOMPARE(paintedColour(), QColor(255, 255, 0));
    }

    void test_colourSurvivesAnApplicationStyleSheet()
    {
        createTargetCovering(1);
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        runLua(qsl("setLabelStyleSheet('lbpTarget', [[qproperty-alignment: 'AlignHCenter';]])"));

        runLua(qsl("setAppStyleSheet([[QScrollBar { background: #0e0a08; }]])"));

        QCOMPARE(paintedColour(), QColor(255, 255, 0));
    }

    // The legacy createGauge() stacks a text label over the two coloured ones and
    // setGaugeStyleSheet() always pushes a stylesheet into that text label, so a text
    // label that paints anything hides the whole gauge - this is the reported repro.
    // Geyser.Gauge leaves the text label alone when given no stylesheet for it, so the
    // same test written against Geyser would pass without the fix.
    void test_gaugeTextLabelDoesNotCoverTheGauge()
    {
        runLua(qsl("createGauge('lbpGauge', %1, %2, 0, 0, nil, 0, 255, 0)").arg(backdropArea.width()).arg(backdropArea.height()));
        runLua(qsl("setGaugeStyleSheet('lbpGauge', [[background-color: rgb(0, 255, 0);]], [[background-color: rgb(0, 0, 255);]])"));
        runLua(qsl("setGauge('lbpGauge', 1, 4)"));

        // front and back are given different colours, so neither alone satisfies both
        QCOMPARE(paintedColour(QPoint(25, 50)), QColor(0, 255, 0));
        QCOMPARE(paintedColour(QPoint(150, 50)), QColor(0, 0, 255));
    }

    // fillBackground = 0 reads like "start transparent" and does not do that, because
    // honouring it would turn every such label in an installed script transparent
    void test_labelAskedNotToFillItsBackgroundStartsOnTheSameGrey()
    {
        createTargetCovering(0);

        QCOMPARE(paintedColour(), QColor(32, 32, 32));
    }

    // what the argument does decide: the colour is painted until a stylesheet replaces
    // it, and then it is gone rather than kept as it is for a label that fills its own
    // background. Lua cannot see this - getBackgroundColor() reports the colour either way.
    void test_labelAskedNotToFillItsBackgroundLosesItsColourToAStyleSheet()
    {
        createTargetCovering(0);
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        QCOMPARE(paintedColour(), QColor(255, 255, 0));

        runLua(qsl("setLabelStyleSheet('lbpTarget', [[qproperty-alignment: 'AlignHCenter';]])"));

        QCOMPARE(paintedColour(), backdropColour());
    }

    // a status bar re-sets its labels' sheets on every prompt, and each identical one
    // used to cost a repolish and a repaint
    void test_identicalStyleSheetIsNotReapplied()
    {
        createTargetCovering(1);
        TLabel* pLabel = mpHost->mpConsole->labelWidget(qsl("lbpTarget"));
        StyleChangeCounter counter;
        pLabel->installEventFilter(&counter);

        runLua(qsl("setLabelStyleSheet('lbpTarget', [[border: 1px solid white;]])"));
        QVERIFY2(counter.mCount > 0, "the counter never saw the first stylesheet arrive");
        counter.mCount = 0;
        runLua(qsl("setLabelStyleSheet('lbpTarget', [[border: 1px solid white;]])"));
        QCOMPARE(counter.mCount, 0);

        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        QVERIFY2(counter.mCount > 0, "a new background colour did not restyle the label");
        counter.mCount = 0;
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));
        QCOMPARE(counter.mCount, 0);
        QCOMPARE(paintedColour(), QColor(255, 255, 0));

        runLua(qsl("setBackgroundColor('lbpTarget', 0, 255, 255, 255)"));
        QVERIFY2(counter.mCount > 0, "a different background colour did not restyle the label");
        QCOMPARE(paintedColour(), QColor(0, 255, 255));
    }

    // a background-color only inside a pseudo-state block leaves the colour to the
    // palette, which an identical sheet that is skipped no longer refreshes
    void test_unchangedSheetStillPaintsANewBackgroundColour()
    {
        createTargetCovering(1);
        runLua(qsl("setLabelStyleSheet('lbpTarget', [[QLabel:hover { background-color: rgba(255, 255, 0, 255); }]])"));
        runLua(qsl("setBackgroundColor('lbpTarget', 255, 255, 0, 255)"));

        QCOMPARE(paintedColour(), QColor(255, 255, 0));
    }

    // resetting the link style replaces the palette, dropping the text colour the sheet
    // set, and re-applying the sheet is what brings it back
    void test_identicalStyleSheetRestoresItsColoursAfterTheLinkStyleIsReset()
    {
        createTargetCovering(1);
        TLabel* pLabel = mpHost->mpConsole->labelWidget(qsl("lbpTarget"));
        const QString script = qsl("setLabelStyleSheet('lbpTarget', [[color: rgb(255, 0, 0);]])");
        runLua(script);
        QCOMPARE(pLabel->palette().color(QPalette::WindowText), QColor(255, 0, 0));

        runLua(qsl("resetLinkStyle('lbpTarget')"));
        runLua(script);

        QCOMPARE(pLabel->palette().color(QPalette::WindowText), QColor(255, 0, 0));
    }

    // re-applying the sheet it already has is how a script makes Qt reload an image
    // it rewrote on disk
    void test_identicalStyleSheetNamingAnImageReloadsIt()
    {
        createTargetCovering(1);
        QTemporaryDir imageDir;
        QVERIFY(imageDir.isValid());
        const QString path = imageDir.filePath(qsl("background.png"));
        const QDateTime now = QDateTime::currentDateTime();
        // Qt caches a file's image by its modification time, so the rewrite has to move it
        auto writeImage = [&path](const QColor& colour, const QDateTime& modified) {
            QImage image(8, 8, QImage::Format_RGB32);
            image.fill(colour);
            if (!image.save(path)) {
                return false;
            }
            QFile file(path);
            return file.open(QIODevice::ReadWrite) && file.setFileTime(modified, QFileDevice::FileModificationTime);
        };
        const QString script = qsl("setLabelStyleSheet('lbpTarget', [[border-image: url(\"%1\");]])").arg(path);

        QVERIFY(writeImage(QColor(255, 0, 0), now.addSecs(-100)));
        runLua(script);
        QCOMPARE(paintedColour(), QColor(255, 0, 0));

        QVERIFY(writeImage(QColor(0, 0, 255), now));
        runLua(script);
        QCOMPARE(paintedColour(), QColor(0, 0, 255));
    }
};

#include "LabelBackgroundPaintTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelBackgroundPaintTest)
