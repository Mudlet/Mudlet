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
 * test_scrollRangeOnceThereAreMoreRowsThanFit() is what keeps the rest honest: a
 * widget that was never laid out reports a zero range too, so without a case that
 * has to come back non-zero the assertions below could all pass for no reason.
 *
 * Bootstrap mirrors the other functional tests.
 */

#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connectedSpy.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
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
        QVERIFY(mpHost->mpConsole);
        QVERIFY(mpHost->mpConsole->mpCommandLine);
        mpHost->commandLineMinimumHeight = 30;
        mpHost->mpConsole->setFontSize(12);
    }

    void cleanup() { fill(mpHost->mpConsole->mpCommandLine, QString()); }

    // Only the families Mudlet bundles, so the sweep is the same everywhere it runs.
    // Row counts are exhaustive, font sizes are a sample; the combinations that were
    // red before the fix are the one-row ones from 14pt up, since below that the 30px
    // commandLineMinimumHeight floor covers the shortfall on its own.
    void test_noScrollRangeWhileTheTextFits()
    {
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;
        QStringList scrollable;

        for (const QString& family : {qsl("Bitstream Vera Sans Mono"), qsl("Bitstream Vera Sans"), qsl("Ubuntu Mono"), qsl("Ubuntu")}) {
            for (const int fontSize : {8, 12, 14, 24, 30}) {
                mpHost->mpConsole->setFont(QFont(family, fontSize), true);
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
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;

        fill(pCommandLine, rowsOfText(11));

        QCOMPARE(rowsShown(pCommandLine), 11);
        QVERIFY2(pCommandLine->verticalScrollBar()->maximum() > 0, "command line past its ten row cap cannot be scrolled, so its last row is out of reach");
    }

    // Wrapping is how a single typed command reaches several rows, and a block
    // spanning several rows is the only thing that divides the row height out.
    void test_noScrollRangeWhenOneCommandWrapsOverSeveralRows()
    {
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;
        mpHost->mpConsole->setFontSize(24);

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
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;
        mpHost->mpConsole->setFontSize(24);

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
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;

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
        TMainConsole* pConsole = mpHost->mpConsole;
        const QString name = qsl("scrollRangeMiniConsole");

        TConsole* pMiniConsole = pConsole->createMiniConsole(QString(), name, 0, 0, 300, 100);
        QVERIFY2(pMiniConsole, "could not create the miniconsole");
        pMiniConsole->setCmdVisible(true); // what Lua enableCommandLine(name) does
        QVERIFY(pMiniConsole->mpCommandLine);
        QCOMPARE(pMiniConsole->mpCommandLine->getType(), TCommandLine::ConsoleCommandLine);

        pMiniConsole->setFontSize(24);
        fill(pMiniConsole->mpCommandLine, rowsOfText(2));

        QCOMPARE(pMiniConsole->mpCommandLine->verticalScrollBar()->maximum(), 0);

        auto [deleted, deleteMsg] = pConsole->deleteMiniConsole(name);
        QVERIFY2(deleted, qPrintable(deleteMsg));
    }
};

#include "CommandLineScrollRangeTest.moc"
MUDLET_GROUPED_TEST_MAIN(CommandLineScrollRangeTest)
