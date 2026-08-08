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

// Regression tests for #9715: the console's "Copy as image" context menu entry
// leaving nothing on the clipboard.
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
    // pane with nothing selected yet. The window is resized so the pane is large
    // enough for the multi-line drags the tests make across it.
    TTextEdit* preparePane()
    {
        mpServer->setWelcomeMessage(fillerText());
        if (!startProfile(mpHostname, mpLocalhost, mpPort)) {
            return nullptr;
        }
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

    void dragSelection(TTextEdit* pane, const QPointF& dragOffset)
    {
        const QPointF startPos = QRectF(pane->rect()).center();
        const QPointF endPos = startPos + dragOffset;
        sendMouse(pane, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, startPos);
        sendMouse(pane, QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, endPos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, endPos);
    }

    TTextEdit* prepareSelectedPane(const QPointF& dragOffset)
    {
        TTextEdit* pane = preparePane();
        if (!pane) {
            return nullptr;
        }
        dragSelection(pane, dragOffset);
        return pane;
    }

    static void copyAsImage(TTextEdit* pane)
    {
        QApplication::clipboard()->clear();
        pane->slot_copySelectionToClipboardImage();
    }

    // The context menu is assembled in mouseReleaseEvent() as a child QMenu of
    // the pane, so it can be opened for real and read back from there. Entries
    // are looked up by the object names the production code sets on them, since
    // their visible text is translated.
    QMenu* openContextMenu(TTextEdit* pane)
    {
        const QPointF pos = QRectF(pane->rect()).center();
        sendMouse(pane, QEvent::MouseButtonPress, Qt::RightButton, Qt::RightButton, pos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::RightButton, Qt::NoButton, pos);
        return pane->findChildren<QMenu*>().value(0);
    }

    static QAction* menuEntry(QMenu* menu, const QString& objectName)
    {
        for (QAction* action : menu->actions()) {
            if (action->objectName() == objectName) {
                return action;
            }
        }
        return nullptr;
    }

    // The four entries that cannot do anything without a selection.
    static QStringList selectionEntryNames() { return {QStringLiteral("consoleCopy"), QStringLiteral("consoleCopyHtml"), QStringLiteral("consoleCopyAsImage"), QStringLiteral("consoleSearchOnline")}; }

    // Every pixel is the console background, i.e. no text was drawn.
    static bool blankImage(const QImage& image, const QColor& backgroundColour)
    {
        const QRgb background = backgroundColour.rgb();
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if ((image.pixel(x, y) | 0xff000000) != (background | 0xff000000)) {
                    return false;
                }
            }
        }
        return true;
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

    // #9715 as reported: right-click the console and pick "Copy as image"
    // without selecting anything first.
    void test_contextMenuOffersNoCopyEntriesWithoutSelection()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        QVERIFY(pane->mSelectedRegion.isEmpty());

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");

        for (const QString& name : selectionEntryNames()) {
            QAction* entry = menuEntry(menu, name);
            QVERIFY2(entry, qPrintable(QStringLiteral("No \"%1\" entry in the console context menu").arg(name)));
            QVERIFY2(!entry->isEnabled(), qPrintable(QStringLiteral("\"%1\" was offered as usable with nothing selected (regression of #9715)").arg(name)));
            QVERIFY2(!entry->toolTip().isEmpty(), qPrintable(QStringLiteral("\"%1\" is disabled without saying why").arg(name)));
        }

        QAction* selectAll = menuEntry(menu, QStringLiteral("consoleSelectAll"));
        QVERIFY(selectAll);
        QVERIFY2(selectAll->isEnabled(), "\"Select all\" should not need an existing selection");
    }

    void test_contextMenuOffersCopyEntriesWithSelection()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 0));
        QVERIFY2(pane, "Could not prepare a console with a selection");
        QVERIFY(!pane->mSelectedRegion.isEmpty());

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");

        for (const QString& name : selectionEntryNames()) {
            QAction* entry = menuEntry(menu, name);
            QVERIFY2(entry, qPrintable(QStringLiteral("No \"%1\" entry in the console context menu").arg(name)));
            QVERIFY2(entry->isEnabled(), qPrintable(QStringLiteral("\"%1\" was disabled even though text is selected").arg(name)));
        }
    }

    // The whole user-facing path: pick the menu entry itself rather than calling
    // the slot, so a mis-wired entry is caught too.
    void test_menuEntryCopiesTheSelectionAsAnImage()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 0));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QAction* copyAsImageEntry = menuEntry(menu, QStringLiteral("consoleCopyAsImage"));
        QVERIFY2(copyAsImageEntry, "No \"Copy as image\" entry in the console context menu");

        QApplication::clipboard()->clear();
        copyAsImageEntry->trigger();

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "\"Copy as image\" put nothing on the clipboard (regression of #9715)");
        QVERIFY2(!blankImage(image, pane->mBgColor), "The copied image is entirely background, no text was drawn into it");
    }

    // The height of a multi-line copy comes from the dragged distance, not from
    // whatever the copy code recomputed.
    void test_multiLineSelectionCopiesEveryLine()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        const int expectedLines = 4;
        dragSelection(pane, QPointF(60, (expectedLines - 1) * pane->mFontHeight));
        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "The drag failed to create a selection");

        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "\"Copy as image\" put nothing on the clipboard (regression of #9715)");
        QCOMPARE(image.height(), expectedLines * pane->mFontHeight);
        QVERIFY2(!blankImage(image, pane->mBgColor), "The copied image is entirely background, no text was drawn into it");
    }

    // Copying twice in a row has to work: the first copy must not destroy the
    // selection state the second one needs.
    void test_repeatedCopyKeepsWorking()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 60));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        copyAsImage(pane);
        const QImage first = QApplication::clipboard()->image();
        QVERIFY2(!first.isNull(), "The first \"Copy as image\" put nothing on the clipboard");

        copyAsImage(pane);
        const QImage second = QApplication::clipboard()->image();
        QVERIFY2(!second.isNull(), "A second \"Copy as image\" of the same selection put nothing on the clipboard");
        // the first copy deselects and reselects to keep inverted colours out of
        // the image, so a mismatch here means it did not put the state back
        QCOMPARE(second, first);
    }

    // Copying is abandoned once it runs out of time, since a user can select a
    // whole buffer's worth of lines. Whatever was drawn by then still has to
    // reach the clipboard, at its original scale.
    void test_abandonedCopyKeepsTheLinesItDrew()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 60));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        const int originalTimeout = mudlet::self()->mCopyAsImageTimeout;
        // 0s of budget stops the drawing after the very first line
        mudlet::self()->mCopyAsImageTimeout = 0;
        copyAsImage(pane);
        const QImage abandoned = QApplication::clipboard()->image();
        mudlet::self()->mCopyAsImageTimeout = originalTimeout;

        QVERIFY2(!abandoned.isNull(), "A copy that ran out of time left nothing at all on the clipboard");
        QCOMPARE(abandoned.height(), pane->mFontHeight);

        copyAsImage(pane);
        const QImage complete = QApplication::clipboard()->image();
        QVERIFY2(complete.height() > abandoned.height(), "The unrestricted copy is no taller, so the abandoned one was not actually cut short");
        // scaling the abandoned copy to fit would have shrunk its width in
        // proportion and resampled the one line it did draw
        QCOMPARE(abandoned.width(), complete.width());
        QCOMPARE(abandoned, complete.copy(QRect(0, 0, complete.width(), abandoned.height())));
    }

    // Blank lines are still lines the user asked for.
    void test_blankLineSelectionCopiesAnImage()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        // with timestamps on, a blank line is still 13 characters wide, which is
        // not the zero width case this covers
        QVERIFY(!mudlet::self()->getActiveHost()->mpConsole->showTimeStamps());

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
        QCOMPARE(image.width(), pane->mFontWidth);
        QVERIFY2(blankImage(image, pane->mBgColor), "A blank line copied as something other than background");
    }

    // Bailing out must not take the user's existing clipboard contents with it.
    void test_copyWithoutSelectionLeavesTheClipboardAlone()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        QVERIFY(pane->mSelectedRegion.isEmpty());

        QApplication::clipboard()->setText(QStringLiteral("something the user copied earlier"));
        pane->slot_copySelectionToClipboardImage();

        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("something the user copied earlier"));
    }

    // Clearing a console leaves a selection covering lines that no longer exist,
    // which used to send the copy off the end of the buffer.
    void test_copyAfterClearingTheConsole()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 60));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        mudlet::self()->getActiveHost()->mpConsole->TConsole::clear();
        QVERIFY2(pane->mSelectedRegion.isEmpty(), "Clearing the console left a selection behind pointing at lines that are gone");

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QAction* copyAsImageEntry = menuEntry(menu, QStringLiteral("consoleCopyAsImage"));
        QVERIFY(copyAsImageEntry);
        QVERIFY2(!copyAsImageEntry->isEnabled(), "\"Copy as image\" was offered after the console was cleared");

        // and the slot itself has to cope, since lines can also disappear off the
        // front of a buffer that has grown past its limit
        pane->mDragStart = QPoint(0, 60);
        pane->mDragSelectionEnd = QPoint(6, 68);
        pane->normaliseSelection();
        pane->mSelectedRegion = QRegion(0, 0, 10, 10);
        copyAsImage(pane);
        QVERIFY2(!QApplication::clipboard()->image().isNull(), "Copying a selection that outlived its lines put nothing on the clipboard");
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mpHostname);

        // Tear down Mudlet (and with it the live cTelnet connection) before the
        // stub server it is talking to, so the socket is closed from the client
        // side rather than being yanked out from under an active connection.
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

private:
    // Returns false rather than only aborting the current QTest slot, so the
    // callers below never go on to dereference a host that never appeared.
    bool startProfile(const QString& hostname, const QString& address, const QString& port)
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
            qWarning() << "Profile took too long to load.";
            return false;
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host || !host->mpConsole) {
            qWarning() << "No active host available for the test.";
            return false;
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            qWarning() << "Could not connect with the host.";
            return false;
        }
        return true;
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
