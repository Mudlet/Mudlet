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
 * A profile's script editor is built the first time something opens it, not
 * with the profile. What it would have shown had it been there all along - the
 * error console's lines and the latest key binding warning - must be there when
 * it opens (#7317).
 *
 * Run with: ctest -R EditorLazyCreationTest -V
 */

#include <QApplication>
#include <QLabel>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <functional>

#include "Host.h"
#include "HostDialogs.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TConsole.h"
#include "TDetachedWindow.h"
#include "TLuaInterpreter.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "edbee/models/textautocompleteprovider.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/textrange.h"
#include "edbee/texteditorwidget.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Hands every event any object receives to a callback for as long as it lives
class EventWatcher : public QObject
{
public:
    explicit EventWatcher(std::function<void(QObject*)> onEvent)
    : mOnEvent(std::move(onEvent))
    {
        qApp->installEventFilter(this);
    }
    ~EventWatcher() override { qApp->removeEventFilter(this); }

protected:
    bool eventFilter(QObject* pWatched, QEvent*) override
    {
        mOnEvent(pWatched);
        return false;
    }

private:
    std::function<void(QObject*)> mOnEvent;
};

class EditorLazyCreationTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    QStringList mProfilesMade;

    Host* openProfile(const QString& profileName)
    {
        QDir(MudletApp::getMudletPath(enums::profileHomePath, profileName)).removeRecursively();
        mProfilesMade.append(profileName);
        // A view's introduction replaces a notice when the editor opens on it
        const QString sanitized = MudletApp::sanitizeForPath(profileName);
        for (const char* view : {"cmtriggerview", "cmaliasview", "cmkeysview", "cmscriptview", "cmtimerview", "cmactionview"}) {
            MudletApp::getQSettings()->setValue(qsl("Editor/banner_permanently_hidden/profiles/%1/%2/intro").arg(sanitized, QLatin1String(view)), true);
        }
        Host* pHost = TestProfile::create(profileName, mLocalhost, mPort);
        if (!pHost) {
            return nullptr;
        }
        QSignalSpy connected(&(pHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2s)) {
            return nullptr;
        }
        // Past the reset the profile load queued
        QTest::qWait(50ms);
        return pHost;
    }

    void closeProfile(Host* pHost, const QString& profileName)
    {
        pHost->forceClose();
        QVERIFY2(pHost->requestClose(), "Closing the profile was refused");
        HostManager::self()->deleteHost(profileName);
        QTest::qWait(50ms);
    }

    // The Lua error, or a null QString when the chunk ran
    static QString runLua(Host* pHost, const QString& code)
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(a Lua error that is not a string)");
        lua_pop(L, 1);
        return error;
    }

    static dlgTriggerEditor* openEditor(Host* pHost)
    {
        mudlet::self()->activateProfile(pHost);
        QTest::qWait(50ms);
        mudlet::self()->slot_showEditorDialog();
        QTest::qWait(100ms);
        return HostDialogs::of(pHost).mpEditorDialog.data();
    }

    // Clearing a message hides the area but leaves its text behind
    static QString bannerText(dlgTriggerEditor* pEditor)
    {
        if (!pEditor || pEditor->mpSystemMessageArea->isHidden()) {
            return QString();
        }
        return pEditor->mpSystemMessageArea->notificationAreaMessageBox->text();
    }

    static int lineHolding(const TBuffer& buffer, const QString& text)
    {
        for (int i = 0; i < buffer.lineBuffer.size(); ++i) {
            if (buffer.lineBuffer.at(i).contains(text)) {
                return i;
            }
        }
        return -1;
    }

    static int countEditorsFor(const Host* pHost)
    {
        int count = 0;
        for (QWidget* pWidget : QApplication::topLevelWidgets()) {
            if (auto* pEditor = qobject_cast<dlgTriggerEditor*>(pWidget); pEditor && pEditor->mpHost == pHost) {
                ++count;
            }
        }
        return count;
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
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            QStringList paths;
            for (const QString& profileName : std::as_const(mProfilesMade)) {
                paths.append(MudletApp::getMudletPath(enums::profileHomePath, profileName));
            }
            delete mudlet::self();
            for (const QString& path : std::as_const(paths)) {
                QDir(path).removeRecursively();
            }
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Declared first: the completions are registered once a process, by whichever comes first
    void test_thePreferencesPreviewCompletesLuaBeforeAnyEditorIsBuilt()
    {
        QVERIFY2(!dlgTriggerEditor::smAutoCompleteInitialized, "setup: an editor was built before this test");
        const QString profileName = qsl("LazyEditor-Preview");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");
        mudlet::self()->activateProfile(pHost);
        QTest::qWait(50ms);

        mudlet::self()->slot_showPreferencesDialog();
        QTest::qWait(100ms);
        dlgProfilePreferences* pPreferences = HostDialogs::of(pHost).mpDlgProfilePreferences.data();
        QVERIFY2(pPreferences, "the preferences did not open");
        QVERIFY2(!HostDialogs::of(pHost).mpEditorDialog, "opening the preferences built the editor");
        edbee::TextDocument* pDocument = pPreferences->edbeePreviewWidget->textDocument();
        QStringList labels;
        for (const edbee::TextAutoCompleteItem* pItem : pDocument->autoCompleteProviderList()->findAutoCompleteItemsForRange(pDocument, edbee::TextRange(), qsl("sendAl"))) {
            labels << pItem->label();
        }
        QVERIFY2(labels.contains(qsl("sendAll")), qPrintable(labels.join(qsl(", "))));

        pPreferences->close();
        QTest::qWait(50ms);
        closeProfile(pHost, profileName);
    }

    // The editor-facing Lua API builds no editor, and the items it made are in
    // the trees when one opens
    void test_aProfileBuildsNoEditorUntilOneIsOpened()
    {
        const QString profileName = qsl("LazyEditor-Load");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");
        QVERIFY2(!HostDialogs::of(pHost).mpEditorDialog, "loading the profile built its editor");

        const QString error = runLua(pHost,
                                     qsl("permRegexTrigger('qaLazyTrigger', '', {'qaLazyPattern'}, [[echo('x')]]) "
                                         "disableTrigger('qaLazyTrigger') enableTrigger('qaLazyTrigger') "
                                         "permAlias('qaLazyAlias', '', '^qaLazy$', [[echo('x')]]) disableAlias('qaLazyAlias') "
                                         "permTimer('qaLazyTimer', '', 100, [[echo('x')]]) disableTimer('qaLazyTimer') "
                                         "permScript('qaLazyScript', '', [[local x = 1]]) setScript('qaLazyScript', [[local x = 2]]) "
                                         "permKey('qaLazyKey', '', mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.F12, [[echo('x')]]) "
                                         "disableKey('qaLazyKey') "
                                         "debugc('qaLazyDebug') "
                                         "setProfileStyleSheet('') "));
        QVERIFY2(error.isNull(), qPrintable(error));
        QTest::qWait(50ms);
        QVERIFY2(!HostDialogs::of(pHost).mpEditorDialog, "the Lua API built the editor");
        QCOMPARE(countEditorsFor(pHost), 0);

        dlgTriggerEditor* pEditor = openEditor(pHost);
        QVERIFY2(pEditor && pEditor->isVisible(), "the Script Editor button did not open an editor");
        QCOMPARE(countEditorsFor(pHost), 1);
        const auto matchFlags = Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive;
        QVERIFY(!pEditor->treeWidget_triggers->findItems(qsl("qaLazyTrigger"), matchFlags, 0).isEmpty());
        QVERIFY(!pEditor->treeWidget_aliases->findItems(qsl("qaLazyAlias"), matchFlags, 0).isEmpty());
        QVERIFY(!pEditor->treeWidget_timers->findItems(qsl("qaLazyTimer"), matchFlags, 0).isEmpty());
        QVERIFY(!pEditor->treeWidget_scripts->findItems(qsl("qaLazyScript"), matchFlags, 0).isEmpty());
        QVERIFY(!pEditor->treeWidget_keys->findItems(qsl("qaLazyKey"), matchFlags, 0).isEmpty());
        QVERIFY2(pEditor->mCurrentView == EditorViewType::cmTriggerView, "the editor did not open on the triggers view");

        closeProfile(pHost, profileName);
    }

    // Stamped when they were raised, and drawn scrolled to the newest without waiting for a line raised after the editor opened
    void test_errorsRaisedBeforeTheFirstOpenAreInTheErrorConsole()
    {
        const QString profileName = qsl("LazyEditor-Errors");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");

        const QString error = runLua(pHost,
                                     qsl("for i = 1, 100 do debugc('qaFiller' .. i) end "
                                         "debugc('qaDebugBeforeOpen') "
                                         "registerAnonymousEventHandler('qaLazyEvent', function() error('qaHandlerErrorBeforeOpen') end) "
                                         "raiseEvent('qaLazyEvent') "
                                         "tempTimer(0, [[error('qaTimerErrorBeforeOpen')]])"));
        QVERIFY2(error.isNull(), qPrintable(error));
        QTest::qWait(200ms);
        const qint64 beforeOpen = QDateTime::currentMSecsSinceEpoch();
        QTest::qWait(50ms);

        mudlet::self()->activateProfile(pHost);
        QTest::qWait(50ms);
        mudlet::self()->dactionShowErrors->trigger();
        QTest::qWait(200ms);
        dlgTriggerEditor* pEditor = HostDialogs::of(pHost).mpEditorDialog.data();
        QVERIFY2(pEditor && pEditor->mpErrorConsole && pEditor->mpErrorConsole->isVisible(), "the Errors menu item did not open the error console");

        QVERIFY2(pEditor->mpErrorConsole->showTimeStamps(), "the error console does not show its time stamps");
        QCOMPARE(pEditor->mpErrorConsole->model().mConsoleName, qsl("errors_%1").arg(profileName));
        const TBuffer& buffer = pEditor->mpErrorConsole->buffer;
        const int header = lineHolding(buffer, dlgTriggerEditor::tr("*** starting new session ***"));
        const int debugLine = lineHolding(buffer, qsl("qaDebugBeforeOpen"));
        const int handlerLine = lineHolding(buffer, qsl("qaHandlerErrorBeforeOpen"));
        const int timerLine = lineHolding(buffer, qsl("qaTimerErrorBeforeOpen"));
        const QString lines = buffer.lineBuffer.join(QChar::LineFeed);
        QVERIFY2(header == 0, qPrintable(lines));
        QVERIFY2(debugLine > header, qPrintable(lines));
        QVERIFY2(handlerLine > debugLine, qPrintable(lines));
        QVERIFY2(timerLine > handlerLine, qPrintable(lines));
        QCOMPARE(lines.count(qsl("qaHandlerErrorBeforeOpen")), 1);
        QVERIFY2(buffer.timeBuffer.at(debugLine) < beforeOpen, "a line raised before the editor opened is stamped with a later time");
        QVERIFY2(buffer.timeBuffer.at(timerLine) < beforeOpen, "a line raised before the editor opened is stamped with a later time");

        // The red of the error lines
        TTextEdit* pPane = pEditor->mpErrorConsole->mUpperPane;
        const QImage image = pPane->grab().toImage();
        int red = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor colour = image.pixelColor(x, y);
                if (colour.red() > 180 && colour.green() < 90 && colour.blue() < 90) {
                    ++red;
                }
            }
        }
        QVERIFY2(red > 0, "the error console does not draw the errors raised before it was opened");
        QVERIFY2(buffer.lineBuffer.size() > pPane->getScreenHeight(), "setup: the lines fit on one screen, so where it is scrolled to cannot be told");
        QVERIFY2(pPane->imageTopLine() + pPane->getScreenHeight() >= timerLine, "the error console opened on its oldest lines, not its newest");

        QVERIFY(runLua(pHost, qsl("debugc('qaDebugAfterOpen')")).isNull());
        QVERIFY2(lineHolding(buffer, qsl("qaDebugAfterOpen")) > timerLine, qPrintable(buffer.lineBuffer.join(QChar::LineFeed)));
        QCOMPARE(buffer.lineBuffer.join(QChar::LineFeed).count(qsl("qaDebugAfterOpen")), 1);

        closeProfile(pHost, profileName);
    }

    // A saved script's sysLoadEvent handler fails before anything can open the editor (#7317)
    void test_anErrorRaisedAsTheProfileLoadsIsInTheErrorConsole()
    {
        const QString profileName = qsl("LazyEditor-LoadError");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");
        const QString error = runLua(pHost,
                                     qsl("permScript('qaLoadScript', '', [[registerAnonymousEventHandler('sysLoadEvent', function() error('qaLoadError') end)]]) "
                                         "enableScript('qaLoadScript')"));
        QVERIFY2(error.isNull(), qPrintable(error));
        QSignalSpy finished(pHost, &Host::profileSaveFinished);
        const auto [saved, file, saveError] = pHost->saveProfile();
        QVERIFY2(saved, qPrintable(saveError));
        QVERIFY2(finished.count() || finished.wait(5s), "the save never finished");
        closeProfile(pHost, profileName);

        mudlet::self()->doAutoLogin(profileName, true);
        pHost = HostManager::self()->getHost(profileName);
        QVERIFY2(pHost, "the saved profile did not load again");
        QTest::qWait(50ms);
        QVERIFY2(!HostDialogs::of(pHost).mpEditorDialog, "loading the profile built its editor");

        mudlet::self()->activateProfile(pHost);
        QTest::qWait(50ms);
        mudlet::self()->dactionShowErrors->trigger();
        QTest::qWait(100ms);
        dlgTriggerEditor* pEditor = HostDialogs::of(pHost).mpEditorDialog.data();
        QVERIFY2(pEditor && pEditor->mpErrorConsole, "the Errors menu item did not open the error console");
        const TBuffer& buffer = pEditor->mpErrorConsole->buffer;
        const QString lines = buffer.lineBuffer.join(QChar::LineFeed);
        QVERIFY2(lineHolding(buffer, dlgTriggerEditor::tr("*** starting new session ***")) == 0, qPrintable(lines));
        QVERIFY2(lineHolding(buffer, qsl("qaLoadError")) > 0, qPrintable(lines));

        closeProfile(pHost, profileName);
    }

    void test_aBindingWarningRaisedBeforeTheFirstOpenShowsWhenItOpens()
    {
        const QString profileName = qsl("LazyEditor-Binding");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");

        // Ctrl+Alt+T is Mudlet's own Toggle Time Stamps on every platform
        QVERIFY(runLua(pHost, qsl("tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.T, [[echo('x')]])")).isNull());
        QTest::qWait(50ms);
        QVERIFY2(!HostDialogs::of(pHost).mpEditorDialog, "warning about the binding built the editor");

        dlgTriggerEditor* pEditor = openEditor(pHost);
        QVERIFY2(pEditor && pEditor->isVisible(), "the Script Editor button did not open an editor");
        const QString text = bannerText(pEditor);
        QVERIFY2(text.contains(qsl("Toggle Time Stamps")), qPrintable(qsl("the binding warning raised before the editor opened is not shown: \"%1\"").arg(text)));
        QVERIFY2(!pEditor->mpSystemMessageArea->notificationAreaIconLabelWarning->isHidden(), "the notice lost its warning icon");

        closeProfile(pHost, profileName);
    }

    // A reset, as a package install asks for, clears a held warning as it does a built editor's notices
    void test_aBindingWarningAResetWouldHaveClearedIsNotShown()
    {
        const QString profileName = qsl("LazyEditor-Reset");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");

        QVERIFY(runLua(pHost, qsl("tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.T, [[echo('x')]])")).isNull());
        emit pHost->signal_editorCleanResetRequested();
        // Before the reset has run, so it goes too
        QVERIFY(runLua(pHost, qsl("tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.L, [[echo('x')]])")).isNull());
        QTest::qWait(50ms);

        dlgTriggerEditor* pEditor = openEditor(pHost);
        QVERIFY2(pEditor && pEditor->isVisible(), "the Script Editor button did not open an editor");
        const QString text = bannerText(pEditor);
        QVERIFY2(!text.contains(qsl("Toggle Time Stamps")) && !text.contains(qsl("Toggle Logging")), qPrintable(qsl("a warning the reset cleared is shown: \"%1\"").arg(text)));

        closeProfile(pHost, profileName);
    }

    // Opened between the reset being asked for and it running, which then clears the notice
    void test_aBindingWarningIsClearedByAResetStillQueuedWhenTheEditorOpens()
    {
        const QString profileName = qsl("LazyEditor-QueuedReset");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");
        mudlet::self()->activateProfile(pHost);
        QTest::qWait(50ms);

        QVERIFY(runLua(pHost, qsl("tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.T, [[echo('x')]])")).isNull());
        emit pHost->signal_editorCleanResetRequested();
        mudlet::self()->slot_showEditorDialog();
        dlgTriggerEditor* pEditor = HostDialogs::of(pHost).mpEditorDialog.data();
        QVERIFY2(pEditor && pEditor->isVisible(), "the Script Editor button did not open an editor");
        const QString shownAtOpen = bannerText(pEditor);
        QTest::qWait(100ms);
        const QString text = bannerText(pEditor);
        QVERIFY2(shownAtOpen.contains(qsl("Toggle Time Stamps")), qPrintable(qsl("the warning was not shown at all: \"%1\"").arg(shownAtOpen)));
        QVERIFY2(!text.contains(qsl("Toggle Time Stamps")), qPrintable(qsl("the queued reset did not clear the warning: \"%1\"").arg(text)));

        closeProfile(pHost, profileName);
    }

    void test_anotherProfilesCommandTakingABindingIsWarnedAboutWhenTheEditorOpens()
    {
        const QString takerName = qsl("LazyEditor-Taker");
        const QString loserName = qsl("LazyEditor-Loser");
        Host* pTaker = openProfile(takerName);
        QVERIFY2(pTaker, "the first profile did not load and connect");
        Host* pLoser = openProfile(loserName);
        QVERIFY2(pLoser, "the second profile did not load and connect");

        QVERIFY(runLua(pLoser, qsl("tempKey(mudlet.keymodifier.Alt, mudlet.key.F9, [[echo('x')]])")).isNull());
        QVERIFY(runLua(pTaker, qsl("_qaCommand = addCommand{name = 'qaLazyClash', menuPath = 'LazyTest', shortcut = 'Alt+F9'} setCommandPinned(_qaCommand, true)")).isNull());
        QTest::qWait(50ms);
        QVERIFY2(!HostDialogs::of(pLoser).mpEditorDialog, "warning about the clash built the editor");

        dlgTriggerEditor* pEditor = openEditor(pLoser);
        QVERIFY2(pEditor && pEditor->isVisible(), "the Script Editor button did not open an editor");
        const QString text = bannerText(pEditor);
        const QString sequence = QKeySequence(QKeyCombination(Qt::AltModifier, Qt::Key_F9)).toString(QKeySequence::NativeText);
        QVERIFY2(text.contains(sequence) && text.contains(qsl("qaLazyClash")) && text.contains(takerName),
                 qPrintable(qsl("the other profile's command taking the key was not warned about: \"%1\"").arg(text)));

        runLua(pTaker, qsl("removeCommand(_qaCommand)"));
        closeProfile(pLoser, loserName);
        closeProfile(pTaker, takerName);
    }

    void test_aProfileStyleSheetSetBeforeTheFirstOpenIsApplied()
    {
        const QString profileName = qsl("LazyEditor-StyleSheet");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");

        const QString styleSheet = qsl("QToolTip { border: 1px solid rgb(1, 2, 3); }");
        QVERIFY(runLua(pHost, qsl("setProfileStyleSheet([[%1]])").arg(styleSheet)).isNull());
        QTest::qWait(50ms);

        dlgTriggerEditor* pEditor = openEditor(pHost);
        QVERIFY2(pEditor && pEditor->isVisible(), "the Script Editor button did not open an editor");
        QCOMPARE(pEditor->styleSheet(), styleSheet);

        closeProfile(pHost, profileName);
    }

    // A detached window opens the editor by a route of its own
    void test_aDetachedWindowOpensTheEditorWithWhatWasHeldForIt()
    {
        const QString stayingName = qsl("LazyEditor-Staying");
        const QString detachedName = qsl("LazyEditor-Detached");
        Host* pStaying = openProfile(stayingName);
        QVERIFY2(pStaying, "the first profile did not load and connect");
        Host* pDetached = openProfile(detachedName);
        QVERIFY2(pDetached, "the second profile did not load and connect");
        QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier test");

        const QString styleSheet = qsl("QToolTip { border: 1px solid rgb(4, 5, 6); }");
        QVERIFY(runLua(pDetached,
                       qsl("permRegexTrigger('qaDetachedTrigger', '', {'qaDetachedPattern'}, [[echo('x')]]) "
                           "debugc('qaDetachedDebug') "
                           "tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.T, [[echo('x')]]) "
                           "setProfileStyleSheet([[%1]])")
                               .arg(styleSheet))
                        .isNull());
        QTest::qWait(50ms);

        TTabBar* pTabBar = mudlet::self()->mpTabBar;
        int tabIndex = -1;
        for (int i = 0, total = pTabBar->count(); i < total; ++i) {
            if (pTabBar->tabData(i).toString() == detachedName) {
                tabIndex = i;
            }
        }
        QVERIFY2(tabIndex >= 0 && pTabBar->count() >= 2, "setup: the profile is not on a tab that can be detached");
        mudlet::self()->slot_tabDetachRequested(tabIndex, QPoint(200, 200));
        TDetachedWindow* pWindow = mudlet::self()->getDetachedWindows().value(detachedName);
        QVERIFY2(pWindow, "detaching the tab made no window");
        QVERIFY2(!HostDialogs::of(pDetached).mpEditorDialog, "detaching the profile built its editor");

        QVERIFY(QMetaObject::invokeMethod(pWindow, "slot_showTriggerDialog", Qt::DirectConnection));
        QTest::qWait(100ms);
        dlgTriggerEditor* pEditor = HostDialogs::of(pDetached).mpEditorDialog.data();
        QVERIFY2(pEditor && pEditor->isVisible(), "the detached window's Triggers button did not open an editor");
        QVERIFY2(lineHolding(pEditor->mpErrorConsole->buffer, qsl("qaDetachedDebug")) > 0, qPrintable(pEditor->mpErrorConsole->buffer.lineBuffer.join(QChar::LineFeed)));
        QCOMPARE(pEditor->styleSheet(), styleSheet);
        const auto matchFlags = Qt::MatchCaseSensitive | Qt::MatchFixedString | Qt::MatchRecursive;
        QVERIFY(!pEditor->treeWidget_triggers->findItems(qsl("qaDetachedTrigger"), matchFlags, 0).isEmpty());
        const QString text = bannerText(pEditor);
        QVERIFY2(text.contains(qsl("Toggle Time Stamps")), qPrintable(qsl("the binding warning raised before the editor opened is not shown: \"%1\"").arg(text)));

        mudlet::self()->slot_tabReattachRequested(detachedName);
        QTest::qWait(50ms);
        closeProfile(pDetached, detachedName);
        closeProfile(pStaying, stayingName);
    }

    void test_anEditorOpenedDuringAProfileSaveCannotStartAnother()
    {
        const QString profileName = qsl("LazyEditor-Saving");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");
        mudlet::self()->activateProfile(pHost);
        QTest::qWait(50ms);

        QSignalSpy finished(pHost, &Host::profileSaveFinished);
        const auto [saved, file, saveError] = pHost->saveProfile();
        QVERIFY2(saved, qPrintable(saveError));
        QVERIFY2(pHost->currentlySavingProfile(), "setup: the save was over before the editor could be opened during it");
        mudlet::self()->slot_showEditorDialog();
        dlgTriggerEditor* pEditor = HostDialogs::of(pHost).mpEditorDialog.data();
        QVERIFY2(pEditor, "the Script Editor button did not open an editor");
        QVERIFY2(!pEditor->mProfileSaveAction->isEnabled(), "an editor opened during a save offers to save");
        QVERIFY2(finished.count() || finished.wait(5s), "the save never finished");
        QVERIFY2(pEditor->mProfileSaveAction->isEnabled(), "the editor still cannot save once the save is done");

        closeProfile(pHost, profileName);
    }

    // With errors and warnings held for it, and a script printing to the error console as the profile goes
    void test_aProfileClosedWithoutOpeningTheEditorNeverBuildsOne()
    {
        const QString profileName = qsl("LazyEditor-Close");
        Host* pHost = openProfile(profileName);
        QVERIFY2(pHost, "the profile did not load and connect");

        QVERIFY(runLua(pHost,
                       qsl("debugc('qaHeld') "
                           "tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.T, [[echo('x')]]) "
                           "registerAnonymousEventHandler('sysDisconnectionEvent', function() debugc('qaDisconnecting') error('qaDisconnectError') end) "
                           "registerAnonymousEventHandler('sysExitEvent', function() debugc('qaExiting') error('qaExitError') end)"))
                        .isNull());
        emit pHost->signal_editorCleanResetRequested();
        QPointer<Host> pWatched = pHost;
        // Sees an editor however briefly it lives, as one built and deleted in the teardown would
        bool built = false;
        EventWatcher watcher([pHost, &built](QObject* pObject) {
            if (auto* pEditor = qobject_cast<dlgTriggerEditor*>(pObject); pEditor && pEditor->mpHost.data() == pHost) {
                built = true;
            }
        });
        closeProfile(pHost, profileName);
        QTest::qWait(100ms);
        QVERIFY2(!pWatched, "the profile is still there");
        QVERIFY2(!built, "closing the profile built its editor");
    }
};

#include "EditorLazyCreationTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorLazyCreationTest)
