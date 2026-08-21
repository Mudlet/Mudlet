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

// The three OSC 8 hyperlink managers were fields of the TConsole widget, and
// THyperlinkVisibilityManager reached back through it fourteen times: twice for
// the text buffer it conceals and reveals in, and twelve times to repaint the
// two panes. They are now fields of the core TConsoleModel, which Host co-owns
// for the main console, and the repaint is the view's alone - the manager emits
// visibilityChanged() and TConsole::slot_hyperlinkVisibilityChanged() forces
// both panes to redraw (#8681).
//
// What these tests hold down is the half that moved. That the repaint still
// happens is FrontendRefreshSeamTest's
// test_aHyperlinkVisibilityChangeForcesBothPanesToRedraw, and is deliberately
// not repeated here.
//
// Two of the cases run against a TConsoleModel that never had a view at all,
// which is the state Host builds the main console's model in and the state a
// headless profile would stay in. The third destroys a real profile's view and
// leaves the model running, which is what a closing profile does for real.

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>
#include <memory>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "TConsoleModel.h"
#include "THyperlinkCompactManager.h"
#include "THyperlinkSelectionManager.h"
#include "THyperlinkStyling.h"
#include "THyperlinkVisibilityManager.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class HyperlinkModelSplitTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-HyperlinkModelSplit");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

