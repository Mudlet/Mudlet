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
 * Round-trip tests for profile XML persistence (XMLexport -> XMLimport).
 *
 * A tree of triggers, aliases, timers, keys and scripts is built on a source
 * profile - including nested groups, duplicate names at the same level,
 * non-ASCII names, multiline scripts and patterns full of XML-hostile
 * characters (< > & " ' and "]]>"). The profile is saved through the
 * production save path (Host::saveProfile -> XMLexport) and imported into a
 * fresh Host (XMLimport, same code path as profile load), then every field
 * that the XML format persists is compared between the two trees.
 *
 * Run with: ctest -R ProfileRoundTripTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>

#include <functional>

#include "AliasUnit.h"
#include "Host.h"
#include "HostManager.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "ScriptUnit.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "XMLimport.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForProfileRoundTripTest();

namespace {
template <typename T>
QString dbgString(const T& value)
{
    QString text;
    QDebug(&text) << value;
    return text.trimmed();
}

// A multiline Lua script stuffed with every character class the XML encoding
// has to escape or that has historically been mangled (see issue #500):
const QString scmHostileScript = qsl("-- 特殊 characters: <tag> & \"double\" 'single' ]]> done\n"
                                     "local weird = \"<b>&amp;</b> \\\" ' ]]> 日本語 émoji🎉\"\n"
                                     "local function profileRoundTripHelper()\n"
                                     "\treturn weird .. [[literal <&> block]]\n"
                                     "end\n");

const QString scmHostileCommand = qsl("say <hi> & \"there\" 'pal' ]]>");
} // namespace

