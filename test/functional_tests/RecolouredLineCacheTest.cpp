/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// setFgColor() and its siblings rewrite the colours of an existing selection
// rather than appending to the buffer. They used to answer that by forcing a
// whole-screen repaint of both panes, which is what made every coloured echo
// cost a full relayout; they now mark only the lines the selection covers.
//
// The hazard that swap introduces is invisible in the buffer, which holds the
// new colour either way: the recoloured row has to reach the pane's CACHED
// screen pixmap. If it only ever reaches the widget, the next paint - which
// seeds itself from that cache - silently restores the old colour. So the
// assertion that matters here is not that the recolour shows up, but that it is
// still there after later output has scrolled the line and forced the cache to
// be the source of those pixels.
class RecolouredLineCacheTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-Recolour";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    // Nothing else on screen is this colour, so counting it is enough to say
    // whether the recoloured line is being drawn.
    static QColor markerColor() { return QColor(255, 0, 255); }

    // Antialiasing means few pixels land on the exact colour, so match the
    // marker by its shape - strong red and blue, little green - rather than
    // demanding equality.
    static int countMarkerPixels(const QImage& frame)
    {
        int count = 0;
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                const QColor pixel = frame.pixelColor(x, y);
                if (pixel.red() > 150 && pixel.blue() > 150 && pixel.green() < 80) {
                    ++count;
                }
            }
        }
        return count;
    }

    QString fillerText() const
    {
        const QString line = QString(100, QLatin1Char('X'));
        QString message;
        for (int i = 0; i < 80; ++i) {
            message.append(line);
            message.append(QStringLiteral("\r\n"));
        }
        return message;
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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_recolouredLineSurvivesTheScrollThatFollowsIt()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        auto console = host->mpConsole;
        QVERIFY(console);
        TTextEdit* pane = console->mUpperPane;
        QVERIFY(pane);

        const int screenRows = pane->getScreenHeight();
        QVERIFY2(screenRows > 12, qPrintable(qsl("the pane draws only %1 rows, too few to scroll a line without losing it off the top").arg(screenRows)));

        // A row in the UPPER half, so the partial render below can be aimed
        // somewhere else entirely.
        const int topLine = pane->imageTopLine();
        const int targetLine = topLine + 3;
        QVERIFY2(targetLine < console->buffer.getLastLineNumber(), "not enough buffered lines to pick one to recolour");

        // Warm the pane's cached screen with the pre-recolour picture, so what
        // the checks below see really is the cache being reused rather than a
        // first paint drawing everything from the buffer.
        QPixmap warmUp(pane->size());
        warmUp.fill(Qt::darkGray);
        pane->render(&warmUp);
        QCOMPARE(countMarkerPixels(warmUp.toImage()), 0);

        // Exactly the path Lua's fg() / setFgColor() takes.
        QVERIFY2(console->moveCursor(0, targetLine), "could not put the user cursor on the line to recolour");
        QVERIFY2(console->selectSection(0, console->buffer.line(targetLine).size()), "could not select the line to recolour");
        console->setFgColor(markerColor());

        // A partial repaint aimed at the BOTTOM of the pane - nowhere near the
        // line just recoloured. Only the dirty-line range can pull that line
        // into this frame; without it the cache keeps the old colour and the
        // full render below never gets a chance to recover it.
        const int rowHeight = std::max(1, pane->height() / screenRows);
        const QRect elsewhere(0, pane->height() - (2 * rowHeight), pane->width(), 2 * rowHeight);
        QPixmap partialFrame(pane->size());
        partialFrame.fill(Qt::darkGray);
        pane->render(&partialFrame, QPoint(), QRegion(elsewhere));

        // Nothing scrolled, so this frame is served almost entirely from the
        // cached screen - which is the point. If the recolour never reached the
        // cache, the old colour is what comes back.
        QPixmap finalFrame(pane->size());
        finalFrame.fill(Qt::darkGray);
        pane->render(&finalFrame);

        QVERIFY2(countMarkerPixels(finalFrame.toImage()) > 0,
                 "recolouring a line left no trace in the pane's cached screen, so the next paint served the old colour back - the dirty-line range was not honoured");
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    bool waitForTextInBuffer(const QString& text, int timeoutMs = 5000)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        return QTest::qWaitFor(
                [&]() {
                    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
                        if (console->buffer.line(i) == text) {
                            return true;
                        }
                    }
                    return false;
                },
                timeoutMs);
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = MudletPaths::getMudletPath(enums::profileHomePath, profileName);
        deleteDirectory(path);
    }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }

private slots:
    void cleanup()
    {
        const QString profilePath = MudletPaths::getMudletPath(enums::profileHomePath, mpHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }
};

#include "RecolouredLineCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(RecolouredLineCacheTest)