private slots:
    void initTestCase()
    {
        // Saved before the skip below, because cleanupTestCase() still runs
        // after a skipped initTestCase() and would otherwise clear a variable
        // this test never set.
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
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
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // An ephemeral OS-assigned port, so parallel runs across worktrees
        // cannot collide on a shared fixed one.
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        // QtTest does not run cleanup() when init() fails, so a bare QCOMPARE
        // here would strand the singleton.
        if (mudlet::getMudletPath(enums::mainPath) != qsl("%1/mudlet").arg(mConfigDir.path())) {
            delete mudlet::self();
            delete mpServer;
            mpServer = nullptr;
            QFAIL("the config root was not redirected, so this would run against the developer's own profile list");
        }
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            // Mudlet writes the profile out as it shuts down, so removing the
            // directory first only has it recreated. The path has to be read
            // while the singleton is alive though - getMudletPath() reaches
            // through self() without checking it.
            const QString profileDir = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(profileDir).removeRecursively();
        }
    }

    // One set of managers per model, not one per widget. A rebase that left the
    // widget building its own would compile and run, and every OSC 8 sequence
    // would go on working - right up until the model was asked what it holds.
    void test_theHyperlinkManagersAreTheModelsNotTheViews()
    {
        Host* host = startProfile();
        QVERIFY(host);
        TMainConsole* console = host->mpConsole;
        QVERIFY(console);

        TConsoleModel& model = host->mainConsoleModel();
        QCOMPARE(&console->getHyperlinkCompactManager(), &model.mHyperlinkCompactManager);
        QCOMPARE(&console->getHyperlinkSelectionManager(), &model.mHyperlinkSelectionManager);
        QCOMPARE(&console->getHyperlinkVisibilityManager(), &model.mHyperlinkVisibilityManager);

        // Written through the model, read back through the view: the same
        // object, not two that happen to agree.
        model.mHyperlinkSelectionManager.setSelected(qsl("splitgroup"), qsl("splitvalue"), true);
        QVERIFY2(console->getHyperlinkSelectionManager().isSelected(qsl("splitgroup"), qsl("splitvalue")),
                 "the view and the model are answering from separate selection managers");

        // A user window carries a model of its own, so its managers have to be
        // its own too - Host's belong to the main console alone.
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('hyperlinkSplitWindow')\n")), "openUserWindow() did not run");
        TConsole* subConsole = console->mSubConsoleMap.value(qsl("hyperlinkSplitWindow"));
        QVERIFY2(subConsole, "the user window was not created");
        QVERIFY2(&subConsole->getHyperlinkVisibilityManager() != &model.mHyperlinkVisibilityManager, "a user window must not share the main console's tracked hyperlinks");
        QVERIFY2(!subConsole->getHyperlinkSelectionManager().isSelected(qsl("splitgroup"), qsl("splitvalue")), "a user window must not share the main console's selection state");
    }

    // Concealing rewrites the buffer, and the buffer is the model's. This drives
    // it against a model that never had a widget of any kind, which is what the
    // manager used to refuse to do: performConcealment() and performReveal()
    // both opened with a null check on the console and returned without touching
    // anything.
    void test_aModelWithNoViewConcealsAndRevealsItsOwnBuffer()
    {
        Host* host = startProfile();
        QVERIFY(host);

        // Not the main console's model, which has a view attached: a model of
        // this test's own, built the way Host builds one and never handed to a
        // widget.
        auto model = std::make_shared<TConsoleModel>(host);

        const QString text = qsl("MODELONLY(HIDDENWORD)MODELONLY");
        const QString line = text + QChar::LineFeed;
        model->buffer.append(line, 0, line.size(), QColorConstants::LightGray, QColorConstants::Black, TChar::None, 0);
        const int lineNumber = model->buffer.getLastLineNumber() - 1;
        QCOMPARE(model->buffer.lineBuffer.at(lineNumber), text);

        // A conceal-on-click link: registered visible, so registerHyperlink()
        // answers that the text must not be blanked out yet.
        Mudlet::HyperlinkStyling styling;
        styling.visibility.hasVisibilitySettings = true;
        styling.visibility.action = Mudlet::HyperlinkStyling::VisibilitySettings::Action::Conceal;

        const int linkId = 4242;
        const int startColumn = text.indexOf(qsl("HIDDENWORD"));
        const int length = QStringLiteral("HIDDENWORD").size();
        QVERIFY(startColumn > 0);
        QVERIFY2(!model->mHyperlinkVisibilityManager.registerHyperlink(linkId, lineNumber, startColumn, length, qsl("HIDDENWORD"), styling),
                 "a conceal link starts visible, so it must not ask for its text to be replaced");

        model->mHyperlinkVisibilityManager.concealLink(linkId);
        QVERIFY2(model->mHyperlinkVisibilityManager.isLinkConcealed(linkId), "the view-less model did not record the link as concealed");
        // Concealment keeps the character count identical so buffer indices stay
        // valid, which is why the text is replaced space for space.
        QCOMPARE(model->buffer.lineBuffer.at(lineNumber), qsl("MODELONLY(          )MODELONLY"));

        model->mHyperlinkVisibilityManager.revealLink(linkId);
        QVERIFY2(!model->mHyperlinkVisibilityManager.isLinkConcealed(linkId), "the view-less model did not record the link as revealed");
        QCOMPARE(model->buffer.lineBuffer.at(lineNumber), text);
    }

    // The shorthand and preset-property tables the compact syntax is parsed
    // against used to be filled in by eight TConsole::initializeOSC8*Feature()
    // calls from the widget's constructor, so a model with no view held none of
    // them and "s=..." stayed "s" instead of becoming "style".
    void test_aModelWithNoViewKnowsTheCompactShorthands()
    {
        Host* host = startProfile();
        QVERIFY(host);

        auto model = std::make_shared<TConsoleModel>(host);
        THyperlinkCompactManager& compact = model->mHyperlinkCompactManager;

        QMap<QString, QString> compactParams;
        compactParams.insert(qsl("s"), qsl("color:red"));
        compactParams.insert(qsl("v"), qsl("conceal"));
        compactParams.insert(qsl("notashorthand"), qsl("left alone"));

        const QMap<QString, QString> expanded = compact.expandShorthand(compactParams);
        QCOMPARE(expanded.value(qsl("style")), qsl("color:red"));
        QCOMPARE(expanded.value(qsl("visibility")), qsl("conceal"));
        QVERIFY2(!expanded.contains(qsl("s")), "the shorthand was left unexpanded");
        QCOMPARE(expanded.value(qsl("notashorthand")), qsl("left alone"));

        QVERIFY2(compact.isPresetProperty(qsl("style")), "a view-less model does not know style can appear in a preset");
        QVERIFY2(compact.isPresetProperty(qsl("selection")), "a view-less model does not know selection can appear in a preset");
        QVERIFY2(!compact.isPresetProperty(qsl("notapresetproperty")), "every property is being treated as preset-aware");
    }

    // The end of it, through the real pipeline: a link registered by feeding an
    // OSC 8 sequence, revealed by the manager's own 100ms timer, with the widget
    // that used to own that timer already destroyed. Host co-owns the model, so
    // the reveal has a buffer to write into and a timer to run on long after the
    // view is gone.
    void test_aDelayedRevealCompletesAfterTheViewIsDestroyed()
    {
        Host* host = startProfile();
        QVERIFY(host);
        host->mEnableOSC8Hyperlinks = true;
        TMainConsole* console = host->mpConsole;
        QVERIFY(console);

        // Long enough that closing the profile below cannot eat the whole delay
        // on a loaded sanitiser runner - the case checks that it has not, rather
        // than assuming.
        //
        // Real escape bytes inside a Lua long-bracket string, rather than Lua's
        // own "\027" escapes inside a C++ raw string literal: moc stops parsing
        // a file at a raw string literal it cannot lex and silently emits no
        // meta-object for whatever follows, which links as an undefined vtable.
        const QString esc = QString(QChar(0x1B));
        const QString stringTerminator = esc + QLatin1Char('\\');
        const QString link = qsl("%1]8;;send:osc8split?config={\"visibility\":{\"action\":\"reveal\",\"delay\":8000}}%2HIDDENWORD%1]8;;%2").arg(esc, stringTerminator);
        const QString feed = qsl("feedTriggers([==[OSCSPLIT1(%1)OSCSPLIT1\n]==])").arg(link);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(feed), "feedTriggers() did not run, so no hyperlink was ever registered");

        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();
        int lineNumber = -1;
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             lineNumber = lineHolding(model->buffer, qsl("OSCSPLIT1"));
                             return lineNumber >= 0;
                         },
                         8000),
                 "the line carrying the link never reached the buffer");
        QCOMPARE(model->buffer.lineBuffer.at(lineNumber), qsl("OSCSPLIT1(          )OSCSPLIT1"));

        destroyTheView(host);
        QCOMPARE(&host->mainConsoleModel(), model.get());
        QVERIFY2(model->buffer.lineBuffer.at(lineNumber) == qsl("OSCSPLIT1(          )OSCSPLIT1"), "the reveal had already run before the view was destroyed, so this case proves nothing");

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return model->buffer.lineBuffer.at(lineNumber) == qsl("OSCSPLIT1(HIDDENWORD)OSCSPLIT1");
                         },
                         30000),
                 "the concealed link never revealed itself once its view had gone");
    }

