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

/*
 * The 2D map room symbol settings - the font, whether only that font may be
 * used, and the scaling factor - now live on TMap, with the preferences dialog
 * and the setConfig() keys as two writers of the same state. Everything here is
 * about the seam between those two: what a change on one side has to do to the
 * other, and what it must leave alone.
 *
 * Only the round-trip through Lua is reachable from a busted spec (see the
 * setConfig section of Other_spec.lua). The rest is not: the QFont style
 * strategy that carries the only-use-selected flag is not visible from Lua at
 * all, and neither are the spin-box, the glyph usage table, or the per-widget
 * caches of rendered symbols.
 *
 * Run with: ctest -R MapSymbolFontTest -V
 */

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QScopeGuard>
#include <QTableWidget>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TelnetServerStub.h"
#include "TMap.h"
#include "TMapView.h"
#include "TMapViewManager.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapSymbolFontTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mSaveDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("MapSymbolFont-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // The symbol settings as this test found them, put back after each case
    QFont mFontBefore;
    bool mOnlyUseSelectedBefore = false;
    qreal mScalingBefore = 1.0;

    static constexpr int scmFirstRoom = 100;
    static constexpr int scmSecondRoom = 101;
    // Marks the glyph usage table's items, so that a rebuild - which throws
    // every item away and makes new ones - can be told from a refresh that did
    // not happen at all
    static constexpr int scmRebuildMarkerRole = Qt::UserRole + 1;

    TMap* map() const { return mpHost->mpMap.data(); }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    QDoubleSpinBox* scalingSpinBox() const
    {
        // Created in code rather than coming from the .ui file, so it has to be
        // found through the group box that holds it:
        return mpPreferences ? mpPreferences->groupBox_mapSymbols->findChild<QDoubleSpinBox*>() : nullptr;
    }

    void openPreferences()
    {
        mudlet::self()->showOptionsDialog(qsl("tab_mapper"), mpHost);
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !mpHost->mpDlgProfilePreferences.isNull();
                         },
                         5000),
                 "Preferences dialog was not created");
        mpPreferences = mpHost->mpDlgProfilePreferences.data();
        QVERIFY2(scalingSpinBox(), "The symbol scaling spin-box was not found in the Symbols group box");
    }

    // Closing the dialog is what writes the last edit back, and the hidden Save
    // button does no more than call close() - so this asks the dialog to close
    // rather than clicking a button that is on its way out of the .ui file
    void closePreferences()
    {
        mpPreferences->close();
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpHost->mpDlgProfilePreferences.isNull();
                         },
                         5000),
                 "Preferences dialog should have been destroyed by closing it");
        mpPreferences = nullptr;
    }

    // A family that is not the one in use, so that setting it cannot pass by
    // doing nothing
    QString anotherFontFamily() const
    {
        const QString inUse = map()->getSymbolFont().family();
        QStringList families = mudlet::self()->getAvailableFonts();
        families.sort();
        for (const QString& family : families) {
            if (family.compare(inUse, Qt::CaseInsensitive)) {
                return family;
            }
        }
        return QString();
    }

    // A factor that differs from the one given by more than the spin-box's
    // precision, so that both of them can tell the two apart
    static qreal anotherScalingFactor(const qreal notThisOne) { return qFuzzyCompare(notThisOne, 1.5) ? 1.75 : 1.5; }

    // Two rooms carrying symbols, which is what the glyph usage table lists and
    // what the rendered symbol caches hold pixmaps for
    void buildMapWithSymbols()
    {
        map()->mapClear();
        const int areaId = map()->mpRoomDB->addArea(qsl("symbols"));
        QVERIFY(areaId > 0);
        for (const int roomId : {scmFirstRoom, scmSecondRoom}) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, areaId));
            QVERIFY(map()->setRoomCoordinates(roomId, roomId - scmFirstRoom, 0, 0));
        }
        map()->mpRoomDB->getRoom(scmFirstRoom)->mSymbol = qsl("A");
        map()->mpRoomDB->getRoom(scmSecondRoom)->mSymbol = qsl("B");
    }

    QTableWidget* glyphUsageTable() const
    {
        auto* pGlyphDialog = mpPreferences ? mpPreferences->findChild<QDialog*>(qsl("dialog")) : nullptr;
        return pGlyphDialog ? pGlyphDialog->findChild<QTableWidget*>(qsl("tableWidget")) : nullptr;
    }

    static void markGlyphUsageTable(QTableWidget* pTable) { pTable->item(0, 1)->setData(scmRebuildMarkerRole, true); }

    static bool glyphUsageTableStillMarked(const QTableWidget* pTable)
    {
        for (int row = 0, rows = pTable->rowCount(); row < rows; ++row) {
            const QTableWidgetItem* pItem = pTable->item(row, 1);
            if (pItem && pItem->data(scmRebuildMarkerRole).toBool()) {
                return true;
            }
        }
        return false;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - see the same block in
        // DialogTeardownTest for why sharing the developer's one does not work
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mSaveDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
        QVERIFY(map());
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        mFontBefore = map()->getSymbolFont();
        mOnlyUseSelectedBefore = map()->getOnlySymbolFontUsed();
        mScalingBefore = map()->getSymbolFontFudgeFactor();
    }

    void cleanup()
    {
        // The whole class shares one profile, so a case that leaves the dialog
        // open or a setting changed would be writing the next one's fixture
        if (!mpHost->mpDlgProfilePreferences.isNull()) {
            delete mpHost->mpDlgProfilePreferences.data();
        }
        mpPreferences = nullptr;
        map()->setSymbolFont(mFontBefore);
        map()->setOnlySymbolFontUsed(mOnlyUseSelectedBefore);
        map()->setSymbolFontFudgeFactor(mScalingBefore);
    }

    // The invariant the whole of TMap::setSymbolFont() exists for: the style
    // strategy carries the NoFontMerging bit that the only-use-selected flag
    // owns, and a font that comes from a font combo-box or from a family name
    // in Lua has none of it. Take the incoming font wholesale and the flag
    // stays ticked while quietly meaning nothing.
    void test_pickingAFontKeepsTheOnlyUseSelectedStrategy()
    {
        QVERIFY(map()->setOnlySymbolFontUsed(true));
        QVERIFY(map()->getSymbolFont().styleStrategy() & QFont::NoFontMerging);
        const int pointSizeBefore = map()->getSymbolFont().pointSize();

        const QString otherFamily = anotherFontFamily();
        QVERIFY2(!otherFamily.isEmpty(), "No second font family available to switch to");
        QVERIFY(map()->setSymbolFont(QFont(otherFamily)));

        QCOMPARE(map()->getSymbolFont().family(), otherFamily);
        QVERIFY2(map()->getSymbolFont().styleStrategy() & QFont::NoFontMerging, "picking a font dropped the only-use-selected style strategy while leaving the flag set");
        QVERIFY(map()->getOnlySymbolFontUsed());
        QCOMPARE(map()->getSymbolFont().pointSize(), pointSizeBefore);
    }

    // Zero is the factor that really does blank every room symbol, and a map
    // file can carry one (issue #10176), so the bound belongs on the setter
    // rather than only at the Lua caller. NaN needs saying separately: it
    // compares false against both bounds.
    void test_theMapRefusesAScalingFactorOutsideItsRange()
    {
        const qreal before = map()->getSymbolFontFudgeFactor();
        for (const qreal refused : {0.0, -1.0, 0.49, 2.01, qQNaN(), qInf(), -qInf()}) {
            QVERIFY2(!map()->setSymbolFontFudgeFactor(refused), qPrintable(qsl("the map took a scaling factor of %1").arg(refused)));
            QCOMPARE(map()->getSymbolFontFudgeFactor(), before);
        }

        QVERIFY(map()->setSymbolFontFudgeFactor(TMap::scmMinimumSymbolFontFudgeFactor));
        QVERIFY(map()->setSymbolFontFudgeFactor(TMap::scmMaximumSymbolFontFudgeFactor));
    }

    // Each of the three is part of what a map file stores, so changing one is
    // an edit the profile must not close without offering to save.
    void test_changingASymbolSettingMarksTheMapUnsaved()
    {
        map()->resetUnsaved();
        QVERIFY(map()->setSymbolFontFudgeFactor(anotherScalingFactor(map()->getSymbolFontFudgeFactor())));
        QVERIFY2(map()->isUnsaved(), "changing the room symbol scaling factor left the map looking saved");

        map()->resetUnsaved();
        QVERIFY(map()->setOnlySymbolFontUsed(!map()->getOnlySymbolFontUsed()));
        QVERIFY2(map()->isUnsaved(), "toggling the only-use-selected flag left the map looking saved");

        const QString otherFamily = anotherFontFamily();
        QVERIFY2(!otherFamily.isEmpty(), "No second font family available to switch to");
        map()->resetUnsaved();
        QVERIFY(map()->setSymbolFont(QFont(otherFamily)));
        QVERIFY2(map()->isUnsaved(), "changing the room symbol font left the map looking saved");
    }

    // Every 2D map keeps its own cache of rendered symbol pixmaps, and a
    // secondary map view has one of its own that no longer matches the settings
    // the symbols were drawn with.
    void test_aSymbolChangeFlushesSecondaryMapViewCaches()
    {
        buildMapWithSymbols();
        const auto [viewId, message] = mpHost->createMapView(0);
        QVERIFY2(viewId > 0, qPrintable(message));
        auto closeView = qScopeGuard([this, viewId = viewId]() {
            mpHost->closeMapView(viewId);
        });

        TMapView* pView = map()->getViewManager()->getView(viewId);
        QVERIFY(pView);
        T2DMap* pViewMap = pView->get2DMap();
        QVERIFY(pViewMap);

        pViewMap->addSymbolToPixmapCache(qsl("test-key"), qsl("A"), Qt::white, false);
        QVERIFY2(pViewMap->symbolPixmapCacheCount() > 0, "nothing was cached, so a flush would prove nothing");

        QVERIFY(map()->setSymbolFontFudgeFactor(anotherScalingFactor(map()->getSymbolFontFudgeFactor())));
        QCOMPARE(pViewMap->symbolPixmapCacheCount(), 0);
    }

    // A script can change any of the three while the preferences are open. The
    // dialog only hears about it through TMap::signal_mapSymbolFontChanged; cut
    // that connect and it goes on showing - and, on Save, writing back - values
    // the map no longer has.
    void test_openPreferencesFollowTheMapSymbolSettings()
    {
        openPreferences();

        const qreal scalingSetElsewhere = anotherScalingFactor(map()->getSymbolFontFudgeFactor());
        const bool onlyUseSetElsewhere = !map()->getOnlySymbolFontUsed();

        // tells "the map never took it" apart from "the dialog never heard it"
        QSignalSpy symbolFontSpy(map(), &TMap::signal_mapSymbolFontChanged);
        QVERIFY2(map()->setSymbolFontFudgeFactor(scalingSetElsewhere), "The map refused the scaling factor this test set");
        QVERIFY2(map()->setOnlySymbolFontUsed(onlyUseSetElsewhere), "The map refused the only-use-selected flag this test set");
        QCOMPARE(symbolFontSpy.count(), 2);

        QCOMPARE(scalingSpinBox()->value(), scalingSetElsewhere);
        QCOMPARE(mpPreferences->checkBox_isOnlyMapSymbolFontToBeUsed->isChecked(), onlyUseSetElsewhere);

        // ...and the open dialog must not have written its own values back over
        // what was set from outside it:
        QCOMPARE(map()->getSymbolFontFudgeFactor(), scalingSetElsewhere);
        QCOMPARE(map()->getOnlySymbolFontUsed(), onlyUseSetElsewhere);
    }

    // The symbol font is the one of the three that only the dialog can set, and
    // closing is what writes back an edit the debounce has not carried yet.
    void test_closingPreferencesWritesTheChosenSymbolFont()
    {
        openPreferences();

        const QString chosen = anotherFontFamily();
        QVERIFY2(!chosen.isEmpty(), "this machine offers only the font already in use, so choosing another proves nothing");
        mpPreferences->fontComboBox_mapSymbols->setCurrentFont(QFont(chosen));

        closePreferences();

        QCOMPARE(map()->getSymbolFont().family(), chosen);
    }

    // The half of that which actually reverts a script's change: Save writes the
    // controls back to the map, so a stale control silently undoes it.
    void test_savingPreferencesKeepsMapSymbolSettingsSetElsewhere()
    {
        openPreferences();

        const qreal scalingSetElsewhere = anotherScalingFactor(map()->getSymbolFontFudgeFactor());
        QVERIFY2(map()->setSymbolFontFudgeFactor(scalingSetElsewhere), "The map refused the scaling factor this test set");
        // Without this the case would also pass on a Save that writes nothing
        // at all, which is indistinguishable from a Save that writes the right
        // value - see test_savingPreferencesWritesTheSpinBoxValue for the other
        // half:
        QCOMPARE(scalingSpinBox()->value(), scalingSetElsewhere);

        closePreferences();

        QCOMPARE(map()->getSymbolFontFudgeFactor(), scalingSetElsewhere);
    }

    // Save is still a path that writes: a factor only the spin-box knows about
    // has to reach the map.
    void test_savingPreferencesWritesTheSpinBoxValue()
    {
        openPreferences();

        const qreal typedIn = anotherScalingFactor(map()->getSymbolFontFudgeFactor());
        {
            // as though the live-apply connection were not there at all, which
            // is the only state in which Save has anything left to do
            const QSignalBlocker blocker(scalingSpinBox());
            scalingSpinBox()->setValue(typedIn);
        }
        QVERIFY2(!qFuzzyCompare(map()->getSymbolFontFudgeFactor(), typedIn), "the map already had the value, so Save writing it would prove nothing");

        closePreferences();

        QCOMPARE(map()->getSymbolFontFudgeFactor(), typedIn);
    }

    // Blocking the spin-box's signals stops the recursion but not
    // QAbstractSpinBox::updateEdit(), which rewrites the line edit. Setting the
    // box to the value it already holds therefore costs the keystroke that
    // comes next, and a factor cannot be typed in past its second decimal.
    void test_typingAScalingFactorIsNotInterrupted()
    {
        openPreferences();
        auto* pSpinBox = scalingSpinBox();
        pSpinBox->setValue(1.0);

        // The box parses with the widget's locale, so type what that locale
        // would write rather than assuming a full stop:
        const QString typed = QLocale().toString(1.25, 'f', 2);
        pSpinBox->setFocus();
        pSpinBox->selectAll();
        QTest::keyClicks(pSpinBox, typed);

        QCOMPARE(pSpinBox->value(), 1.25);
        QCOMPARE(map()->getSymbolFontFudgeFactor(), 1.25);
    }

    // Qt's default of two decimals would have the dialog report a factor a
    // script set as something it is not - setConfig() takes any value in the
    // range, not just the ones two decimals can write.
    void test_theSpinBoxShowsAFactorToThePrecisionTheApiAccepts()
    {
        openPreferences();
        QVERIFY(map()->setSymbolFontFudgeFactor(1.234));
        QCOMPARE(scalingSpinBox()->value(), 1.234);
    }

    // The spin-box holds no more precision than it displays, so writing it back
    // on every Save rounds off a factor a script set. setConfig() accepts any
    // value in the range, so the two layers would disagree about what is stored.
    void test_savingDoesNotRoundAScalingFactorSetElsewhere()
    {
        openPreferences();

        // needs more decimals than the spin-box shows, or it would round to
        // itself and the case would pass either way
        const qreal preciseScaling = 1.2378;
        QVERIFY(map()->setSymbolFontFudgeFactor(preciseScaling));
        QVERIFY2(!qFuzzyCompare(scalingSpinBox()->value(), preciseScaling), "the spin-box showed the factor exactly, so rounding could not be detected");

        closePreferences();

        QCOMPARE(map()->getSymbolFontFudgeFactor(), preciseScaling);
    }

    // Rebuilding the glyph usage table walks every room in the map through
    // TMap::roomSymbolsHash(). The table shows which glyphs the font has, so
    // the scaling factor cannot change it - and holding the spin-box's arrows
    // down would otherwise mean one whole-map scan per auto-repeat tick.
    void test_theGlyphUsageTableIsNotRebuiltForAScalingChange()
    {
        buildMapWithSymbols();
        openPreferences();
        mpPreferences->pushButton_showGlyphUsage->click();
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return glyphUsageTable() != nullptr;
                         },
                         5000),
                 "the glyph usage dialog did not appear");

        QTableWidget* pTable = glyphUsageTable();
        QVERIFY2(pTable->rowCount() > 0, "the glyph usage table listed no symbols, so a rebuild could not be detected");
        markGlyphUsageTable(pTable);

        QVERIFY(map()->setSymbolFontFudgeFactor(anotherScalingFactor(map()->getSymbolFontFudgeFactor())));
        QVERIFY2(glyphUsageTableStillMarked(pTable), "changing the room symbol scaling factor rebuilt the glyph usage table, which does not depend on it");

        // ...whereas the font, which it does depend on, still refreshes it
        const QString otherFamily = anotherFontFamily();
        QVERIFY2(!otherFamily.isEmpty(), "No second font family available to switch to");
        QVERIFY(map()->setSymbolFont(QFont(otherFamily)));
        QVERIFY2(!glyphUsageTableStillMarked(pTable), "changing the room symbol font left the glyph usage table showing the previous font's coverage");
    }

    // qRound() returns an int, so dividing its result by an int literal made
    // the JSON reader round every loaded factor to a whole number - and the 0
    // that anything below 1.0 became stops room symbols being drawn at all
    // (issue #10176). The setConfig key this test file is about is what makes
    // that visible to a script.
    void test_theScalingFactorSurvivesAJsonRoundTrip()
    {
        buildMapWithSymbols();
        const QString file = qsl("%1/scaling.json").arg(mSaveDir.path());
        for (const qreal scaling : {0.5, 0.75, 1.25, 1.5, 2.0}) {
            QVERIFY(map()->setSymbolFontFudgeFactor(scaling));
            const auto [wrote, writeMessage] = map()->writeJsonMapFile(file);
            QVERIFY2(wrote, qPrintable(writeMessage));

            QVERIFY(map()->setSymbolFontFudgeFactor(anotherScalingFactor(scaling)));
            const auto [read, readMessage] = map()->readJsonMapFile(file);
            QVERIFY2(read, qPrintable(readMessage));
            QCOMPARE(map()->getSymbolFontFudgeFactor(), scaling);
        }
    }

    // Loading cannot go through the setter - that would mark the freshly loaded
    // map as needing a save - so the bound has to be applied on the way in as
    // well. A file can hold anything: hand-edited, from another tool, or
    // written by a Mudlet from before #10176 was fixed.
    void test_aMapFileCannotCarryAScalingFactorThatBlanksTheSymbols()
    {
        buildMapWithSymbols();
        const QString file = qsl("%1/out-of-range.json").arg(mSaveDir.path());
        const auto [wrote, writeMessage] = map()->writeJsonMapFile(file);
        QVERIFY2(wrote, qPrintable(writeMessage));

        QFile mapFile(file);
        QVERIFY(mapFile.open(QIODevice::ReadOnly));
        QJsonObject mapObject = QJsonDocument::fromJson(mapFile.readAll()).object();
        mapFile.close();
        QVERIFY(!mapObject.isEmpty());
        mapObject[qsl("mapSymbolFontFudgeFactor")] = 0.0;
        QVERIFY(mapFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        mapFile.write(QJsonDocument(mapObject).toJson());
        mapFile.close();

        const auto [read, readMessage] = map()->readJsonMapFile(file);
        QVERIFY2(read, qPrintable(readMessage));
        QCOMPARE(map()->getSymbolFontFudgeFactor(), TMap::scmMinimumSymbolFontFudgeFactor);
    }

    // The binary format never truncated the factor, but it can still be holding
    // a 0: saving a map that was loaded from a JSON file written before #10176
    // was fixed carries the truncated value straight into the .dat, where it
    // stays for good.
    void test_aBinaryMapFileCannotCarryAScalingFactorThatBlanksTheSymbols()
    {
        buildMapWithSymbols();
        // What such a map holds - the setter would not take it, which is the
        // whole reason the load path has to check as well:
        map()->mMapSymbolFontFudgeFactor = 0.0;

        const QString file = qsl("%1/blank-symbols.dat").arg(mSaveDir.path());
        QSaveFile mapFile(file);
        QVERIFY(mapFile.open(QIODevice::WriteOnly));
        QDataStream out(&mapFile);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        QVERIFY(map()->serialize(out, map()->mDefaultVersion));
        QVERIFY(mapFile.commit());

        map()->mMapSymbolFontFudgeFactor = 1.0;
        QVERIFY2(map()->restore(file), "failed to restore the map that was just saved");
        QCOMPARE(map()->getSymbolFontFudgeFactor(), TMap::scmMinimumSymbolFontFudgeFactor);
    }

    // Every cached symbol pixmap was rendered with the settings the map had
    // before the load, so both file formats have to drop them once another
    // map's settings are in place.
    void test_loadingAMapFlushesTheRenderedSymbolCaches()
    {
        buildMapWithSymbols();
        const QString jsonFile = qsl("%1/flush.json").arg(mSaveDir.path());
        const auto [wroteJson, jsonWriteMessage] = map()->writeJsonMapFile(jsonFile);
        QVERIFY2(wroteJson, qPrintable(jsonWriteMessage));

        const QString binaryFile = qsl("%1/flush.dat").arg(mSaveDir.path());
        QSaveFile mapFile(binaryFile);
        QVERIFY(mapFile.open(QIODevice::WriteOnly));
        QDataStream out(&mapFile);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            out.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        QVERIFY(map()->serialize(out, map()->mDefaultVersion));
        QVERIFY(mapFile.commit());

        const auto [viewId, message] = mpHost->createMapView(0);
        QVERIFY2(viewId > 0, qPrintable(message));
        auto closeView = qScopeGuard([this, viewId = viewId]() {
            mpHost->closeMapView(viewId);
        });
        TMapView* pView = map()->getViewManager()->getView(viewId);
        QVERIFY(pView);
        T2DMap* pViewMap = pView->get2DMap();
        QVERIFY(pViewMap);

        pViewMap->addSymbolToPixmapCache(qsl("test-key"), qsl("A"), Qt::white, false);
        QVERIFY2(pViewMap->symbolPixmapCacheCount() > 0, "nothing was cached, so a flush would prove nothing");
        const auto [readJson, jsonReadMessage] = map()->readJsonMapFile(jsonFile);
        QVERIFY2(readJson, qPrintable(jsonReadMessage));
        QCOMPARE(pViewMap->symbolPixmapCacheCount(), 0);

        pViewMap->addSymbolToPixmapCache(qsl("test-key"), qsl("A"), Qt::white, false);
        QVERIFY(pViewMap->symbolPixmapCacheCount() > 0);
        QVERIFY2(map()->restore(binaryFile), "failed to restore the map that was just saved");
        QCOMPARE(pViewMap->symbolPixmapCacheCount(), 0);
    }

    // Loading a map is the most obvious writer of these settings other than
    // Lua, and the "Load map" button is in this very dialog. The file's values
    // go straight into the members, so nothing downstream hears about them
    // unless the load says so itself.
    void test_loadingAMapTellsTheOpenPreferences()
    {
        buildMapWithSymbols();

        // a map file carrying symbol settings of its own...
        const qreal scalingInFile = 1.75;
        QVERIFY(map()->setSymbolFontFudgeFactor(scalingInFile));
        QVERIFY(map()->setOnlySymbolFontUsed(true));
        const QString file = qsl("%1/symbols.json").arg(mSaveDir.path());
        const auto [wrote, writeMessage] = map()->writeJsonMapFile(file);
        QVERIFY2(wrote, qPrintable(writeMessage));

        // ...loaded into a map that currently has different ones
        const qreal scalingBeforeLoad = 1.25;
        QVERIFY(map()->setSymbolFontFudgeFactor(scalingBeforeLoad));
        QVERIFY(map()->setOnlySymbolFontUsed(false));

        openPreferences();
        QCOMPARE(scalingSpinBox()->value(), scalingBeforeLoad);

        QSignalSpy symbolFontSpy(map(), &TMap::signal_mapSymbolFontChanged);
        const auto [read, readMessage] = map()->readJsonMapFile(file);
        QVERIFY2(read, qPrintable(readMessage));

        QVERIFY2(symbolFontSpy.count() > 0, "loading a map said nothing about the symbol settings it brought with it");
        QCOMPARE(map()->getSymbolFontFudgeFactor(), scalingInFile);
        QCOMPARE(scalingSpinBox()->value(), scalingInFile);
        QVERIFY(mpPreferences->checkBox_isOnlyMapSymbolFontToBeUsed->isChecked());

        // and Save must not now put the pre-load values back:
        closePreferences();
        QCOMPARE(map()->getSymbolFontFudgeFactor(), scalingInFile);
        QVERIFY(map()->getOnlySymbolFontUsed());
    }
};

#include "MapSymbolFontTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapSymbolFontTest)
