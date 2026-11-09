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
 * The command line grows to show everything typed into it, up to ten rows, and
 * its vertical scroll bar is switched off - so while the text fits it must have
 * no scroll range at all. A range of even one row is reachable by click-and-
 * dragging inside the command line, which scrolls the text out of position with
 * no scroll bar to show that it happened or to put it back.
 *
 * The height comes from TCommandLine::adjustHeight(). Its (fontH + 1) * lines +
 * marginH estimate of what the text takes up is the part that falls short, so
 * anything that drops the measured height it is now taken against regresses this.
 *
 * Rows centered in a box taller than they are leave a viewport exactly as tall as the rows,
 * so the centering is held to the same no-range rule.
 *
 * test_scrollRangeOnceThereAreMoreRowsThanFit() is what keeps the rest honest: a
 * widget that was never laid out reports a zero range too, so without a case that
 * has to come back non-zero the assertions below could all pass for no reason.
 *
 * Bootstrap mirrors the other functional tests.
 */

#include <QScopeGuard>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QtTest/QtTest>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TCommandLine.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class CommandLineScrollRangeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "CommandLineScrollRange-Test-Host";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    // Hard line feeds rather than wrapped text: the row count is then the same
    // whatever width the command line happens to have been given.
    static QString rowsOfText(const int rows)
    {
        QStringList lines;
        for (int row = 0; row < rows; ++row) {
            lines << qsl("row%1").arg(row);
        }
        return lines.join(QChar::LineFeed);
    }

    static int rowsShown(const TCommandLine* pCommandLine) { return static_cast<int>(pCommandLine->document()->size().height()); }

    static void fill(TCommandLine* pCommandLine, const QString& text)
    {
        pCommandLine->setPlainText(text);
        pCommandLine->adjustHeight();
        qApp->processEvents();
    }

    // Typed rather than set, so the keys go through adjustHeight() as a player's do. A
    // space is not one of them, so each burst ends on a letter.
    static void typeUntilItWrapsTo(TCommandLine* pCommandLine, const int rows)
    {
        for (int words = 0; rowsShown(pCommandLine) < rows && words < 500; ++words) {
            QTest::keyClicks(pCommandLine, qsl(" wrap me"));
        }
        qApp->processEvents();
    }

    static void backspaceUntilItIsDownTo(TCommandLine* pCommandLine, const int rows)
    {
        for (int keys = 0; rowsShown(pCommandLine) > rows && keys < 4000; ++keys) {
            QTest::keyClick(pCommandLine, Qt::Key_Backspace);
        }
        qApp->processEvents();
    }

    static QRectF rowsRect(const TCommandLine* pCommandLine)
    {
        QTextCursor last(pCommandLine->document());
        last.movePosition(QTextCursor::End);
        const QRectF first = QRectF(pCommandLine->cursorRect(QTextCursor(pCommandLine->document())));
        return first.united(QRectF(pCommandLine->cursorRect(last))).translated(pCommandLine->viewport()->pos());
    }

    static qreal offCenter(const TCommandLine* pCommandLine) { return rowsRect(pCommandLine).center().y() - pCommandLine->height() / 2.0; }

    static int spaceAbove(const TCommandLine* pCommandLine) { return pCommandLine->viewport()->y(); }
    static int spaceBelow(const TCommandLine* pCommandLine) { return pCommandLine->height() - pCommandLine->viewport()->geometry().bottom() - 1; }

    static QString describeRows(const TCommandLine* pCommandLine)
    {
        const QRectF rows = rowsRect(pCommandLine);
        return qsl("%1 row(s) %2px tall from %3px in a %4px box, viewport %5px with %6px above it and %7px below, scroll range %8")
                .arg(QString::number(rowsShown(pCommandLine)),
                     QString::number(rows.height()),
                     QString::number(rows.top()),
                     QString::number(pCommandLine->height()),
                     QString::number(pCommandLine->viewport()->height()),
                     QString::number(spaceAbove(pCommandLine)),
                     QString::number(spaceBelow(pCommandLine)),
                     QString::number(pCommandLine->verticalScrollBar()->maximum()));
    }

    static QString centeringProblem(const TCommandLine* pCommandLine, const QString& when)
    {
        if (qAbs(offCenter(pCommandLine)) > 1.0) {
            return qsl("%1 the rows sit %2px off the middle of the box - %3").arg(when, QString::number(offCenter(pCommandLine)), describeRows(pCommandLine));
        }
        if (pCommandLine->verticalScrollBar()->maximum() != 0) {
            return qsl("%1 the command line can be scrolled - %2").arg(when, describeRows(pCommandLine));
        }
        return QString();
    }

    static QRgb pixelAt(QWidget* pWidget, const QPoint& local)
    {
        const QImage shot = pWidget->grab().toImage();
        return shot.pixel(local * shot.devicePixelRatio());
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState && !connectedSpy.wait(8s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The font size and the minimum height are profile-wide, so every test has to
    // start from the same place rather than inherit whatever ran before it.
    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mainConsoleView());
        QVERIFY(mpHost->mainConsoleView()->mpCommandLine);
        mpHost->commandLineMinimumHeight = 30;
        mpHost->mainConsoleView()->setFontSize(12);
    }

    void cleanup() { fill(mpHost->mainConsoleView()->mpCommandLine, QString()); }

    // Only the families Mudlet bundles, so the sweep is the same everywhere it runs.
    // Row counts are exhaustive, font sizes are a sample; the combinations that were
    // red before the fix are the one-row ones from 14pt up, since below that the 30px
    // commandLineMinimumHeight floor covers the shortfall on its own.
    void test_noScrollRangeWhileTheTextFits()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        QStringList scrollable;

        for (const QString& family : {qsl("Bitstream Vera Sans Mono"), qsl("Bitstream Vera Sans"), qsl("Ubuntu Mono"), qsl("Ubuntu")}) {
            for (const int fontSize : {8, 12, 14, 24, 30}) {
                mpHost->mainConsoleView()->setFont(QFont(family, fontSize), true);
                for (int rows = 1; rows <= 10; ++rows) {
                    fill(pCommandLine, rowsOfText(rows));
                    if (pCommandLine->verticalScrollBar()->maximum() != 0) {
                        scrollable << qsl("%1 %2pt/%3 row(s): range %4, widget %5px, viewport %6px")
                                              .arg(family,
                                                   QString::number(fontSize),
                                                   QString::number(rows),
                                                   QString::number(pCommandLine->verticalScrollBar()->maximum()),
                                                   QString::number(pCommandLine->height()),
                                                   QString::number(pCommandLine->viewport()->height()));
                    }
                }
            }
        }

        QVERIFY2(scrollable.isEmpty(), qPrintable(qsl("command line can be scrolled although it is showing all its text - %1").arg(scrollable.join(qsl("; ")))));
    }

    // The other half of the sizing rule, and the case that proves the assertions
    // above are being made against a widget that really was laid out.
    void test_scrollRangeOnceThereAreMoreRowsThanFit()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;

        fill(pCommandLine, rowsOfText(11));

        QCOMPARE(rowsShown(pCommandLine), 11);
        QVERIFY2(pCommandLine->verticalScrollBar()->maximum() > 0, "command line past its ten row cap cannot be scrolled, so its last row is out of reach");
    }

    // Wrapping is how a single typed command reaches several rows, and a block
    // spanning several rows is the only thing that divides the row height out.
    void test_noScrollRangeWhenOneCommandWrapsOverSeveralRows()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        mpHost->mainConsoleView()->setFontSize(24);

        QString command;
        do {
            command += qsl("wrap me around ");
            fill(pCommandLine, command);
        } while (rowsShown(pCommandLine) < 3 && command.length() < 4000);

        QCOMPARE(pCommandLine->document()->blockCount(), 1);
        QVERIFY2(rowsShown(pCommandLine) >= 3, "the command line never wrapped, so nothing was measured");
        QCOMPARE(pCommandLine->verticalScrollBar()->maximum(), 0);
    }

    // The preference is only ever a floor, so neither a tiny nor a generous
    // setting may bring the range back.
    void test_noScrollRangeAtEitherEndOfTheMinimumHeightPreference()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        mpHost->mainConsoleView()->setFontSize(24);

        for (const int minimumHeight : {10, 30, 200}) {
            mpHost->commandLineMinimumHeight = minimumHeight;
            fill(pCommandLine, rowsOfText(2));
            QVERIFY2(pCommandLine->verticalScrollBar()->maximum() == 0, qPrintable(qsl("command line can be scrolled with a minimum height of %1px").arg(minimumHeight)));
        }
    }

    // Shrinking back from a legitimately scrolled state has to bring the first row
    // into view again - text left sitting out of position is the damage being fixed.
    void test_shrinkingBackFromAScrolledStateShowsTheFirstRowAgain()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;

        fill(pCommandLine, rowsOfText(15));
        pCommandLine->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
        qApp->processEvents();
        QVERIFY(pCommandLine->verticalScrollBar()->value() > 0);

        fill(pCommandLine, rowsOfText(3));

        QCOMPARE(pCommandLine->verticalScrollBar()->maximum(), 0);
        QCOMPARE(pCommandLine->verticalScrollBar()->value(), 0);
    }

    // A miniconsole's command line is a ConsoleCommandLine, sized by the same code
    // but carrying a stylesheet - and a stylesheet hands frame metrics over to
    // QStyleSheetStyle, so its frameWidth() need not match the main one's.
    void test_noScrollRangeOnAConsoleCommandLine()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        const QString name = qsl("scrollRangeMiniConsole");

        TConsole* pMiniConsole = pConsole->createMiniConsole(QString(), name, 0, 0, 300, 100);
        QVERIFY2(pMiniConsole, "could not create the miniconsole");
        pMiniConsole->setCmdVisible(true); // what Lua enableCommandLine(name) does
        QVERIFY(pMiniConsole->mpCommandLine);
        QCOMPARE(pMiniConsole->mpCommandLine->getType(), enums::ConsoleCommandLine);

        pMiniConsole->setFontSize(24);
        fill(pMiniConsole->mpCommandLine, rowsOfText(2));

        QCOMPARE(pMiniConsole->mpCommandLine->verticalScrollBar()->maximum(), 0);

        auto [deleted, deleteMsg] = mpHost->deleteMiniConsole(name);
        QVERIFY2(deleted, qPrintable(deleteMsg));
    }

    // One 12pt row is shorter than the 30px floor, and the hidden-input box laid over the same
    // box centers its text, so the slack must not all go under the row
    void test_oneRowSitsInTheMiddleOfABoxTheMinimumHeightHoldsUp()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;

        fill(pCommandLine, qsl("look"));

        QCOMPARE(rowsShown(pCommandLine), 1);
        QVERIFY2(pCommandLine->height() == mpHost->commandLineMinimumHeight,
                 qPrintable(qsl("the minimum height is not what holds the box up, so there is no slack to place - %1").arg(describeRows(pCommandLine))));
        QVERIFY2(spaceAbove(pCommandLine) >= 3, qPrintable(qsl("the slack is not above the row - %1").arg(describeRows(pCommandLine))));
        const QString problem = centeringProblem(pCommandLine, qsl("with one row under the minimum height"));
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }

    // adjustHeight()'s estimate leaves slack over the measured rows, so a grown box still has some to split
    void test_typingPastOneRowGrowsTheBoxAndKeepsTheRowsCentered()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        fill(pCommandLine, QString());
        const int oneRowHeight = pCommandLine->height();

        typeUntilItWrapsTo(pCommandLine, 3);

        QVERIFY2(rowsShown(pCommandLine) >= 3, "the command line never wrapped to three rows, so nothing was measured");
        QVERIFY2(pCommandLine->height() > oneRowHeight, qPrintable(qsl("three rows did not grow the box - %1").arg(describeRows(pCommandLine))));
        const QString problem = centeringProblem(pCommandLine, qsl("typed to three rows"));
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }

    void test_deletingBackToOneRowCentersItAgain()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        fill(pCommandLine, QString());
        typeUntilItWrapsTo(pCommandLine, 3);
        QVERIFY2(rowsShown(pCommandLine) >= 3, "the command line never wrapped to three rows, so nothing was measured");

        backspaceUntilItIsDownTo(pCommandLine, 1);

        QCOMPARE(rowsShown(pCommandLine), 1);
        QCOMPARE(pCommandLine->height(), mpHost->commandLineMinimumHeight);
        QVERIFY2(spaceAbove(pCommandLine) >= 3, qPrintable(qsl("the slack is not above the row again - %1").arg(describeRows(pCommandLine))));
        const QString problem = centeringProblem(pCommandLine, qsl("deleted back to one row"));
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }

    // Under the floor, where the slack changes with the row, and over it
    void test_aFontChangeKeepsTheRowCentered()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        fill(pCommandLine, qsl("look"));

        for (const int fontSize : {8, 10, 14, 20, 24}) {
            mpHost->mainConsoleView()->setFontSize(fontSize);
            qApp->processEvents();
            const QString problem = centeringProblem(pCommandLine, qsl("at %1pt").arg(fontSize));
            QVERIFY2(problem.isEmpty(), qPrintable(problem));
        }
    }

    // setCmdLineStyleSheet() changes the font with no adjustHeight() after it, so the
    // slack placed for the old font would squeeze the new one's row into a scroll range
    void test_aScriptsStyleSheetFontKeepsTheRowCenteredAndInRange()
    {
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        const auto restore = qScopeGuard([this]() {
            mpHost->mainConsoleView()->setCmdLineStyleSheet(qsl("main"), QString());
        });
        fill(pCommandLine, qsl("look"));
        QVERIFY2(spaceAbove(pCommandLine) >= 3, qPrintable(qsl("no slack was placed before the sheet - %1").arg(describeRows(pCommandLine))));

        mpHost->mainConsoleView()->setCmdLineStyleSheet(qsl("main"), qsl("QPlainTextEdit { font-size: 14pt; }"));
        qApp->processEvents();

        QCOMPARE(pCommandLine->font().pointSize(), 14);
        const QString problem = centeringProblem(pCommandLine, qsl("after a sheet's 14pt font"));
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }

    // A script-placed command line keeps the size it was given, and the strips round its row fill like the viewport under a sheet that names no background
    void test_aSubCommandLineCentersItsRowAndFillsTheSlack()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        const QString name = qsl("scrollRangeSubCommandLine");
        auto [created, message] = pConsole->createCommandLine(QString(), name, 0, 0, 300, 50);
        QVERIFY2(created, qPrintable(message));
        const auto remove = qScopeGuard([pConsole, name]() {
            pConsole->deleteCommandLine(name);
        });
        TCommandLine* pSubCommandLine = pConsole->subCommandLineWidget(name);
        QVERIFY(pSubCommandLine);
        QCOMPARE(pSubCommandLine->getType(), enums::SubCommandLine);
        pSubCommandLine->mSaveCommands = false;
        QVERIFY(pConsole->setCmdLineStyleSheet(name, qsl("QPlainTextEdit { color: #ff0000; }")).first);

        fill(pSubCommandLine, qsl("look"));

        QCOMPARE(pSubCommandLine->height(), 50);
        const QString problem = centeringProblem(pSubCommandLine, qsl("in a 50px command line made by a script"));
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
        const int stripAbove = spaceAbove(pSubCommandLine) - pSubCommandLine->frameWidth();
        QVERIFY2(stripAbove >= 3, qPrintable(qsl("the slack is not above the row - %1").arg(describeRows(pSubCommandLine))));
        const int x = pSubCommandLine->width() / 2;
        const QRgb strip = pixelAt(pSubCommandLine, QPoint(x, pSubCommandLine->viewport()->y() - 1));
        const QRgb viewport = pixelAt(pSubCommandLine, QPoint(x, pSubCommandLine->viewport()->y() + 1));
        QVERIFY2(strip == viewport, qPrintable(qsl("the strip above the row is painted %1 and the viewport beside it %2").arg(QColor::fromRgb(strip).name(), QColor::fromRgb(viewport).name())));
    }

    // Padding one side of the command line, the way the manual moves its text over,
    // must cost that side its padding and nothing more: not twice over, nor any height.
    void test_sidePaddingCostsOnlyThePaddingItself()
    {
        TMainConsole* pConsole = mpHost->mainConsoleView();
        TCommandLine* pCommandLine = pConsole->mpCommandLine;
        const QString savedStyleSheet = pCommandLine->styleSheet();

        fill(pCommandLine, rowsOfText(1));
        const QSize unpaddedHint = pCommandLine->minimumSizeHint();
        const int unpaddedHeight = pCommandLine->height();

        auto [applied, message] = pConsole->setCmdLineStyleSheet(qsl("main"), qsl("QPlainTextEdit {padding-left: 1000px;}"));
        QVERIFY2(applied, qPrintable(message));
        fill(pCommandLine, rowsOfText(1));
        const QSize paddedHint = pCommandLine->minimumSizeHint();
        const int paddedLayerHint = pConsole->layerCommandLine->minimumSizeHint().width();
        const int paddedHeight = pCommandLine->height();

        pConsole->setCmdLineStyleSheet(qsl("main"), savedStyleSheet);
        fill(pCommandLine, QString());

        // A style sheet swaps the native frame for its own border, so allow a few pixels either way
        const auto report = qsl("minimum %1x%2 unpadded, %3x%4 padded; height %5 unpadded, %6 padded")
                                    .arg(unpaddedHint.width())
                                    .arg(unpaddedHint.height())
                                    .arg(paddedHint.width())
                                    .arg(paddedHint.height())
                                    .arg(unpaddedHeight)
                                    .arg(paddedHeight);
        QVERIFY2(qAbs(paddedHint.width() - unpaddedHint.width() - 1000) <= 4, qPrintable(report));
        QVERIFY2(qAbs(paddedHint.height() - unpaddedHint.height()) <= 4, qPrintable(report));
        QVERIFY2(qAbs(paddedHeight - unpaddedHeight) <= 4, qPrintable(report));
        QCOMPARE(paddedLayerHint, paddedHint.width());
    }
};

#include "CommandLineScrollRangeTest.moc"
MUDLET_GROUPED_TEST_MAIN(CommandLineScrollRangeTest)
