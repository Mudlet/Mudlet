/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * The editor's undo stack keeps each item it may have to restore as an XML
 * snapshot: exportXToXML() takes one, importXFromXML() recreates a deleted item
 * from it and updateXFromXML() puts an edited item back the way it was. None of
 * that is reachable from Lua, and dlgTriggerEditorUndoRedoTest only reaches it
 * for the item kinds and properties its editor-driven cases happen to edit.
 *
 * Each case here restores an item from a snapshot - as a new item, or over an
 * item edited away from it - and checks that what the snapshot held came back.
 *
 * Run with: ctest -R EditorItemXMLHelpersTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ActionUnit.h"
#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
// The snapshots are compressed, so a failure shows the XML they hold instead.
QString readable(const QString& snapshot)
{
    return QString::fromUtf8(qUncompress(QByteArray::fromBase64(snapshot.toLatin1())));
}
} // namespace

class EditorItemXMLHelpersTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mHostName = qsl("EditorItemXMLHelpers-Test");

    TTrigger* newTrigger(const QString& name, TTrigger* parent = nullptr)
    {
        auto* trigger = new TTrigger(parent, mpHost);
        trigger->setName(name);
        mpHost->getTriggerUnit()->registerTrigger(trigger);
        return trigger;
    }

    TScript* newScript(const QString& name, TScript* parent = nullptr)
    {
        auto* script = new TScript(parent, mpHost);
        script->setName(name);
        mpHost->getScriptUnit()->registerScript(script);
        return script;
    }

    TAction* newAction(const QString& name, TAction* parent = nullptr)
    {
        auto* action = new TAction(parent, mpHost);
        action->setName(name);
        mpHost->getActionUnit()->registerAction(action);
        return action;
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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();

        // A bare Host is all the helpers ask for: the units and the Lua
        // interpreter the restored items compile into.
        QVERIFY(HostManager::self()->addHost(mHostName, qsl("23"), QString(), QString()));
        mpHost = HostManager::self()->getHost(mHostName);
        QVERIFY(mpHost);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostName)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A snapshot is written by XMLexport, which numbers the sixteen basic colours
    // of a colour pattern the way a save file does. Read back without turning
    // them into ANSI numbers again, a deleted colour trigger came back matching
    // some other colour, or none.
    void test_colourTriggerIsRecreatedMatchingTheSameColours()
    {
        TTrigger* trigger = newTrigger(qsl("eixh colour trigger"));
        trigger->mColorTrigger = true;
        // red on blue, which a save file calls 4 and 10
        QVERIFY(trigger->setRegexCodeList({TTrigger::createColorPatternText(1, 4)}, {REGEX_COLOR_PATTERN}));
        const QString snapshot = exportTriggerToXML(trigger);
        QVERIFY2(readable(snapshot).contains(qsl("<string>FG4BG10</string>")), qPrintable(readable(snapshot)));

        TTrigger* restored = importTriggerFromXML(snapshot, nullptr, mpHost);

        QVERIFY(restored);
        QCOMPARE(restored->getPatternsList(), trigger->getPatternsList());
        QVERIFY2(exportTriggerToXML(restored) == snapshot, qPrintable(readable(exportTriggerToXML(restored)) + qsl("\n---- expected ----\n") + readable(snapshot)));
    }

    void test_colourTriggerIsRestoredOverAnEditMatchingTheSameColours()
    {
        TTrigger* trigger = newTrigger(qsl("eixh colour trigger edited"));
        trigger->mColorTrigger = true;
        QVERIFY(trigger->setRegexCodeList({TTrigger::createColorPatternText(1, 4)}, {REGEX_COLOR_PATTERN}));
        const QStringList patternsBefore = trigger->getPatternsList();
        const QString before = exportTriggerToXML(trigger);
        QVERIFY(trigger->setRegexCodeList({TTrigger::createColorPatternText(2, 3)}, {REGEX_COLOR_PATTERN}));

        QVERIFY(updateTriggerFromXML(trigger, before));

        QCOMPARE(trigger->getPatternsList(), patternsBefore);
    }

    // An undo puts back what the snapshot holds, and a snapshot of a trigger
    // with no patterns yet holds none - which has to take away the patterns the
    // edit being undone gave it, rather than leave them in place.
    void test_patternsAddedByAnEditAreTakenAwayByTheRestore()
    {
        TTrigger* trigger = newTrigger(qsl("eixh trigger gaining patterns"));
        const QString before = exportTriggerToXML(trigger);
        QVERIFY(trigger->setRegexCodeList({qsl("eixh gained pattern")}, {REGEX_SUBSTRING}));

        QVERIFY(updateTriggerFromXML(trigger, before));

        QVERIFY2(trigger->getPatternsList().isEmpty(), qPrintable(trigger->getPatternsList().join(qsl(", "))));
    }

    // The same for a script given its first event handler: undoing that has to
    // stop the script being called for the event.
    void test_eventHandlersAddedByAnEditAreTakenAwayByTheRestore()
    {
        TScript* script = newScript(qsl("eixh script gaining a handler"));
        const QString before = exportScriptToXML(script);
        script->setEventHandlerList({qsl("eixhGainedEvent")});
        QVERIFY(mpHost->mEventHandlerMap.value(qsl("eixhGainedEvent")).contains(script));

        QVERIFY(updateScriptFromXML(script, before));

        QVERIFY2(script->getEventHandlerList().isEmpty(), qPrintable(script->getEventHandlerList().join(qsl(", "))));
        QVERIFY(!mpHost->mEventHandlerMap.value(qsl("eixhGainedEvent")).contains(script));
    }

    // A deleted push-down button comes back up or down, the way it was.
    void test_pushDownButtonIsRecreatedInTheStateItWasIn()
    {
        TAction* action = newAction(qsl("eixh push-down button"));
        action->setIsPushDownButton(true);
        action->mButtonState = true;
        const QString down = exportActionToXML(action);

        TAction* restored = importActionFromXML(down, nullptr, mpHost);

        QVERIFY(restored);
        QVERIFY(restored->mButtonState);
    }

    // Whether a push-down button is down is the player's doing, not the
    // editor's: no edit changes it, so undoing one must not put it back to how
    // it was when the edit was made.
    void test_undoingAnEditLeavesAPushDownButtonAsItIs()
    {
        TAction* action = newAction(qsl("eixh clicked push-down button"));
        action->setIsPushDownButton(true);
        action->mButtonState = false;
        const QString before = exportActionToXML(action);
        action->setName(qsl("eixh clicked push-down button renamed"));
        action->mButtonState = true;

        QVERIFY(updateActionFromXML(action, before));

        QCOMPARE(action->getName(), qsl("eixh clicked push-down button"));
        QVERIFY(action->mButtonState);
    }
};

#include "EditorItemXMLHelpersTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorItemXMLHelpersTest)
