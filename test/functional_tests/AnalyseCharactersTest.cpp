/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include <QDialog>
#include <QMenu>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// The console's "Analyse characters" entry opens its table in a window that
// takes keyboard focus, and the menu holding it can be opened from the keyboard.
class AnalyseCharactersTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-AnalyseCharacters";
    QString mPort;
    const QString mLocalhost = "localhost";

    static void sendMouse(QWidget* w, QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons, const QPointF& localPos)
    {
        const QPointF globalPos = w->mapToGlobal(localPos.toPoint());
        QMouseEvent event(type, localPos, globalPos, button, buttons, Qt::NoModifier);
        QApplication::sendEvent(w, &event);
    }

    static QMenu* openContextMenu(TTextEdit* pane)
    {
        const QPointF pos = QRectF(pane->rect()).center();
        sendMouse(pane, QEvent::MouseButtonPress, Qt::RightButton, Qt::RightButton, pos);
        sendMouse(pane, QEvent::MouseButtonRelease, Qt::RightButton, Qt::NoButton, pos);
        return pane->findChildren<QMenu*>().value(0);
    }

    static QAction* analyseEntry(QMenu* menu) { return menu->findChild<QAction*>(qsl("consoleAnalyseCharacters")); }

    static QDialog* analysisWindow()
    {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (auto* dialog = qobject_cast<QDialog*>(widget); dialog && dialog->objectName() == QLatin1String("consoleCharacterAnalysis") && dialog->isVisible()) {
                return dialog;
            }
        }
        return nullptr;
    }

    // A, e-acute and a grinning face: one, two and four bytes of UTF-8, the
    // last a surrogate pair in UTF-16
    TTextEdit* paneWithSelection()
    {
        Host* host = startOfflineProfile();
        if (!host) {
            return nullptr;
        }
        host->mEnableTextAnalyzer = true;
        if (!host->getLuaInterpreter()->compileAndExecuteScript(qsl("clearWindow() echo('A\\195\\169\\240\\159\\152\\128')"))) {
            return nullptr;
        }
        TTextEdit* pane = host->mainConsoleView()->mUpperPane;
        pane->slot_selectAll();
        return pane;
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

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteDirectory(MudletApp::getMudletPath(enums::profileHomePath, mHostname));
    }

    void choosingTheEntryOpensAReadableWindow()
    {
        TTextEdit* pane = paneWithSelection();
        QVERIFY2(pane, "Could not prepare a console with a selection");

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QAction* entry = analyseEntry(menu);
        QVERIFY2(entry, "No \"Analyse characters\" entry in the console context menu");
        QVERIFY(entry->isEnabled());

        entry->trigger();
        menu->close();
        QApplication::processEvents();

        QDialog* window = analysisWindow();
        QVERIFY2(window, "Choosing \"Analyse characters\" opened no window");
        QVERIFY2(!window->isModal(), "The analysis window blocks the console while open");
        auto* browser = window->findChild<QTextBrowser*>();
        const QString text = browser->toPlainText();
        for (const QString& codePoint : {qsl("0041"), qsl("00E9"), qsl("1F600")}) {
            QVERIFY2(text.contains(codePoint), qPrintable(qsl("U+%1 is missing from the analysis: %2").arg(codePoint, text)));
        }
        QVERIFY2(browser->focusPolicy() & Qt::TabFocus, "The analysis cannot be reached with the keyboard");
        QVERIFY2(browser->textInteractionFlags() & Qt::TextSelectableByKeyboard, "The analysis cannot be selected with the keyboard");
        QCOMPARE(window->focusWidget(), browser);
        QVERIFY(!browser->accessibleName().isEmpty());

        window->close();
    }

    void aLongMultiLineSelectionIsLabelledThroughout()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        host->mEnableTextAnalyzer = true;
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("clearWindow() echo(string.rep('x', 40) .. '\\nsecond line')")));
        TTextEdit* pane = host->mainConsoleView()->mUpperPane;
        pane->slot_selectAll();

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QAction* entry = analyseEntry(menu);
        QVERIFY2(entry, "No \"Analyse characters\" entry in the console context menu");
        entry->trigger();
        menu->close();
        QApplication::processEvents();

        QDialog* window = analysisWindow();
        QVERIFY2(window, "Choosing \"Analyse characters\" opened no window");
        const QString text = window->findChild<QTextBrowser*>()->toPlainText();
        QVERIFY2(text.contains(qsl("Only the first line")), qPrintable(text));
        QVERIFY2(!text.contains(qsl("second")), qPrintable(text));
        const auto tables = text.count(qsl("Lua character or code"));
        QVERIFY2(tables > 1, qPrintable(qsl("40 code points fit one table: %1").arg(text)));
        for (const QString& heading : {qsl("Index (UTF-16)"), qsl("Visual"), qsl("Index (UTF-8)"), qsl("Byte")}) {
            QVERIFY2(text.count(heading) == tables, qPrintable(qsl("\"%1\" heads %2 of %3 tables").arg(heading).arg(text.count(heading)).arg(tables)));
        }
        window->close();
    }

    void theMenuKeyOpensTheMenuAwayFromALink()
    {
        TTextEdit* pane = paneWithSelection();
        QVERIFY2(pane, "Could not prepare a console with a selection");
        Host* host = mudlet::self()->getActiveHost();
        host->setCaretEnabled(true);
        auto caretOff = qScopeGuard([host]() {
            if (host->caretEnabled()) {
                host->setCaretEnabled(false);
            }
        });
        QCOMPARE(host->mainConsoleView()->buffer.getFocusedLink(), 0);

        QTest::keyClick(pane, Qt::Key_Menu);
        auto* menu = pane->findChild<QMenu*>();
        QVERIFY2(menu, "The Menu key opened no menu in caret mode");
        QAction* entry = analyseEntry(menu);
        QVERIFY2(entry, "No \"Analyse characters\" entry in the menu the keyboard opened");

        entry->trigger();
        menu->close();
        QApplication::processEvents();
        QDialog* window = analysisWindow();
        QVERIFY2(window, "Choosing \"Analyse characters\" from the keyboard opened no window");
        window->close();
    }

    void aStaleSelectionIsNotOffered()
    {
        TTextEdit* pane = paneWithSelection();
        QVERIFY2(pane, "Could not prepare a console with a selection");
        // What a cleared window or a dropped keyboard selection leaves behind
        pane->mSelectedRegion = QRegion();
        QVERIFY(pane->mDragStart != pane->mDragSelectionEnd);

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QVERIFY2(!analyseEntry(menu), "\"Analyse characters\" was offered with nothing selected");
        menu->close();
    }

    void hoveringStillShowsTheTooltip()
    {
        TTextEdit* pane = paneWithSelection();
        QVERIFY2(pane, "Could not prepare a console with a selection");

        QMenu* menu = openContextMenu(pane);
        QVERIFY2(menu, "Right-clicking the console opened no context menu");
        QAction* entry = analyseEntry(menu);
        QVERIFY2(entry, "No \"Analyse characters\" entry in the console context menu");

        entry->hover();
        QVERIFY2(entry->toolTip().contains(qsl("1F600")), qPrintable(entry->toolTip()));
        QVERIFY2(!analysisWindow(), "Hovering opened the analysis window");
        menu->close();
    }

    void cleanup()
    {
        const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

private:
    Host* startOfflineProfile()
    {
        if (!TestProfile::create(mHostname, mLocalhost, mPort)) {
            return nullptr;
        }
        QSignalSpy connected(&(mudlet::self()->getActiveHost()->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2s)) {
            return nullptr;
        }
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }
        mudlet::self()->resize(1200, 800);
        QApplication::processEvents();
        // Printing into a live connection would race with the stub's traffic
        host->mTelnet.disconnectIt();
        const bool disconnected = QTest::qWaitFor(
                [host]() {
                    return host->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                },
                5s);
        return disconnected ? host : nullptr;
    }

    static void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "AnalyseCharactersTest.moc"
MUDLET_GROUPED_TEST_MAIN(AnalyseCharactersTest)