private:
    // Answers rather than asserting: everything after the call dereferences the
    // host, so a caller has to be able to stop.
    Host* startProfile()
    {
        Host* host = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!host) {
            qWarning("no active host available for the test");
            return nullptr;
        }
        QSignalSpy connected(&host->mTelnet, &cTelnet::signal_connected);
        if (host->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState && !connected.wait(8000)) {
            qWarning("could not connect to the stub");
            return nullptr;
        }
        return host;
    }

    // Destroys the main console widget while Host - and the model it co-owns -
    // live on, which is the window every view-less assertion here needs.
    void destroyTheView(Host* host)
    {
        QPointer<TMainConsole> console = host->mpConsole;
        // Forcing the close stops TMainConsole::closeEvent() asking whether the
        // profile should be saved, which would block on a modal dialog here.
        host->forceClose();
        QVERIFY2(host->requestClose(), "closing the profile was refused");
        QTest::qWait(500ms); // the console carries WA_DeleteOnClose

        QVERIFY2(console.isNull(), "the main console view was not destroyed by closing the profile");
        QVERIFY2(host->mpConsole.isNull(), "the host still points at a main console");
    }

    static int lineHolding(TBuffer& buffer, const QString& needle)
    {
        const int lastLine = buffer.getLastLineNumber();
        for (int i = lastLine; i >= qMax(0, lastLine - 20); --i) {
            if (i < buffer.lineBuffer.size() && buffer.lineBuffer.at(i).contains(needle)) {
                return i;
            }
        }
        return -1;
    }

    void deleteProfileDirectory()
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mHostname));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "HyperlinkModelSplitTest.moc"
MUDLET_GROUPED_TEST_MAIN(HyperlinkModelSplitTest)
