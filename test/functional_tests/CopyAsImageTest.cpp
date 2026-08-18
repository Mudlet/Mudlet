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
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Regression tests for #9715: the console's "Copy as image" context menu entry
// leaving nothing on the clipboard.
class CopyAsImageTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
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

    TTextEdit* preparePane()
    {
        mpServer->setWelcomeMessage(fillerText());
        if (!startProfile(mpHostname, mpLocalhost, mpPort)) {
            return nullptr;
        }
        if (!waitForTextInBuffer(QString(100, QLatin1Char('X')))) {
            return nullptr;
        }

        // big enough for the multi-line drags the tests make across the pane
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

    // mouseReleaseEvent() parents the menu to the pane, so it can be read back
    // from there rather than having to be intercepted as it pops up.
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

    // "Copy as image" is deliberately not one of these: it falls back to the
    // visible screen instead of needing a selection.
    static QStringList selectionEntryNames() { return {QStringLiteral("consoleCopy"), QStringLiteral("consoleCopyHtml"), QStringLiteral("consoleSearchOnline")}; }

    static int backgroundPixels(const QImage& image, const QColor& backgroundColour)
    {
        const QRgb background = backgroundColour.rgb() | 0xff000000;
        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if ((image.pixel(x, y) | 0xff000000) == background) {
                    ++count;
                }
            }
        }
        return count;
    }

    static bool blankImage(const QImage& image, const QColor& backgroundColour) { return backgroundPixels(image, backgroundColour) == image.width() * image.height(); }

    // Text drawn normally leaves most of the cell as background; a line drawn
    // with its selection still on has the two swapped over.
    static bool invertedImage(const QImage& image, const QColor& backgroundColour) { return backgroundPixels(image, backgroundColour) * 2 < image.width() * image.height(); }

    // Mimics TBuffer::shrinkBuffer() dropping the oldest lines once the buffer
    // reaches its size limit, which shifts every remaining line's index down.
    void shrinkBuffer(TBuffer& buffer, int lines)
    {
        buffer.mBatchDeleteSize = lines;
        for (int i = 0; i < lines; ++i) {
            buffer.lineBuffer.pop_front();
            buffer.promptBuffer.pop_front();
            buffer.timeBuffer.pop_front();
            buffer.buffer.pop_front();
            buffer.mCursorY--;
        }
    }

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
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

    // #9715 as reported, driven through the menu so a mis-wired entry is caught too.
    void test_noSelectionCopiesTheVisibleScreen()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        QVERIFY(pane->mSelectedRegion.isEmpty());

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QAction* copyAsImageEntry = menuEntry(menu, QStringLiteral("consoleCopyAsImage"));
        QVERIFY2(copyAsImageEntry, "No \"Copy as image\" entry in the console context menu");
        QVERIFY2(copyAsImageEntry->isEnabled(), "\"Copy as image\" was not offered with nothing selected (regression of #9715)");

        QApplication::clipboard()->clear();
        copyAsImageEntry->trigger();

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "\"Copy as image\" put nothing on the clipboard (regression of #9715)");
        QVERIFY2(!blankImage(image, pane->mBgColor), "The copied image is entirely background, no text was drawn into it");
        QCOMPARE(image.height() % pane->mFontHeight, 0);
        QVERIFY2(image.height() >= 10 * pane->mFontHeight, qPrintable(QStringLiteral("Only %1 lines copied, expected a screenful").arg(image.height() / pane->mFontHeight)));
        QVERIFY2(image.height() <= pane->height(), qPrintable(QStringLiteral("Copied %1px, taller than the %2px pane").arg(image.height()).arg(pane->height())));
    }

    void test_visibleScreenCopyIncludesTimestamps()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QVERIFY(!console->showTimeStamps());

        copyAsImage(pane);
        const int widthWithoutTimestamps = QApplication::clipboard()->image().width();
        QVERIFY(widthWithoutTimestamps > 0);

        console->slot_toggleTimeStamps(true);
        QVERIFY(console->showTimeStamps());
        QTest::qWait(100ms);

        copyAsImage(pane);
        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "\"Copy as image\" put nothing on the clipboard with timestamps showing");
        QCOMPARE(image.width(), widthWithoutTimestamps + mudlet::smTimeStampFormat.size() * pane->mFontWidth);
    }

    void test_contextMenuOffersNoSelectionOnlyEntriesWithoutSelection()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        QVERIFY(pane->mSelectedRegion.isEmpty());

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");

        for (const QString& name : selectionEntryNames()) {
            QAction* entry = menuEntry(menu, name);
            QVERIFY2(entry, qPrintable(QStringLiteral("No \"%1\" entry in the console context menu").arg(name)));
            QVERIFY2(!entry->isEnabled(), qPrintable(QStringLiteral("\"%1\" was offered as usable with nothing selected").arg(name)));
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

        for (const QString& name : selectionEntryNames() + QStringList{QStringLiteral("consoleCopyAsImage")}) {
            QAction* entry = menuEntry(menu, name);
            QVERIFY2(entry, qPrintable(QStringLiteral("No \"%1\" entry in the console context menu").arg(name)));
            QVERIFY2(entry->isEnabled(), qPrintable(QStringLiteral("\"%1\" was disabled even though text is selected").arg(name)));
        }
    }

    void test_selectionCopiesOnlyTheSelectedLines()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 0));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "\"Copy as image\" put nothing on the clipboard (regression of #9715)");
        QCOMPARE(image.height(), pane->mFontHeight);
        QVERIFY2(!blankImage(image, pane->mBgColor), "The copied image is entirely background, no text was drawn into it");
    }

    // The height must come from the dragged distance, not from a recomputed
    // mScreenHeight.
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

    // A selection can be a whole buffer long, so the copy gives up on a timeout -
    // whatever it drew by then still has to reach the clipboard at its own scale.
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

    void test_emptyBufferCopiesNothingAndKeepsTheClipboard()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        // TConsole::clear() leaves an empty line behind, so empty it by hand
        auto& buffer = mudlet::self()->getActiveHost()->mpConsole->buffer;
        buffer.lineBuffer.clear();
        buffer.timeBuffer.clear();
        buffer.promptBuffer.clear();
        buffer.buffer.clear();
        buffer.mCursorY = 0;

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QAction* copyAsImageEntry = menuEntry(menu, QStringLiteral("consoleCopyAsImage"));
        QVERIFY(copyAsImageEntry);
        QVERIFY2(!copyAsImageEntry->isEnabled(), "\"Copy as image\" was offered for a console holding no text at all");
        QVERIFY2(!copyAsImageEntry->toolTip().isEmpty(), "\"Copy as image\" is disabled without saying why");

        QApplication::clipboard()->setText(QStringLiteral("something the user copied earlier"));
        pane->slot_copySelectionToClipboardImage();
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("something the user copied earlier"));
    }

    // Clearing a console strands the selection on lines that no longer exist.
    void test_copyAfterClearingTheConsole()
    {
        TTextEdit* pane = prepareSelectedPane(QPointF(60, 60));
        QVERIFY2(pane, "Could not prepare a console with a selection");

        mudlet::self()->getActiveHost()->mpConsole->TConsole::clear();
        QVERIFY2(pane->mSelectedRegion.isEmpty(), "Clearing the console left a selection behind pointing at lines that are gone");

        copyAsImage(pane);
        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "Copying a cleared console put nothing on the clipboard");
        QVERIFY2(blankImage(image, pane->mBgColor), "A cleared console copied as something other than background");
    }

    // Lines dropped off the front of a full buffer shift every remaining index
    // down, so the selection has to be followed down with them.
    void test_copyFollowsTheSelectionThroughABufferShrink()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        auto& buffer = mudlet::self()->getActiveHost()->mpConsole->buffer;

        dragSelection(pane, QPointF(60, 3 * pane->mFontHeight));
        QVERIFY2(!pane->mSelectedRegion.isEmpty(), "The drag failed to create a selection");
        const int selectedLines = pane->mPB.y() - pane->mPA.y() + 1;

        // enough to push the selection past the end of the buffer, so that the
        // shift is what has to bring it back rather than it happening to still fit
        const int droppedLines = buffer.lineBuffer.size() - pane->mPB.y() + 2;
        QVERIFY2(droppedLines > 0 && droppedLines <= pane->mPA.y(), "Could not size a buffer shrink that strands the selection");
        shrinkBuffer(buffer, droppedLines);
        QVERIFY2(pane->mPB.y() > buffer.getLastLineNumber(), "The selection still fits, so the shift is not being exercised");

        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "Copying after a buffer shrink put nothing on the clipboard");
        QCOMPARE(image.height(), selectedLines * pane->mFontHeight);
        QVERIFY2(!blankImage(image, pane->mBgColor), "The copied image is entirely background, the selection was not followed down");
        QVERIFY2(!invertedImage(image, pane->mBgColor), "The copied image still has the selection's inverted colours on it");
    }

    // A selection outliving its lines must not be reinterpreted as whatever now
    // sits at those buffer indices.
    void test_copyOfASelectionPastTheEndOfTheBuffer()
    {
        TTextEdit* pane = preparePane();
        QVERIFY2(pane, "Could not prepare a console");
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        const int lastLine = console->buffer.getLastLineNumber();

        // beyond the buffer and beyond any batch-delete adjustment, i.e. gone
        pane->mDragStart = QPoint(0, lastLine + console->buffer.mBatchDeleteSize + 10);
        pane->mDragSelectionEnd = QPoint(6, lastLine + console->buffer.mBatchDeleteSize + 12);
        pane->normaliseSelection();
        pane->mSelectedRegion = QRegion(0, 0, 10, 10);

        copyAsImage(pane);

        const QImage image = QApplication::clipboard()->image();
        QVERIFY2(!image.isNull(), "Copying a selection that outlived its lines put nothing on the clipboard");
        QVERIFY2(image.height() >= 10 * pane->mFontHeight, qPrintable(QStringLiteral("Only %1 lines copied, expected the visible screen").arg(image.height() / pane->mFontHeight)));
        QVERIFY2(!blankImage(image, pane->mBgColor), "The copied image is entirely background, no text was drawn into it");
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
    // Returns false rather than QVERIFYing, which would only abort the caller's
    // helper and let the test go on to dereference a host that never appeared.
    bool startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
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

#include "CopyAsImageTest.moc"
MUDLET_GROUPED_TEST_MAIN(CopyAsImageTest)
