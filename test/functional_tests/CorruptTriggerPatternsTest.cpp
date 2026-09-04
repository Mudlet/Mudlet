/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org   *
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
 * A trigger is saved as two parallel lists: <regexCodeList> holds the pattern
 * texts and <regexCodePropertyList> holds one kind per pattern.
 * TTrigger::setRegexCodeList() loops over the patterns but indexes the kinds,
 * so a save holding fewer kinds than patterns reads past the end of the kinds
 * unless the two are made the same length first. That read happens while the
 * profile is loading, and on the `mudlet -p <profile>` path it lands before the
 * main window opens - so the profile cannot be reached to repair it and every
 * relaunch dies the same way (#9882).
 *
 * A trigger whose kind list is entirely empty escapes it if it is an ordinary
 * trigger, because "this trigger has no patterns defined" returns first -
 * folders and colour triggers are excluded from that check. A merely shorter
 * kind list escapes nothing, ordinary triggers included, hence one fixture of
 * each shape below.
 *
 * The two empty-kind-list fixtures are the deterministic ones: an empty QList
 * has a null data pointer, so the pre-fix at(0) is a null dereference rather
 * than an unchecked read of whatever follows the allocation.
 *
 * The repair keeps the pattern text and defaults the missing kind, so the two
 * assertions worth making are that nothing crashes and that nothing the user
 * wrote is lost - an emptied folder would also stop gating its children and
 * start handing them every line.
 *
 * Run with: ctest -R CorruptTriggerPatternsTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>

#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include "GroupedTest.h"

namespace {
const QString scmCorruptFolderName = qsl("corrupt folder");
const QString scmFolderChildName = qsl("child of the corrupt folder");
const QString scmCorruptColorTriggerName = qsl("corrupt colour trigger");
const QString scmSurplusKindsName = qsl("more kinds than patterns");
const QString scmMissingKindsName = qsl("more patterns than kinds");
const QString scmHealthyTriggerName = qsl("healthy trigger");

// One saved profile holding, in order: a folder whose pattern has no kind and
// which contains an undamaged child, a colour trigger with the same damage, an
// ordinary trigger whose kind list is short but not empty, one with a spare
// kind and no pattern to go with it, and an undamaged trigger that has to come
// through intact.
const QString scmProfileXml = qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<TriggerPackage>
<TriggerGroup isActive="yes" isFolder="yes" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="no" isColorTriggerFg="no" isColorTriggerBg="no">
<name>corrupt folder</name>
<script></script>
<triggerType>0</triggerType>
<conditonLineDelta>0</conditonLineDelta>
<mStayOpen>0</mStayOpen>
<mCommand></mCommand>
<packageName></packageName>
<mFgColor>#ff0000</mFgColor>
<mBgColor>#ffff00</mBgColor>
<mSoundFile></mSoundFile>
<colorTriggerFgColor>#000000</colorTriggerFgColor>
<colorTriggerBgColor>#000000</colorTriggerBgColor>
<regexCodeList>
<string>orphaned pattern</string>
</regexCodeList>
<regexCodePropertyList />
<Trigger isActive="yes" isFolder="no" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="no" isColorTriggerFg="no" isColorTriggerBg="no">
<name>child of the corrupt folder</name>
<script></script>
<triggerType>0</triggerType>
<conditonLineDelta>0</conditonLineDelta>
<mStayOpen>0</mStayOpen>
<mCommand></mCommand>
<packageName></packageName>
<mFgColor>#ff0000</mFgColor>
<mBgColor>#ffff00</mBgColor>
<mSoundFile></mSoundFile>
<colorTriggerFgColor>#000000</colorTriggerFgColor>
<colorTriggerBgColor>#000000</colorTriggerBgColor>
<regexCodeList>
<string>^child pattern$</string>
</regexCodeList>
<regexCodePropertyList>
<integer>1</integer>
</regexCodePropertyList>
</Trigger>
</TriggerGroup>
<Trigger isActive="yes" isFolder="no" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="yes" isColorTriggerFg="yes" isColorTriggerBg="no">
<name>corrupt colour trigger</name>
<script></script>
<triggerType>0</triggerType>
<conditonLineDelta>0</conditonLineDelta>
<mStayOpen>0</mStayOpen>
<mCommand></mCommand>
<packageName></packageName>
<mFgColor>#ff0000</mFgColor>
<mBgColor>#ffff00</mBgColor>
<mSoundFile></mSoundFile>
<colorTriggerFgColor>#ff0000</colorTriggerFgColor>
<colorTriggerBgColor>#000000</colorTriggerBgColor>
<regexCodeList>
<string>FG1BG-2</string>
</regexCodeList>
<regexCodePropertyList />
</Trigger>
<Trigger isActive="yes" isFolder="no" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="no" isColorTriggerFg="no" isColorTriggerBg="no">
<name>more patterns than kinds</name>
<script></script>
<triggerType>0</triggerType>
<conditonLineDelta>0</conditonLineDelta>
<mStayOpen>0</mStayOpen>
<mCommand></mCommand>
<packageName></packageName>
<mFgColor>#ff0000</mFgColor>
<mBgColor>#ffff00</mBgColor>
<mSoundFile></mSoundFile>
<colorTriggerFgColor>#000000</colorTriggerFgColor>
<colorTriggerBgColor>#000000</colorTriggerBgColor>
<regexCodeList>
<string>^the pattern with a kind$</string>
<string>^the pattern without one$</string>
</regexCodeList>
<regexCodePropertyList>
<integer>1</integer>
</regexCodePropertyList>
</Trigger>
<Trigger isActive="yes" isFolder="no" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="no" isColorTriggerFg="no" isColorTriggerBg="no">
<name>more kinds than patterns</name>
<script></script>
<triggerType>0</triggerType>
<conditonLineDelta>0</conditonLineDelta>
<mStayOpen>0</mStayOpen>
<mCommand></mCommand>
<packageName></packageName>
<mFgColor>#ff0000</mFgColor>
<mBgColor>#ffff00</mBgColor>
<mSoundFile></mSoundFile>
<colorTriggerFgColor>#000000</colorTriggerFgColor>
<colorTriggerBgColor>#000000</colorTriggerBgColor>
<regexCodeList>
<string>the only pattern</string>
</regexCodeList>
<regexCodePropertyList>
<integer>0</integer>
<integer>1</integer>
</regexCodePropertyList>
</Trigger>
<Trigger isActive="yes" isFolder="no" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="no" isColorTriggerFg="no" isColorTriggerBg="no">
<name>healthy trigger</name>
<script>-- intentionally empty</script>
<triggerType>0</triggerType>
<conditonLineDelta>0</conditonLineDelta>
<mStayOpen>0</mStayOpen>
<mCommand></mCommand>
<packageName></packageName>
<mFgColor>#ff0000</mFgColor>
<mBgColor>#ffff00</mBgColor>
<mSoundFile></mSoundFile>
<colorTriggerFgColor>#000000</colorTriggerFgColor>
<colorTriggerBgColor>#000000</colorTriggerBgColor>
<regexCodeList>
<string>^survivor$</string>
</regexCodeList>
<regexCodePropertyList>
<integer>1</integer>
</regexCodePropertyList>
</Trigger>
</TriggerPackage>
</MudletPackage>
)");
} // namespace

class CorruptTriggerPatternsTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("CorruptTriggerPatterns-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    // Registered before the load so an assertion cannot pass vacuously: if the
    // repair stops being reached, QTest fails the test on the message that
    // never arrived.
    static void expectRepairWarning(const QString& triggerName, int patternCount, int kindCount)
    {
        QTest::ignoreMessage(QtWarningMsg,
                             qsl(R"(TTrigger::setRegexCodeList(...) ERROR: trigger "%1" was saved with %2 pattern(s) but %3 pattern kind(s))")
                                     .arg(triggerName, QString::number(patternCount), QString::number(kindCount))
                                     .toUtf8()
                                     .constData());
    }

private slots:
    void initTestCase()
    {
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        if (portableMarkerPresent()) {
            QSKIP("portable.txt marker present - config dir cannot be redirected for this test");
        }
        QVERIFY2(MudletPaths::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "test config dir redirection did not take effect");
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aTriggerWithMismatchedPatternListsDoesNotCrashTheLoad()
    {
        const QString folder = MudletPaths::getMudletPath(enums::profileXmlFilesPath, mProfileName);
        QVERIFY(QDir().mkpath(folder));
        {
            QFile xmlFile(qsl("%1/2020-01-01#00-00-00.xml").arg(folder));
            QVERIFY(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text));
            QVERIFY(xmlFile.write(scmProfileXml.toUtf8()) > 0);
        }

        expectRepairWarning(scmCorruptFolderName, 1, 0);
        expectRepairWarning(scmCorruptColorTriggerName, 1, 0);
        expectRepairWarning(scmMissingKindsName, 2, 1);
        expectRepairWarning(scmSurplusKindsName, 1, 2);

        // Without the repair in TTrigger::setRegexCodeList() this line never
        // returns - the process dies inside the loader:
        Host* pHost = mudlet::self()->loadProfile(mProfileName, false);
        QVERIFY(pHost);
        QVERIFY2(pHost->mLoadedOk, "profile with mismatched trigger pattern lists failed to load");

        TriggerUnit* pTriggerUnit = pHost->getTriggerUnit();

        // The damaged items load with their pattern text intact and the missing
        // kind defaulted:
        TTrigger* pFolder = pTriggerUnit->findTrigger(scmCorruptFolderName);
        QVERIFY2(pFolder, "corrupt trigger folder was dropped from the profile");
        QVERIFY(pFolder->isFolder());
        QCOMPARE(pFolder->getPatternsList(), QStringList{qsl("orphaned pattern")});
        QCOMPARE(pFolder->getRegexCodePropertyList(), QList<int>{REGEX_SUBSTRING});

        // and take their children with them - the folder's contents are read
        // before the folder's own lists are:
        TTrigger* pChild = pTriggerUnit->findTrigger(scmFolderChildName);
        QVERIFY2(pChild, "the corrupt folder's child trigger was dropped from the profile");
        QCOMPARE(pChild->getParent(), pFolder);
        QCOMPARE(pChild->getPatternsList(), QStringList{qsl("^child pattern$")});

        TTrigger* pColorTrigger = pTriggerUnit->findTrigger(scmCorruptColorTriggerName);
        QVERIFY2(pColorTrigger, "corrupt colour trigger was dropped from the profile");
        QCOMPARE(pColorTrigger->getPatternsList(), QStringList{qsl("FG1BG-2")});
        QCOMPARE(pColorTrigger->getRegexCodePropertyList(), QList<int>{REGEX_SUBSTRING});

        // An ordinary trigger with a short - but not empty - kind list is the
        // third crashing shape, and keeps both patterns:
        TTrigger* pMissingKinds = pTriggerUnit->findTrigger(scmMissingKindsName);
        QVERIFY2(pMissingKinds, "trigger with a missing pattern kind was dropped from the profile");
        QCOMPARE(pMissingKinds->getPatternsList(), (QStringList{qsl("^the pattern with a kind$"), qsl("^the pattern without one$")}));
        QCOMPARE(pMissingKinds->getRegexCodePropertyList(), (QList<int>{REGEX_PERL, REGEX_SUBSTRING}));

        // The surplus direction never crashed - the loop is bounded by the
        // pattern list - so this pins that a kind with no pattern is the one
        // thing the repair does drop:
        TTrigger* pSurplusKinds = pTriggerUnit->findTrigger(scmSurplusKindsName);
        QVERIFY2(pSurplusKinds, "trigger with a surplus pattern kind was dropped from the profile");
        QCOMPARE(pSurplusKinds->getPatternsList(), QStringList{qsl("the only pattern")});
        QCOMPARE(pSurplusKinds->getRegexCodePropertyList(), QList<int>{REGEX_SUBSTRING});

        // and the rest of the profile is untouched:
        TTrigger* pHealthy = pTriggerUnit->findTrigger(scmHealthyTriggerName);
        QVERIFY2(pHealthy, "undamaged trigger from the same save is missing");
        QCOMPARE(pHealthy->getPatternsList(), QStringList{qsl("^survivor$")});
        QCOMPARE(pHealthy->getRegexCodePropertyList(), QList<int>{REGEX_PERL});

        // A repaired trigger still matches on the defaulted kind. Its substring
        // pattern sits at index 1, behind a perl one, so a per-pattern structure
        // that fell out of step with the pattern list - or was indexed by
        // anything other than the pattern's own index - reaches the wrong entry
        // here rather than the one this pattern owns.
        QVERIFY2(matches(pMissingKinds, qsl("before ^the pattern without one$ after")),
                 "the substring pattern the repair defaulted, at index 1 of a mixed-kind trigger, stopped matching");
        QVERIFY2(!matches(pMissingKinds, qsl("nothing either pattern of this trigger asks for")),
                 "a mixed-kind trigger matched a line holding neither of its patterns");

        // The undamaged neighbour is a perl pattern at index 0, so it pins that
        // the substring path above did not simply match everything.
        QVERIFY2(matches(pHealthy, qsl("survivor")), "the undamaged perl trigger stopped matching");

        // Every trigger the save produced, damaged or not, has to satisfy the
        // invariant match_substring() dereferences on:
        for (TTrigger* pTrigger : {pFolder, pChild, pColorTrigger, pMissingKinds, pSurplusKinds, pHealthy}) {
            const QString violation = matcherInvariantViolation(pTrigger);
            QVERIFY2(violation.isEmpty(), qPrintable(violation));
        }

        // Loading a save only reaches setRegexCodeList(). setupTmpColorTrigger()
        // - the tempColorTrigger() path - grows the pattern list on its own, and
        // is the other place the invariant has to be maintained.
        auto* pTempColour = new TTrigger(nullptr, pHost);
        pTempColour->setIsActive(true);
        pTempColour->registerTrigger();
        QVERIFY2(pTempColour->setupTmpColorTrigger(TTrigger::scmIgnored, 2), "the temporary colour trigger refused to set itself up");
        QCOMPARE(pTempColour->getPatternsList().size(), 1);
        const QString tempViolation = matcherInvariantViolation(pTempColour);
        QVERIFY2(tempViolation.isEmpty(), qPrintable(tempViolation));

        // and a second call appends again, so the two lists have to move together
        // more than once
        QVERIFY(pTempColour->setupTmpColorTrigger(3, TTrigger::scmIgnored));
        QCOMPARE(pTempColour->getPatternsList().size(), 2);
        const QString secondViolation = matcherInvariantViolation(pTempColour);
        QVERIFY2(secondViolation.isEmpty(), qPrintable(secondViolation));
        QVERIFY2(!matches(pHealthy, qsl("survivor with more after it")), "the perl pattern's anchors stopped being honoured");
    }

private:
    // TTrigger::match() takes the line as both UTF-8 bytes and a QString, the
    // way TriggerUnit::processDataStream() hands it over.
    static bool matches(TTrigger* pTrigger, const QString& line)
    {
        const QByteArray utf8 = line.toUtf8();
        return pTrigger->match(utf8.constData(), utf8.size(), line, -1);
    }

    // match_substring() indexes mSubstringMatchers by pattern number and
    // dereferences the entry without a bounds or null check, so it relies on one
    // entry per pattern, holding a matcher for exactly the substring kinds.
    // Answers rather than asserting: a QVERIFY here would return from this
    // helper and let the slot run on, burying the failure.
    static QString matcherInvariantViolation(TTrigger* pTrigger)
    {
        const QStringList patterns = pTrigger->getPatternsList();
        const QList<int> kinds = pTrigger->getRegexCodePropertyList();
        if (static_cast<int>(pTrigger->mSubstringMatchers.size()) != patterns.size()) {
            return qsl("trigger '%1' holds %2 pattern(s) but %3 matcher slot(s)")
                    .arg(pTrigger->getName(), QString::number(patterns.size()), QString::number(pTrigger->mSubstringMatchers.size()));
        }
        for (int i = 0; i < patterns.size(); ++i) {
            const bool wantMatcher = kinds.at(i) == REGEX_SUBSTRING;
            const bool haveMatcher = pTrigger->mSubstringMatchers[i] != nullptr;
            if (haveMatcher != wantMatcher) {
                return qsl("trigger '%1' pattern %2 is kind %3 but %4 a matcher")
                        .arg(pTrigger->getName(), QString::number(i), QString::number(kinds.at(i)), haveMatcher ? qsl("has") : qsl("lacks"));
            }
        }
        return {};
    }
};

#include "CorruptTriggerPatternsTest.moc"
MUDLET_GROUPED_TEST_MAIN(CorruptTriggerPatternsTest)
