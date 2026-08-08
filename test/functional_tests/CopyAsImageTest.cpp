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

#include <QClipboard>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

// Regression test for #9715: the console's "Copy as image" context menu entry
// left nothing on the clipboard.
class CopyAsImageTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-CopyAsImage";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

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

    // Boots a profile whose console is filled with text, and returns its upper
    // pane with nothing selected yet.
    TTextEdit* preparePane()
    {
        mpServer->setWelcomeMessage(fillerText());
        startProfile(mpHostname, mpLocalhost, mpPort);
        if (!waitForTextInBuffer(QString(100, QLatin1Char('X')))) {
            return nullptr;
        }

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        TTextEdit* pane = upperPane();
        if (!pane) {
            return nullptr;
        }
        pane->unHighlight();
        pane->mSelectedRegion = QRegion();
        return pane;
    }

    TTextEdit* prepareSelectedPane(const QPointF& dragOffset)
    {
        TTextEdit* pane = preparePane();
        if (!pane) {
            return nullptr;
        }

        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + dragOffset;
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, endPos);
        return pane;
    }

    static void copyAsImage(TTextEdit* pane)
    {
        QApplication::clipboard()->clear();
        pane->slot_copySelectionToClipboardImage();
    }

    struct MenuEntry
    {
        bool enabled = false;
        QString toolTip;
    };

    // The context menu is assembled in mouseReleaseEvent() as a child QMenu of
    // the pane. Its entries are read back by construction order rather than by
    // their translated text: Copy, Copy HTML, Copy as image, Select all.
    QList<MenuEntry> openContextMenuEntries(TTextEdit* pane)
    {
        const QPointF pos = QRectF(pane->rect()).center();
        sendMouse(pane, QEvent::MouseButtonPress, Qt::RightButton, Qt::RightButton, pos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::RightButton, Qt::NoButton, pos);

        QList<MenuEntry> entries;
        QMenu* menu = pane->findChild<QMenu*>();
        if (!menu) {
            return entries;
        }
        for (const QAction* action : menu->actions()) {
            if (!action->isSeparator()) {
                entries.append({action->isEnabled(), action->toolTip()});
            }
        }
        menu->close();
        return entries;
    }

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // #9715 as reported: right-clicking the console and picking "Copy as image"
    // without selecting anything first. The entry used to be offered as usable
    // and then quietly did nothing at all.
    void test_contextMenuOffersNoCopyEntriesWithoutSelection()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        QVERIFY(pane->mSelectedRegion.isEmpty());

        const QList<MenuEntry> entries = openContextMenuEntries(pane);
        QVERIFY2(entries.size() >= 4, qPrintable(QStringLiteral("Expected at least 4 context menu entries, got %1").arg(entries.size())));

        // Copy, Copy HTML and Copy as image
        for (int i = 0; i < 3; ++i) {
            QVERIFY2(!entries.at(i).enabled, qPrintable(QStringLiteral("Context menu entry %1 was offered as usable with nothing selected (regression of #9715)").arg(i)));
            QVERIFY2(!entries.at(i).toolTip.isEmpty(), qPrintable(QStringLiteral("Context menu entry %1 is disabled without saying why").arg(i)));
        }
        // Select all does not need a selection, so it stays available
        QVERIFY2(entries.at(3).enabled, "\"Select all\" should not need an existing selection");
    }

    void test_contextMenuOffersCopyEntriesWithSelection()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 0));
        QVERIFY2(pane, "Could not prepare a console with a selection");
        QVERIFY(!pane->mSelectedRegion.isEmpty());

        const QList<MenuEntry> entries = openContextMenuEntries(pane);
        QVERIFY2(entries.size() >= 4, qPrintable(QStringLiteral("Expected at least 4 context menu entries, got %1").arg(entries.size())));
        for (int i = 0; i < 4; ++i) {
            QVERIFY2(entries.at(i).enabled, qPrintable(QStringLiteral("Context menu entry %1 was disabled even though text is selected").arg(i)));
        }
    }

    // A single-line selection has to reach the clipboard as an image.
    void test_singleLineSelectionCopiesAnImage()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 0));
        QVERIFY2(pane, "Could not prepare a console with a selection");
        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "The drag failed to create a selection");

        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "\"Copy as image\" put nothing on the clipboard (regression of #9715)");
        QVERIFY2(image.width() > 0 && image.height() > 0, qPrintable(QStringLiteral("Clipboard image is empty: %1x%2").arg(image.width()).arg(image.height())));
    }

    // A multi-line selection has to reach the clipboard with a line for each
    // selected line, rather than being truncated away to nothing.
    void test_multiLineSelectionCopiesEveryLine()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 60));
        QVERIFY2(pane, "Could not prepare a console with a selection");
        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "The drag failed to create a selection");

        const int selectedLines = pane->mPB.y() - pane->mPA.y() + 1;
        QVERIFY2(selectedLines > 1, qPrintable(QStringLiteral("Expected a multi-line selection, got %1 line(s)").arg(selectedLines)));

        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "\"Copy as image\" put nothing on the clipboard (regression of #9715)");
        QCOMPARE(image.height(), selectedLines * pane->mFontHeight);
    }

    // Copying twice in a row has to work: the first copy must not destroy the
    // selection state the second one needs.
    void test_repeatedCopyKeepsWorking()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 60));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        copyAsImage(pane);
        QVERIFY2(!QApplication::clipboard()->image().isNull(), "The first \"Copy as image\" put nothing on the clipboard");

        copyAsImage(pane);
        QVERIFY2(!QApplication::clipboard()->image().isNull(), "A second \"Copy as image\" of the same selection put nothing on the clipboard");
    }

    // Selecting a lot of the buffer is the expected way to use this feature, so
    // the copy is deliberately abandoned once it runs out of time. Whatever was
    // drawn by then still has to reach the clipboard, at its original scale.
    void test_abandonedCopyKeepsTheLinesItDrew()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 60));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        const int originalTimeout = mudlet::self()->mCopyAsImageTimeout;
        // 0s of budget stops the drawing after the very first line
        mudlet::self()->mCopyAsImageTimeout = 0;
        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "A copy that ran out of time left nothing at all on the clipboard");
        QCOMPARE(image.height(), pane->mFontHeight);

        // an unscaled crop of the drawn lines, not the whole selection squashed
        // down to fit into the height of the lines that were drawn
        mudlet::self()->mCopyAsImageTimeout = originalTimeout;
        copyAsImage(pane);
        QCOMPARE(image.width(), QApplication::clipboard()->image().width());
    }

    // Blank lines are still lines the user asked for.
    void test_blankLineSelectionCopiesAnImage()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");

        const int blankLine = firstBlankLine();
        QVERIFY2(blankLine >= 0, "The console has no blank line to select");

        pane->mDragStart = QPoint(0, blankLine);
        pane->mDragSelectionEnd = pane->mDragStart;
        pane->normaliseSelection();
        pane->highlightSelection();
        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "Could not select a blank line");

        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "Copying a selection of blank lines left nothing on the clipboard");
        QCOMPARE(image.height(), pane->mFontHeight);
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
        QTimer::singleShot(0ms, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    int firstBlankLine() const
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.lineBuffer.at(i).isEmpty()) {
                return i;
            }
        }
        return -1;
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

void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "CopyAsImageTest.moc"
QTEST_MAIN(CopyAsImageTest)
