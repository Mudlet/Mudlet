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
 * Each case here takes a snapshot of an item with every exported field off its
 * default, restores it - as a new item, and over an item edited away from it -
 * and asks for a second snapshot, which has to equal the first: anything a
 * restore drops or gets wrong shows up as a difference between the two. A
 * snapshot that cannot be read at all has to be refused without creating or
 * touching anything.
 *
 * Run with: ctest -R EditorItemXMLHelpersTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ActionUnit.h"
#include "AliasUnit.h"
#include "EditorItemXMLHelpers.h"
#include "Host.h"
#include "HostManager.h"
#include "KeyUnit.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
// The snapshots are compressed, so a failure shows the XML they hold instead.
QString readable(const QString& snapshot)
{
    return QString::fromUtf8(qUncompress(QByteArray::fromBase64(snapshot.toLatin1())));
}

// Each of these would be refused for a different reason, and each item kind
// checks for every one of them in code of its own.
const QStringList scmUnreadableSnapshots{
        // not base64 of anything qUncompress() accepts
        qsl("bm90IGEgc25hcHNob3Q="),
        // uncompressed XML is taken as it is, but this is not well formed
        qsl("<TriggerSnapshot><Trigger>"),
        // well formed, but without the snapshot root of any kind
        qsl("<SomethingElse/>"),
};
} // namespace

class EditorItemXMLHelpersTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mHostName = qsl("EditorItemXMLHelpers-Test");

    // A well formed snapshot of the right kind that holds no item.
    static QString emptySnapshot(const QString& kind) { return qsl("<%1Snapshot><Unrelated/></%1Snapshot>").arg(kind); }

    TTrigger* newTrigger(const QString& name, TTrigger* parent = nullptr)
    {
        auto* trigger = new TTrigger(parent, mpHost);
        trigger->setName(name);
        mpHost->getTriggerUnit()->registerTrigger(trigger);
        return trigger;
    }

    // Everything the snapshot carries, set away from what a new trigger has.
    void dressTrigger(TTrigger* trigger, const QString& tag)
    {
        trigger->setScript(qsl("local undone = \"%1 <&>\"").arg(tag));
        trigger->setCommand(qsl("%1 command").arg(tag));
        trigger->setIsMultiline(true);
        trigger->setConditionLineDelta(4);
        trigger->mStayOpen = 3;
        trigger->mPerlSlashGOption = true;
        trigger->setIsColorizerTrigger(true);
        trigger->setColorizerFgColor(QColor(qsl("#112233")));
        trigger->setColorizerBgColor(QColor(qsl("#445566")));
        trigger->mFilterTrigger = true;
        trigger->mSoundTrigger = true;
        trigger->setSound(qsl("/sounds/%1.wav").arg(tag));
        trigger->mPackageName = qsl("%1 package").arg(tag);
        QVERIFY(trigger->setRegexCodeList({qsl("%1 substring").arg(tag), qsl("^%1 (\\d+)$").arg(tag), qsl("%1 start").arg(tag), qsl("%1 exact").arg(tag), qsl("return true"), qsl("2"), QString()},
                                          {REGEX_SUBSTRING, REGEX_PERL, REGEX_BEGIN_OF_LINE_SUBSTRING, REGEX_EXACT_MATCH, REGEX_LUA_CODE, REGEX_LINE_SPACER, REGEX_PROMPT}));
        trigger->setIsActive(false);
    }

    TAlias* newAlias(const QString& name, TAlias* parent = nullptr)
    {
        auto* alias = new TAlias(parent, mpHost);
        alias->setName(name);
        mpHost->getAliasUnit()->registerAlias(alias);
        return alias;
    }

    void dressAlias(TAlias* alias, const QString& tag)
    {
        alias->setRegexCode(qsl("^%1 (\\w+)$").arg(tag));
        alias->setScript(qsl("local undone = \"%1 <&>\"").arg(tag));
        alias->setCommand(qsl("%1 command").arg(tag));
        alias->mPackageName = qsl("%1 package").arg(tag);
        alias->setIsActive(false);
    }

    TTimer* newTimer(const QString& name, TTimer* parent = nullptr)
    {
        auto* timer = new TTimer(parent, mpHost);
        timer->setName(name);
        mpHost->getTimerUnit()->registerTimer(timer);
        return timer;
    }

    void dressTimer(TTimer* timer, const QString& tag)
    {
        timer->setTime(QTime(1, 2, 3, 456));
        timer->setScript(qsl("local undone = \"%1 <&>\"").arg(tag));
        timer->setCommand(qsl("%1 command").arg(tag));
        timer->mPackageName = qsl("%1 package").arg(tag);
    }

    TScript* newScript(const QString& name, TScript* parent = nullptr)
    {
        auto* script = new TScript(parent, mpHost);
        script->setName(name);
        mpHost->getScriptUnit()->registerScript(script);
        return script;
    }

    void dressScript(TScript* script, const QString& tag)
    {
        script->setScript(qsl("local undone = \"%1 <&>\"").arg(tag));
        script->setEventHandlerList({qsl("%1FirstEvent").arg(tag), qsl("%1SecondEvent").arg(tag)});
        script->mPackageName = qsl("%1 package").arg(tag);
        script->setIsActive(false);
    }

    TKey* newKey(const QString& name, TKey* parent = nullptr)
    {
        auto* key = new TKey(parent, mpHost);
        key->setName(name);
        mpHost->getKeyUnit()->registerKey(key);
        return key;
    }

    void dressKey(TKey* key, const QString& tag)
    {
        key->setScript(qsl("local undone = \"%1 <&>\"").arg(tag));
        key->setCommand(qsl("%1 command").arg(tag));
        key->setKeyCode(Qt::Key_F7);
        key->setKeyModifiers(Qt::ControlModifier | Qt::ShiftModifier);
        key->mPackageName = qsl("%1 package").arg(tag);
        key->setIsActive(false);
    }

    TAction* newAction(const QString& name, TAction* parent = nullptr)
    {
        auto* action = new TAction(parent, mpHost);
        action->setName(name);
        mpHost->getActionUnit()->registerAction(action);
        return action;
    }

    void dressAction(TAction* action, const QString& tag)
    {
        action->setScript(qsl("local undone = \"%1 <&>\"").arg(tag));
        action->css = qsl("color: red; /* %1 */").arg(tag);
        action->setCommandButtonUp(qsl("%1 up").arg(tag));
        action->setCommandButtonDown(qsl("%1 down").arg(tag));
        action->setIcon(qsl("/icons/%1.png").arg(tag));
        action->setIsPushDownButton(true);
        action->setButtonFlat(true);
        action->mUseCustomLayout = true;
        action->mOrientation = 1;
        action->mLocation = 4;
        action->mPosX = 12;
        action->mPosY = 34;
        action->setSizeX(56);
        action->setSizeY(78);
        action->setButtonColumns(3);
        action->setButtonFillerOffset(2);
        action->setButtonRotation(2);
        action->mPackageName = qsl("%1 package").arg(tag);
        action->setIsActive(false);
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

    // A deleted group comes back with everything it held, in its place.
    void test_triggerGroupIsRecreatedFromItsSnapshot()
    {
        TTrigger* group = newTrigger(qsl("eixh trigger group"));
        group->setIsFolder(true);
        TTrigger* child = newTrigger(qsl("eixh trigger child"), group);
        dressTrigger(child, qsl("trigger"));
        const QString snapshot = exportTriggerToXML(group);
        QVERIFY(!snapshot.isEmpty());

        TTrigger* restored = importTriggerFromXML(snapshot, nullptr, mpHost, 0);

        QVERIFY(restored);
        QVERIFY(restored != group);
        QCOMPARE(mpHost->getTriggerUnit()->getTriggerRootNodeList().front(), restored);
        QCOMPARE(restored->getChildrenList()->size(), 1);
        QVERIFY2(exportTriggerToXML(restored) == snapshot, qPrintable(readable(exportTriggerToXML(restored)) + qsl("\n---- expected ----\n") + readable(snapshot)));
    }

    void test_triggerIsRestoredOverAnEditFromItsSnapshot()
    {
        TTrigger* trigger = newTrigger(qsl("eixh trigger edited"));
        dressTrigger(trigger, qsl("before"));
        const QString before = exportTriggerToXML(trigger);
        trigger->setName(qsl("eixh trigger renamed"));
        dressTrigger(trigger, qsl("after"));
        trigger->setIsMultiline(false);
        trigger->mFilterTrigger = false;
        trigger->setConditionLineDelta(9);
        trigger->mStayOpen = 1;
        trigger->mPerlSlashGOption = false;
        trigger->setIsColorizerTrigger(false);
        trigger->setColorizerFgColor(QColor(qsl("#abcdef")));
        trigger->setColorizerBgColor(QColor(qsl("#fedcba")));
        trigger->mSoundTrigger = false;
        QVERIFY(exportTriggerToXML(trigger) != before);

        QVERIFY(updateTriggerFromXML(trigger, before));

        QVERIFY2(exportTriggerToXML(trigger) == before, qPrintable(readable(exportTriggerToXML(trigger)) + qsl("\n---- expected ----\n") + readable(before)));
    }

    void test_aliasGroupIsRecreatedFromItsSnapshot()
    {
        TAlias* group = newAlias(qsl("eixh alias group"));
        group->setIsFolder(true);
        TAlias* child = newAlias(qsl("eixh alias child"), group);
        dressAlias(child, qsl("alias"));
        const QString snapshot = exportAliasToXML(group);

        TAlias* restored = importAliasFromXML(snapshot, nullptr, mpHost, 0);

        QVERIFY(restored);
        QCOMPARE(mpHost->getAliasUnit()->getAliasRootNodeList().front(), restored);
        QCOMPARE(restored->getChildrenList()->size(), 1);
        QVERIFY2(exportAliasToXML(restored) == snapshot, qPrintable(readable(exportAliasToXML(restored)) + qsl("\n---- expected ----\n") + readable(snapshot)));
    }

    void test_aliasIsRestoredOverAnEditFromItsSnapshot()
    {
        TAlias* alias = newAlias(qsl("eixh alias edited"));
        dressAlias(alias, qsl("before"));
        const QString before = exportAliasToXML(alias);
        alias->setName(qsl("eixh alias renamed"));
        dressAlias(alias, qsl("after"));
        alias->setIsActive(true);
        QVERIFY(exportAliasToXML(alias) != before);

        QVERIFY(updateAliasFromXML(alias, before));

        QVERIFY2(exportAliasToXML(alias) == before, qPrintable(readable(exportAliasToXML(alias)) + qsl("\n---- expected ----\n") + readable(before)));
    }

    void test_timerGroupIsRecreatedFromItsSnapshot()
    {
        TTimer* group = newTimer(qsl("eixh timer group"));
        group->setIsFolder(true);
        TTimer* child = newTimer(qsl("eixh timer child"), group);
        dressTimer(child, qsl("timer"));
        const QString snapshot = exportTimerToXML(group);

        TTimer* restored = importTimerFromXML(snapshot, nullptr, mpHost, 0);

        QVERIFY(restored);
        QCOMPARE(mpHost->getTimerUnit()->getTimerRootNodeList().front(), restored);
        QCOMPARE(restored->getChildrenList()->size(), 1);
        QVERIFY2(exportTimerToXML(restored) == snapshot, qPrintable(readable(exportTimerToXML(restored)) + qsl("\n---- expected ----\n") + readable(snapshot)));
    }

    void test_timerIsRestoredOverAnEditFromItsSnapshot()
    {
        TTimer* timer = newTimer(qsl("eixh timer edited"));
        dressTimer(timer, qsl("before"));
        const QString before = exportTimerToXML(timer);
        timer->setName(qsl("eixh timer renamed"));
        dressTimer(timer, qsl("after"));
        timer->setTime(QTime(0, 0, 9));
        QVERIFY(exportTimerToXML(timer) != before);

        QVERIFY(updateTimerFromXML(timer, before));

        QVERIFY2(exportTimerToXML(timer) == before, qPrintable(readable(exportTimerToXML(timer)) + qsl("\n---- expected ----\n") + readable(before)));
    }

    void test_scriptGroupIsRecreatedFromItsSnapshot()
    {
        TScript* group = newScript(qsl("eixh script group"));
        group->setIsFolder(true);
        TScript* child = newScript(qsl("eixh script child"), group);
        dressScript(child, qsl("script"));
        const QString snapshot = exportScriptToXML(group);

        TScript* restored = importScriptFromXML(snapshot, nullptr, mpHost, 0);

        QVERIFY(restored);
        QCOMPARE(mpHost->getScriptUnit()->getScriptRootNodeList().front(), restored);
        QCOMPARE(restored->getChildrenList()->size(), 1);
        QVERIFY2(exportScriptToXML(restored) == snapshot, qPrintable(readable(exportScriptToXML(restored)) + qsl("\n---- expected ----\n") + readable(snapshot)));
    }

    void test_scriptIsRestoredOverAnEditFromItsSnapshot()
    {
        TScript* script = newScript(qsl("eixh script edited"));
        dressScript(script, qsl("before"));
        const QString before = exportScriptToXML(script);
        script->setName(qsl("eixh script renamed"));
        dressScript(script, qsl("after"));
        QVERIFY(exportScriptToXML(script) != before);

        QVERIFY(updateScriptFromXML(script, before));

        QVERIFY2(exportScriptToXML(script) == before, qPrintable(readable(exportScriptToXML(script)) + qsl("\n---- expected ----\n") + readable(before)));
    }

    void test_keyGroupIsRecreatedFromItsSnapshot()
    {
        TKey* group = newKey(qsl("eixh key group"));
        group->setIsFolder(true);
        TKey* child = newKey(qsl("eixh key child"), group);
        dressKey(child, qsl("key"));
        const QString snapshot = exportKeyToXML(group);

        TKey* restored = importKeyFromXML(snapshot, nullptr, mpHost, 0);

        QVERIFY(restored);
        QCOMPARE(mpHost->getKeyUnit()->getKeyRootNodeList().front(), restored);
        QCOMPARE(restored->getChildrenList()->size(), 1);
        QVERIFY2(exportKeyToXML(restored) == snapshot, qPrintable(readable(exportKeyToXML(restored)) + qsl("\n---- expected ----\n") + readable(snapshot)));
    }

    void test_keyIsRestoredOverAnEditFromItsSnapshot()
    {
        TKey* key = newKey(qsl("eixh key edited"));
        dressKey(key, qsl("before"));
        const QString before = exportKeyToXML(key);
        key->setName(qsl("eixh key renamed"));
        dressKey(key, qsl("after"));
        key->setKeyCode(Qt::Key_F8);
        key->setKeyModifiers(Qt::AltModifier);
        QVERIFY(exportKeyToXML(key) != before);

        QVERIFY(updateKeyFromXML(key, before));

        QVERIFY2(exportKeyToXML(key) == before, qPrintable(readable(exportKeyToXML(key)) + qsl("\n---- expected ----\n") + readable(before)));
    }

    void test_buttonGroupIsRecreatedFromItsSnapshot()
    {
        TAction* group = newAction(qsl("eixh button group"));
        group->setIsFolder(true);
        TAction* child = newAction(qsl("eixh button child"), group);
        dressAction(child, qsl("button"));
        const QString snapshot = exportActionToXML(group);

        TAction* restored = importActionFromXML(snapshot, nullptr, mpHost, 0);

        QVERIFY(restored);
        QCOMPARE(mpHost->getActionUnit()->getActionRootNodeList().front(), restored);
        QCOMPARE(restored->getChildrenList()->size(), 1);
        QVERIFY2(exportActionToXML(restored) == snapshot, qPrintable(readable(exportActionToXML(restored)) + qsl("\n---- expected ----\n") + readable(snapshot)));
    }

    void test_buttonIsRestoredOverAnEditFromItsSnapshot()
    {
        TAction* action = newAction(qsl("eixh button edited"));
        dressAction(action, qsl("before"));
        const QString before = exportActionToXML(action);
        action->setName(qsl("eixh button renamed"));
        dressAction(action, qsl("after"));
        action->setButtonRotation(1);
        action->setSizeX(1);
        action->setSizeY(2);
        action->setIsPushDownButton(false);
        action->setButtonFlat(false);
        action->mUseCustomLayout = false;
        action->mOrientation = 0;
        action->mLocation = 1;
        action->mPosX = 5;
        action->mPosY = 6;
        action->setButtonColumns(7);
        action->setButtonFillerOffset(0);
        QVERIFY(exportActionToXML(action) != before);

        QVERIFY(updateActionFromXML(action, before));

        QVERIFY2(exportActionToXML(action) == before, qPrintable(readable(exportActionToXML(action)) + qsl("\n---- expected ----\n") + readable(before)));
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

    // A push-down button is saved up or down, and comes back the way it was.
    void test_pushDownButtonIsRestoredInTheStateItWasIn()
    {
        TAction* action = newAction(qsl("eixh push-down button"));
        action->setIsPushDownButton(true);
        action->mButtonState = true;
        const QString down = exportActionToXML(action);

        TAction* restored = importActionFromXML(down, nullptr, mpHost);

        QVERIFY(restored);
        QVERIFY(restored->mButtonState);

        action->mButtonState = false;
        QVERIFY(updateActionFromXML(action, down));
        QVERIFY(action->mButtonState);
    }

    // A snapshot the undo stack cannot read must not turn into a blank item, and
    // must leave an item it was asked to restore exactly as it is.
    void test_anUnreadableSnapshotIsRefused()
    {
        const auto triggers = mpHost->getTriggerUnit()->getTriggerRootNodeList().size();
        const auto aliases = mpHost->getAliasUnit()->getAliasRootNodeList().size();
        const auto timers = mpHost->getTimerUnit()->getTimerRootNodeList().size();
        const auto scripts = mpHost->getScriptUnit()->getScriptRootNodeList().size();
        const auto keys = mpHost->getKeyUnit()->getKeyRootNodeList().size();
        const auto actions = mpHost->getActionUnit()->getActionRootNodeList().size();

        TTrigger* trigger = newTrigger(qsl("eixh untouched trigger"));
        TAlias* alias = newAlias(qsl("eixh untouched alias"));
        TTimer* timer = newTimer(qsl("eixh untouched timer"));
        TScript* script = newScript(qsl("eixh untouched script"));
        TKey* key = newKey(qsl("eixh untouched key"));
        TAction* action = newAction(qsl("eixh untouched button"));
        const QString triggerBefore = exportTriggerToXML(trigger);
        const QString aliasBefore = exportAliasToXML(alias);
        const QString timerBefore = exportTimerToXML(timer);
        const QString scriptBefore = exportScriptToXML(script);
        const QString keyBefore = exportKeyToXML(key);
        const QString actionBefore = exportActionToXML(action);

        QStringList snapshots = scmUnreadableSnapshots;
        snapshots << QString();
        for (const QString& kind : {qsl("Trigger"), qsl("Alias"), qsl("Timer"), qsl("Script"), qsl("Key"), qsl("Action")}) {
            snapshots << emptySnapshot(kind);
        }
        for (const QString& snapshot : std::as_const(snapshots)) {
            QVERIFY2(!importTriggerFromXML(snapshot, nullptr, mpHost), qPrintable(snapshot));
            QVERIFY2(!importAliasFromXML(snapshot, nullptr, mpHost), qPrintable(snapshot));
            QVERIFY2(!importTimerFromXML(snapshot, nullptr, mpHost), qPrintable(snapshot));
            QVERIFY2(!importScriptFromXML(snapshot, nullptr, mpHost), qPrintable(snapshot));
            QVERIFY2(!importKeyFromXML(snapshot, nullptr, mpHost), qPrintable(snapshot));
            QVERIFY2(!importActionFromXML(snapshot, nullptr, mpHost), qPrintable(snapshot));
            QVERIFY2(!updateTriggerFromXML(trigger, snapshot), qPrintable(snapshot));
            QVERIFY2(!updateAliasFromXML(alias, snapshot), qPrintable(snapshot));
            QVERIFY2(!updateTimerFromXML(timer, snapshot), qPrintable(snapshot));
            QVERIFY2(!updateScriptFromXML(script, snapshot), qPrintable(snapshot));
            QVERIFY2(!updateKeyFromXML(key, snapshot), qPrintable(snapshot));
            QVERIFY2(!updateActionFromXML(action, snapshot), qPrintable(snapshot));
        }

        // and there is no item for a restore to write into
        QVERIFY(!updateTriggerFromXML(nullptr, triggerBefore));
        QVERIFY(!updateAliasFromXML(nullptr, aliasBefore));
        QVERIFY(!updateTimerFromXML(nullptr, timerBefore));
        QVERIFY(!updateScriptFromXML(nullptr, scriptBefore));
        QVERIFY(!updateKeyFromXML(nullptr, keyBefore));
        QVERIFY(!updateActionFromXML(nullptr, actionBefore));
        QVERIFY(!importTriggerFromXML(triggerBefore, nullptr, nullptr));
        QVERIFY(!importAliasFromXML(aliasBefore, nullptr, nullptr));
        QVERIFY(!importTimerFromXML(timerBefore, nullptr, nullptr));
        QVERIFY(!importScriptFromXML(scriptBefore, nullptr, nullptr));
        QVERIFY(!importKeyFromXML(keyBefore, nullptr, nullptr));
        QVERIFY(!importActionFromXML(actionBefore, nullptr, nullptr));
        QVERIFY(exportTriggerToXML(nullptr).isEmpty());
        QVERIFY(exportAliasToXML(nullptr).isEmpty());
        QVERIFY(exportTimerToXML(nullptr).isEmpty());
        QVERIFY(exportScriptToXML(nullptr).isEmpty());
        QVERIFY(exportKeyToXML(nullptr).isEmpty());
        QVERIFY(exportActionToXML(nullptr).isEmpty());

        QCOMPARE(mpHost->getTriggerUnit()->getTriggerRootNodeList().size(), triggers + 1);
        QCOMPARE(mpHost->getAliasUnit()->getAliasRootNodeList().size(), aliases + 1);
        QCOMPARE(mpHost->getTimerUnit()->getTimerRootNodeList().size(), timers + 1);
        QCOMPARE(mpHost->getScriptUnit()->getScriptRootNodeList().size(), scripts + 1);
        QCOMPARE(mpHost->getKeyUnit()->getKeyRootNodeList().size(), keys + 1);
        QCOMPARE(mpHost->getActionUnit()->getActionRootNodeList().size(), actions + 1);
        QVERIFY(exportTriggerToXML(trigger) == triggerBefore);
        QVERIFY(exportAliasToXML(alias) == aliasBefore);
        QVERIFY(exportTimerToXML(timer) == timerBefore);
        QVERIFY(exportScriptToXML(script) == scriptBefore);
        QVERIFY(exportKeyToXML(key) == keyBefore);
        QVERIFY(exportActionToXML(action) == actionBefore);
    }
};

#include "EditorItemXMLHelpersTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorItemXMLHelpersTest)
