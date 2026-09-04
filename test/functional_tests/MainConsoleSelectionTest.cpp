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

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Regression test for #3922: left-clicking into the main console to give it
// focus must not leave a one-character selection behind. Such a stray
// selection hijacks Ctrl+C away from the command line (TCommandLine prioritises
// any console selection over the input box).
class MainConsoleSelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-Selection";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    // Send a screenful of text so a click in the middle of the upper pane lands
    // on a real, filled line rather than empty space.
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

    TTextEdit* upperPane() const
    {
        auto host = mudlet::self()->getActiveHost();
        if (!host || !host->mpConsole) {
            return nullptr;
        }
        return host->mpConsole->mUpperPane;
    }

    void sendMouse(QWidget* w, QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons, const QPointF& localPos)
    {
        const QPointF globalPos = w->mapToGlobal(localPos.toPoint());
        QMouseEvent event(type, localPos, globalPos, button, buttons, Qt::NoModifier);
        QApplication::sendEvent(w, &event);
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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // A plain click (press + a move that stays in the same character cell +
    // release) must not create a selection.
    void test_clickWithoutDragLeavesNoSelection()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");
        QVERIFY2(pane->width() > 400 && pane->height() > 100, qPrintable(QStringLiteral("Upper pane too small: %1x%2").arg(pane->width()).arg(pane->height())));

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();
        QVERIFY(pane->mSelectedRegion.isEmpty());

        // Click in the middle of the pane (well past the timestamp gutter), then
        // a move event at the very same pixel - i.e. no movement to a different
        // character cell - then release.
        const QPointF clickPos = QRectF(pane->rect()).center();
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, clickPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, clickPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, clickPos);

        QVERIFY2(pane->mSelectedRegion.isEmpty(), "A click with no drag left a stray selection in the console (regression of #3922)");
    }

    // Control case: a genuine drag across cells must still produce a selection,
    // so the fix above does not over-correct.
    void test_dragStillSelects()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();

        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + QPointF(60, 0); // several character cells to the right
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, endPos);

        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "A real drag failed to create a selection");
    }

    // Regression case for the review fix: once a drag has genuinely selected
    // text, dragging back to the original press cell must collapse the extent
    // instead of leaving the previous selection frozen.
    void test_dragBackToOriginCollapsesSelectionExtent()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        pane->unHighlight();
        pane->mSelectedRegion = QRegion();

        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + QPointF(60, 0); // several character cells to the right
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);

        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "The initial drag failed to create a selection");
        const QRect expandedSelection = pane->mSelectedRegion.boundingRect();

        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, startPos);

        const QRect collapsedSelection = pane->mSelectedRegion.boundingRect();
        QVERIFY2(collapsedSelection.width() < expandedSelection.width(), "Dragging back to the press cell left the earlier selection extent frozen");
    }

    // The mouse selection is a flag on each TChar, so whatever the line's
    // characters are held in has to keep them where they are once a selection
    // has been made over them. Text arriving on the line that is already under
    // selection - a prompt, an echo - is how that gets tested in practice.
    void test_appendingToASelectedLineKeepsItHighlighted()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(QString(100, QLatin1Char('X'))), "Filler text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        TMainConsole* console = mudlet::self()->getActiveHost()->mpConsole;
        console->print(qsl("\nselected"));
        const int y = console->buffer.getLastLineNumber();
        QCOMPARE(console->buffer.line(y), qsl("selected"));

        pane->slot_selectAll();
        QVERIFY2(console->buffer.buffer.at(y).at(0).isSelected(), "selecting all did not mark the last line, so a dropped flag below could not be told from one that was never set");

        // enough for the line to outgrow whatever it is held in, but short
        // enough that it cannot wrap and move to a line of its own
        console->print(QString(20, QLatin1Char('z')));
        QCOMPARE(console->buffer.getLastLineNumber(), y);

        QVERIFY2(console->buffer.buffer.at(y).at(0).isSelected(), "text arriving on a selected line deselected the characters that were already on it");
    }

    // #6363: the mouse cursor becomes a hand over a link, and the reset back to
    // the I-beam lives inside two bounds checks in updateTextCursor(). Leaving
    // the link sideways lands on a character that answers those checks, so the
    // reset runs; leaving it downwards lands past the last line of the buffer,
    // where neither check is satisfied and the hand is left on screen.
    void test_theCursorStopsBeingAHandAfterTheMouseLeavesALinkDownwards()
    {
        mpServer->setWelcomeMessage(qsl("cursor test\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(qsl("cursor test")), "the welcome text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        auto host = mudlet::self()->getActiveHost();
        // the profile's own startup output otherwise fills the pane, leaving no
        // blank rows under the link for the mouse to move down into
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("clearWindow()\nechoLink('LinkForCursorTest', [[ ]], '', true)\n")), "the echoLink() call failed");
        QVERIFY2(waitForTextInBuffer(qsl("LinkForCursorTest")), "the link text never reached the buffer");
        QTest::qWait(100ms);

        // the link's own pixel is found rather than calculated, so a timestamp
        // gutter or a font of another size cannot put this on the wrong cell
        QPoint overTheLink;
        for (int y = pane->mFontHeight / 2; y < pane->height() && overTheLink.isNull(); y += pane->mFontHeight) {
            for (int x = pane->mFontWidth / 2; x < pane->width(); x += pane->mFontWidth) {
                sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(x, y));
                if (pane->cursor().shape() == Qt::PointingHandCursor) {
                    overTheLink = QPoint(x, y);
                    break;
                }
            }
        }
        QVERIFY2(!overTheLink.isNull(), "no pixel of the pane produced the hand cursor, so the move below proves nothing");
        QCOMPARE(pane->cursor().shape(), Qt::PointingHandCursor);

        // straight down from the link, into the empty part of the pane below
        // every line the buffer holds
        const int emptyRowY = pane->height() - (pane->mFontHeight / 2);
        QVERIFY2((emptyRowY / pane->mFontHeight) + pane->imageTopLine() >= static_cast<int>(pane->mpBuffer->buffer.size()),
                 "the chosen row still holds text, so it does not exercise the reported case");
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(overTheLink.x(), emptyRowY));

        QCOMPARE(pane->cursor().shape(), Qt::IBeamCursor);
    }

    // The same reset, reached by the other bounds check: an empty line inside
    // the buffer answers the line check but has no character to look a link up
    // on, so convertMouseXToBufferX() falls out of its loop and the reset has to
    // come from the character check instead. That check also stands between the
    // condition and its own at(tCharIndex) call, so losing it on this arm throws
    // rather than merely stranding the hand.
    void test_theCursorStopsBeingAHandOverAnEmptyLineInsideTheBuffer()
    {
        mpServer->setWelcomeMessage(qsl("cursor test\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(qsl("cursor test")), "the welcome text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        QVERIFY2(pane, "No upper pane available");

        auto host = mudlet::self()->getActiveHost();
        // Hide would keep the blank line out of the buffer and ReplaceWithSpace
        // would give it a character to find, either of which leaves this
        // exercising the same arm as the test above
        host->mBlankLineBehaviour = Host::BlankLineBehaviour::Show;
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("clearWindow()\nechoLink('LinkAboveEmptyRow', [[ ]], '', true)\necho('\\n\\nrowBelowTheBlankOne\\n')\n")),
                 "the echoLink() call failed");
        QVERIFY2(waitForTextInBuffer(qsl("rowBelowTheBlankOne")), "the text under the blank line never reached the buffer");
        QTest::qWait(100ms);

        QPoint overTheLink;
        for (int y = pane->mFontHeight / 2; y < pane->height() && overTheLink.isNull(); y += pane->mFontHeight) {
            for (int x = pane->mFontWidth / 2; x < pane->width(); x += pane->mFontWidth) {
                sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(x, y));
                if (pane->cursor().shape() == Qt::PointingHandCursor) {
                    overTheLink = QPoint(x, y);
                    break;
                }
            }
        }
        QVERIFY2(!overTheLink.isNull(), "no pixel of the pane produced the hand cursor, so the move below proves nothing");
        QCOMPARE(pane->cursor().shape(), Qt::PointingHandCursor);

        // straight down onto the blank line, which the row under it keeps inside
        // the buffer
        const int blankRowY = overTheLink.y() + pane->mFontHeight;
        const int blankRowLine = (blankRowY / pane->mFontHeight) + pane->imageTopLine();
        QVERIFY2(blankRowLine < static_cast<int>(pane->mpBuffer->buffer.size()),
                 "the row below the link is past the buffer, which is the case the test above covers rather than this one");
        QVERIFY2(pane->mpBuffer->buffer.at(blankRowLine).empty(),
                 qPrintable(qsl("the row below the link holds %1 characters rather than none, so the character check is not what has to reject it")
                                    .arg(pane->mpBuffer->buffer.at(blankRowLine).size())));
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::NoButton, QPointF(overTheLink.x(), blankRowY));

        QCOMPARE(pane->cursor().shape(), Qt::IBeamCursor);
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mpHostname);

        // Tear down Mudlet (and with it the live cTelnet connection) before the
        // stub server it is talking to, so the socket is closed from the client
        // side rather than being yanked out from under an active connection when
        // the server is destroyed - the latter ordering can flake or crash.
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
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
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
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
};

#include "MainConsoleSelectionTest.moc"
MUDLET_GROUPED_TEST_MAIN(MainConsoleSelectionTest)