// Compares one field of the pair of tree nodes held in the local variables
// original/imported, reporting the node path on mismatch:
#define COMPARE_MEMBER(accessor)                                                                                                                                                                       \
    do {                                                                                                                                                                                               \
        if (!(imported->accessor == original->accessor)) {                                                                                                                                             \
            QFAIL(qPrintable(qsl("%1: " #accessor " mismatch: imported %2, original %3").arg(path, dbgString(imported->accessor), dbgString(original->accessor))));                                    \
        }                                                                                                                                                                                              \
    } while (false)

class ProfileRoundTripTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpSource = nullptr;
    Host* mpTarget = nullptr;
    const QString mSourceName = qsl("ProfileRoundTrip-Test");
    const QString mTargetName = qsl("ProfileRoundTripTarget-Test");
    const QString mPort = qsl("4013");
    const QString mLocalhost = qsl("localhost");
    QTemporaryDir mSaveDir;

    // Expected item totals, kept explicit so an "everything got lost and both
    // sides are empty" scenario cannot pass the pairwise comparison:
    static const int scmTriggerCount = 7;
    static const int scmAliasCount = 6;
    static const int scmTimerCount = 6;
    static const int scmKeyCount = 6;
    static const int scmScriptCount = 6;

    // -----------------------------------------------------------------------
    // Tree builders - these mirror the construction order XMLimport uses
    // -----------------------------------------------------------------------

    TTrigger* addTrigger(TTrigger* parent, const QString& name, bool folder, bool active, const QStringList& patterns, const QList<int>& kinds, const QString& script)
    {
        auto pT = new TTrigger(parent, mpSource);
        mpSource->getTriggerUnit()->registerTrigger(pT);
        pT->setIsFolder(folder);
        pT->setName(name);
        pT->setScript(script);
        pT->setRegexCodeList(patterns, kinds);
        pT->setIsActive(active);
        return pT;
    }

    TAlias* addAlias(TAlias* parent, const QString& name, bool folder, bool active, const QString& regex, const QString& command, const QString& script)
    {
        auto pT = new TAlias(parent, mpSource);
        mpSource->getAliasUnit()->registerAlias(pT);
        pT->setIsFolder(folder);
        pT->setName(name);
        pT->setScript(script);
        pT->setCommand(command);
        pT->setRegexCode(regex);
        pT->setIsActive(active);
        return pT;
    }

    TTimer* addTimer(TTimer* parent, const QString& name, bool folder, bool active, const QTime& time, const QString& command, const QString& script)
    {
        auto pT = new TTimer(parent, mpSource);
        pT->setIsFolder(folder);
        mpSource->getTimerUnit()->registerTimer(pT);
        pT->setName(name);
        pT->setScript(script);
        pT->setCommand(command);
        pT->setTime(time);
        pT->setIsActive(active);
        return pT;
    }

    TKey* addKey(TKey* parent, const QString& name, bool folder, bool active, const Qt::Key keyCode, const Qt::KeyboardModifiers modifiers, const QString& command, const QString& script)
    {
        auto pT = new TKey(parent, mpSource);
        mpSource->getKeyUnit()->registerKey(pT);
        pT->setIsFolder(folder);
        pT->setName(name);
        pT->setScript(script);
        pT->setCommand(command);
        pT->setKeyCode(keyCode);
        pT->setKeyModifiers(modifiers);
        pT->setIsActive(active);
        return pT;
    }

    TScript* addScript(TScript* parent, const QString& name, bool folder, bool active, const QStringList& events, const QString& script)
    {
        auto pT = new TScript(parent, mpSource);
        mpSource->getScriptUnit()->registerScript(pT);
        pT->setIsFolder(folder);
        pT->setName(name);
        pT->setScript(script);
        pT->setEventHandlerList(events);
        pT->setIsActive(active);
        return pT;
    }

    void buildSourceTree()
    {
        // Triggers: nested groups, duplicate names, non-ASCII names, hostile
        // patterns, a multiline+filter trigger and an ANSI color pattern:
        auto trGroup = addTrigger(nullptr, qsl("日本語トリガー"), true, true, {}, {}, QString());
        addTrigger(trGroup, qsl("dup <>&\"' trigger"), false, true, {qsl("<tag> & \"quoted\" 'single'"), qsl("^(\\w+) <[^>]*>&\"']]>$")}, {REGEX_SUBSTRING, REGEX_PERL}, scmHostileScript);
        auto trDup2 = addTrigger(trGroup, qsl("dup <>&\"' trigger"), false, false, {qsl("simple")}, {REGEX_SUBSTRING}, qsl("-- second twin\n"));
        trDup2->setIsMultiline(true);
        trDup2->setConditionLineDelta(2);
        trDup2->mStayOpen = 3;
        auto trInner = addTrigger(trGroup, qsl("inner <group> & \"stuff\""), true, false, {}, {}, QString());
        auto trEmoji = addTrigger(trInner, qsl("émoji🎉 trigger"), false, true, {qsl("^prefix"), qsl("exact & <line>")}, {REGEX_BEGIN_OF_LINE_SUBSTRING, REGEX_EXACT_MATCH}, qsl("-- colorize\n"));
        trEmoji->setIsColorizerTrigger(true);
        trEmoji->setColorizerFgColor(QColor(64, 160, 255));
        trEmoji->setColorizerBgColor(QColor(20, 20, 20));
        trEmoji->setCommand(scmHostileCommand);
        auto trLone = addTrigger(
                nullptr, qsl("lone & <trigger>"), false, true, {qsl("total recall"), qsl("3"), qsl("return true")}, {REGEX_EXACT_MATCH, REGEX_LINE_SPACER, REGEX_LUA_CODE}, qsl("-- lone\n"));
        trLone->mPerlSlashGOption = true;
        trLone->mFilterTrigger = true;
        trLone->mSoundTrigger = true;
        trLone->mSoundFile = qsl("/tmp/beep 日本語 & <sound>.wav");
        addTrigger(nullptr,
                   qsl("ansi color trigger"),
                   false,
                   true,
                   {TTrigger::createColorPatternText(5, 2), TTrigger::createColorPatternText(TTrigger::scmDefault, TTrigger::scmIgnored)},
                   {REGEX_COLOR_PATTERN, REGEX_COLOR_PATTERN},
                   qsl("-- color\n"));

        // Aliases:
        auto alGroup = addAlias(nullptr, qsl("Ålias Gruppe"), true, true, QString(), QString(), QString());
        addAlias(alGroup, qsl("dup alias"), false, true, qsl("^say (.+) & <em>\"$"), qsl("say %1 <verbatim> & \"quoted\""), scmHostileScript);
        addAlias(alGroup, qsl("dup alias"), false, false, qsl("^другой$"), QString(), qsl("-- twin\n"));
        auto alInner = addAlias(alGroup, qsl("sub <alias> & 'group'"), true, true, QString(), QString(), QString());
        addAlias(alInner, qsl("émoji🎉 alias"), false, true, qsl("^x ]]> \"quote\" & <tag>$"), QString(), qsl("-- emoji\n"));
        addAlias(nullptr, qsl("lone alias"), false, true, qsl("^\\d+ <[^>]+>$"), scmHostileCommand, qsl("-- lone\n"));

        // Timers - intervals are long enough that nothing fires during the
        // test; the child of a real (non-folder) timer becomes an offset timer:
        auto tiGroup = addTimer(nullptr, qsl("타이머 그룹"), true, true, QTime(0, 0, 0, 0), QString(), QString());
        auto tiDup1 = addTimer(tiGroup, qsl("dup timer"), false, true, QTime(1, 30, 0, 0), qsl("look"), qsl("-- tick\n"));
        addTimer(tiGroup, qsl("dup timer"), false, false, QTime(0, 45, 10, 250), QString(), scmHostileScript);
        addTimer(tiDup1, qsl("offset ⏱ timer"), false, false, QTime(0, 0, 30, 500), QString(), qsl("-- offset child\n"));
        addTimer(tiGroup, qsl("empty subgroup <>&"), true, false, QTime(0, 0, 0, 0), QString(), QString());
        addTimer(nullptr, qsl("solo & <timer>"), false, false, QTime(2, 3, 4, 5), scmHostileCommand, qsl("-- solo\n"));

        // Keys:
        auto keGroup = addKey(nullptr, qsl("キーグループ"), true, true, Qt::Key_unknown, Qt::NoModifier, QString(), QString());
        addKey(keGroup, qsl("dup key"), false, true, Qt::Key_F5, Qt::ControlModifier | Qt::ShiftModifier, qsl("kick"), scmHostileScript);
        addKey(keGroup, qsl("dup key"), false, false, Qt::Key_F5, Qt::NoModifier, QString(), qsl("-- twin\n"));
        auto keInner = addKey(keGroup, qsl("nested <key> & group"), true, false, Qt::Key_unknown, Qt::NoModifier, QString(), QString());
        addKey(keInner, qsl("émoji🎉 key"), false, true, Qt::Key_Eacute, Qt::AltModifier, scmHostileCommand, qsl("-- emoji key\n"));
        addKey(nullptr, qsl("lone key"), false, false, Qt::Key_ScrollLock, Qt::KeypadModifier, QString(), qsl("-- lone\n"));

        // Scripts, with event handler registrations:
        auto scGroup = addScript(nullptr, qsl("脚本组"), true, true, {}, QString());
        addScript(scGroup, qsl("dup script"), false, true, {qsl("sysDataEvent"), qsl("custom<>&\"'Event"), qsl("日本語イベント")}, scmHostileScript);
        addScript(scGroup, qsl("dup script"), false, false, {}, qsl("-- twin ]]> script\n"));
        auto scInner = addScript(scGroup, qsl("scripts <sub> & 'group'"), true, true, {}, QString());
        addScript(scInner, qsl("émoji🎉 script"), false, true, {qsl("emojiEvent🎉")}, qsl("-- emoji script\n"));
        addScript(nullptr, qsl("lone script"), false, false, {}, QString());
    }

    // -----------------------------------------------------------------------
    // Recursive comparators, one per item type
    // -----------------------------------------------------------------------

    void compareTriggerNodes(TTrigger* original, TTrigger* imported, const QString& parentPath)
    {
        const QString path = qsl("%1/%2").arg(parentPath, original->getName());
        COMPARE_MEMBER(getName());
        COMPARE_MEMBER(isFolder());
        COMPARE_MEMBER(shouldBeActive());
        COMPARE_MEMBER(isActive());
        COMPARE_MEMBER(isTemporary());
        COMPARE_MEMBER(getScript());
        COMPARE_MEMBER(mPatterns);
        COMPARE_MEMBER(getRegexCodePropertyList());
        COMPARE_MEMBER(isMultiline());
        COMPARE_MEMBER(getConditionLineDelta());
        COMPARE_MEMBER(mStayOpen);
        COMPARE_MEMBER(getCommand());
        COMPARE_MEMBER(mFilterTrigger);
        COMPARE_MEMBER(mPerlSlashGOption);
        COMPARE_MEMBER(mSoundTrigger);
        COMPARE_MEMBER(mSoundFile);
        COMPARE_MEMBER(isColorizerTrigger());
        COMPARE_MEMBER(getFgColor());
        COMPARE_MEMBER(getBgColor());
        COMPARE_MEMBER(mColorTrigger);
        COMPARE_MEMBER(mColorTriggerFgAnsi);
        COMPARE_MEMBER(mColorTriggerBgAnsi);
        COMPARE_MEMBER(getTriggerType());
        if (original->mpMyChildrenList->size() != imported->mpMyChildrenList->size()) {
            QFAIL(qPrintable(qsl("%1: child count mismatch: imported %2, original %3").arg(path).arg(imported->mpMyChildrenList->size()).arg(original->mpMyChildrenList->size())));
        }
        auto itOriginal = original->mpMyChildrenList->begin();
        auto itImported = imported->mpMyChildrenList->begin();
        while (itOriginal != original->mpMyChildrenList->end()) {
            compareTriggerNodes(*itOriginal, *itImported, path);
            if (QTest::currentTestFailed()) {
                return;
            }
            ++itOriginal;
            ++itImported;
        }
    }

    void compareAliasNodes(TAlias* original, TAlias* imported, const QString& parentPath)
    {
        const QString path = qsl("%1/%2").arg(parentPath, original->getName());
        COMPARE_MEMBER(getName());
        COMPARE_MEMBER(isFolder());
        COMPARE_MEMBER(shouldBeActive());
        COMPARE_MEMBER(isActive());
        COMPARE_MEMBER(getScript());
        COMPARE_MEMBER(getRegexCode());
        COMPARE_MEMBER(getCommand());
        if (original->mpMyChildrenList->size() != imported->mpMyChildrenList->size()) {
            QFAIL(qPrintable(qsl("%1: child count mismatch: imported %2, original %3").arg(path).arg(imported->mpMyChildrenList->size()).arg(original->mpMyChildrenList->size())));
        }
        auto itOriginal = original->mpMyChildrenList->begin();
        auto itImported = imported->mpMyChildrenList->begin();
        while (itOriginal != original->mpMyChildrenList->end()) {
            compareAliasNodes(*itOriginal, *itImported, path);
            if (QTest::currentTestFailed()) {
                return;
            }
            ++itOriginal;
            ++itImported;
        }
    }

    void compareTimerNodes(TTimer* original, TTimer* imported, const QString& parentPath)
    {
        const QString path = qsl("%1/%2").arg(parentPath, original->getName());
        COMPARE_MEMBER(getName());
        COMPARE_MEMBER(isFolder());
        // Unlike the other item types the import path only fully activates
        // root timers (children are activated at runtime when their parent
        // fires or the profile connects), so only the persisted user intent -
        // shouldBeActive() - is compared here:
        COMPARE_MEMBER(shouldBeActive());
        COMPARE_MEMBER(isTemporary());
        COMPARE_MEMBER(getScript());
        COMPARE_MEMBER(getCommand());
        COMPARE_MEMBER(getTime());
        COMPARE_MEMBER(isOffsetTimer());
        if (original->mpMyChildrenList->size() != imported->mpMyChildrenList->size()) {
            QFAIL(qPrintable(qsl("%1: child count mismatch: imported %2, original %3").arg(path).arg(imported->mpMyChildrenList->size()).arg(original->mpMyChildrenList->size())));
        }
        auto itOriginal = original->mpMyChildrenList->begin();
        auto itImported = imported->mpMyChildrenList->begin();
        while (itOriginal != original->mpMyChildrenList->end()) {
            compareTimerNodes(*itOriginal, *itImported, path);
            if (QTest::currentTestFailed()) {
                return;
            }
            ++itOriginal;
            ++itImported;
        }
    }

    void compareKeyNodes(TKey* original, TKey* imported, const QString& parentPath)
    {
        const QString path = qsl("%1/%2").arg(parentPath, original->getName());
        COMPARE_MEMBER(getName());
        COMPARE_MEMBER(isFolder());
        COMPARE_MEMBER(shouldBeActive());
        COMPARE_MEMBER(isActive());
        COMPARE_MEMBER(getScript());
        COMPARE_MEMBER(getCommand());
        COMPARE_MEMBER(getKeyCode());
        COMPARE_MEMBER(getKeyModifiers());
        if (original->mpMyChildrenList->size() != imported->mpMyChildrenList->size()) {
            QFAIL(qPrintable(qsl("%1: child count mismatch: imported %2, original %3").arg(path).arg(imported->mpMyChildrenList->size()).arg(original->mpMyChildrenList->size())));
        }
        auto itOriginal = original->mpMyChildrenList->begin();
        auto itImported = imported->mpMyChildrenList->begin();
        while (itOriginal != original->mpMyChildrenList->end()) {
            compareKeyNodes(*itOriginal, *itImported, path);
            if (QTest::currentTestFailed()) {
                return;
            }
            ++itOriginal;
            ++itImported;
        }
    }

    void compareScriptNodes(TScript* original, TScript* imported, const QString& parentPath)
    {
        const QString path = qsl("%1/%2").arg(parentPath, original->getName());
        COMPARE_MEMBER(getName());
        COMPARE_MEMBER(isFolder());
        COMPARE_MEMBER(shouldBeActive());
        COMPARE_MEMBER(isActive());
        COMPARE_MEMBER(getScript());
        COMPARE_MEMBER(getEventHandlerList());
        if (original->mpMyChildrenList->size() != imported->mpMyChildrenList->size()) {
            QFAIL(qPrintable(qsl("%1: child count mismatch: imported %2, original %3").arg(path).arg(imported->mpMyChildrenList->size()).arg(original->mpMyChildrenList->size())));
        }
        auto itOriginal = original->mpMyChildrenList->begin();
        auto itImported = imported->mpMyChildrenList->begin();
        while (itOriginal != original->mpMyChildrenList->end()) {
            compareScriptNodes(*itOriginal, *itImported, path);
            if (QTest::currentTestFailed()) {
                return;
            }
            ++itOriginal;
            ++itImported;
        }
    }

    // The source profile may contain pre-installed package items (and items
    // flagged as not exportable), so the comparison is restricted to the roots
    // this test created - identified by their (unique to this test) names:
    template <typename T>
    static QList<T*> collectRootsByName(const std::list<T*>& rootNodes, const QStringList& names)
    {
        QList<T*> collected;
        for (auto* node : rootNodes) {
            if (names.contains(node->getName())) {
                collected << node;
            }
        }
        return collected;
    }

    template <typename T>
    static int countNodes(const QList<T*>& rootNodes)
    {
        int total = 0;
        std::function<void(T*)> walk = [&](T* node) {
            ++total;
            for (auto* child : *node->mpMyChildrenList) {
                walk(child);
            }
        };
        for (auto* node : rootNodes) {
            walk(node);
        }
        return total;
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForProfileRoundTripTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mSourceName);
        deleteProfileDirectory(mTargetName);

        QVERIFY(mSaveDir.isValid());

        // The source profile needs the full production boot (with a console)
        // as XMLexport::writeHost() saves console state too:
        startProfile(mSourceName, mLocalhost, mPort);
        if (QTest::currentTestFailed()) {
            return;
        }
        mpSource = mudlet::self()->getActiveHost();
        QVERIFY2(mpSource, "No active host after profile creation");

        buildSourceTree();
        if (QTest::currentTestFailed()) {
            return;
        }

        auto [saved, xmlPath, saveError] = mpSource->saveProfile(mSaveDir.path(), qsl("roundtrip"));
        QVERIFY2(saved, qPrintable(saveError));
        mpSource->waitForProfileSave();
        QVERIFY2(QFileInfo::exists(xmlPath), qPrintable(qsl("profile XML was not written to %1").arg(xmlPath)));

        // The import target is a bare Host, matching the state a profile is
        // in when mudlet::loadProfile() imports its XML at startup:
        auto& hostManager = mudlet::self()->getHostManager();
        QVERIFY2(hostManager.addHost(mTargetName, mPort, QString(), QString()), "failed to create the target Host");
        mpTarget = hostManager.getHost(mTargetName);
        QVERIFY(mpTarget);

        QFile file(xmlPath);
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text), qPrintable(file.errorString()));
        XMLimport importer(mpTarget);
        auto [imported, importError] = importer.importPackage(&file);
        QVERIFY2(imported, qPrintable(importError));
    }

    void cleanupTestCase()
    {
        mpSource = nullptr;
        mpTarget = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mSourceName);
        deleteProfileDirectory(mTargetName);
        delete mudlet::self();
    }

    void test_triggersRoundTrip()
    {
        const QStringList rootNames{qsl("日本語トリガー"), qsl("lone & <trigger>"), qsl("ansi color trigger")};
        const auto originalRoots = collectRootsByName(mpSource->getTriggerUnit()->getTriggerRootNodeList(), rootNames);
        const auto importedRoots = collectRootsByName(mpTarget->getTriggerUnit()->getTriggerRootNodeList(), rootNames);
        QCOMPARE(originalRoots.size(), 3);
        QCOMPARE(importedRoots.size(), 3);
        QCOMPARE(countNodes(originalRoots), scmTriggerCount);
        QCOMPARE(countNodes(importedRoots), scmTriggerCount);
        for (qsizetype i = 0; i < originalRoots.size(); ++i) {
            compareTriggerNodes(originalRoots.at(i), importedRoots.at(i), qsl("triggers"));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    void test_aliasesRoundTrip()
    {
        const QStringList rootNames{qsl("Ålias Gruppe"), qsl("lone alias")};
        const auto originalRoots = collectRootsByName(mpSource->getAliasUnit()->getAliasRootNodeList(), rootNames);
        const auto importedRoots = collectRootsByName(mpTarget->getAliasUnit()->getAliasRootNodeList(), rootNames);
        QCOMPARE(originalRoots.size(), 2);
        QCOMPARE(importedRoots.size(), 2);
        QCOMPARE(countNodes(originalRoots), scmAliasCount);
        QCOMPARE(countNodes(importedRoots), scmAliasCount);
        for (qsizetype i = 0; i < originalRoots.size(); ++i) {
            compareAliasNodes(originalRoots.at(i), importedRoots.at(i), qsl("aliases"));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    void test_timersRoundTrip()
    {
        const QStringList rootNames{qsl("타이머 그룹"), qsl("solo & <timer>")};
        const auto originalRoots = collectRootsByName(mpSource->getTimerUnit()->getTimerRootNodeList(), rootNames);
        const auto importedRoots = collectRootsByName(mpTarget->getTimerUnit()->getTimerRootNodeList(), rootNames);
        QCOMPARE(originalRoots.size(), 2);
        QCOMPARE(importedRoots.size(), 2);
        QCOMPARE(countNodes(originalRoots), scmTimerCount);
        QCOMPARE(countNodes(importedRoots), scmTimerCount);
        for (qsizetype i = 0; i < originalRoots.size(); ++i) {
            compareTimerNodes(originalRoots.at(i), importedRoots.at(i), qsl("timers"));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    void test_keysRoundTrip()
    {
        const QStringList rootNames{qsl("キーグループ"), qsl("lone key")};
        const auto originalRoots = collectRootsByName(mpSource->getKeyUnit()->getKeyRootNodeList(), rootNames);
        const auto importedRoots = collectRootsByName(mpTarget->getKeyUnit()->getKeyRootNodeList(), rootNames);
        QCOMPARE(originalRoots.size(), 2);
        QCOMPARE(importedRoots.size(), 2);
        QCOMPARE(countNodes(originalRoots), scmKeyCount);
        QCOMPARE(countNodes(importedRoots), scmKeyCount);
        for (qsizetype i = 0; i < originalRoots.size(); ++i) {
            compareKeyNodes(originalRoots.at(i), importedRoots.at(i), qsl("keys"));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    void test_scriptsRoundTrip()
    {
        const QStringList rootNames{qsl("脚本组"), qsl("lone script")};
        const auto originalRoots = collectRootsByName(mpSource->getScriptUnit()->getScriptRootNodeList(), rootNames);
        const auto importedRoots = collectRootsByName(mpTarget->getScriptUnit()->getScriptRootNodeList(), rootNames);
        QCOMPARE(originalRoots.size(), 2);
        QCOMPARE(importedRoots.size(), 2);
        QCOMPARE(countNodes(originalRoots), scmScriptCount);
        QCOMPARE(countNodes(importedRoots), scmScriptCount);
        for (qsizetype i = 0; i < originalRoots.size(); ++i) {
            compareScriptNodes(originalRoots.at(i), importedRoots.at(i), qsl("scripts"));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    // The imported scripts registered their event handlers in the fresh Host:
    void test_scriptEventHandlersRegisteredAfterImport()
    {
        QVERIFY(mpTarget->mEventHandlerMap.contains(qsl("sysDataEvent")));
        QVERIFY(mpTarget->mEventHandlerMap.contains(qsl("custom<>&\"'Event")));
        QVERIFY(mpTarget->mEventHandlerMap.contains(qsl("日本語イベント")));
        QVERIFY(mpTarget->mEventHandlerMap.contains(qsl("emojiEvent🎉")));
    }
};

void initializeQRCResourcesForProfileRoundTripTest()
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

#include "ProfileRoundTripTest.moc"
QTEST_MAIN(ProfileRoundTripTest)
